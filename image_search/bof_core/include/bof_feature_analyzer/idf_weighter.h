#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_IDF_WEIGHTER_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_IDF_WEIGHTER_H_

#include <bof_feature_analyzer/bof_weighter.h>
#include <opencv2/opencv.hpp>

class IDFWeighter : public BOFWeighter {
 public:
  IDFWeighter();
  ~IDFWeighter();

  BOFWeighter* Clone();
  std::string GetName();
  int Init();
  int Weighting(int word_id, float &w, const void *context = NULL);

 private:
  cv::Mat word_weight_;
};

#endif
