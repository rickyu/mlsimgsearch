#ifndef IMAGE_SEARCH_IMAGE_SIMILAR_FEATURE_DATA_CONTAINER_H_
#define IMAGE_SEARCH_IMAGE_SIMILAR_FEATURE_DATA_CONTAINER_H_

#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <SSDB_client.h>

class ImageFeatureLoader {
 public:
  ImageFeatureLoader();
  ~ImageFeatureLoader();
  int ConnectSSDB(const std::string &host, const int port);
  int LoadFeatureFromUrl(const std::string &url);
  int LoadFeatureFromSSDB(const std::string &key, cv::Mat &descs, std::vector<cv::KeyPoint> &points);
  void GetKeyPoints(std::vector<cv::KeyPoint> & points) { points = key_points_;}
  void GetDescriptors(cv::Mat &descriptors) { descriptors = descriptors_;}
  int SaveFeatures(const std::string &key);

 private:
  std::vector<cv::KeyPoint> key_points_;
  cv::Mat descriptors_;
  CLogger *logger_;
  ssdb::Client *ssdb_;
};


#endif
