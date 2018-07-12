// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// Http协议编码和解码. 

#ifndef __BLITZ_HTTP_COMMON_H_
#define __BLITZ_HTTP_COMMON_H_ 1
#include <unordered_map>
#include "framework.h"
#include "msg.h"
#include "utils/compile_aux.h"
namespace blitz {
 
class Url {
 public:
  int Parse(const char* url);
  const std::string& get_protocol() const {return protocol_;}
  const std::string& get_host() const { return host_; }
  const std::string& get_uri() const {return uri_; }

 private:
  std::string protocol_;
  std::string host_;
  std::string uri_;
};
class HttpMsgBase {
 public:
  HttpMsgBase() {
    content_length_ = 0;
    version_minor_ = 1;
  }
  virtual ~HttpMsgBase() {
  }
  void SetHeader(const char* key, const char* value) {
    params_[key] = value;
  }
  const char* GetHeader(const char* key) {
    TParamMapConstIter iter = params_.find(key);
    if (iter == params_.end()) {
      return NULL;
    }
    return iter->second.c_str();
  }
  void SetContent(const char* content) {
    int len = strlen(content);
    char* str = new char[len+1];
    strcpy(str, content);
    content_ = std::shared_ptr<char>(str, Deleter());
    content_length_ = len;
  }
  void set_version_minor( int v ) { version_minor_ = v;}
  int get_version_minor( )  const { return version_minor_ ;}
  int get_content_length() const { return content_length_; }
  void set_content_length(int len) { content_length_ = len; }
  void set_content(const std::shared_ptr<char>& content) { content_ = content; }
  const char* get_content() const {return content_.get(); }
  virtual std::ostream& OutputDebug(std::ostream& os);

 protected:
  typedef std::unordered_map<std::string, std::string> TParamMap;
  typedef TParamMap::iterator TParamMapIter;
  typedef TParamMap::const_iterator TParamMapConstIter;
  struct Deleter {
    void operator() (void* p) {
      char* str = reinterpret_cast<char*>(p);
      delete [] str;
    }
  };
 protected:
  int version_minor_;
  int content_length_;
  std::shared_ptr<char> content_;
  TParamMap params_;
};
class HttpRequest : public HttpMsgBase {
 public:
  enum Method {
    kGet = 1,
    kPost = 2

  };
  HttpRequest() {
    content_length_ = 0;
  }
  const std::string& get_uri() const { return uri_; }
  void set_uri(const char* uri, int length)  {uri_.assign(uri, length);}
  void set_method(Method method) { method_ = method; }
  Method get_method() const { return method_; }
  const std::string& get_method_string() const ;  
  std::ostream& OutputDebug(std::ostream& os);
  int Write(OutMsg* outmsg) const {
    AVOID_UNUSED_VAR_WARN(outmsg);
    return 0;
  }
  static int Encode(OutMsg* outmsg,
                    const std::string& method,
                    const std::string& url,
                    const std::string* body,
                    std::map<std::string, std::string>* header);
 
 protected:
  std::string uri_;
  Method method_;
};
inline const std::string& HttpRequest::get_method_string() const {
  static std::string GET("GET");
  static std::string POST("GET");
  static std::string EMPTY;
  switch(method_)  {
    case kGet:
      return GET;
    case kPost:
      return POST;
    default:
      return EMPTY;
  }
}

class HttpResponse : public HttpMsgBase {
 public:
  HttpResponse() {
    code_ = 200;
  }
  ~HttpResponse() {
  }
  void set_code(int code) { code_ = code; }
  int get_code() const { return code_; }
  void Write(OutMsg* outmsg);
  std::ostream& OutputDebug(std::ostream& os);
  static const char*  GetCodeStr(int code) {
    switch(code) {
      case 200:
        return "OK";
      case 404:
        return "Not Found";
      case 500:
      default:
        return "Internal Server Error";
    }
  }
 private:
  int code_;
};
// http请求解码.
// 实现：buffer分三块:第一行 , header, body.
// 争取将第一行和header放在一块缓冲区中, 用4K大小
// body可以根据Content-Length做到精确分配, 不支持chunk, 不支持body大于5M.
class HttpReqDecoder : public MsgDecoder {
 public:
  HttpReqDecoder() {
    Reset();
    request_ = NULL;
  }
  ~HttpReqDecoder() {
    if (body_) {
      delete [] body_;
      body_ = NULL;
    }
    if (request_) {
      delete request_;
      request_ = NULL;
    }
  }
  virtual int GetBuffer(void** buf,uint32_t* length); 
  virtual int Feed(void* buf,uint32_t length,std::vector<InMsg>* msg); 
  static void DeleteRequest(void* data, void* context) {
    AVOID_UNUSED_VAR_WARN(context);
    delete reinterpret_cast<HttpRequest*>(data);
  }
  struct BodyDeleter {
    void operator() (char* p) {
      delete [] p;
    }
  };
 protected:
  enum {
    kFirstLine = 1,
    kHeader,
    kBody
  };
  // 搜索空白.
  // @return >=0 找到了，返回位置.
  int SearchBlank();
  int SkipBlank(char* str);
  // 搜索换行符.
  int SearchEOL();
  int ParseFirstLine();
  int ParseHeader();
  int ReadBody();
  void Reset() {
    header_length_ = 0;
    status_ = kFirstLine;
    line_start_ = 0;
    line_end_ = 0;
    parse_pos_ = 0;
    body_ = NULL;
    body_length_ = 0;
    body_read_ = 0;
    request_ = NULL;
  }
 private:
  HttpRequest* request_;
  int status_;
  char header_[4096];
  int HEADER_MAX_LENGTH; // 最大允许的header length.
  int header_length_; // 当前已经有的header length.
  int line_start_;
  int line_end_;
  int parse_pos_;
  char* body_;
  int body_length_;
  int body_read_;
};


// TODO (余佐) : HttpRespDecoder是从HttpReqDecoder拷贝的,存在代码重复，
// 需要提炼.
class HttpRespDecoder : public MsgDecoder {
 public:
  HttpRespDecoder() {
    Reset();
    response_ = NULL;
    body_ = NULL;
    buf_ = NULL;
  }
  ~HttpRespDecoder() {
    if (body_) {
      delete []body_;
      body_ = NULL;
    }
    if (buf_) {
      delete [] buf_;
      buf_ = NULL;
    }
    if (response_) {
      delete response_;
      response_ = NULL;
    }

  }
  virtual int GetBuffer(void** buf,uint32_t* length); 
  virtual int Feed(void* buf,uint32_t length,std::vector<InMsg>* msg); 
  static void DeleteResponse(void* data, void* context) {
    AVOID_UNUSED_VAR_WARN(context);
    delete reinterpret_cast<HttpResponse*>(data);
  }
  struct BodyDeleter {
    void operator() (char* p) {
      delete [] p;
    }
  };
 protected:
  enum {
    kFirstLine = 1,
    kHeader,
    kBody,
    kChunkSize, // 读chunk的第一行.
    kChunkData, // 读chunk的数据.
    kChunkTrailer
  };
  // 搜索空白.
  // @return >=0 找到了，返回位置.
  int SearchBlank();
  int SkipBlank(char* str);
  // 搜索换行符.
  int SearchEOL();
  int ParseFirstLine();
  int ParseHeader();
  int ReadBody();
  int ReadChunkSize();
  int ReadChunkData();
  int ReadChunkTrailer();
  void Reset() {
    status_ = kFirstLine;
    body_length_ = 0;
    body_read_ = 0;
    buf_length_ = 0;
    line_start_ = 0;
    line_end_ = 0;
    parse_pos_ = 0;
    chunk_size_ = 0;
    chunk_left_ = 0;
  }
  void AppendBody(const void* data, int length);
 private:
  HttpRespDecoder(const HttpRespDecoder&);
  HttpRespDecoder& operator=(const HttpRespDecoder&);
 private:
  HttpResponse* response_;
  int status_;
  char* body_;
  int body_length_;
  int body_read_;

  char* buf_;
  int buf_capacity_;
  int buf_length_;
  int line_start_;
  int line_end_;
  int parse_pos_;

  int chunk_size_; // 当前读取的chunk大小.
  int chunk_left_; // 当前chunk还需要多少数据.
};

} // namespace blitz.
#endif // __BLITZ_HTTP_COMMON_H_
