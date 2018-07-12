#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_EMD_TEXTURE_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_EMD_TEXTURE_H_

#include <bof_feature_analyzer/gabor_filter.h>

class EmdTexture {
 public:
  float CalcSimilar(const IplImage *src1, const IplImage *src2);
  
 private:
  float CalcImageMean(const IplImage *img);
  float CalcImageVariance(const IplImage *img, float mean);
  int CalcGaborFeatureVector(const IplImage *src, cv::Mat &mat);
};

#endif
