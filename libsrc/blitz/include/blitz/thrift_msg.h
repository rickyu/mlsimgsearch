// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_THRIFT_MSG_H_
#define __BLITZ_THRIFT_MSG_H_ 1
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <string>
#include <TProcessor.h>
#include <transport/TTransport.h>
#include "utils/logger.h"
#include "fix_head_msg.h"
namespace blitz {
// thrift framed消息解码器支持,  对应的是TFramedTransport,
// 前面有4个字节的字段说明包长.
class ThriftFramedMsgFactory : public MsgDecoderFactory {
 public:
  virtual MsgDecoder* Create()  {
    return new FixHeadMsgDecoder(4,FnGetBodyLen);
  }
  virtual void Revoke(MsgDecoder* parser) {
    delete parser;
  }
  static int FnGetBodyLen(void* data,uint32_t* body_len){
    *body_len = ntohl(*(uint32_t*)data);
    return 0;
  }
};
// 内存InputTransport.
class ThriftMemoryInTransport : public apache::thrift::transport::TTransport {
 public:
  ThriftMemoryInTransport(uint8_t* data,uint32_t data_len){
    data_ = data;
    data_len_ = data_len;
    pos_ = data_;
    data_end_ = data_ + data_len_;
  }
  virtual uint32_t read_virt(uint8_t* buf,uint32_t len){
    uint32_t left = data_end_ - pos_;
    if(left >= len){
      memcpy(buf,pos_,len);
      pos_ += len;
      return len;
    }
    memcpy(buf,pos_,left);
    pos_ += left;
    return left;
    
  }
  virtual uint32_t readAll_virt(uint8_t* buf,uint32_t len){
    static CLogger& g_log_framework = CLogger::GetInstance("framework");
    uint32_t left = data_end_ - pos_;
    if(left >= len){
      memcpy(buf,pos_,len);
      pos_ += len;
      return len;
    }
    else {
      LOGGER_ERROR(g_log_framework, "readAll err=NotEnoughData left=%u len=%u",
                  left, len);
      throw std::string("no data");
    }
  }

  uint8_t* data_;
  uint32_t data_len_;
  uint8_t* pos_;
  uint8_t* data_end_;
};
// 内存OutputTransport,直接输出到内存，保留4个字节用于存长度信息.
class ThriftMemoryOutTransport : public apache::thrift::transport::TTransport {
 public:
  ThriftMemoryOutTransport(){
    buffer_ = NULL;
    length_ = 0;
    data_len_ = 0;
  }
  ~ThriftMemoryOutTransport(){
    if(buffer_){
      free(buffer_);
      buffer_ = NULL;
    }
  }
  void write_virt(const uint8_t* buffer,uint32_t len){
    if(data_len_ + 4 + len  < length_){
      memcpy(buffer_ + data_len_ + 4,buffer,len);
      data_len_ += len;
    }
    else {
      Extend(len);
      memcpy(buffer_ + data_len_ + 4,buffer,len);
      data_len_ += len;
    }
  }
  void flush(){
    *(uint32_t*)buffer_ = htonl(data_len_);
  }
  void Detach(uint8_t** buffer,uint32_t* data_len){
    *buffer = buffer_;
    *data_len = data_len_ + 4;
    buffer_ = NULL;
    length_ = 0;
    data_len_ = 0;
  }
  static void FreeBuffer(void* buffer,void* ){
    if(buffer){
      free(buffer);
    }
  }

  void Extend(uint32_t len){
    uint32_t total_len = data_len_ + len + 4;
    length_ = (total_len/4096+1)*4096;
    uint8_t* buffer =(uint8_t*) malloc((total_len/4096 + 1) * 4096);
    if(buffer_){
      memcpy(buffer + 4,buffer_ + 4,data_len_);
      free(buffer_);
    }
    buffer_ = buffer;
  }
  uint8_t* buffer_;
  uint32_t length_;
  uint32_t data_len_;
};

} // namespace blitz.
#endif // __BLITZ_THRIFT_MSG_H_
