#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_WEIGHTER_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_WEIGHTER_H_

#include <string>
#include <vector>
#include <bof_common/object_selector.h>

class BOFWeighter {
 public:
  BOFWeighter();
  ~BOFWeighter();
  
  void SetConf(const std::string &conf_file);
  void SetWordsNum(int K);
  virtual BOFWeighter* Clone() = 0;
  virtual std::string GetName() = 0;
  virtual int Init() = 0;
  /**
   * @param word_id
   * @param w [in|out] 权值，既为输入参数也为输出参数
   */
  virtual int Weighting(int word_id, float &w, const void *context = NULL) = 0;
  
 protected:
  int words_number_;
  std::string conf_file_;
};

typedef ObjectSelector<BOFWeighter> BOFWeighterSelector;

#endif
