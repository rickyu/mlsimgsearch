#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_HESSIAN_AFFINE_FEATURE_DETECTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_HESSIAN_AFFINE_FEATURE_DETECTOR_H_

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <vector>
#include <bof_common/object_selector.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <vlfeat/hesaff.h>

class HessianFeatureDetector : public ImageFeatureDetector {
 public:
  HessianFeatureDetector();
  ~HessianFeatureDetector();

  virtual void GetDescriptor(cv::Mat &desc);
  virtual ImageFeatureDetector* Clone();
  virtual std::string GetName() const;
  virtual int Detect(const cv::Mat &img, std::vector<cv::KeyPoint> &points);

 private:
  AffineHessianDetector *detector_;
  cv::Mat descriptor_;
};

#endif
