#include "image_quantizer.h"
#include <bof_common/object_selector.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <common/configure.h>
#include <common/http_client.h>
#include <beansdb_client/beansdb_clusters.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <sys/time.h>
#include <bof_feature_analyzer/idf_weighter.h>

CLogger& ImageQuantizer::logger_ = CLogger::GetInstance("indexer");

ImageQuantizer::ImageQuantizer()
  : quantizer_(NULL)
  , detector_(NULL)
  , extractor_(NULL) 
  , need_extractor_(true) {
}

ImageQuantizer::~ImageQuantizer() {
  if (quantizer_) {
    BOFQuantizerSelector::GetInstance()->ReleaseObject(quantizer_);
  }
  if (detector_) {
    FeatureDetectorSelector::GetInstance()->ReleaseObject(detector_);
  }
  if (extractor_) {
    FeatureDescriptorExtractorSelector::GetInstance()->ReleaseObject(extractor_);
  }
  if (weighter_) {
    BOFWeighterSelector::GetInstance()->ReleaseObject(weighter_);
  }
}

int ImageQuantizer::Init() {
  std::string detector_name = Configure::GetInstance()->GetString("detector_name");
  if ("HESSIAN" == detector_name || "DSIFT" == detector_name ||"CSIFT" == detector_name) {
    need_extractor_ = false;
  }
  detector_ = FeatureDetectorSelector::GetInstance()->GenerateObject(detector_name);
  if (NULL == detector_) {
    LOGGER_ERROR(logger_, "Generate detector failed. (%s)", detector_name.c_str());
    return -1;
  }else {
    LOGGER_LOG(logger_, "Generate detector sucess. (%s)", detector_name.c_str());
  }
  std::string extractor_name = Configure::GetInstance()->GetString("extractor_name");
  extractor_ = FeatureDescriptorExtractorSelector::GetInstance()->GenerateObject(extractor_name);
  if (NULL == extractor_) {
    LOGGER_ERROR(logger_, "Generate extractor failed. (%s)", extractor_name.c_str());
    return -2;
  }

  std::string quantizer_name = Configure::GetInstance()->GetString("quantizer_name");
  quantizer_ = BOFQuantizerSelector::GetInstance()->GenerateObject(quantizer_name);
  if (NULL == quantizer_) {
    LOGGER_ERROR(logger_, "Generate quantizer failed. (%s)", quantizer_name.c_str());
    return -3;
  }
  //he and tf-idf init
  std::string project_p = Configure::GetInstance()->GetString("project_p");
  std::string median_path = Configure::GetInstance()->GetString("median");
  std::string word_weight_path = Configure::GetInstance()->GetString("word_weight");
  quantizer_->SetPath(median_path,project_p,word_weight_path);
  std::string weighter_name = Configure::GetInstance()->GetString("weighter_name");
  weighter_ = BOFWeighterSelector::GetInstance()->GenerateObject(weighter_name);
  if (NULL == weighter_) {
    LOGGER_ERROR(logger_, "Generate weighter failed. (%s)", weighter_name.c_str());
    return -4;
  }
  weighter_->SetConf(word_weight_path);

  std::string vocabulary_path = Configure::GetInstance()->GetString("vocpath");
  std::string voc_node_name = Configure::GetInstance()->GetString("voc_node_name");
  quantizer_->SetVocabulary(vocabulary_path, voc_node_name);
  int ret = quantizer_->Init();
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Quantizer init failed. (%d)", ret);
    return -5;
  }
  weighter_->SetWordsNum(quantizer_->GetVocabularySize());
  /*ret = weighter_->Init();
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Weighter init failed. (%d)", ret);
    return -6;
  }*/

  return 0;
}

int ImageQuantizer::Analyze(const int64_t pic_id,
                            const std::string &pic_key,
                            BOFQuantizeResultMap &result,
                            size_t &total_features, 
                            QuantizerFlag idf_label ) {
  char *img_data = NULL;
  size_t img_size = 0;
  int ret = 0;
  total_features = 0;
  timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  if (BCR_SUCCESS != BeansdbClusters::GetInstance()->Get(pic_key, img_data, img_size)) {
    delete img_data;
    LOGGER_ERROR(logger_, "Beansdb get pic(%s) failed, ret=%d", pic_key.c_str(), ret);
    img_data = NULL;
    return -1;
    
  }

  gettimeofday(&tv2, NULL);
  unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
  LOGGER_INFO(logger_, "Get key(%s) from beansdb timed consumed: %ld", pic_key.c_str(), ts);

  CvMat mat_tmp = cvMat(1, img_size, CV_32FC1, img_data);
  CvMat *newmat = cvDecodeImageM(&mat_tmp);
  cv::Mat img(newmat);

  std::vector<cv::KeyPoint> points;
  cv::Mat descriptors;
  ret = detector_->Detect(img, points);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Detect descriptor points failed, ret=%d", ret);
    delete img_data;
    cvReleaseMat(&newmat);
    return -2;
  }
  gettimeofday(&tv1, NULL);
  ts = ((tv1.tv_sec-tv2.tv_sec)*1000000+(tv1.tv_usec-tv2.tv_usec)) / 1000;
  if (need_extractor_) {
    ret = extractor_->Compute(img, points, descriptors);
  }else {
    detector_->GetDescriptor(descriptors);
    ret = 0;
  }
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Extract descriptor failed, ret=%d", ret);
    delete img_data;
    cvReleaseMat(&newmat);
    return -3;
  }
  gettimeofday(&tv2, NULL);
  ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
  LOGGER_INFO(logger_, "Picture(%s) extract decriptor =  ****%d ****  timed consumed: %ld",
              pic_key.c_str(), descriptors.rows,ts);

  total_features = descriptors.rows;
  ret = quantizer_->Compute(descriptors, points , result,idf_label);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Image quantizer failed, ret=%d", ret);
    delete img_data;
    cvReleaseMat(&newmat);
    return -4;
  }
  LOGGER_INFO(logger_, "quantized %ld words", result.size());
  if (QUANTIZER_FLAG_HE == idf_label) {
    if (0 != ret) {
      LOGGER_ERROR(logger_, "Weighter init failed. (%d)", ret);
      return -5;
    }
    /*float sum = 0.0f;
    BOFQuantizeResultMap::iterator it = result.begin();
    while (it != result.end()) {
      //weighter_->Weighting(it->first, it->second.q_, &total_features);
      sum += it->second.q_ ;
      it ++;
    }
    it = result.begin();
    while (it != result.end()) {
      it->second.q_  = sqrt( it->second.q_ / sum );
      it ++;
    }*/
  }
  gettimeofday(&tv1, NULL);
  ts = ((tv1.tv_sec-tv2.tv_sec)*1000000+(tv1.tv_usec-tv2.tv_usec)) / 1000;
  LOGGER_INFO(logger_, "Picture(%s) quantizer compute =  ****  %ld ****   time: %ld",
              pic_key.c_str(),result.size(), ts);

  cvReleaseMat(&newmat);
  delete img_data;
  
  return 0;
}
