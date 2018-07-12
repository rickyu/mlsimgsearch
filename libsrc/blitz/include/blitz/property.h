// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_PROPERTY_H_
#define __BLITZ_PROPERTY_H_ 1
#include <string>
#include <unordered_map>
#include <cassert>
namespace blitz {
class PropertyMap {
 public:
  template <typename T>
  T GetProperty(const char* name, const T& default_value) const {
    assert(name !=  NULL);
    // 如果不存在，返回default_value
    T v = default_value;
    // v.parse(value);
    return v;
  }
  std::pair<int64_t, bool> GetInt(const char* name) const {
    assert(name);
    TMap::const_iterator iter = map_.find(name);
    if (iter == map_.end()) {
      return std::pair<int64_t, bool>(0l, false);
    }
    std::pair<int64_t, bool> ret(0, true);
    ret.first = atoll(iter->second.c_str());
    return ret;
  }
  void SetInt(const char* name, int64_t v) {
    char str[32];
    str[0]  = '\0';
    snprintf(str, sizeof(str), "%ld", v);
    map_[name] = str;
  }
  void SetString(const char* name, const std::string& val) {
    map_[name] = val;
  }

  std::pair<const std::string*, bool> GetString(const char* name) const {
    assert(name);
    TMap::const_iterator iter = map_.find(name);
    if (iter == map_.end()) {
      return std::pair<const std::string*, bool>((const std::string*)NULL, false);
    }
    return std::pair<const std::string*, bool>(&(iter->second), true);

  }
 private:
  typedef std::unordered_map<std::string, std::string> TMap;
 private:
  TMap map_;
};
}
#endif // __BLITZ_PROPERTY_H_
