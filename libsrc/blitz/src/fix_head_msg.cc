#include <stdlib.h>
#include <string.h>
#include "blitz/fix_head_msg.h"
namespace blitz {
FixHeadMsgDecoder::~FixHeadMsgDecoder(){
  if(buffer_){
    free(buffer_);
    buffer_ = NULL;
  }
}
int32_t FixHeadMsgDecoder::GetBuffer(void** buf,uint32_t* length)  {
  if (!buffer_) {
    buffer_ = malloc(1024);
    buffer_len_ = 1024;
  }
  if(data_len_ < head_len_){
    *buf = (uint8_t*)buffer_ + data_len_;
    *length = head_len_ - data_len_;
    return 0;
  }

  if(data_len_ < msg_len_){
    *buf = (uint8_t*)buffer_ + data_len_;
    *length = msg_len_ - data_len_;
    return 0;
  }
  return -1;
}
int32_t FixHeadMsgDecoder::Feed(void* buf,uint32_t length,std::vector<InMsg>* msgs) {
  AVOID_UNUSED_VAR_WARN(buf);
  if (length == 0) {
    return 1;
  }
  data_len_ += length;
  if (data_len_ == head_len_) {
    uint32_t body_len = 0;
    if(fn_get_body_len_(buffer_, &body_len) != 0){
      return -1;
    }
    msg_len_ = body_len + head_len_;
    if (msg_len_ > 256*1024*1024) {
      return -2;
    }
    if(buffer_len_ < msg_len_){
      void* buffer_new = malloc(msg_len_);
      memcpy(buffer_new,buffer_,head_len_);
      free(buffer_);
      buffer_ = buffer_new;
      buffer_len_ = msg_len_;
    }
    // 如果 body_len == 0,则走到下一步 .
  }
  if(data_len_ == msg_len_){
    InMsg msg;
    msg.type = "fix_head_msg";
    msg.bytes_length = msg_len_;
    msg.data = buffer_;
    msg.fn_free = FreeData;
    msg.free_ctx = NULL;
    msgs->push_back(msg);
    buffer_ = NULL;
    buffer_len_ = 0;
    data_len_ = 0;
    msg_len_ = 0;
    return 0;
  }
  if(data_len_ > msg_len_){
    return -3;
  }
  return 1;
}
} // namespace blitz.
