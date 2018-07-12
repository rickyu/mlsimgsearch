#ifndef IMAGE_SEARCH_INDEXER_IMAGE_QUANTIZER_H_
#define IMAGE_SEARCH_INDEXER_IMAGE_QUANTIZER_H_

#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_feature_analyzer/bof_weighter.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>


class ImageQuantizer {
 public:
  ImageQuantizer();
  ~ImageQuantizer();
  
  int Init();
  int Analyze(const int64_t pic_id,
              const std::string &pic_key,
              BOFQuantizeResultMap &result,
              size_t &total_features,
              QuantizerFlag idf_label );

 private:
  BOFQuantizer *quantizer_;
  BOFWeighter *weighter_;
  ImageFeatureDetector *detector_;
  ImageFeatureDescriptorExtractor *extractor_;
  bool need_extractor_;
  static CLogger &logger_;
};

#endif
