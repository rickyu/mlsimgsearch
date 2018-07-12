// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// Http服务相关类. 
#ifndef __BLITZ_HTTP_SERVER_H_
#define __BLITZ_HTTP_SERVER_H_ 1
#include <unordered_map>
#include <memory>
#include <blitz2/reactor.h>
#include <blitz2/http/http_common.h>
#include <blitz2/stream_socket_channel.h>
#include <blitz2/acceptor.h>
namespace blitz2 {
using std::unique_ptr;
class HttpHandler;
// 简单功能的Http服务器，主要用来做内部的http service.
class SimpleHttpServer : public StreamSocketChannel<HttpReqDecoder>::Observer,
                         public Acceptor::Observer
{
 public:
  typedef StreamSocketChannel<HttpReqDecoder> TChannel;
  typedef SharedStreamSocketChannel<HttpReqDecoder> TSharedChannel;
  typedef std::function<void(TSharedChannel*, unique_ptr<HttpRequest>&&)> THttpHandler;
  typedef std::unordered_map<std::string, THttpHandler> THandlerMap;
  typedef THandlerMap::iterator THandlerMapIter;
  typedef THandlerMap::const_iterator THandlerMapConstIter;
  SimpleHttpServer(Reactor& reactor, Logger& logger )
    :reactor_(reactor), logger_(logger)   {
  }
  int RegisterHandler(const char* path, const THttpHandler& handler);
  void UnregisterHandler(const string& path);
  THttpHandler GetHandler(const string& path) {
    THandlerMapConstIter iter = handlers_.find( path );
    if (iter == handlers_.end()) {
      return THttpHandler();
    }
    return iter->second;
  }
  virtual void OnConnectAccepted(Acceptor* acceptor, int fd)  {
    (void)(acceptor);
    TSharedChannel* channel = TSharedChannel::CreateInstance(reactor_, 
        decoder_init_param_);
    channel->BeConnected(fd);
    channel->SetObserver(this);
  }
  virtual void OnClosed(TChannel* channel) {
    TSharedChannel* shared_channel = dynamic_cast<TSharedChannel*>(channel);
    shared_channel->Release();
  }
  virtual void OnMsgArrived( TChannel* channel, HttpRequest* request );   
  static void SendErrorResp( TChannel* channel, int code, const char* str );
 protected:
  Reactor& reactor_;
  Logger& logger_;
  THandlerMap handlers_;
  THttpHandler default_handler_;
  bool keepalive_;
  HttpReqDecoder::InitParam decoder_init_param_;

};

}
#endif // __BLITZ_HTTP_SERVER_H_

