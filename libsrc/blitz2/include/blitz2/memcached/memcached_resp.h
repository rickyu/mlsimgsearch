// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// MemcachedResp related class. 
#ifndef __BLITZ2_MEMCACHED_RESP_H_
#define __BLITZ2_MEMCACHED_RESP_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/logger.h>
#include <blitz2/outmsg.h>
#include <blitz2/buffer.h>
#include <blitz2/memcached/memcached_common.h>
namespace blitz2 {

class MemcachedResp {
 public:
  enum EType {
    TYPE_UNKNOWN = 0,
    TYPE_BUSY,
    TYPE_BADCLASS,
    TYPE_CLIENT_ERROR,
    TYPE_DELETED,
    TYPE_END,
    TYPE_ERROR,
    TYPE_EXISTS,
    TYPE_NOSPARE,
    TYPE_NOTFULL,
    TYPE_NOT_FOUND,
    TYPE_NOT_STORED,
    TYPE_OK,
    TYPE_SAME,
    TYPE_STORED,
    TYPE_STAT,
    TYPE_SERVER_ERROR,
    TYPE_TOUCHED,
    TYPE_UNSAFE,
    TYPE_VALUE,
    TYPE_VERSION
  };
  MemcachedResp() {type_ = TYPE_UNKNOWN;}
  virtual ~MemcachedResp() {msg_.Release();}
  MemcachedResp(EType type) {
    type_ = type;
    msg_ = OutMsg::CreateInstance();
  }
  SharedPtr<OutMsg>& msg() { return msg_; }
  const SharedPtr<OutMsg>& msg() const { return msg_; }
  EType type() const { return type_;}
  EType& type() { return type_;}
 protected:
  EType type_;
  SharedPtr<OutMsg> msg_;
};
// one reply once recved.
class MemcachedRespDecoder {
 public:
  typedef MemcachedResp TMsg;
  typedef InitParam TInitParam;
  MemcachedRespDecoder():logger_(Logger::GetLogger("blitz2::MemcachedResp")) {
    msg_ = NULL;
    buffer_data_len_ = 0;
    parsed_data_len_ = 0;
    data_offset_  = 0;
    recved_data_len_ = 0;
    req_total_len_ = 0;
    first_line_len_ = 0;
  }
  
  ~MemcachedRespDecoder() {
    Uninit();
  }
  static const size_t kMaxTokenCount = 7;
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
  int ProcessGet(MemcachedResp* resp, ReportFunc report);
  template<typename ReportFunc>
  int ProcessSingleFirstLine(MemcachedResp::EType type, ReportFunc report);
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
  MemcachedResp* msg_;
};

// const size_t MemcachedRespDecoder::kMaxTokenCount = 7;
inline int MemcachedRespDecoder::GetBuffer(void** buf, uint32_t* length) {
  *buf = buffer_->mem() + buffer_data_len_;
  *length = buffer_->capacity() - buffer_data_len_;
  return 0;
}
template<typename ReportFunc>
inline int MemcachedRespDecoder::Feed(void* buf,uint32_t length, ReportFunc report) {
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
      case MemcachedResp::TYPE_VALUE:
        return ProcessGet(dynamic_cast<MemcachedResp*>(msg_), report);
      default:
        return -1;
    }
  }
}
template<typename ReportFunc>
inline int MemcachedRespDecoder::ProcessFirstLine(ReportFunc report) {
  char* ptr = reinterpret_cast<char*>(buffer_->mem());
  Token tokens[kMaxTokenCount];
  uint32_t tokens_count = MemcachedUtils::TokenizerCommand(ptr, first_line_len_,
      tokens, kMaxTokenCount);
  // 通过第一个TOKEN判断回报,先判断get、set包
  if (0 == strncmp(tokens[0].str, "ERROR", strlen("ERROR"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_ERROR, report);
  } else if (0 == strncmp(tokens[0].str, "VALUE", strlen("VALUE"))) {
    return ProcessGetFirstLine(tokens, tokens_count, report);
  } else if (0 == strncmp(tokens[0].str, "STORED", strlen("STORED"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_STORED, report);
  } else if (0 == strncmp(tokens[0].str, "END", strlen("END"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_END, report);
  } else if ( 0 == strncmp(tokens[0].str, "SERVER_ERROR", strlen("SERVER_ERROR"))) {
    // SERVER_ERROR <error>\r\n
    return ProcessSingleFirstLine(MemcachedResp::TYPE_SERVER_ERROR, report);
  } else if (0 == strncmp(tokens[0].str, "CLIENT_ERROR", strlen("CLIENT_ERROR"))) {
    // CLIENT_ERROR <error>\r\n
    return ProcessSingleFirstLine(MemcachedResp::TYPE_CLIENT_ERROR, report);
  } else if (0 == strncmp(tokens[0].str, "SAME", strlen("SAME"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_SAME, report);
  } else if (0 == strncmp(tokens[0].str, "NOT_STORED", strlen("NOT_STORED"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_NOT_STORED, report);
  } else if (0 == strncmp(tokens[0].str, "NOSPARE", strlen("NOSPARE"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_NOSPARE, report);
  } else if (0 == strncmp(tokens[0].str, "NOTFULL", strlen("NOTFULL"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_NOTFULL, report);
  } else if (0 == strncmp(tokens[0].str, "EXISTS", strlen("EXISTS"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_EXISTS, report);
  } else if (0 == strncmp(tokens[0].str, "NOT_FOUND", strlen("NOT_FOUND"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_NOT_FOUND, report);
  } else if (0 == strncmp(tokens[0].str, "DELETED", strlen("DELETED"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_DELETED, report);
  } else if (0 == strncmp(tokens[0].str, "TOUCHED", strlen("TOUCHED"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_TOUCHED, report);
  } else if (0 == strncmp(tokens[0].str, "BADCLASS", strlen("BADCLASS"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_BADCLASS, report);
  } else if (0 == strncmp(tokens[0].str, "BUSY", strlen("BUSY"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_BUSY, report);
  } else if (0 == strncmp(tokens[0].str, "OK", strlen("OK"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_OK, report);
  } else if (0 == strncmp(tokens[0].str, "UNSAFE", strlen("UNSAFE"))) {
    return ProcessSingleFirstLine(MemcachedResp::TYPE_UNSAFE, report);
  } else if (0 == strncmp(tokens[0].str, "VERSION", strlen("VERSION"))) {
    // VERSION <version>\r\n
    return ProcessSingleFirstLine(MemcachedResp::TYPE_VERSION, report);
  } else if (0 == strncmp(tokens[0].str, "STAT", strlen("STAT"))) {
    // STAT items:<slabclass>:<stat> <value>\r\n
    // STAT items:<slabclass>:<stat> <value>\r\n
    // END\r\n
    return -1; 
  } else {
    return -1;
  }
}
template<typename ReportFunc>
inline int MemcachedRespDecoder::ProcessSingleFirstLine(MemcachedResp::EType type, 
    ReportFunc report) {
  if (first_line_len_ != recved_data_len_) {
    return -1;
  }
  MemcachedResp* resp = new MemcachedResp(type);
  resp->msg()->AppendData(MsgData(buffer_, 0, first_line_len_));
  buffer_ = SharedPtr<SharedBuffer>(0);
  report(resp);
  delete resp;
  Reset();
  return 0;
}
template<typename ReportFunc>
inline int MemcachedRespDecoder::ProcessGetFirstLine(const Token* tokens, 
    uint32_t tokens_count, ReportFunc report) {
  using boost::lexical_cast;
  // VALUE <key> <flags> <bytes>\r\n
  // <data block>\r\n
  // END\r\n
  if (tokens_count != 4) {
    last_error_ = "get:tokens_count";
    return -1;
  }
  uint32_t bytes = lexical_cast<uint32_t>(std::string(tokens[3].str, tokens[3].len));
  req_total_len_ = first_line_len_ + bytes + 2 +  5;
  if (req_total_len_ > params_.buf_max_len()) {
    last_error_ = "request too long";
    return -1;
  }
  MemcachedResp* resp = new MemcachedResp(MemcachedResp::TYPE_VALUE);
  msg_ = resp;
  return ProcessGet(resp, report);
}
template<typename ReportFunc>
inline int MemcachedRespDecoder::ProcessGet(MemcachedResp* resp, ReportFunc report) {
  if (recved_data_len_ == req_total_len_) {
    // finish one get reply.
    resp->msg()->AppendData(MsgData(buffer_, 0, buffer_data_len_));
    buffer_ = SharedPtr<SharedBuffer>(0);
    report(resp);
    Reset();
    return 0;
  } else if (recved_data_len_ < req_total_len_) {
    // continue recv.
    if (buffer_->capacity() < req_total_len_) {
      resp->msg()->AppendData(MsgData(buffer_, 0, buffer_data_len_));
      buffer_ = SharedBuffer::CreateInstance();
      buffer_->Alloc(req_total_len_ - recved_data_len_);
      data_offset_ = 0;
      buffer_data_len_ = 0;
    }
    return 0;
  } else {
    // server send too much data.
    last_error_ = "request too long";
    return -1;
  }
}
inline void MemcachedRespDecoder::Reset() {
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

#endif // __BLITZ2_MEMCACHED_RESP_H_
