#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_FEATURE_DETECTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_FEATURE_DETECTOR_H_

#include <opencv2/opencv.hpp>
#include <vector>
#include <bof_common/object_selector.h>

class ImageFeatureDetector {
 public:
  ImageFeatureDetector() {}
  virtual ~ImageFeatureDetector() {}

  virtual void GetDescriptor(cv::Mat &desc) = 0;
  virtual ImageFeatureDetector* Clone() = 0;
  virtual std::string GetName() const = 0;
  virtual int Detect(const cv::Mat &img, std::vector<cv::KeyPoint> &points) = 0;
};

typedef ObjectSelector<ImageFeatureDetector> FeatureDetectorSelector;

#endif
