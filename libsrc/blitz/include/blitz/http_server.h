// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// Http服务相关类. 
#ifndef __BLITZ_HTTP_SERVER_H_
#define __BLITZ_HTTP_SERVER_H_ 1
#include <unordered_map>
#include "framework.h"
#include "utils/compile_aux.h"
#include "http_common.h"
namespace blitz {
class HttpHandler;

struct HttpConnection {
  uint64_t io_id;
  bool keep_alive;
  int  minor_version;
};
// 简单功能的Http服务器，主要用来做内部的http service.
class SimpleHttpServer : public MsgProcessor {
 public:
  typedef std::unordered_map<std::string, HttpHandler*> THandlerMap;
  typedef THandlerMap::iterator THandlerMapIter;
  typedef THandlerMap::const_iterator THandlerMapConstIter;
  SimpleHttpServer(Framework& framework) : framework_(framework) {
  }
  int Bind(const char* addr);
  int RegisterHandler(const char* uri, HttpHandler* handler);
  HttpHandler* UnregisterHandler(const char* uri);
  HttpHandler* GetHandler(const char* uri) {
    THandlerMapConstIter iter = handlers_.find(uri);
    if (iter == handlers_.end()) {
      return NULL;
    }
    return iter->second;
  }
  int ProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  int SendResponse(const HttpConnection& conn, HttpResponse* resp);
 protected:
  blitz::Framework& framework_;
  THandlerMap handlers_;
  SimpleMsgDecoderFactory<HttpReqDecoder> decoder_factory_;
};

class HttpHandler {
 public:
  virtual int Process(const std::shared_ptr<HttpRequest>& request,
                      const HttpConnection& conn) = 0;
};
}
#endif // __BLITZ_HTTP_SERVER_H_

