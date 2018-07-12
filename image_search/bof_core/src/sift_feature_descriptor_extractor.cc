#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>

SiftFeatureDescriptorExtractor::SiftFeatureDescriptorExtractor() {
  extractor_ = new cv::SiftDescriptorExtractor();
}

SiftFeatureDescriptorExtractor::~SiftFeatureDescriptorExtractor() {
  if (extractor_) {
    delete extractor_;
    extractor_ = NULL;
  }
}

ImageFeatureDescriptorExtractor* SiftFeatureDescriptorExtractor::Clone() {
  return new SiftFeatureDescriptorExtractor();
}

std::string SiftFeatureDescriptorExtractor::GetName() const {
  return "SIFT";
}

int SiftFeatureDescriptorExtractor::Compute(const cv::Mat &mat,
                                            std::vector<cv::KeyPoint> &points,
                                            cv::Mat &descriptors) {
  extractor_->compute(mat, points, descriptors);

  return 0;
}

int SiftFeatureDescriptorExtractor::DescriptorSize() const {
  return extractor_->descriptorSize();
}

int SiftFeatureDescriptorExtractor::DescriptorType() const {
  return extractor_->descriptorType();
}
