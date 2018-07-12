// Copyright 2013 meilishuo Inc.
// Author: Lichao Wang.
#ifndef __BLITZ_MEMCACHED_SERVER_MSG_H_
#define __BLITZ_MEMCACHED_SERVER_MSG_H_ 1

#include <vector>
#include "msg.h"

namespace blitz {
class MemcachedServerMsgDecoder : public MsgDecoder {
 public:
   MemcachedServerMsgDecoder();
  explicit MemcachedServerMsgDecoder(int capacity);
  ~MemcachedServerMsgDecoder();
  //从缓存区中拿出数据
  virtual int GetBuffer(void** buf,uint32_t* length) ;
  //将数据放到msg中
  virtual int Feed(void* buf,uint32_t length,std::vector<InMsg>* msg);
  //定义最大接收数据长度
  static const int kDefaultMaxMemcachedServerLength ;
  const uint8_t* get_buffer() const { return buffer_; }
  int get_max_length() const  { return max_length_; }
  int get_data_length() const { return data_length_; }

 private:
  MemcachedServerMsgDecoder(const MemcachedServerMsgDecoder&);
  MemcachedServerMsgDecoder& operator=(const MemcachedServerMsgDecoder&);
  int PrepareBuffer();

 private:
  uint8_t* buffer_;
  int max_length_;
  int data_length_;
  int body_len_;
  int waited_;
};

inline MemcachedServerMsgDecoder::MemcachedServerMsgDecoder() {
  buffer_ = NULL;
  max_length_ = kDefaultMaxMemcachedServerLength;
  data_length_ = 0;
}

inline MemcachedServerMsgDecoder::MemcachedServerMsgDecoder(int max_length) {
  assert(max_length > 0);
  buffer_ = NULL;
  max_length_ = max_length;
  data_length_ = 0;
  body_len_ = 0;
  waited_ = 0;
}

inline MemcachedServerMsgDecoder::~MemcachedServerMsgDecoder() {
  if (buffer_) {
    Alloc::Free(buffer_, NULL);
  }
}

class MemcachedServerMsgDecoderFactory : public MsgDecoderFactory {
 public:
  virtual MsgDecoder* Create()  {
    return new MemcachedServerMsgDecoder();
  }

  virtual void Revoke(MsgDecoder* parser) {
    if (parser) delete parser;
  }
};

} // namespace blitz.

#endif 
