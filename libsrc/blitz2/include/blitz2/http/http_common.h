// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// Http协议编码和解码. 

#ifndef __BLITZ_HTTP_COMMON_H_
#define __BLITZ_HTTP_COMMON_H_ 1
#include <memory>
#include <unordered_map>
#include <map>
#include <boost/utility.hpp>
#include <blitz2/outmsg.h>
#include <blitz2/ref_object.h>
namespace blitz2 {
using boost::noncopyable;
using std::string;
using std::vector;
using std::unordered_map;
 
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
class HttpMsgBase : public noncopyable {
 public:
  HttpMsgBase() {
    version_minor_ = 1;
  }
  virtual ~HttpMsgBase() {
  }
  void SetHeader( const string& key, const string& val ) {
    headers_[key] = val;
  }
  void SetHeader( const char* key, const char* value ) {
    headers_[key] = value;
  }
  const string* GetHeader(const string& key) {
    auto iter = headers_.find( key );
    if ( iter == headers_.end() ) {
      return NULL;
    }
    return &( iter->second );
  }
  void set_version_minor( int v ) { version_minor_ = v;}
  int get_version_minor( )  const { return version_minor_ ;}
  int get_content_length() const { return content_.length(); }
  /**
   * content will be delete by this class.
   */
  void AttachContent( char* content, int length ) { 
    content_.Attach( reinterpret_cast<uint8_t*>(content), (size_t)length,
        Buffer::DestroyDeleteImpl );
    content_.length() = length;
  }
  void AttachContent( DataBuffer& c ) {
    content_.Attach( c.mem(), c.capacity(), c.destroy_functor() );
    content_.offset() = c.offset();
    content_.length() = c.length();
    c.Detach();
  }

  int CopyContent( const char* content, int length ) {
    if ( !content_.Alloc( length ) ) {
      return -1;
    }
    memcpy( content_.mem(), content, length );
    content_.length() = length;
    return 0;
  }
  int CopyContent( const char* content ) {
    return CopyContent( content, strlen( content ) );
  }

  
  const char* get_content() const { 
    return (const char*) content_.mem() + content_.offset(); 
  }
  virtual std::ostream& OutputDebug(std::ostream& os);

 protected:
  typedef std::unordered_map<std::string, std::string> THeaderMap;
 protected:
  int version_minor_;
  DataBuffer content_;
  THeaderMap headers_;
};
class HttpRequest : public HttpMsgBase {
 public:
  typedef std::vector< std::string > TParam;
  typedef unordered_map< std::string, TParam > TParamsMap;
  enum Method {
    kGet = 1,
    kPost = 2
  };
  HttpRequest() {
  }
  TParam* GetParam( const std::string& name, bool add_if_not_exist ) {
    using std::vector;
    using std::string;
    auto it = params_.find( name );
    if ( it == params_.end() ) {
      if ( !add_if_not_exist ) {
        return NULL;
      } else {
        auto result = params_.insert( std::make_pair( name, vector<string>() ) );
        return &( result.first->second );
      }
    }
    return &( it->second );
  }
  const TParam* GetParam( const std::string& name ) const  {
    auto it = params_.find( name );
    if ( it == params_.end() ) {
      return NULL;
    }
    return &( it->second );
  }

  void RemoveParam( const std::string& name ) {
    params_.erase( name );
  }
  void SetParam( const std::string& name, const std::string& val ) {
    TParam* param = GetParam( name, true );
    param->push_back( val );
  }
  const string& anchor() const { return anchor_; }
  int ParseUri( const char* uri, int length ); 
  const string& path() const { return path_; }
  void set_method( Method method ) { 
    method_ = method; 
  }
  Method get_method() const { 
    return method_; 
  }
  const std::string& get_method_string() const ;  
  std::ostream& OutputDebug(std::ostream& os);
  static int Encode(OutMsg* outmsg,
                    const std::string& method,
                    const std::string& url,
                    const std::string* body,
                    std::map<std::string, std::string>* header);
 
 protected:
  Method method_;
  string path_;
  string anchor_;
  TParamsMap params_;

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
  SharedPtr<OutMsg> CreateOutMsg();
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
class HttpReqDecoder {
 public:
  typedef HttpRequest TMsg;
  struct InitParam {
    InitParam() {
      max_header_size = 4096;
      max_body_size = 5*1024*1024;
    }
    size_t max_header_size;
    size_t max_body_size;
  };
  typedef InitParam TInitParam;
  
  HttpReqDecoder() {
    Reset();
    header_ = NULL;
    request_ = NULL;
  }
  ~HttpReqDecoder() {
    Uninit();
  }
  int Init( const InitParam& param ); 
  void Uninit(); 
  int GetBuffer( void** buf,uint32_t* length ); 
  template<typename Reporter>
  int Feed(void* buf,uint32_t length,Reporter report); 
  static void DeleteRequest(void* data, void* context) {
    (void)(context);
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
  InitParam param_;
  char *header_;
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
class HttpRespDecoder {
 public:
  typedef SharedObject<HttpResponse> TMsg;
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
  int GetBuffer(void** buf,uint32_t* length); 
  template<typename Reporter>
  int Feed(void* buf,uint32_t length, Reporter report);
  static void DeleteResponse(void* data, void* context) {
    (void)(context);
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
template<typename Reporter>
int HttpReqDecoder::Feed( void* buf, uint32_t length, Reporter report ) {
  int ret = -1;
  switch (status_) {
    case kFirstLine: 
      assert (header_+header_length_ == reinterpret_cast<char*>(buf) );
      header_length_ += length;
      ret = ParseFirstLine();
      break;
    case kHeader:
      assert( header_+header_length_ == reinterpret_cast<char*>(buf));
      header_length_ += length;
      ret = ParseHeader();
      break;
    case kBody:
      assert( body_+body_read_ == reinterpret_cast<char*>(buf) );
      body_read_ += length;
      ret = ReadBody();
      break;
    default:
      break;
  }
  if (ret == 0) {
    return 0;
  } else if (ret == 1) {
    // 读完了一个请求.
    if (request_->get_content_length() > 0) {
      request_->AttachContent( body_, body_length_ );
    }
    report( request_ );
    request_ = NULL;
    Reset();
    return 0;
  } else {
    Reset();
    return -1;
  }
  return 0;
}

} // namespace blitz2.
#endif // __BLITZ_HTTP_COMMON_H_
