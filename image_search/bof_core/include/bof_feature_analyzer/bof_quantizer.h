#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_QUANTIZER_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_QUANTIZER_H_

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <bof_common/object_selector.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

class BOFQuantizeResult {
 public:
  BOFQuantizeResult() {
    q_ = 0.0f;
    word_info_.clear();
    word_info_.push_back("");//HE
    word_info_.push_back("");//angle
  }
  ~BOFQuantizeResult() {
  }

  float q_;
  std::vector<std::string> word_info_;

 private:
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar,
                                         const unsigned int version) {
    ar & q_;
    ar & word_info_;
  }
};

// key: word_id, value: BOFQuantizeResult
typedef std::map<int, BOFQuantizeResult> BOFQuantizeResultMap;

typedef enum {
  QUANTIZER_FLAG_NONE,
  QUANTIZER_FLAG_HE,
} QuantizerFlag;

class BOFQuantizer {
 public:
  BOFQuantizer();
  virtual ~BOFQuantizer();

  //void SetVocabulary(const cv::Mat &voc);
  void SetVocabulary(const std::string &voc_path, const std::string &node_name);
  void SetPath(const std::string &median_path,
               const std::string &p_path,
               const std::string &word_path);
  virtual int GetVocabularySize() const = 0;
  virtual int Init() = 0;
  virtual std::string GetName() const = 0;
  virtual BOFQuantizer* Clone() = 0;
  /**
   * @param line [in] 需要计算的单独一个特征向量
   * @return 如果能找到合适的word，返回word_id，如果不能或者错误，返回值小于0
   */
  virtual int Compute(const cv::Mat &line) = 0;
  virtual int Compute(const cv::Mat &descriptors,
                      const std::vector<cv::KeyPoint> &detect,
                      BOFQuantizeResultMap &result,
                      QuantizerFlag flag = QUANTIZER_FLAG_HE) = 0;

 protected:
  //cv::Mat vocabulary_;
  std::string vocabulary_file_path_;
  std::string vocabulary_node_name_;

  std::string median_path_;
  std::string project_p_path_;
  std::string word_weight_path_;
};

typedef ObjectSelector<BOFQuantizer> BOFQuantizerSelector;

#endif
