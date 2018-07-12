#include <bof_feature_analyzer/mser_feature_detector.h>

void MserFeatureDetector::GetDescriptor(cv::Mat&) {
}
MserFeatureDetector::MserFeatureDetector() {
  detector_ = new cv::MserFeatureDetector();
}

MserFeatureDetector::~MserFeatureDetector() {
  if (detector_) {
    delete detector_;
    detector_ = NULL;
  }
}

ImageFeatureDetector* MserFeatureDetector::Clone() {
  return new MserFeatureDetector();
}

std::string MserFeatureDetector::GetName() const {
  return "MSER";
}

int MserFeatureDetector::Detect(const cv::Mat &img, std::vector<cv::KeyPoint> &points) {
  detector_->detect(img, points);
  return 0;
}
