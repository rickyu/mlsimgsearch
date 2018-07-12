#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_SIFT_FEATURE_DESCRIPTOR_EXTRACTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_SIFT_FEATURE_DESCRIPTOR_EXTRACTOR_H_

#include <opencv2/opencv.hpp>
#include <vector>
#include <bof_feature_analyzer/feature_descriptor_extractor.h>

class SiftFeatureDescriptorExtractor : public ImageFeatureDescriptorExtractor {
 public:
  SiftFeatureDescriptorExtractor();
  virtual ~SiftFeatureDescriptorExtractor();

  virtual ImageFeatureDescriptorExtractor* Clone();
  virtual std::string GetName() const;
  virtual int Compute(const cv::Mat &mat,
                      std::vector<cv::KeyPoint> &points,
                      cv::Mat &descriptors);
  virtual int DescriptorSize() const;
  virtual int DescriptorType() const;

 private:
  cv::DescriptorExtractor *extractor_;
};

#endif
