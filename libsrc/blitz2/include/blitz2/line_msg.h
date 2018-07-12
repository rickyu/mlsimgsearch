// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo.
#ifndef __BLITZ_LINE_MSG_H_
#define __BLITZ_LINE_MSG_H_ 1
#include <cstring>
#include <string>
#include <vector>
#include <blitz2/ref_object.h>
#include <blitz2/buffer.h>
namespace blitz2 {

/**
 * 收到的line, 包含\n,不是以\0结尾.
 */
class SharedLine {
 public:
  SharedLine(const SharedPtr<SharedBuffer>& mem, char* start, size_t len)
    :mem_(mem) {
      start_ = start;
      length_ = len;
  }
  const char* line() const { return start_; }
  size_t length() const { return length_;}
  size_t offset() const {
    return reinterpret_cast<uint8_t*>(start_) - mem_->mem();
  }
  SharedBuffer* buffer() { return mem_;}
 private:
  SharedPtr<SharedBuffer> mem_;
  char* start_;
  size_t length_;
};
class LineMsgDecoder {
 public:
  typedef SharedLine TMsg;
  typedef size_t TInitParam;
  LineMsgDecoder();
  ~LineMsgDecoder();
  int Init(size_t max_length);
  int GetBuffer(void** buf,uint32_t* length) ;
  template<typename Reporter>
  int Feed(void* buf,uint32_t length, Reporter report) ;
  void Uninit();
  size_t data_length() const { return data_length_;}
  size_t max_length() const {return max_length_;}
  size_t buffer_capacity() const { return buffer_->capacity(); }
  static const size_t kDefaultMaxLineLength ;
 private:
  LineMsgDecoder(const LineMsgDecoder&);
  LineMsgDecoder& operator=(const LineMsgDecoder&);
 private:
  SharedPtr<SharedBuffer> buffer_;
  // char* buffer_;
  size_t max_length_;
  size_t data_length_;
};

inline LineMsgDecoder::LineMsgDecoder() {
  max_length_ = kDefaultMaxLineLength;
  data_length_ = 0;
}
inline int LineMsgDecoder::Init(size_t max_length) {
  max_length_ = max_length;
  buffer_ = SharedBuffer::CreateInstance();
  if (!buffer_->Alloc(max_length_)) {
    return -1;
  }
  return 0;
}

inline void LineMsgDecoder::Uninit() {
  buffer_.Release();
  data_length_ = 0;
}

inline  LineMsgDecoder::~LineMsgDecoder() {
  Uninit();
}
inline int LineMsgDecoder::GetBuffer(void** buf,uint32_t* length) {
  if ( data_length_ >= max_length_) { return -1; }
  *buf = buffer_->mem() + data_length_;
  *length = buffer_->capacity() - data_length_;
  return 0;
}

template<typename Reporter>
int LineMsgDecoder::Feed(void* buf, uint32_t length, Reporter report) {
  assert(buf == buffer_->mem() + data_length_);
  char* prev = reinterpret_cast<char*>(buffer_->mem());
  char* iter = reinterpret_cast<char*>(buf);
  char* end = iter + length;
  size_t reported_line_count = 0;
  while (iter < end) {
    if (*iter == '\n') {
      SharedLine line(buffer_, prev, iter-prev+1);
      report(&line);
      ++reported_line_count;
      ++iter;
      prev = iter;
    } else {
      ++iter;
    }
  }
  if (reported_line_count > 0) {
    SharedPtr<SharedBuffer> new_buffer = SharedBuffer::CreateInstance();
    if (!new_buffer->Alloc(max_length_)) {
      return -2;
    }
    if (prev < end) {
      // 还剩余了未完成的行.
      data_length_ = end - prev;
      memcpy(new_buffer->mem(), prev, data_length_);
    } 
    buffer_ = new_buffer;
  } else {
    data_length_ += length;
    if (data_length_ >= max_length_) {
      return -1;
    }
  }
  return 0;
}

} // namespace blitz.

#endif // __BLITZ_LINE_MSG_H_
