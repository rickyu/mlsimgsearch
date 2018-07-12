#include <bof_feature_analyzer/idf_weighter.h>
#include <fstream>
#include <math.h>

IDFWeighter::IDFWeighter() {
}

IDFWeighter::~IDFWeighter() {
}

BOFWeighter* IDFWeighter::Clone() {
  return new IDFWeighter;
}

std::string IDFWeighter::GetName() {
  return "IDF";
}

int IDFWeighter::Init() {
  std::fstream fc(conf_file_.c_str(), std::ios::in);
  if (!fc.is_open()) {
    return -1;
  }

  word_weight_ = cv::Mat::zeros(words_number_, 1, CV_32FC1);
  float *word_mat = word_weight_.ptr<float>(0);
  int word;
  float frequence;
  while ( fc >> word >> frequence) {
    word_mat[word] = frequence ;
  }
  return 0;
}

int IDFWeighter::Weighting(int word_id, float &w, const void *context /*= NULL*/) {
  if (word_id >= words_number_ || NULL == context ) {
    return -1;
  }
  int feature_number = *((int*)context);
  float *word_mat = word_weight_.ptr<float>(0);
  if (0 >= feature_number) {
    w *= word_mat[word_id];
  }else {
    w *= word_mat[word_id] / feature_number;
  }
  return 0;
}
