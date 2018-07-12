// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <utils/logger.h>
#include <utils/string_util.h>
#include "blitz/http_common.h"
namespace blitz {

static CLogger& g_log_framework = CLogger::GetInstance("framework");
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
  TParamMapConstIter iter = params_.begin();
  TParamMapConstIter end = params_.end();
  while (iter != end) {
    os<<iter->first<<':'<<iter->second<<std::endl;
    ++iter;
  }
  if (content_) {
    os.write(content_.get(), content_length_);
  }
  return os;
}
int HttpRequest::Encode(OutMsg* outmsg,
                        const std::string& method,
                        const std::string& url,
                        const std::string* body,
                        std::map<std::string, std::string>* header) {
  typedef std::map<std::string, std::string> __HeaderMap;
  typedef __HeaderMap::const_iterator __HeaderMapConstIter;
  __HeaderMap fake_header;
  __HeaderMap *myheader = header;
  if (!myheader) {
    myheader = &fake_header;
  }
  BufferWriter writer;
  int eval_length = static_cast<int>(url.size()) + 1024;
  if (body) {
    eval_length += static_cast<int>(body->size());
  }
  writer.Init(eval_length);
  Url url_obj;
  int ret = url_obj.Parse(url.c_str());
  if (ret != 0) {
    return -1;
  }
  std::string upper_method = method;
  StringUtil::ToUpper(upper_method);
  writer.WriteData(upper_method.c_str(), upper_method.size());
  writer.WriteData(" ", 1);
  writer.WriteData(url_obj.get_uri().c_str(), url_obj.get_uri().size());
  writer.WriteData(" HTTP/1.1\r\n", 11);
  __HeaderMapConstIter iter = myheader->find("Host");
  __HeaderMapConstIter end = myheader->end();
  if (iter == end) {
    (*myheader)["Host"] = url_obj.get_host();
  }
  int content_length = 0;
  if (body) {
    content_length = static_cast<int>(body->size());
    if (content_length > 0) {
      char str_length[16];
      sprintf(str_length, "%d", content_length);
      (*myheader)["Content-Length"] = str_length;
    }
  }
  // 保持连接.
  (*myheader)["Connection"] = "keep-alive";
  // 禁用压缩.
   (*myheader)["Accept-Encoding"] = "";

  iter = myheader->begin();
  while (iter != end) {
    writer.WriteData(iter->first.c_str(), iter->first.size());
    writer.WriteData(": ", 2);
    writer.WriteData(iter->second.c_str(), iter->second.size());
    writer.WriteData("\r\n", 2);
    ++iter;
  }
  writer.WriteData("\r\n", 2);
  if (content_length > 0) {
    writer.WriteData(body->c_str(), content_length);
  }
  ret = writer.get_data_length();
  outmsg->AppendBuffers(writer.get_buffers(), writer.get_buffer_count());
  writer.DetachBuffers();
  return ret;
}
int HttpReqDecoder::GetBuffer(void** buf,uint32_t* length) { 
  HEADER_MAX_LENGTH = 4096;
  if (!request_) {
    request_ = new HttpRequest();
  }
  switch (status_) {
    case kFirstLine:
    case kHeader:
      *buf = header_ + header_length_;
      *length = HEADER_MAX_LENGTH - header_length_;
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
int HttpReqDecoder::Feed(void* buf,uint32_t length,std::vector<InMsg>* msg) {
  int ret = -1;
  switch (status_) {
    case kFirstLine: 
      if (header_+header_length_ == reinterpret_cast<char*>(buf)) {
        header_length_ += length;
        ret = ParseFirstLine();
      }
      break;
    case kHeader:
      if (header_+header_length_ == reinterpret_cast<char*>(buf)) {
        header_length_ += length;
        ret = ParseHeader();
      }
      break;
    case kBody:
      if (body_+body_read_ != reinterpret_cast<char*>(buf)) {
        body_read_ += length;
        ret = ReadBody();
      }
      break;
    default:
      break;
  }
  if (ret == 0) {
    return 0;
  } else if (ret == 1) {
    // 读完了一个请求.
    if (request_->get_content_length() > 0) {
      std::shared_ptr<char> content(body_, BodyDeleter());
      request_->set_content(content);
    }
    InMsg one_msg;
    one_msg.type = "HttpRequest";
    one_msg.data = request_;
    one_msg.fn_free = DeleteRequest;
    one_msg.free_ctx = NULL;
    one_msg.bytes_length = 0;
    request_ = NULL;
    msg->push_back(one_msg);
    Reset();
    return 0;
  } else {
    Reset();
    return -1;
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
    request_->set_uri(uri, version-uri);
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
          body_ = reinterpret_cast<char*>(malloc(request_->get_content_length()));
          body_length_ = request_->get_content_length();
          body_read_ = 0;
          int left_length = header_length_ - parse_pos_;
          assert(left_length <= body_length_);
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
          request_->set_content_length(atoi(value));
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
void HttpResponse::Write(OutMsg* outmsg) {
  BufferWriter writer;
  writer.Init(get_content_length() + 1024);
  char first_line[32];
  int line_len = sprintf(first_line, "HTTP/1.%d %d %s\r\n",
                         version_minor_, code_, GetCodeStr(code_));
  // 写第一行 HTTP/1.1 200 OK\r\n
  writer.WriteData(first_line, line_len);
  TParamMapConstIter iter = params_.begin();
  TParamMapConstIter end = params_.end();
  while (iter != end) {
    writer.WriteData(iter->first.c_str(), iter->first.size());
    writer.WriteData(": ", 2);
    writer.WriteData(iter->second.c_str(), iter->second.size());
    writer.WriteData("\r\n", 2);
    ++iter;
  }
  if (content_length_ > 0) {
    writer.WriteData("Content-Length: ", 16);
    writer.PrintInt(content_length_, '\r');
    writer.WriteData("\n", 1);
  }
  writer.WriteData("\r\n", 2);
  if (content_length_ > 0) {
    writer.WriteData(content_.get(), content_length_);
  }
  outmsg->AppendBuffers(writer.get_buffers(), writer.get_buffer_count());
  writer.DetachBuffers();
}
std::ostream& HttpRequest::OutputDebug(std::ostream& os) {
  os<<" HttpRequest [ " << get_method_string() << ' ' << get_uri() ;
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
          LOGGER_DEBUG(g_log_framework, "CopyData from=%d size=%d",
                       line_start_, buf_length_ - line_start_);
          delete [] buf_;
          buf_ = new_buf;
          parse_pos_ = buf_length_ - parse_pos_;
          buf_length_ = buf_length_ - line_start_;
          line_start_ = 0;
        }
        *buf = buf_ + buf_length_;
        *length = buf_capacity_ - buf_length_;
        LOGGER_DEBUG(g_log_framework, "buf_length=%d line_start=%d"
                    " length=%d",
                     buf_length_, line_start_, *length);
      }
      break;
    default:
      return -1;
      break;
  }
  return 0;
}
int HttpRespDecoder::Feed(void* buf,uint32_t length,std::vector<InMsg>* msg) {
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
    
    LOGGER_DEBUG(g_log_framework, "finish_one_response ");
    if (body_read_ > 0 && body_) {
      response_->set_content_length(body_read_);
      std::shared_ptr<char> content(body_, BodyDeleter());
      response_->set_content(content);
      body_ = NULL;
    }
    InMsg one_msg;
    one_msg.type = "HttpResponse";
    one_msg.data = response_;
    one_msg.fn_free = DeleteResponse;
    one_msg.free_ctx = NULL;
    one_msg.bytes_length = 0;
    response_ = NULL;
    msg->push_back(one_msg);
    Reset();
    return 0;
  } else {
    LOGGER_ERROR(g_log_framework, "ret=%d status=%d parse_pos=%d buf_length=%d buf=%*s",
                 ret, status_, parse_pos_, buf_length_, buf_length_, buf_);
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
        const char* str = response_->GetHeader("Transfer-Encoding");
        if (str && strcasecmp(str, "chunked") == 0) {
          status_ = kChunkSize;
          return ReadChunkSize();
        }
        str = response_->GetHeader("Content-Length");
        if (str) {
          int content_length = atoi(str);
          response_->set_content_length(content_length);
        }
        if (response_->get_content_length() > 0) {
          body_ = new char[response_->get_content_length()];
          body_length_ = response_->get_content_length();
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
    LOGGER_INFO(g_log_framework, "chunk-size size=%d", chunk_size_ );
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
  LOGGER_DEBUG(g_log_framework, "append_body=%d", length);
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
