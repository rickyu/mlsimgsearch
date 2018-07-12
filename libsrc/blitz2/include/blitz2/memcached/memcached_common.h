// Copyright 2013 meilishuo Inc.
// Memcached common class. 
#ifndef __BLITZ2_MEMCACHED_COMMON_H_
#define __BLITZ2_MEMCACHED_COMMON_H_ 1

namespace blitz2 {

struct CharArrayWrap {
  CharArrayWrap() {
    this->str = NULL;
    this->len = 0;
  }
  void Set(const char* str, uint32_t length) {
    this->str = str;
    this->len = length;
  }
  const char* str;
  uint32_t len;
};
typedef CharArrayWrap SimpleString;
typedef CharArrayWrap Token;

class InitParam {
 public:
  InitParam() { 
    buf_start_len_ = 512;
    buf_max_len_ = 4*1024*1024;
  }
  InitParam(uint32_t start_len, uint32_t max_len) {
    buf_start_len_ = start_len;
    buf_max_len_ = max_len;
  }
  uint32_t buf_start_len() const { return buf_start_len_;}
  uint32_t& buf_start_len() { return buf_start_len_;}
  uint32_t buf_max_len() const { return buf_max_len_;}
  uint32_t& buf_max_len() {return buf_max_len_ ;}
 protected:
  uint32_t buf_start_len_;
  uint32_t buf_max_len_;
};
class MemcachedUtils {
 public:    
  static uint32_t TokenizerCommand(const char*, uint32_t, Token*, uint32_t);
  static char* SearchEol(const char* p, uint32_t len);
};
inline uint32_t MemcachedUtils::TokenizerCommand(const char* begin,
    uint32_t len, Token* tokens, uint32_t max) {
  char *sword = NULL;
  char *eword = NULL;
  char *end = const_cast<char*>(begin) + len;
  uint32_t index = 0;
  for ( sword = eword = const_cast<char*>(begin); eword < end; ++ eword) {
    if (index >= max) {
      return -1;
    }
    if (*eword == ' ') {
      if (*sword != *eword) {
        tokens[index].Set(sword, eword - sword);
        ++ index;
      }
      sword = eword + 1;
    } else if (*eword == '\r' && *(eword + 1) == '\n') {
      tokens[index].Set(sword, eword - sword);
      ++ index;
      break;
    }
  }
  return index;
}

inline char* MemcachedUtils::SearchEol(const char* p, uint32_t len) {
  for (uint32_t i=1; i<len; ++i) {
    if (p[i] == '\n') {
      if (p[i-1] == '\r') {
        return const_cast<char*>(p+i-1);
      }
    }
  }
  return NULL;
}
} // namespace blitz2.
#endif // __BLITZ2_MEMCACHED_COMMON_H_
