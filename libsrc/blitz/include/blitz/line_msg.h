// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo.
#ifndef __BLITZ_LINE_MSG_H_
#define __BLITZ_LINE_MSG_H_ 1
#include <vector>
#include "msg.h"
namespace blitz {
class LineMsgDecoder : public MsgDecoder {
 public:
   LineMsgDecoder();
  explicit LineMsgDecoder(int capacity);
  ~LineMsgDecoder();
  virtual int GetBuffer(void** buf,uint32_t* length) ;
  virtual int Feed(void* buf,uint32_t length,std::vector<InMsg>* msg) ;
  static const int kDefaultMaxLineLength ;
  const uint8_t* get_buffer() const { return buffer_; }
  int get_max_length() const  { return max_length_; }
  int get_data_length() const { return data_length_; }
 private:
  LineMsgDecoder(const LineMsgDecoder&);
  LineMsgDecoder& operator=(const LineMsgDecoder&);
  int PrepareBuffer();
 private:
  uint8_t* buffer_;
  int max_length_;
  int data_length_;
};

inline LineMsgDecoder::LineMsgDecoder() {
  buffer_ = NULL;
  max_length_ = kDefaultMaxLineLength;
  data_length_ = 0;
}
inline LineMsgDecoder::LineMsgDecoder(int max_length) {
  assert(max_length > 0);
  buffer_ = NULL;
  max_length_ = max_length;
  data_length_ = 0;
}
inline  LineMsgDecoder::~LineMsgDecoder() {
  if (buffer_) {
    Alloc::Free(buffer_, NULL);
  }
}
} // namespace blitz.

#endif // __BLITZ_LINE_MSG_H_
