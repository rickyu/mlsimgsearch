#include "ranker_processor.h"
#include <common/http_client.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_feature_analyzer/knn_quantizer.h>
#include <bof_feature_analyzer/idf_weighter.h>
#include <bof_searcher/ransac_ranker.h>
#include <common/configure.h>

extern CLogger& g_logger;

RankerProcessor::RankerProcessor()
  : detector_(NULL)
  , extractor_(NULL)
  , quantizer_(NULL) {
}

RankerProcessor::~RankerProcessor() {
  FeatureDetectorSelector::GetInstance()->ReleaseObject(detector_);
  FeatureDescriptorExtractorSelector::GetInstance()->ReleaseObject(extractor_);
}

int RankerProcessor::Init() {
  std::string detector_name = Configure::GetInstance()->GetString("detector_name");
  if("HESSIAN" == detector_name || "DSIFT" == detector_name ||"CSIFT" == detector_name ) {
    need_extractor_ = false;
  }else {
  }
  detector_ = FeatureDetectorSelector::GetInstance()->GenerateObject(detector_name);
  if (NULL == detector_) {
    LOGGER_ERROR(g_logger, "Generate detector failed. (%s)", detector_name.c_str());
    return -1;
  }
  std::string extractor_name = Configure::GetInstance()->GetString("extractor_name");
  extractor_ = FeatureDescriptorExtractorSelector::GetInstance()->GenerateObject(extractor_name);
  if (NULL == extractor_) {
    LOGGER_ERROR(g_logger, "Generate extractor failed. (%s)", extractor_name.c_str());
    return -2;
  }

  spatial_ranker_name_ = Configure::GetInstance()->GetString("spatial_ranker_name");
  image_service_server_ = Configure::GetInstance()->GetString("image_service_server");

  return 0;
}

int RankerProcessor::Rank(SpatialRankerResponse& response,
                          const SpatialRankerRequest& request) {
  if (request.pics.size() == 0) {
    return -1;
  }
  SearchResult sr;
  for (size_t i = 0; i < request.pics.size(); ++i) {
    sr.AddItem(request.pics[i], 1.f);
    //LOGGER_INFO(g_logger, "Recvd ranker request, picid=%ld", request.pics[i]);
  }
  
  LOGGER_INFO(g_logger, "Begin spatial ranker");
  BOFSpatialRanker *spatial_ranker =
    BOFSpatialRankerSelector::GetInstance()->GenerateObject(spatial_ranker_name_);
  if (NULL == spatial_ranker) {
    LOGGER_ERROR(g_logger, "BOFSpatialRankerSelector generate object failed, name=%s",
                 spatial_ranker_name_.c_str());
    return -2;
  }
  if ("RANSAC" == spatial_ranker_name_) {
    RansacRankerConf conf;
    conf.detector_ = detector_;
    conf.extractor_ = extractor_;
    conf.image_service_server_ = image_service_server_;
    conf.min_inliers_number_ = Configure::GetInstance()->GetNumber<int>("min_inliers_number");
    conf.max_rank_pic_number_ = Configure::GetInstance()->GetNumber<int>("max_rank_pic_number");
    conf.rank_ignore_same_result_ = true;
    conf.should_rank_ = false;
    spatial_ranker->SetConf(&conf);
  }
  std::vector<cv::KeyPoint> points;
  cv::Mat descriptors;
  cv::FileStorage fs_points(request.keypoints, cv::FileStorage::READ | cv::FileStorage::MEMORY);
  cv::FileNode fnode = fs_points["points"];
  cv::read(fnode, points);
  fs_points.release();
  cv::FileStorage fs_desc(request.descriptors, cv::FileStorage::READ | cv::FileStorage::MEMORY);
  fs_desc["descriptors"] >> descriptors;
  fs_desc.release();
  int ret = spatial_ranker->Rank(points, descriptors, sr);
  if (0 != ret) {
    BOFSpatialRankerSelector::GetInstance()->ReleaseObject(spatial_ranker);
    LOGGER_ERROR(g_logger, "spatial rank failed, ret=%d", ret);
    return -3;
  }
  BOFSpatialRankerSelector::GetInstance()->ReleaseObject(spatial_ranker);
  LOGGER_INFO(g_logger, "End spatial ranker");

  for (size_t i = 0; i < sr.items_.size(); ++i) {
    if (sr.items_[i].sim_ > 0.f) {
      SpatialSimilar ss;
      ss.picid = sr.items_[i].pic_id_;
      ss.sim = sr.items_[i].sim_;
      response.result.push_back(ss);
      LOGGER_INFO(g_logger, "SpatialRanker result: picid=%ld, sim=%.f", ss.picid, ss.sim);
    }
  }

  return 0;
}
