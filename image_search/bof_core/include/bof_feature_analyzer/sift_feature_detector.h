#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_SIFT_FEATURE_DETECTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_SIFT_FEATURE_DETECTOR_H_

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <vector>
#include <bof_feature_analyzer/feature_detector.h>

class SiftFeatureDetector : public ImageFeatureDetector {
 public:
  SiftFeatureDetector();
  ~SiftFeatureDetector();

  
  virtual void GetDescriptor(cv::Mat &desc);
  virtual ImageFeatureDetector* Clone();
  virtual std::string GetName() const;
  virtual int Detect(const cv::Mat &img, std::vector<cv::KeyPoint> &points);

 private:
  cv::SiftFeatureDetector *detector_;
};

#endif
