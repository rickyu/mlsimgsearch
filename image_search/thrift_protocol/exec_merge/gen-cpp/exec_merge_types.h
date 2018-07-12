/**
 * Autogenerated by Thrift Compiler (0.7.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#ifndef exec_merge_TYPES_H
#define exec_merge_TYPES_H

#include <Thrift.h>
#include <TApplicationException.h>
#include <protocol/TProtocol.h>
#include <transport/TTransport.h>





typedef struct _TBOFQuantizeResult__isset {
  _TBOFQuantizeResult__isset() : q(false), word_info(false) {}
  bool q;
  bool word_info;
} _TBOFQuantizeResult__isset;

class TBOFQuantizeResult {
 public:

  static const char* ascii_fingerprint; // = "87131FDCF88ADB8D54FE4C4CAE331DBD";
  static const uint8_t binary_fingerprint[16]; // = {0x87,0x13,0x1F,0xDC,0xF8,0x8A,0xDB,0x8D,0x54,0xFE,0x4C,0x4C,0xAE,0x33,0x1D,0xBD};

  TBOFQuantizeResult() : q(0) {
  }

  virtual ~TBOFQuantizeResult() throw() {}

  double q;
  std::vector<std::string>  word_info;

  _TBOFQuantizeResult__isset __isset;

  void __set_q(const double val) {
    q = val;
  }

  void __set_word_info(const std::vector<std::string> & val) {
    word_info = val;
  }

  bool operator == (const TBOFQuantizeResult & rhs) const
  {
    if (!(q == rhs.q))
      return false;
    if (!(word_info == rhs.word_info))
      return false;
    return true;
  }
  bool operator != (const TBOFQuantizeResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TBOFQuantizeResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _ExecMergeRequest__isset {
  _ExecMergeRequest__isset() : img_qdata(false) {}
  bool img_qdata;
} _ExecMergeRequest__isset;

class ExecMergeRequest {
 public:

  static const char* ascii_fingerprint; // = "4FDC91E31793E434ABC36BDFE3689D04";
  static const uint8_t binary_fingerprint[16]; // = {0x4F,0xDC,0x91,0xE3,0x17,0x93,0xE4,0x34,0xAB,0xC3,0x6B,0xDF,0xE3,0x68,0x9D,0x04};

  ExecMergeRequest() {
  }

  virtual ~ExecMergeRequest() throw() {}

  std::map<int64_t, TBOFQuantizeResult>  img_qdata;

  _ExecMergeRequest__isset __isset;

  void __set_img_qdata(const std::map<int64_t, TBOFQuantizeResult> & val) {
    img_qdata = val;
  }

  bool operator == (const ExecMergeRequest & rhs) const
  {
    if (!(img_qdata == rhs.img_qdata))
      return false;
    return true;
  }
  bool operator != (const ExecMergeRequest &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ExecMergeRequest & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _ExecMergeResponse__isset {
  _ExecMergeResponse__isset() : ret_num(false), result(false), result_len(false), query_pow(false) {}
  bool ret_num;
  bool result;
  bool result_len;
  bool query_pow;
} _ExecMergeResponse__isset;

class ExecMergeResponse {
 public:

  static const char* ascii_fingerprint; // = "E2BE2A55C86F2F05EC286F12668E31FA";
  static const uint8_t binary_fingerprint[16]; // = {0xE2,0xBE,0x2A,0x55,0xC8,0x6F,0x2F,0x05,0xEC,0x28,0x6F,0x12,0x66,0x8E,0x31,0xFA};

  ExecMergeResponse() : ret_num(0), result(""), result_len(0), query_pow(0) {
  }

  virtual ~ExecMergeResponse() throw() {}

  int32_t ret_num;
  std::string result;
  int32_t result_len;
  double query_pow;

  _ExecMergeResponse__isset __isset;

  void __set_ret_num(const int32_t val) {
    ret_num = val;
  }

  void __set_result(const std::string& val) {
    result = val;
  }

  void __set_result_len(const int32_t val) {
    result_len = val;
  }

  void __set_query_pow(const double val) {
    query_pow = val;
  }

  bool operator == (const ExecMergeResponse & rhs) const
  {
    if (!(ret_num == rhs.ret_num))
      return false;
    if (!(result == rhs.result))
      return false;
    if (!(result_len == rhs.result_len))
      return false;
    if (!(query_pow == rhs.query_pow))
      return false;
    return true;
  }
  bool operator != (const ExecMergeResponse &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ExecMergeResponse & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};



#endif
