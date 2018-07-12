// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_MSG_H_
#define __BLITZ_MSG_H_ 1
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <utils/atomic.h>
#include "buffer.h"
 namespace blitz {
using blitz::DataBuffer;
using blitz::DataBufferUtil;
typedef void (*FnMsgContentFree)(void* content, void* ctx);
/**
 * 用于表示从字节流decoded后的结构化数据.
 */
struct MsgContent {
  const char* type;
  void* data;//由Decoder自行决定存放什么，比如可以是HttpRequest,HttpResponse
  FnMsgContentFree fn_free;// 用来释放msg. TODO (YuZuo), 去掉该函数指针, 在MsgDecoder中增加接口进行释放
  void* free_ctx;
  uint32_t bytes_length;

};
typedef MsgContent InMsg;

class OutMsg {
 public:
  // 创建实例.由于使用了引用计数，所以必须用new的方式.
  static int CreateInstance(OutMsg** msg);
  static int get_instance_count();
  int AppendData(void* data, size_t length,
             FnMsgContentFree fn_free, void* free_ctx);
  int AppendBuffers(const DataBuffer* buffers, int count);
  // 
  int Detach();
  // 释放所拥有的data所占用的内存，即对所有的DataBuffer调用fn_free.
  int Clear();
  int32_t AddRef();
  int32_t Release();
  int32_t GetRefCount() const { return ref_count_; }
  const DataBuffer* get_buffers() const { return buffers_; }
  int get_buffer_count() const { return buffer_count_; }
  int get_buffer_capacity() const { return buffer_capacity_; }
  int get_msgid() const { return msgid_; }
 private:
  OutMsg();
  ~OutMsg(); 
  OutMsg(const OutMsg&);
  OutMsg& operator=(const OutMsg&);
  int PrepareBuffers(int capacity);
 private:
  int32_t ref_count_;
  DataBuffer* buffers_;
  int buffer_count_;
  int buffer_capacity_;
  int msgid_;
  static int s_instance_count_;
  static int s_next_msgid_;
};
class MsgDecoder {
  public:
    /**
     * 请求存放数据的缓冲区.
     */
    virtual int GetBuffer(void** buf,uint32_t* length) = 0;
    /**
     * 读到数据后，马上汇报.
     * @param buf : in 
     * @param length : [IN] 读取到的数据长度 .
     * @param msg : [OUT]
     * @return 0: 成功读到一枚消息，信息放在msg中.
     *         1: 成功，消息不完整.
     *         other: 失败.
     */
    virtual int Feed(void* buf,uint32_t length,std::vector<InMsg>* msg) = 0;
    virtual ~MsgDecoder() {}
};


class MsgDecoderFactory {
  public:
    virtual MsgDecoder* Create() = 0;
    virtual void Revoke(MsgDecoder* parser) = 0;
    virtual ~MsgDecoderFactory(){}
};

template <typename TDecoder> 
class SimpleMsgDecoderFactory : public MsgDecoderFactory {
  public:
    virtual MsgDecoder* Create()  { 
      return new TDecoder();
    }
    virtual void Revoke(MsgDecoder* parser)  {
      delete parser;
    }

};


// inline function of OutMsg.
inline int OutMsg::CreateInstance(OutMsg** msg) {
  *msg = new OutMsg();
  return 0;
}
inline int OutMsg::get_instance_count() {
  return s_instance_count_;
}
inline int OutMsg::AppendData(void*  data, size_t length,
                          FnMsgContentFree fn_free, void* free_ctx) {
  int ret = PrepareBuffers(1);
  if (ret != 0) { return ret; }
  DataBuffer* buffer = buffers_ + buffer_count_;
  buffer->ptr = reinterpret_cast<uint8_t*>(data);
  buffer->length = length; 
  buffer->data_length = length;
  buffer->read_pos = 0;
  buffer->fn_free = fn_free;
  buffer->free_context = free_ctx;
  ++buffer_count_;
  return 0;
}
inline int OutMsg::AppendBuffers(const DataBuffer* buffers, int count) {
  int ret = PrepareBuffers(count);
  if (ret != 0) { return ret; }
  DataBuffer* buffer = buffers_ + buffer_count_;
  memcpy(buffer, buffers, count*sizeof(*buffer));
  buffer_count_ += count;
  return 0;
}
inline int OutMsg::Detach() {
  buffer_count_ = 0;
  return 0;
}
inline int OutMsg::Clear() {
  for (int i = 0; i < buffer_count_; ++i) {
    DataBufferUtil::Free(buffers_+i);
  }
  buffer_count_ = 0;
  return 0;
}
inline int32_t OutMsg::AddRef() {
  return InterlockedIncrement(&ref_count_);
}
inline int32_t OutMsg::Release() {
 int32_t ref =  InterlockedDecrement(&ref_count_);
 if (ref == 0) { delete this; }
 return ref;
}

inline  OutMsg::OutMsg() {
  ref_count_ = 1;
  buffers_ = NULL;
  buffer_count_ = 0;
  buffer_capacity_ = 0;
  msgid_ = InterlockedAdd(1, &s_next_msgid_);
  if (msgid_ < 0) {
    msgid_ = -msgid_;
  }
  InterlockedIncrement(&s_instance_count_);
}

inline  OutMsg::~OutMsg() {
  Clear();
  if (buffers_) {
    free(buffers_);
  }
  buffer_count_ = 0;
  buffer_capacity_ = 0;
  msgid_ = 0;
  InterlockedDecrement(&s_instance_count_);
}
inline int OutMsg::PrepareBuffers(int capacity) {
  if (buffer_capacity_ >= capacity + buffer_count_) { return 0; }
  int new_capacity = buffer_count_ + capacity;
  DataBuffer* new_buffers = reinterpret_cast<DataBuffer*>(
                               malloc(sizeof(DataBuffer) * new_capacity));
  if (!new_buffers) { return -100; }
  if (buffers_) {
    memcpy(new_buffers, buffers_, sizeof(DataBuffer) * buffer_count_);
    free(buffers_);
  }
  buffers_ = new_buffers;
  buffer_capacity_ = new_capacity;
  return 0;
}
 } // namespace blitz

#endif // __BLITZ_MSG_H_
