#include <common/configure.h>
#include <common/http_client.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <fstream>
#include <boost/filesystem.hpp>
#include "feature_data_container.h"
#include <unistd.h>
#include <boost/lexical_cast.hpp>


ImageFeatureLoader::ImageFeatureLoader() {
  logger_ = &CLogger::GetInstance("goods_similar");
}

ImageFeatureLoader::~ImageFeatureLoader() {
  if (ssdb_) {
    delete ssdb_;
  }
}

int ImageFeatureLoader::ConnectSSDB(const std::string &host, const int port) {
  ssdb_ = ssdb::Client::connect(host, port);
  if (ssdb_ == NULL) {
    return 1;
  }
  return 0;
}

int ImageFeatureLoader::LoadFeatureFromUrl(const std::string &url) {
  int ret = 0;
  char *data = NULL;
  size_t data_len = 0;
  HttpClient http_client;
  http_client.Init();
  if (CR_SUCCESS != http_client.SendRequest(HTTP_GET,
                                            url.c_str(),
                                            &data,
                                            &data_len)) {
    LOGGER_ERROR(*logger_, "Send HTTP request failed. (%s)", url.c_str());
    return -1;
  }
  std::auto_ptr<char> apdata(data);//智能指针
  CvMat mat_tmp = cvMat(1, data_len, CV_32FC1, data);
  CvMat *cur_mat = cvDecodeImageM(&mat_tmp);
  cv::Mat img(cur_mat);
   
  SiftFeatureDetector *detector = new SiftFeatureDetector();
  SiftFeatureDescriptorExtractor *extractor = new SiftFeatureDescriptorExtractor();

  ret = detector->Detect(img, key_points_);
  if (0 != ret) {
    LOGGER_ERROR(*logger_, "Detect feature points failed. (%d)", ret);
    return -2;
  }
  LOGGER_INFO(*logger_, "Detect %ld feature points.", key_points_.size());
  ret = extractor->Compute(img, key_points_, descriptors_);
  if (0 != ret) {
    LOGGER_ERROR(*logger_, "Extractor feature descriptors failed. (%d)", ret);
    return -3;
  }
  return 0;
}

int ImageFeatureLoader::LoadFeatureFromSSDB(const std::string &key,
                                            cv::Mat &descs,
                                            std::vector<cv::KeyPoint> &points) {
  std::string data;
  ssdb_->get(key, &data);
  if (data.empty()) {
    LOGGER_ERROR(*logger_, "there is no data in ssdb. [%s]", key.c_str());
    return -1;
  }
  std::string tmp_file = ".tmpfile_" + key + boost::lexical_cast<std::string>(getpid()) + ".yaml";
  std::fstream ifs(tmp_file.c_str(), std::ios::out|std::ios::binary);
  ifs.write(data.c_str(), data.size());
  ifs.close();
  cv::FileStorage fs(tmp_file, cv::FileStorage::READ);
  cv::read(fs["desc"], descs);
  cv::read(fs["point"], points);
  fs.release();
  boost::filesystem::remove(tmp_file);
  return 0;
}

int ImageFeatureLoader::SaveFeatures(const std::string &key) {
  if (descriptors_.rows < 20) {
    LOGGER_ERROR(*logger_, "may be wrong image: %s!", key.c_str());
    return -1;
  }
  std::string tmp_file = ".tmpfile_" + key + boost::lexical_cast<std::string>(getpid()) + ".yaml";
  cv::FileStorage fs(tmp_file,
                     cv::FileStorage::WRITE);
  cv::write(fs, "desc", descriptors_);
  cv::write(fs, "point", key_points_);
  fs.release();
  std::ifstream ifs(tmp_file.c_str(), std::ifstream::binary);
  ifs.seekg (0, ifs.end);
  int length = ifs.tellg();
  ifs.seekg (0, ifs.beg);
  char *buffer = new char [length+1];
  ifs.read (buffer,length);
  buffer[length] = '\0';
  ssdb_->set(key, buffer);
  delete []buffer;
  ifs.close();
  boost::filesystem::remove(tmp_file);
  return 0;
}
