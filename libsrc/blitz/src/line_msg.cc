#include "blitz/line_msg.h"
namespace blitz {
const int LineMsgDecoder::kDefaultMaxLineLength = 80;
int LineMsgDecoder::GetBuffer(void** buf,uint32_t* length) {
  int ret = PrepareBuffer();
  if (ret != 0) { return ret; }
  if ( data_length_ >= max_length_) { return -1; }
  *buf = reinterpret_cast<char*>(buffer_) + data_length_;
  *length = max_length_ - data_length_;
  return 0;
}
static int FillInMsg(InMsg* msg, uint8_t* start, uint8_t* end) {
  msg->type = "line";
  msg->data = start;
  msg->fn_free = Alloc::Free;
  msg->free_ctx = NULL;
  msg->bytes_length = end - start + 1;
  return 0;
}
static int AllocInMsg(InMsg* msg, uint8_t* start, uint8_t* end) {
  msg->type = "line";
  msg->bytes_length = end - start + 1;
  msg->data = Alloc::Malloc(msg->bytes_length+1);
  if (!msg->data) {
    return -100;
  }
  msg->fn_free = Alloc::Free;
  msg->free_ctx = NULL;
  memcpy(msg->data, start, msg->bytes_length);
  reinterpret_cast<char*>(msg->data)[msg->bytes_length] = '\0';
  return 0;
}

int LineMsgDecoder::Feed(void* buf, uint32_t length, std::vector<InMsg>* msgs) {
  assert(buf == buffer_ + data_length_);
  assert(msgs != NULL);
  printf("recvd %.*s\n", length, reinterpret_cast<char*>(buf));
  uint8_t* prev = buffer_;
  uint8_t* iter = reinterpret_cast<uint8_t*>(buf);
  uint8_t* end = iter + length;
  // 处理第一行，行首是ptr_.
  while (iter < end) {
    if (*iter == '\n') {
      InMsg msg;
      FillInMsg(&msg, prev, iter);
      msgs->push_back(msg);
      ++iter;
      prev = iter;
      break;
    } else {
      ++iter;
    }
  }
  // 处理一次接收多行数据的情况.
  while (iter < end) {
    if (*iter == '\n') {
      InMsg msg;
      AllocInMsg(&msg, prev, iter);
      msgs->push_back(msg);
      ++iter;
      prev = iter;
    } else {
      ++iter;
    }
  }
  // 判断是否还剩余数据,如果最后一个字母是'\n', 那prev==end.
  if (prev < end) {
    if (prev == buffer_) {
      data_length_ += length;
      if (data_length_ >= max_length_) {
        return -1;
      }
    } else {
      buffer_ = NULL;
      data_length_ = 0;
      InMsg* first_msg = &(*msgs)[0];
      reinterpret_cast<char*>(first_msg->data)[first_msg->bytes_length] = '\0';
      int ret = PrepareBuffer();
      if (ret != 0) { return ret; }
      data_length_ = end - prev;
      memcpy(buffer_, prev, data_length_);
    }
  } else {
    buffer_  = NULL;
    data_length_ = 0;
  }
  return 0;
}

int LineMsgDecoder::PrepareBuffer() {
  if (!buffer_) {
    buffer_ = reinterpret_cast<uint8_t*>(Alloc::Malloc(max_length_+1));
    if (!buffer_) { return -100; }
  } 
  return 0;
}
} // namespace blitz.
