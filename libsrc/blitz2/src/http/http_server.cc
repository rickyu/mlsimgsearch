// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <blitz2/http/http_server.h>

namespace blitz2 {
int SimpleHttpServer::RegisterHandler( const char* path,
                                       const THttpHandler& handler ) {
  std::pair<THandlerMapIter, bool> result = handlers_.insert(
      std::make_pair( path, handler ) );
  if ( !result.second ) {
    // 已经存在，需要先unregister.
    return 1;
  }
  return 0;
}
void SimpleHttpServer::UnregisterHandler( const string& path ) {
  THandlerMapIter iter = handlers_.find( path );
  if ( iter == handlers_.end() ) {
    return ;
  }
  handlers_.erase(iter);
}
void SimpleHttpServer::OnMsgArrived( TChannel* channel, HttpRequest* req ) {
  unique_ptr<HttpRequest> request(req);
  TSharedChannel* shared_channel = dynamic_cast<TSharedChannel*>(channel);
  if ( !channel ) {
    LOG_FATAL( logger_, "TypeError" );
    return;
  }
  SharedPtr<TSharedChannel> channel_wrap(shared_channel);
  THttpHandler handler =  GetHandler( req->path() );
  if ( handler ) {
    handler( shared_channel, std::move( request ) );
  } else {
    if ( default_handler_ ) {
      default_handler_( shared_channel, std::move( request ) );
    } else {
      // 返回404
      HttpResponse resp;
      resp.set_code(404);
      resp.CopyContent("Wrong Uri!!");
      resp.set_version_minor( req->get_version_minor() );
      SharedPtr<OutMsg> outmsg = resp.CreateOutMsg();
      shared_channel->SendMsg( outmsg );
    }
  }
}
void SimpleHttpServer::SendErrorResp( TChannel* channel,
                                      int code,
                                      const char* str ) {
  // 返回404
  HttpResponse resp;
  resp.set_code( code );
  resp.CopyContent( str );
  SharedPtr<OutMsg> outmsg = resp.CreateOutMsg();
  channel->SendMsg( outmsg );
}
} // namespace blitz2.
