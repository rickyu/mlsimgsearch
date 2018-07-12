#ifndef IMAGE_SEARCH_INDEXER_IMAGE_QUANTIZED_SERIALIZER_H_
#define IMAGE_SEARCH_INDEXER_IMAGE_QUANTIZED_SERIALIZER_H_

#include <bof_feature_analyzer/bof_quantizer.h>

class ImageQuantizedDataItem {
 public:
  ImageQuantizedDataItem() {
    word_id_ = 0;
    weight_ = 0.f;
  }
  int word_id_;
  float weight_;
};

typedef std::vector<ImageQuantizedDataItem> ImageQuantizedData;

class ImageQuantizedSerialzer {
 public:
  static int Serialize(const BOFQuantizeResultMap &result,
                       int feature_num,
                       std::string &out);
  static int Unserialize(const std::string &src,
                         ImageQuantizedData &data);
};

#endif
