// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ2_OUTMSG_H_
#define __BLITZ2_OUTMSG_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/buffer.h>

namespace blitz2 {
class LogId {
 public: 
  LogId() {id1=id2=0;}
  LogId(uint32_t _id1, uint32_t _id2) {id1=_id1; id2=_id2;}
  void set(uint32_t _id1, uint32_t _id2) {id1=_id1; id2=_id2;}
  uint32_t id1;
  uint32_t id2;
};
typedef SharedPtr<SharedBuffer> SharedBufferPtr;
class MsgData {
 public:
  MsgData() {
    offset_ = length_ = 0;
  }
  MsgData(SharedBuffer* buffer, size_t offset, size_t length)
    : buffer_(buffer), offset_(offset), length_(length) {
 }
  const SharedBuffer* buffer() const { return buffer_;}
  const uint8_t* mem() const { return buffer_.get_pointer()->mem();}
  size_t offset() const {return offset_;}
  size_t& offset() {return offset_;}
  size_t length() const { return length_;}
  size_t& length()  { return length_;}
 protected:
  SharedPtr<SharedBuffer> buffer_;
  size_t offset_;
  size_t length_;
};

class NotSharedOutMsg {
 public:
  NotSharedOutMsg();
  virtual ~NotSharedOutMsg(); 
  /**
   * will call data->AddRef().
   */
  int AppendData(const MsgData& data);
  /**
   * send const char*, the data will not free.
   */
  int AppendConstString(const char* str, size_t len);
  int AppendData(void* data, size_t length,
      const Buffer::TDestroyFunc& destroy_func);
  int AppendDataBuffer( DataBuffer& data ) {
    return AppendData( data.mem() + data.offset(), data.length(), 
        data.destroy_functor() );
  }
  // 释放所拥有的data所占用的内存，即对所有的DataBuffer调用fn_free.
  void Clear();
  const std::vector<MsgData>& data_vec() const { return data_vec_;}
  size_t data_vec_count() const { return data_vec_.size(); }
  size_t length() const { return length_; }
  LogId& logid() { return logid_;}
  const LogId& logid() const { return logid_; }
  /**
   * convert to iovec array.
   * @param vec: vec array, caller must ensure vec size larger than data_vec_.
   * @param total_data_len, vec中的数据长度.
   * @return iovec count.
   */
  size_t Convert2iovec(iovec* vec);
 protected:
  void Init() {}
 protected:
  std::vector<MsgData> data_vec_;
  size_t length_;
  LogId logid_;
};
inline int NotSharedOutMsg::AppendData(const MsgData& data) {
  data_vec_.push_back(data);
  length_ += data.length();
  return 0;
}

inline int NotSharedOutMsg::AppendConstString(const char* str, size_t length) {
  return AppendData(const_cast<char*>(str), length, Buffer::DestroyNothingImpl);
}
inline int NotSharedOutMsg::AppendData(void* data, size_t length,
      const Buffer::TDestroyFunc& destroy_func) {
  SharedPtr<SharedBuffer> buffer = SharedBuffer::CreateInstance();
  buffer->Attach((uint8_t*)data, length, destroy_func);
  return AppendData(MsgData(buffer, 0, length));
}
inline void NotSharedOutMsg::Clear() {
  data_vec_.clear();
  length_ = 0;
}

inline  NotSharedOutMsg::NotSharedOutMsg() {
  data_vec_.reserve(8);
  length_ = 0;
}
inline  NotSharedOutMsg::~NotSharedOutMsg() {
  Clear();
}
typedef SharedObject<NotSharedOutMsg> OutMsg;

} // namespace blitz2

#endif // __BLITZ2_OUTMSG_H_
