#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_CSIFT_FEATURE_DETECTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_CSIFT_FEATURE_DETECTOR_H_

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <vector>
#include <bof_common/object_selector.h>
#include <bof_feature_analyzer/feature_detector.h>

class CsiftFeatureDetector : public ImageFeatureDetector {
 public:
  CsiftFeatureDetector();
  ~CsiftFeatureDetector();

  virtual void GetDescriptor(cv::Mat &desc);
  virtual ImageFeatureDetector* Clone();
  virtual std::string GetName() const;
  virtual int Detect(const cv::Mat &img, 
                     std::vector<cv::KeyPoint> &points);

 private:
  int getGaussianKernel(int size,float t,
                        cv::Mat &kernel_mat,int x,int y);
  void GaussianDBlur(const cv::Mat &img, cv::Mat &src,
                     int size, float t,int x ,int y);
  void SplitMat(const cv::Mat &img , 
                cv::Mat &out1,  cv::Mat &out2, cv::Mat &out3);
  void ColorInvariant(const cv::Mat & e, const cv::Mat &el, 
                      const cv::Mat &ell,float t,cv::Mat &h);

  cv::Mat descriptor_;
};

#endif
