#include "common.h"

ServiceRandomGenerator::ServiceRandomGenerator(){
  cur_max_ = -1;
  max_number_ = -1;
  candidates_ = NULL;
}

ServiceRandomGenerator::~ServiceRandomGenerator(){
  if (NULL != this->candidates_) {
    delete [](this->candidates_);
    this->candidates_ = NULL;
  }
}

void ServiceRandomGenerator::Init(int max){
  if (max > (1 << 16)) {
    max = (1 << 16);
  }
  this->max_number_ = max;
  this->cur_max_ = max;
  this->candidates_ = new int[max + 1];
  for (int i = 0; i <= max; i ++) {
    this->candidates_[i] = i;
  }
}

int ServiceRandomGenerator::GetRandom() {
  if (-1 >= this->cur_max_ || NULL == this->candidates_) {
    return -1;
  }
  srand(static_cast<unsigned int>(time(NULL)));
  int ret = rand() % (this->cur_max_+1);
  int tmp = this->candidates_[ret];
  this->candidates_[ret] = this->candidates_[this->cur_max_];
  this->candidates_[this->cur_max_] = tmp;
  this->cur_max_ --;
  return tmp;
}

unsigned int iptoint(std::string ip) {
  std::vector<std::string> res;
  unsigned int ans = 0;
  StringUtil::Split(ip, ".", res);
  if(res.size() != 4){
    return 0;
  }
  for(int i = 0; i < 4; i++){
    ans <<= 8;
    int tmp = atoi(res[i].c_str());
    ans += tmp;
  }
  return ans;
}
