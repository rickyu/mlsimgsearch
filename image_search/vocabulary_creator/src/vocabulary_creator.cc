#include "vocabulary_creator.h"
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <dirent.h>
#include <sys/stat.h>
#include <common/configure.h>
#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <fstream>

extern CLogger &g_logger;

VocabularyCreator::VocabularyCreator() {
}

VocabularyCreator::~VocabularyCreator() {
}

int VocabularyCreator::ListFiles() {
  std::string images_dir = Configure::GetInstance()->GetString("vocdatadir");

  struct dirent *dirp;
  struct stat filestat;
  DIR *dp = opendir(images_dir.c_str() );
  if (NULL == dp) {
    LOGGER_ERROR(g_logger, "error images path %s", images_dir.c_str());
    return -1;
  }
  std::string filepath;
  int count = 0;
  while ((dirp = readdir( dp ))) {
    filepath = images_dir + "/" + dirp->d_name;
                
    // If the file is a directory (or is in some way invalid) we'll skip it 
    if (stat( filepath.c_str(), &filestat )) {
      continue;
    }
    if (S_ISDIR( filestat.st_mode )) {
      continue;
    }
    if (dirp->d_name[0] == '.') {
      continue; //hidden file!
    }
    files_.push_back(filepath);
    
    ++count;
  }
  LOGGER_LOG(g_logger, "find %d files", count);
  return 0;
}

int VocabularyCreator::ExtractDescriptors(const std::string &save_file) {
  if (0 != ListFiles()) {
    return -1;
  }

  std::string detector_name = Configure::GetInstance()->GetString("detector_name");
  ImageFeatureDetector *detector =
    FeatureDetectorSelector::GetInstance()->GenerateObject(detector_name);
  if (NULL == detector) {
    LOGGER_ERROR(g_logger, "can't generate object %s", detector_name.c_str());
    return -2;
  }
  std::string extractor_name = Configure::GetInstance()->GetString("extractor_name");
  ImageFeatureDescriptorExtractor *extractor =
    FeatureDescriptorExtractorSelector::GetInstance()->GenerateObject(extractor_name);
  if (NULL == extractor) {
    LOGGER_ERROR(g_logger, "can't generate object %s", extractor_name.c_str());
    return -3;
  }

  descriptors_ = new cv::Mat();
  if (!descriptors_) {
    LOGGER_ERROR(g_logger, "out of memory");
    return -4;
  }

  std::vector<std::string>::iterator it = files_.begin();
  cv::Mat img;
  std::vector<cv::KeyPoint> keypoints;
  cv::Mat descriptor;

  std::fstream fs;
  fs.open(save_file.c_str(), std::fstream::out | std::fstream::binary);
  if (!fs.is_open()) {
    return -1;
  }

  while (it != files_.end()) {
    img = cv::imread(*it);
    if (!img.data) {
      ++it;
      continue;
    }
    detector->Detect(img, keypoints);
    extractor->Compute(img, keypoints, descriptor);

    for (int i = 0; i < descriptor.rows; i++) {
      for (int j = 0; j < descriptor.cols; j++) {
        if (j) {
          fs << " ";
        }
        fs << descriptor.at<float>(i, j);
      }
      fs << std::endl;
    }

    //descriptors_->push_back(descriptor);
    ++it;
  }

  FeatureDetectorSelector::GetInstance()->ReleaseObject(detector);
  FeatureDescriptorExtractorSelector::GetInstance()->ReleaseObject(extractor);

  LOGGER_LOG(g_logger, "extract (%d,%d) descriptors",
             descriptors_->size().width, descriptors_->size().height);
  return 0;
}

int VocabularyCreator::SaveDescriptors(const std::string &file_path) {
  std::fstream fs;
  fs.open(file_path.c_str(), std::fstream::out | std::fstream::binary);
  if (!fs.is_open()) {
    return -1;
  }
  for (int i = 0; i < descriptors_->rows; i++) {
    for (int j = 0; j < descriptors_->cols; j++) {
      if (j) {
        fs << " ";
      }
      fs << descriptors_->at<float>(i, j);
    }
    if (i < descriptors_->rows-1) {
      fs << std::endl;
    }
  }
  fs.close();
  return 0;
}

int VocabularyCreator::Cluster() {
  int k = Configure::GetInstance()->GetNumber<int>("cluster_k");

  cv::Mat labels, centroids;
  cv::kmeans(*descriptors_, k, labels, cv::TermCriteria(), 2, cv::KMEANS_PP_CENTERS, vocabulary_);
  
  return 0;
}

int VocabularyCreator::SaveVocabulary() {
  std::string filepath = Configure::GetInstance()->GetString("vocsavepath");

  cv::FileStorage fs(filepath.c_str(), cv::FileStorage::WRITE);
  fs << "vocabulary" << vocabulary_;
  fs.release();
  return 0;
}
