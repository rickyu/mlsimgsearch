// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// Memcached Request related class. 
#ifndef __BLITZ2_MEMCACHED_REQ_H_
#define __BLITZ2_MEMCACHED_REQ_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/buffer.h>
#include <blitz2/outmsg.h>
#include <blitz2/logger.h>
#include <blitz2/memcached/memcached_common.h>
namespace blitz2 {

class MemcachedReq {
 public:
  enum EType {
    TYPE_UNKNOWN = 0,
    TYPE_GET,
    TYPE_SET
  };
  MemcachedReq() {type_ = TYPE_UNKNOWN;}
  virtual ~MemcachedReq() {}
  EType type() const { return type_;}
  EType& type() { return type_;}
 protected:
  EType type_;
};
class MemcachedGetReq : public MemcachedReq {
 public:
  MemcachedGetReq() {
    type_ = TYPE_GET; 
    msg_ = OutMsg::CreateInstance();
  }
  virtual ~MemcachedGetReq() { msg_.Release(); }
  void set_key(const char* mem, uint32_t length) {
    key_.Set(mem, length);
  }
  const SimpleString& key() const {
    return key_;
  }
  SharedPtr<OutMsg>& msg() { return msg_; }
  const SharedPtr<OutMsg>& msg() const { return msg_; }
 protected:
  SimpleString key_;
  SharedPtr<OutMsg> msg_;
};
// set <key> <flags> <exptime> <bytes> [noreply]\r\n
// <data>\r\n\r\n
class MemcachedSetReq : public MemcachedReq {
 public:
  MemcachedSetReq() {
    type_ = TYPE_SET; 
    value_ = NULL;
  }
  virtual ~MemcachedSetReq() { buffer_.Release(); }
  void set_key(const char* mem, uint32_t length) {
    key_.assign(mem, length);
  }
  const std::string& key() const { return key_; }
  void set_total(uint32_t total) {total_ = total;}
  void set_bytes(uint32_t bytes) {bytes_ = bytes; }
  void set_exptime(uint32_t exptime) {exptime_ = exptime; }
  void set_flags(int flags) {flags_ = flags; }
  void set_noreply(bool noreply) {noreply_ = noreply; }
  uint32_t bytes() const { return bytes_; }
  uint32_t total() const { return total_; }
  int flags() const { return flags_; }
  bool noreply() const { return noreply_; }
  void set_value(const uint8_t* value) {value_ = value; }
  const uint8_t* value() const { return value_; }
  void set_buffer(SharedBuffer *buffer) { buffer_ = buffer; }
  const SharedBuffer* buffer() const { return buffer_; }
 protected:
  std::string key_;
  int flags_;
  uint32_t bytes_;
  uint32_t total_;
  uint32_t exptime_;
  bool noreply_;
  // SharedPtr<OutMsg> msg_;
  // SharedPtr<OutMsg> value_;
  const uint8_t* value_;
  SharedPtr<SharedBuffer> buffer_;
};
// one request once recved.
// client can't sent multi request at one time.
// client can send multi request only because of set can noreply.
class MemcachedReqDecoder {
 public:
  typedef MemcachedReq TMsg;
  typedef InitParam TInitParam;
  MemcachedReqDecoder():logger_(Logger::GetLogger("blitz2::MemcachedReq")) {
    msg_ = NULL;
    buffer_data_len_ = 0;
    parsed_data_len_ = 0;
    data_offset_  = 0;
    recved_data_len_ = 0;
    req_total_len_ = 0;
    first_line_len_ = 0;
  }
  
  ~MemcachedReqDecoder() {
    Uninit();
  }
  static const size_t kMaxTokenCount = 8;
 public:

  /**
   * decoder init operation such as memory allocation is executed here.
   * @return  0 - success.
   *          other - failed.
   */
  int Init(const InitParam& params) {
    buffer_ = SharedBuffer::CreateInstance();
    buffer_->Alloc(params.buf_start_len());
    params_ = params;
    Reset();
    return 0;
  }
  int GetBuffer(void** buf, uint32_t* length);
  template<typename ReportFunc>
  int Feed(void* buf, uint32_t length, ReportFunc report);
  void Uninit() { 
    if (msg_) {
      delete msg_; 
    }
    msg_ = NULL;
  }
  const std::string& last_error() const { return last_error_;}
 private:
  template<typename ReportFunc>
  int ProcessFirstLine(ReportFunc report);
  template<typename ReportFunc>
  int ProcessGetFirstLine(const Token* tokens, 
      uint32_t count, ReportFunc report);
  template<typename ReportFunc>
  int ProcessSetFirstLine(const Token* tokens, 
      uint32_t count, ReportFunc report);
  template<typename ReportFunc>
  int ProcessGet(MemcachedGetReq* req, ReportFunc report);
  template<typename ReportFunc>
  int ProcessSet(MemcachedSetReq* req, ReportFunc report);
  void Reset();
 private:
  Logger &logger_;
  InitParam params_;
  //< 接收的数据存放在buffer_中.
  SharedPtr<SharedBuffer> buffer_;
  //< buffer_中已有的数据长度.
  uint32_t buffer_data_len_;
  //< 已经分析到位置。
  uint32_t parsed_data_len_;
  //< 当前buffer_中，数据块开始的位置.
  uint32_t data_offset_;
  //< 该request中已经接收到的数据长度.
  uint32_t recved_data_len_;
  //< request总长度.
  uint32_t req_total_len_;
  uint32_t first_line_len_;
  std::string last_error_;
  MemcachedReq* msg_;
};

//const size_t MemcachedReqDecoder::kMaxTokenCount = 8;
inline int MemcachedReqDecoder::GetBuffer(void** buf, uint32_t* length) {
  *buf = buffer_->mem() + buffer_data_len_;
  *length = buffer_->capacity() - buffer_data_len_;
  return 0;
}

template<typename ReportFunc>
inline int MemcachedReqDecoder::Feed(void* buf,uint32_t length, ReportFunc report) {
  (void)(buf);
  if (length == 0) {
    return 0;
  }
  recved_data_len_ += length;
  buffer_data_len_ += length;
  if (msg_ == NULL) {
    char* ptr = reinterpret_cast<char*>(buffer_->mem()) + parsed_data_len_;
    char* end = ptr + length;
    char* eol = MemcachedUtils::SearchEol(ptr, buffer_data_len_ - parsed_data_len_);
    if (!eol) {
      parsed_data_len_ = buffer_data_len_;
      if (*end == '\r') {
        parsed_data_len_ -= 1;
      }
      if (recved_data_len_ >= buffer_->capacity()) {
        // first line too long.
        last_error_ = "first line too long " ;
        return -1;
      }
      return 0;
    } else {
      // we got the first line.
      first_line_len_ = eol - reinterpret_cast<char*>(buffer_->mem()) + 2;
      return ProcessFirstLine(report);
    }
  } else {
    switch(msg_->type()) {
      case MemcachedReq::TYPE_SET:
        return ProcessSet(dynamic_cast<MemcachedSetReq*>(msg_), report);
      // case MemcachedReq::TYPE_GET:
      //   return ProcessGet(dynamic_cast<MemcachedGetReq*>(msg_), report);
      default:
        return -1;
    }
  }
}
template<typename ReportFunc>
inline int MemcachedReqDecoder::ProcessFirstLine(ReportFunc report) {
  char* ptr = reinterpret_cast<char*>(buffer_->mem());
  Token tokens[kMaxTokenCount];
  uint32_t tokens_count = MemcachedUtils::TokenizerCommand(ptr, first_line_len_,
      tokens, kMaxTokenCount);
  if (0 == strncmp(tokens[0].str, "get", strlen("get"))) {
    return ProcessGetFirstLine(tokens, tokens_count, report);
  } else if (0 == strncmp(tokens[0].str, "set", strlen("set"))) {
    return ProcessSetFirstLine(tokens, tokens_count, report);
  } else {
    return -1;
  }
}
template<typename ReportFunc>
inline int MemcachedReqDecoder::ProcessGetFirstLine(const Token* tokens, uint32_t count,
    ReportFunc report) {
  // get <key>*\r\n
  if (first_line_len_ != recved_data_len_) {
    return -1;
  }

  if (count != 2 && count != 3) {
    last_error_ = "get:tokens_count";
    return -1;
  }
  MemcachedGetReq* get_req = new MemcachedGetReq();
  get_req->set_key(tokens[1].str, tokens[1].len);
  get_req->msg()->AppendData(MsgData(buffer_, 0, first_line_len_));
  buffer_ = SharedPtr<SharedBuffer>(0);
  report(get_req);
  delete get_req;
  Reset();
  return 0;
}
template<typename ReportFunc>
inline int MemcachedReqDecoder::ProcessSetFirstLine(const Token* tokens, 
    uint32_t tokens_count, ReportFunc report) {
  using boost::lexical_cast;
  // set <key> <flags> <exptime> <bytes> [noreply]\r\n
  // <data block>\r\n
  if (tokens_count != 5 && tokens_count != 6) {
    last_error_ = "set:tokens_count";
    return -1;
  }
  uint32_t bytes = lexical_cast<uint32_t>(std::string(tokens[4].str, tokens[4].len));
  req_total_len_ = bytes + 2 + first_line_len_;
  if (req_total_len_ > params_.buf_max_len()) {
    last_error_ = "request too long";
    return -1;
  }
  MemcachedSetReq* set_req = new MemcachedSetReq();
  if (tokens_count == 6) {
    if (strncmp(tokens[5].str, "noreply", strlen("noreply"))) {
      set_req->set_noreply(true);
    } else {
      last_error_ = "set:tokens_count";
      return -1;
    }
  }
  set_req->set_total(req_total_len_);
  set_req->set_key(tokens[1].str, tokens[1].len);
  set_req->set_flags(lexical_cast<uint32_t>(std::string(tokens[2].str, tokens[2].len)));
  set_req->set_exptime(lexical_cast<uint32_t>(std::string(tokens[3].str, tokens[3].len)));
  set_req->set_bytes(bytes);
  msg_ = set_req;
  return ProcessSet(set_req, report);
}
template<typename ReportFunc>
inline int MemcachedReqDecoder::ProcessSet(MemcachedSetReq* req, ReportFunc report) {
  if (recved_data_len_ == req_total_len_) {
    // finish one set request.
    req->set_buffer(buffer_);
    req->set_value(buffer_->mem() + first_line_len_);
    buffer_ = SharedPtr<SharedBuffer>(0);
    report(req);
    // req->msg()->AppendData(MsgData(buffer_, 0, buffer_data_len_));
    // req->value()->AppendData(MsgData(buffer_, data_offset_, buffer_data_len_ - 2));
    Reset();
    return 0;
  } else if (recved_data_len_ < req_total_len_) {
    // continue recv.
    if (buffer_->capacity() < req_total_len_) {
      // 缓冲区扩容 
      SharedPtr<SharedBuffer> newbuffer = SharedBuffer::CreateInstance();
      newbuffer->Alloc(req_total_len_);
      memcpy(newbuffer->mem(), buffer_->mem(), buffer_data_len_);
      buffer_->Clear();
      buffer_ = newbuffer;
      // req->msg()->AppendData(MsgData(buffer_, 0, buffer_data_len_));
      // req->value()->AppendData(MsgData(buffer_, data_offset_, buffer_data_len_));
      // buffer_->Alloc(req_total_len_ - recved_data_len_);
      // data_offset_ = 0;
      // buffer_data_len_ = 0;
    }
    return 0;
  } else {
    // client send too much data.
    last_error_ = "request too long";
    return -1;
  }
}
inline void MemcachedReqDecoder::Reset() {
  if (!buffer_) {
    buffer_ = SharedBuffer::CreateInstance();
    buffer_->Alloc(params_.buf_start_len());
  }
  buffer_data_len_ = 0;
  parsed_data_len_ = 0;
  data_offset_  = 0;
  recved_data_len_ = 0;
  req_total_len_ = 0;
  first_line_len_ = 0;
  if (msg_!=NULL) {
    delete msg_;
    msg_ = NULL;
  }
}
} // namespace blitz2.

#endif // __BLITZ2_MEMCACHED_REQ_H_
