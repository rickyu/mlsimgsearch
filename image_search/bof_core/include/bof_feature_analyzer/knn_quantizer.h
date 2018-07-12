#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_KNN_QUANTIZER_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_KNN_QUANTIZER_H_

#include <bof_feature_analyzer/bof_quantizer.h>

class KnnQuantizer : public BOFQuantizer {
 public:
  KnnQuantizer();
  ~KnnQuantizer();

  int Init();
  std::string GetName() const;
  BOFQuantizer* Clone();
  void SetFlnn(cv::flann::Index *index);
  int GetVocabularySize() const;
  int Compute(const cv::Mat &line);
  int Compute(const cv::Mat &descriptors,
              const std::vector<cv::KeyPoint> &detect,
              BOFQuantizeResultMap &result,
              QuantizerFlag flag = QUANTIZER_FLAG_HE);

 private:
  //cv::Mat vocabulary_;
  cv::Mat p_mat_;
  cv::Mat median_mat_;
  cv::flann::Index *tree_flann_index_;
  float nndr_;
};

#endif
