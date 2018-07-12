// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <blitz2/logger.h>
#include <blitz2/http/http_common.h>
#include <blitz2/buffer_writer.h>
namespace blitz2 {
int Url::Parse(const char* url) {
  const char* protocol = strstr(url, "://");
  if (!protocol) {
    return -1;
  }
  protocol_.assign(url, protocol-url);
  const char* host = protocol+3;
  if (!host) {
    return -2;
  }
  const char* uri = strchr(host, '/');
  if (!uri) {
    uri_ = "/";
    return 0;
  }
  host_.assign(host, uri-host);
  uri_.assign(uri);
  return 0;
}
std::ostream& HttpMsgBase::OutputDebug(std::ostream& os) {
  auto iter = headers_.begin();
  auto end = headers_.end();
  while (iter != end) {
    os<<iter->first<<':'<<iter->second<<std::endl;
    ++iter;
  }
  if ( content_.length() > 0 ) {
    os.write( (const char*) content_.mem()+content_.offset(), content_.length() );
  }
  return os;
}
const char* GetPath( const char* start, const char* end ) {
  while ( start < end ) {
    if ( *start == '?' || *start == '#' ) {
      return start;
    }
    ++ start;
  }
  return start;
}

const char* SearchChar( const char* start, const char* end , char c ) {
  while ( start < end ) {
    if ( *start == c ) {
      return start;
    }
    ++ start;
  }
  return start;
}
int HexVal( char s ) {
  if ( s >= '0' && s <= '9' ) {
    return s-'0';
  }
  if ( s >= 'a' && s <= 'f' ) {
    return s-'a' + 10;
  }
  if ( s >= 'A' && s <= 'F' ) {
    return s-'A'+10;
  }
  return -1;
}
template< typename T>
const char* ParseValue( const char* start, const char* end,  T t) {
  while ( start < end ) {
    if ( *start == '&' ) {
      // %AB 
      return start;
    } else {
      if ( *start == '%' ) {
        if ( end - start >= 2 ) {
          char ch = HexVal( start[1] ) << 4 | HexVal( start[2] );
          t( &ch, 1 );
          start += 3;
        } else {
          // input error
          return end;
        }
      } else {
        t( start, 1);
        ++ start;
      }
    }
  }
  return start;
}
struct ValueAccept {
  ValueAccept( std::string* s ) {
    str = s;
  }
  void operator()(const char* s, size_t n ) {
    str->append( s, n );
  }
  std::string* str;
};
int HttpRequest::ParseUri( const char* uri, int length ) {
  // /abc/dev#haha?name=v&name=v&name=v
  const char* end = uri + length;
  const char* path = uri;
  uri = GetPath( uri, end );
  path_.assign( path, uri );
  if ( uri == end ) {
    return 0;
  }
  if ( *uri == '#' ) {
    const char* anchor = uri+1;
    uri = SearchChar( uri+1, end, '?' );
    anchor_.assign( anchor, uri );
    if ( uri == end ) {
      return 0;
    }
  }
  ++uri;
  while ( uri < end ) {
    const char* name = uri;
    uri = SearchChar( uri, end, '=' );
    TParam* param = GetParam( string( name, uri - name), true );
    param->push_back( std::string() );
    uri = ParseValue( uri+1, end, ValueAccept( &(*param)[param->size()-1] ) );
    ++uri;
  }
  return 0;
}
int HttpRequest::Encode(OutMsg* outmsg,
                        const std::string& method,
                        const std::string& url,
                        const std::string* body,
                        std::map<std::string, std::string>* header) {
  typedef std::map<std::string, std::string> __HeaderMap;
  typedef __HeaderMap::const_iterator __HeaderMapConstIter;
  (void)(outmsg);
  (void)(method);
  (void)(url);
  (void)(body);
  (void)(header);
  assert(0);
  // __HeaderMap fake_header;
  // __HeaderMap *myheader = header;
  // if (!myheader) {
  //   myheader = &fake_header;
  // }
  // BufferWriter writer;
  // int eval_length = static_cast<int>(url.size()) + 1024;
  // if (body) {
  //   eval_length += static_cast<int>(body->size());
  // }
  // writer.Init(eval_length);
  // Url url_obj;
  // int ret = url_obj.Parse(url.c_str());
  // if (ret != 0) {
  //   return -1;
  // }
  // std::string upper_method = method;
  // StringUtil::ToUpper(upper_method);
  // writer.WriteData(upper_method.c_str(), upper_method.size());
  // writer.WriteData(" ", 1);
  // writer.WriteData(url_obj.get_uri().c_str(), url_obj.get_uri().size());
  // writer.WriteData(" HTTP/1.1\r\n", 11);
  // __HeaderMapConstIter iter = myheader->find("Host");
  // __HeaderMapConstIter end = myheader->end();
  // if (iter == end) {
  //   (*myheader)["Host"] = url_obj.get_host();
  // }
  // int content_length = 0;
  // if (body) {
  //   content_length = static_cast<int>(body->size());
  //   if (content_length > 0) {
  //     char str_length[16];
  //     sprintf(str_length, "%d", content_length);
  //     (*myheader)["Content-Length"] = str_length;
  //   }
  // }
  // // 保持连接.
  // (*myheader)["Connection"] = "keep-alive";
  // // 禁用压缩.
  //  (*myheader)["Accept-Encoding"] = "";

  // iter = myheader->begin();
  // while (iter != end) {
  //   writer.WriteData(iter->first.c_str(), iter->first.size());
  //   writer.WriteData(": ", 2);
  //   writer.WriteData(iter->second.c_str(), iter->second.size());
  //   writer.WriteData("\r\n", 2);
  //   ++iter;
  // }
  // writer.WriteData("\r\n", 2);
  // if (content_length > 0) {
  //   writer.WriteData(body->c_str(), content_length);
  // }
  // ret = writer.get_data_length();
  // outmsg->AppendBuffers(writer.get_buffers(), writer.get_buffer_count());
  // writer.DetachBuffers();
  return 0;
}
int HttpReqDecoder::Init( const InitParam& param ) {
  Uninit();
  param_ = param;
  header_ = new char[param.max_header_size];
  if ( !header_ ) {
    return -1;
  }
  return 0;
}
void HttpReqDecoder::Uninit() {
  if ( header_ ) {
    delete[] header_;
    header_ = NULL;
  }
  if (body_) {
    delete [] body_;
    body_ = NULL;
  }
  if (request_) {
    delete request_;
    request_ = NULL;
  }
  Reset();
}

int HttpReqDecoder::GetBuffer(void** buf,uint32_t* length) { 
  if (!request_) {
    request_ = new HttpRequest();
  }
  switch (status_) {
    case kFirstLine:
    case kHeader:
      if ( !header_  || (size_t)header_length_ >= param_.max_header_size ) {
        return -1;
      }
      *buf = header_ + header_length_;
      *length = param_.max_header_size - header_length_;
      break;
    case kBody:
      *buf = body_ + body_length_ - body_read_;
      *length = body_length_ - body_read_;
      break;
    default:
      return -1;
      break;
  }
  return 0;
}

int HttpReqDecoder::ParseFirstLine() {
  int ret = SearchEOL();
  if (ret == 0) {
    // 找到了 :从line_start_到line_end_是一行.
    int line_len = line_end_ - line_start_;
    char* line = header_+line_start_;
    line[line_end_] = '\0';
    line_start_ = parse_pos_;
    line_end_ = -1;
    
    int uri_start = 4;
    if (strncasecmp(line, "GET ", 4) == 0) {
      request_->set_method(HttpRequest::Method::kGet);
    } else if (strncasecmp(line, "POST ", 5) == 0) {
      request_->set_method(HttpRequest::Method::kPost);
      uri_start=5;
    } else {
      // 暂时不支持其他的METHOD.
      return -1;
    }
    char* uri = line+uri_start;
    char* version = strchr(uri, ' ');
    if (version == NULL) {
      // 出错.
      return -1;
    }
    request_->ParseUri(uri, version-uri);
    ++version;
    if (line[line_len-1] == '0') {
      request_->set_version_minor(0);
    } else {
      request_->set_version_minor(1);
    }
    // request_->set_version(version);
    return ParseHeader();
  } else {
    return 0;
  }
}
int HttpReqDecoder::ParseHeader() {
  while (1) {
    int ret = SearchEOL();
    if (ret == 0) {
      char* line = header_+line_start_;
      int line_len = line_end_ - line_start_;
      line_start_ = parse_pos_;
      if (line_len == 0) {
        // 空行, header结束.
        status_ = kBody;
        if (request_->get_content_length() > 0) {
          // 看看header缓冲区还剩余多少数据.
          body_length_ = request_->get_content_length();
          body_ = new char[body_length_];
          body_read_ = 0;
          int left_length = header_length_ - parse_pos_;
          if (left_length > 0) {
            memcpy(body_, header_+parse_pos_, left_length);
            body_read_ = left_length;
          }
          return ReadBody();
        } else {
          return 1;
        }
        return 0;
      } else {
        header_[line_end_] = '\0';
        char* value = strchr(line, ':');
        if (value == NULL) {
          return -1;
        }
        *value = '\0';
        ++value;
        if (*value == ' ') {
          ++value;
        }
        if (strncasecmp(line, "content-length", strlen("content-length")) == 0) {
          // request_->set_content_length(atoi(value));
        } else {
          request_->SetHeader(line, value);
        }
      }
    } else {
      return 0;
    }
  }
}
int HttpReqDecoder::ReadBody() {
  if (body_read_ == body_length_) {
    return 1;
  } else if (body_read_ > body_length_) {
    return -1;
  }
  return 0;
}
int HttpReqDecoder::SearchEOL() {
  // 从parse_pos_开始扫描，直到header_length_;
  while (1) {
    while (parse_pos_<header_length_ && header_[parse_pos_] != '\r')  {
      ++parse_pos_;
    }
    if (parse_pos_ == header_length_) {
      // 没找到.
      return 1;
    }
    if (parse_pos_ < header_length_-1) {
      if (header_[parse_pos_+1] == '\n') {
        // 找到了.
        line_end_ = parse_pos_;
        parse_pos_ += 2;
        return 0;
      } else {
        parse_pos_ += 2;
     }
    } else {
      // \r是最后一个字符, 需要继续找\n.
      return 1;
    }
  }
}


// ============HttpResponse===============
SharedPtr<OutMsg> HttpResponse::CreateOutMsg() {
  BufferWriter writer;
  writer.Reserve(get_content_length() + 1024);
  char first_line[32];
  int line_len = sprintf(first_line, "HTTP/1.%d %d %s\r\n",
                         version_minor_, code_, GetCodeStr(code_));
  // 写第一行 HTTP/1.1 200 OK\r\n
  writer.WriteData(first_line, line_len);
  auto iter = headers_.begin();
  auto end = headers_.end();
  while (iter != end) {
    writer.WriteData(iter->first.c_str(), iter->first.size());
    writer.WriteData(": ", 2);
    writer.WriteData(iter->second.c_str(), iter->second.size());
    writer.WriteData("\r\n", 2);
    ++iter;
  }
  if (content_.length() > 0) {
    writer.WriteData("Content-Length: ", 16);
    writer.PrintInt(content_.length(), '\r');
    writer.WriteData("\n", 1);
  }
  writer.WriteData("\r\n", 2);
  if (content_.length() > 0) {
    writer.WriteData( (const char*)content_.mem()+content_.offset(), 
                    content_.length() );
  }
  SharedPtr<OutMsg> outmsg = OutMsg::CreateInstance();
  outmsg->AppendDataBuffer( writer.data() );
  writer.DetachData();
  return outmsg;
}
std::ostream& HttpRequest::OutputDebug(std::ostream& os) {
  os<<" HttpRequest [ " << get_method_string() << ' ' << path() ;
  HttpMsgBase::OutputDebug(os);
  os << ']';
  return os;
}

std::ostream& HttpResponse::OutputDebug(std::ostream& os) {
  os<<" HttpResponse code:" << code_ << " HTTP/1." << version_minor_ << ' ';
  HttpMsgBase::OutputDebug(os);
  return os;
}
// HttpRespDecoder 实现 ========================================
int HttpRespDecoder::GetBuffer(void** buf,uint32_t* length) { 
  if (!buf_) {
    buf_capacity_ = 4096;
    buf_ = new char[buf_capacity_];
    assert(buf_);
  }
  if (!response_) {
    response_ = new HttpResponse();
  }
  switch (status_) {
    case kFirstLine:
    case kHeader:
      *buf = buf_ + buf_length_;
      *length = buf_capacity_ - buf_length_;
      break;
    case kBody:
      *buf = body_  + body_read_;
      *length = body_length_ - body_read_;
      break;
    case kChunkSize:
    case kChunkData:
    case kChunkTrailer:
      {
        // 根据上个chunk大小预备下一次的chunk缓冲区.
        // [chunk-size] \r\n[chunk-data]\r\n0\r\n 
        int need_min = chunk_left_ + 64 + chunk_size_;
        if (need_min < 64*1024) {
          need_min = 64*1024;
        }
        int left = buf_capacity_ - buf_length_;
        if (need_min > left) {
          char* new_buf = new char[need_min];
           buf_capacity_ = need_min;
          memcpy(new_buf, buf_+line_start_, buf_length_ - line_start_);
          delete [] buf_;
          buf_ = new_buf;
          parse_pos_ = buf_length_ - parse_pos_;
          buf_length_ = buf_length_ - line_start_;
          line_start_ = 0;
        }
        *buf = buf_ + buf_length_;
        *length = buf_capacity_ - buf_length_;
      }
      break;
    default:
      return -1;
      break;
  }
  return 0;
}
template<typename Reporter>
int HttpRespDecoder::Feed(void* buf,uint32_t length, Reporter report) {
  int ret = -1;
  
  switch (status_) {
    case kFirstLine: 
      if (buf_+buf_length_ == reinterpret_cast<char*>(buf)) {
        buf_length_ += length;
        ret = ParseFirstLine();
      }
      break;
    case kHeader:
      if (buf_+buf_length_ == reinterpret_cast<char*>(buf)) {
        buf_length_ += length;
        ret = ParseHeader();
      }
      break;
    case kBody:
      if (body_+body_read_ == reinterpret_cast<char*>(buf)) {
        body_read_ += length;
        ret = ReadBody();
      }
      break;
    case kChunkSize:
      if (buf_+buf_length_ == reinterpret_cast<char*>(buf)) {
        buf_length_ += length;
        ret = ReadChunkSize();
      }
      break;
    case kChunkData:
      if (buf_+buf_length_ == reinterpret_cast<char*>(buf)) {
        buf_length_ += length;
        ret = ReadChunkData();
      }
      break;
    case kChunkTrailer:
      if (buf_+buf_length_ == reinterpret_cast<char*>(buf)) {
        buf_length_ += length;
        ret = ReadChunkData();
      }
      break;

    default:
      break;
  }
  if (ret == 0) {
    return 0;
  } else if (ret == 1) {
    // 读完了一个请求.
    if (body_read_ > 0 && body_) {
      response_->AttachContent( body_, body_read_ );
      body_ = NULL;
    }
    report(response_);
    response_ = NULL;
    Reset();
    return 0;
  } else {
    if (response_) {
      delete response_;
      response_ = NULL;
    }
    Reset();
    return -1;
  }
  return 0;
}

int HttpRespDecoder::ParseFirstLine() {
  int ret = SearchEOL();
  if (ret == 0) {
    // HTTP/1.1 200 OK\r\n
    // 找到了 :从line_start_到line_end_是一行.
    // int line_len = line_end_ - line_start_;
    char* line = buf_+line_start_;
    line[line_end_] = '\0';
    line_start_ = parse_pos_;
    line_end_ = -1;
    if (strncasecmp(line, "HTTP/1.", 7) != 0) {
      return -1;
    }
    if (line[7] == '1') {
      response_->set_version_minor(1);
    } else if (line[7] == '0') {
      response_->set_version_minor(0);
    } else {
      return -1;
    }
    char* pcode = line+9;
    int code = atoi(pcode);
    response_->set_code(code);
    return ParseHeader();
  } else {
    return 0;
  }
}
int HttpRespDecoder::ParseHeader() {
  while (1) {
    int ret = SearchEOL();
    if (ret == 0) {
      char* line = buf_+line_start_;
      int line_len = line_end_ - line_start_;
      line_start_ = parse_pos_;
      if (line_len == 0) {
        // 空行, header结束.
        const string* str = response_->GetHeader("Transfer-Encoding");
        if ( str && *str == "chunked" ) {
          status_ = kChunkSize;
          return ReadChunkSize();
        }
        str = response_->GetHeader("Content-Length");
        int content_length = atoi(str->c_str());
        
        if ( content_length > 0 ) {
          body_ = new char[ content_length ];
          body_length_ = content_length;
          body_read_ = 0;
          int left_length = buf_length_ - parse_pos_;
          assert(left_length <= body_length_);
          if (left_length > 0) {
            memcpy(body_, buf_+parse_pos_, left_length);
            body_read_ = left_length;
          }
          status_ = kBody;
          return ReadBody();
        } else {
          return 1;
        }
        return 0;
      } else {
        buf_[line_end_] = '\0';
        char* value = strchr(line, ':');
        if (value == NULL) {
          return -1;
        }
        *value = '\0';
        ++value;
        if (*value == ' ') {
          ++value;
        }
        response_->SetHeader(line, value);
      }
    } else {
      return 0;
    }
  }
}
int HttpRespDecoder::ReadBody() {
  if (body_read_ == body_length_) {
    return 1;
  } else if (body_read_ > body_length_) {
    return -1;
  }
  return 0;
}
int HttpRespDecoder::ReadChunkSize() {
  // chunked body在buf_缓冲区中读取.
  int ret = SearchEOL();
  if (ret == 0) {
    char* line = buf_+line_start_;
    // TODO(余佐) : 检查size是否格式正确.
    chunk_size_ = static_cast<int>(strtol(line, NULL, 16));
    chunk_left_ = chunk_size_;
    status_ = kChunkData;
    return ReadChunkData();
  } else {
    // 数据读完了，没有找到\r\n, 需要读取更多的数据，但是缓冲区可能不够.
    // CheckMoveBuffer();
    return 0;
  }
}
int HttpRespDecoder::ReadChunkData() {
  int left_length = buf_length_ - parse_pos_;
  if (left_length >= chunk_left_) {
    // 当前chunk的数据都在header缓冲区中.
    AppendBody(buf_+parse_pos_, chunk_left_);
    parse_pos_ += chunk_left_;
    chunk_left_ = 0;
    status_ = kChunkTrailer;
    return ReadChunkTrailer();
  } else {
    // 缓冲区中的数据读取完了.
    AppendBody(buf_+parse_pos_, left_length);
    chunk_left_ -= left_length;
    // 从buffer开头写数据.
    parse_pos_ = 0;
    buf_length_ = 0;
    line_start_ = 0;
    return 0;
  }
}
int HttpRespDecoder::ReadChunkTrailer() {
  int left_length = buf_length_ - parse_pos_;
  if (left_length >= 2) {
    if (chunk_size_ == 0) {
      if (left_length > 2) {
        // 错误，数据多了.
        return -1;
      }
      return 1;
    } else {
      parse_pos_ += 2;
      line_start_ = parse_pos_; 
      status_ = kChunkSize;
      return ReadChunkSize();
    }
  } else {
    // 只剩下一个字符了.
    buf_[0] = buf_[buf_length_-1];
    buf_length_ = 1;
    parse_pos_ = 0;
    line_start_ = 0;
    // 继续读取数据.
    return 0;
  }
}
int HttpRespDecoder::SearchEOL() {
  // 从parse_pos_开始扫描，直到buf_length_;
  while (1) {
    while (parse_pos_<buf_length_ && buf_[parse_pos_] != '\r')  {
      ++parse_pos_;
    }
    if (parse_pos_ == buf_length_) {
      // 没找到.
      return 1;
    }
    if (parse_pos_ < buf_length_-1) {
      if (buf_[parse_pos_+1] == '\n') {
        // 找到了.
        line_end_ = parse_pos_;
        parse_pos_ += 2;
        return 0;
      } else {
        parse_pos_ += 2;
     }
    } else {
      // \r是最后一个字符, 需要继续找\n.
      return 1;
    }
  }
}
void HttpRespDecoder::AppendBody(const void* data, int length) {
  if (body_ == NULL ) {
    body_length_ = length * 4;
    body_ = new char[body_length_];
    assert(body_);
    body_read_ = 0;
  } else if (body_read_+length > body_length_) {
    int new_length = body_length_+length+4096;
    char* new_buf = new char[new_length];
    memcpy(new_buf, body_, body_read_);
    delete [] body_;
    body_ = new_buf;
    body_length_ = new_length;
  }
  memcpy(body_+body_read_, data, length);
  body_read_ += length;
}

} // namespace blitz.
