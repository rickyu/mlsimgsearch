#include "image_quantized_serializer.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <sstream>


int ImageQuantizedSerialzer::Serialize(const BOFQuantizeResultMap &result,
                                       int feature_num,
                                       std::string &out) {
  if (feature_num <= 0) {
    return -1;
  }
  BOFQuantizeResultMap::const_iterator it = result.begin();
  out.clear();
  std::stringstream ss;
  while (it != result.end()) {
    BOFQuantizeResultMap::const_iterator it_cur = it++;
    ss.precision(1);
    int q = (int)(it_cur->second.q_/feature_num * 10000);
    ss << std::hex << it_cur->first << ":" << std::hex << q;
    if (it != result.end()) {
      ss << ";";
    }
    else {
      break;
    }
  }
  out.assign(ss.str());

  return 0;
}

int ImageQuantizedSerialzer::Unserialize(const std::string &src,
                                         ImageQuantizedData &data) {
  std::vector<std::string> words;
  boost::algorithm::split(words, src, boost::algorithm::is_any_of(";:"));
  if (words.size() <= 0 || words.size() % 2 != 0) {
    return -1;
  }
  data.clear();
  bool word_got = false;
  ImageQuantizedDataItem item;
  for (size_t i = 0; i < words.size(); ++i) {
    if (!word_got) {
      word_got = !word_got;
      item.word_id_ = boost::lexical_cast<int>(words[i]);
    }
    else {
      word_got = !word_got;
      item.weight_ = (float)boost::lexical_cast<int>(words[i]);
      item.weight_ /= 10000;
      data.push_back(item);
    }
  }
  return 0;
}
