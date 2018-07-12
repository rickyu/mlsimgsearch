#include <bof_feature_analyzer/bof_weighter.h>


BOFWeighter::BOFWeighter() {
  conf_file_ = "";
}

BOFWeighter::~BOFWeighter() {
}
  
void BOFWeighter::SetConf(const std::string &conf_file) {
  conf_file_ = conf_file;
}

void BOFWeighter::SetWordsNum(int K) {
  words_number_ = K;
}
