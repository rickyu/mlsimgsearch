#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_GMM_QUANTIZER_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_GMM_QUANTIZER_H_

#include <opencv2/legacy/legacy.hpp>
#include <bof_feature_analyzer/bof_quantizer.h>

class GMMQuantizer : public BOFQuantizer {
 public:
  int Init();
  std::string GetName() const;
  int GetVocabularySize() const;
  BOFQuantizer* Clone();
  void SetFlnn(cv::flann::Index *index);
  int Compute(const cv::Mat &line);
  int Compute(const cv::Mat &descriptors,
              const std::vector<cv::KeyPoint> &detect,
              BOFQuantizeResultMap &result,
              QuantizerFlag flag = QUANTIZER_FLAG_HE);

 private:
  CvEM em_model_;
  cv::Mat p_mat_;
  cv::Mat median_mat_;
};

#endif
