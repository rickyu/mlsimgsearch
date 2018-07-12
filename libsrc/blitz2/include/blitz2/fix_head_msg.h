#ifndef __BLITZ2_FIX_HEAD_MSG_H_
#define __BLITZ2_FIX_HEAD_MSG_H_ 1
#include <cstddef>
#include <memory>
#include <blitz2/ref_object.h>

namespace blitz2 {

class BytesMsg {
 public:
  BytesMsg() {
    data_ = NULL;
    data_len_ = 0;
    buffer_len_ = 0;
  }
  ~BytesMsg() {Clear(); }
  // for SharedObject
  void Init(uint8_t* data, int data_len, int buffer_len) {
    data_ = data;
    data_len_ = data_len;
    buffer_len_ = buffer_len;
  }
  void Clear() {
    if(data_) {delete []data_;}
    data_=NULL;
    data_len_ = 0;
    buffer_len_ = 0;
  }
  uint8_t* data() { return data_; }
  int data_len() { return data_len_;}
  int buffer_len() { return buffer_len_; }
 private:
  uint8_t *data_;
  int data_len_;
  int buffer_len_;
};
template<typename GetBodyLen, uint32_t kHeadLen>
class FixHeadMsgDecoder {
  public:
    typedef GetBodyLen  TGetBodyLenFunctor;
    typedef SharedObject<BytesMsg> TMsg;
    class InitParam {
     public:
      InitParam() { 
        buf_start_len_ = 4096;
        buf_max_len_ = 4*1024*1024;
      }
      InitParam(uint32_t start_len, uint32_t max_len) {
        buf_start_len_ = start_len;
        buf_max_len_ = max_len;
      }
      uint32_t get_buf_start_len() const { return buf_start_len_;}
      void set_buf_start_len(uint32_t start_len) {buf_start_len_ = start_len;}
      uint32_t get_buf_max_len() const { return buf_max_len_;}
      void set_buf_max_len(uint32_t max_len) {buf_max_len_ = max_len;}
     protected:
      uint32_t buf_start_len_;
      uint32_t buf_max_len_;
    };
    typedef InitParam TInitParam;
    
   
    FixHeadMsgDecoder()
    : head_len_(kHeadLen) {
      buffer_ = NULL;
      buffer_len_ = 0;
      data_len_ = 0;
      msg_len_ = 0;
    }
    ~FixHeadMsgDecoder() {
      Uninit();
    }

  private:
    FixHeadMsgDecoder(const FixHeadMsgDecoder&);
    FixHeadMsgDecoder& operator=(const FixHeadMsgDecoder&);
  public:
    int Init(const InitParam& params) {
      params_ = params;
      if(!buffer_) {
        buffer_ = new uint8_t[params_.get_buf_start_len()];
        buffer_len_ = params_.get_buf_start_len();
      }
      return 0;
    }
    int GetBuffer(void** buf,uint32_t* length) ;
    template<typename ReportFunc>
    int Feed(void* buf,uint32_t length, ReportFunc report) ;
    void Uninit() {
      if(buffer_) {
        delete [] buffer_;
        buffer_ = NULL; 
      }
      data_len_ = 0;
      msg_len_ = 0;
    }
  private:
    const uint32_t head_len_;
    TGetBodyLenFunctor fn_get_body_len_;
    uint8_t* buffer_;
    uint32_t buffer_len_;
    uint32_t data_len_;
    uint32_t msg_len_;
    InitParam params_;
};
template<typename GetBodyLen,uint32_t kHeadLen>
int FixHeadMsgDecoder<GetBodyLen,kHeadLen>::GetBuffer(
    void** buf,uint32_t* length)  {
  assert( buffer_ != NULL );
  if(data_len_ < head_len_) {
    *buf = (uint8_t*)buffer_ + data_len_;
    *length = head_len_ - data_len_;
    return 0;
  }

  if(data_len_ < msg_len_) {
    *buf = (uint8_t*)buffer_ + data_len_;
    *length = msg_len_ - data_len_;
    return 0;
  }
  return -1;
}
template<typename GetBodyLen,uint32_t kHeadLen>
template<typename ReportFunc>
int FixHeadMsgDecoder<GetBodyLen, kHeadLen>::Feed(
    void* buf,uint32_t length,ReportFunc report) {
  (void)(buf);
  if (length == 0) { return 1; }
  data_len_ += length;
  if (data_len_ == head_len_) {
    uint32_t body_len = 0;
    if(fn_get_body_len_(buffer_, &body_len) != 0) {
      return -1;
    }
    msg_len_ = body_len + head_len_;
    if (msg_len_ > params_.get_buf_max_len()) {
      return -2;
    }
    if(buffer_len_ < msg_len_) {
      // extend buffer;
      uint8_t* buffer_new = new uint8_t[msg_len_];
      memcpy(buffer_new,buffer_,head_len_);
      delete []buffer_;
      buffer_ = buffer_new;
      buffer_len_ = msg_len_;
    }
    // 如果 body_len == 0,则走到下一步 .
  } else if(data_len_ == msg_len_) {
    SharedPtr<TMsg> msg = TMsg::CreateInstance(buffer_, data_len_, buffer_len_);
    report(msg);
    buffer_ = new uint8_t[params_.get_buf_start_len()];
    buffer_len_ = params_.get_buf_start_len();
    data_len_ = 0;
    msg_len_ = 0;
  } else if(data_len_ > msg_len_) {
    return -3;
  }
  return 0;
}

} // namespace blitz2.

#endif // __BLITZ2_FIX_HEAD_MSG_H_
