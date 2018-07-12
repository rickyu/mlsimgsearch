#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_FEATURE_DESCRIPTOR_EXTRACTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_FEATURE_DESCRIPTOR_EXTRACTOR_H_

#include <opencv2/opencv.hpp>
#include <vector>
#include <bof_common/object_selector.h>

class ImageFeatureDescriptorExtractor {
 public:
  ImageFeatureDescriptorExtractor() {}
  virtual ~ImageFeatureDescriptorExtractor() {}

  virtual ImageFeatureDescriptorExtractor* Clone() = 0;
  virtual std::string GetName() const = 0;
  virtual int Compute(const cv::Mat &mat,
                      std::vector<cv::KeyPoint> &points,
                      cv::Mat &descriptors) = 0;
  virtual int DescriptorSize() const = 0;
  virtual int DescriptorType() const = 0;
};

typedef ObjectSelector<ImageFeatureDescriptorExtractor> FeatureDescriptorExtractorSelector;

#endif
