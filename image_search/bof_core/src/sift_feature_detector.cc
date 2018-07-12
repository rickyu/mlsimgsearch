#include <bof_feature_analyzer/sift_feature_detector.h>

void SiftFeatureDetector::GetDescriptor(cv::Mat&) {
}
SiftFeatureDetector::SiftFeatureDetector() {
  detector_ = new cv::SiftFeatureDetector();
}

SiftFeatureDetector::~SiftFeatureDetector() {
  if (detector_) {
    delete detector_;
    detector_ = NULL;
  }
}

ImageFeatureDetector* SiftFeatureDetector::Clone() {
  return new SiftFeatureDetector();
}

std::string SiftFeatureDetector::GetName() const {
  return "SIFT";
}

int SiftFeatureDetector::Detect(const cv::Mat &img, std::vector<cv::KeyPoint> &points) {
  detector_->detect(img, points);
  return 0;
}
