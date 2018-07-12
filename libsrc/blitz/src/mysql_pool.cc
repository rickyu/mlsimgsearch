#include <iostream>
#include "utils/logger.h"
#include "blitz/mysql_pool.h"
#include "blitz/framework.h"
#include <my_global.h>
namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("mysql");


MysqlReplyDecoder::~MysqlReplyDecoder() {
  if(buffer_){
    free(buffer_);
    buffer_ = NULL;
  }
}
const uint32_t kMysqlMaxPacketLen = 256L*256L*256L-1;
int MysqlReplyDecoder::PrepareBuffer(uint32_t hint) {
  if(!buffer_){
    buffer_len_ = ((hint - 1)/4096 + 1) * 4096;
    buffer_ = (uint8_t*)malloc(buffer_len_);
    return 0;
  }
  else {
    if(buffer_len_ >= data_len_ + hint){
      return 0;
    }
    uint32_t new_buffer_len = ((hint - 1)/4096 + 1) * 4096 + buffer_len_;
    uint8_t* new_buffer = (uint8_t*)malloc(new_buffer_len);
    memcpy(new_buffer,buffer_,data_len_);
    free(buffer_);
    buffer_ = new_buffer;
    buffer_len_ = new_buffer_len;
    return 0;
  }
}
int32_t MysqlReplyDecoder::GetBuffer(void** buf,uint32_t* length)  {
  PrepareBuffer(remain_);
  *buf = (uint8_t*)buffer_ + data_len_;
  *length = remain_;
  return 0;
}
int32_t MysqlReplyDecoder::Feed(void* buf,uint32_t length, std::vector<InMsg>* msgs) {
  AVOID_UNUSED_VAR_WARN(buf);
  if(length == 0){
    return 1;
  }
  if(length > remain_){
    return -1;
  }
  data_len_ += length;
  remain_ -= length;
  switch(status_){
    case Status::kReadHead:
      if(remain_ == 0){
        status_ = kReadBody;
        reading_packet_len_  = uint3korr(buffer_ + data_len_);
        remain_ = reading_packet_len_;
        data_len_ -= 4;
        return 1;
      }
      else {
        return 1;
      }
      break;
    case Status::kReadBody:
      if(remain_ == 0){
        if(reading_packet_len_ == kMysqlMaxPacketLen){
          remain_ = 4;
          status_ = Status::kReadHead;
          return 1;
        }
        else {
          InMsg msg;
          msg.type = "mysql";
          msg.bytes_length = data_len_;
          msg.data = buffer_;
          msg.fn_free = FreeData;
          msg.free_ctx = NULL;
          msgs->push_back(msg);
          buffer_ = NULL;
          buffer_len_ = 0;
          data_len_ = 0;
          remain_ = 4;
          status_ = Status::kReadHead;
          return 0;
        }
      }
      break;
  }
  return 0;
}
int MysqlConnect::Init(const MysqlNode& node){
  node_ = node;
  int ret = framework_.Connect(node_.addr, &decoder_factory_, this, this, &io_id_);
  status_ = MysqlConnectStatus::kConnecting;
  if (ret == 0 && IoId::IsValid(io_id_)) {
    return 0;
  }
  return -1;
}
int MysqlConnect::OnOpened(uint64_t io_id,const char* id,const char* addr) {
  AVOID_UNUSED_VAR_WARN(io_id);
  AVOID_UNUSED_VAR_WARN(id);
  AVOID_UNUSED_VAR_WARN(addr);
  status_ = MysqlConnectStatus::kConnected;
  return 0;
}
int MysqlConnect::OnClosed(uint64_t io_id,const char* id,const char* addr) {
  AVOID_UNUSED_VAR_WARN(io_id);
  AVOID_UNUSED_VAR_WARN(id);
  AVOID_UNUSED_VAR_WARN(addr);
  return 0;
}
int MysqlConnect::ProcessMsg(uint64_t io_id,const InMsg& msg) {
  AVOID_UNUSED_VAR_WARN(io_id);
  switch(status_) {
    case MysqlConnectStatus::kConnected:
      greeting_.Parse((const uint8_t*)msg.data,msg.bytes_length);
      msg.fn_free(msg.data,msg.free_ctx);
      //发送客户端的第一个包.
      //framework_.Send();
      break;
    default:
      assert(0);
      break;
  }
  return 0;
}
} // namespace blitz.
