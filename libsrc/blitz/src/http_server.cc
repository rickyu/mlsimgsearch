// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "blitz/http_server.h"

namespace blitz {
int SimpleHttpServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}
int SimpleHttpServer::RegisterHandler(const char* uri, HttpHandler* handler) {
  std::pair<THandlerMapIter, bool> result = handlers_.insert(
      std::make_pair(uri, handler));
  if (!result.second) {
    // 已经存在，需要先unregister.
    return 1;
  }
  return 0;
}
HttpHandler* SimpleHttpServer::UnregisterHandler(const char* uri) {
  THandlerMapIter iter = handlers_.find(uri);
  if (iter == handlers_.end()) {
    return NULL;
  }
  HttpHandler* handler = iter->second;
  handlers_.erase(iter);
  return handler;
}
int SimpleHttpServer::ProcessMsg(const IoInfo& ioinfo, const blitz::InMsg& msg) {
  HttpRequest* req = reinterpret_cast<HttpRequest*>(msg.data);
  HttpConnection conn;
  conn.io_id = ioinfo.get_id();
  conn.keep_alive = true;
  conn.minor_version = 1;
  // http1.0必须有Connection:Keep-Alive字段, http1.1只要没有Connection:close
  if (req->get_version_minor() == 0) {
    conn.minor_version = 0;
    const char* v = req->GetHeader("Connection");
    if (!v || strcasecmp(v, "Keep-Alive") != 0) {
      conn.keep_alive = false;
    }
  } else {
    const char* v = req->GetHeader("Connection");
    if (v && strcasecmp(v, "Close") == 0) {
      conn.keep_alive = false;
    }
  }
  
  HttpHandler* handler = GetHandler(req->get_uri().c_str());
  if (handler) {
    std::shared_ptr<HttpRequest> req_ptr(req);
    return handler->Process(req_ptr, conn);
  } 
  // 返回404
  HttpResponse resp;
  resp.set_code(404);
  resp.SetContent("Wrong Uri!!");
  SendResponse(conn, &resp);
  if (msg.fn_free) {
    msg.fn_free(msg.data, msg.free_ctx);
  }
  return 0;
}

int SimpleHttpServer::SendResponse(const HttpConnection& conn,
                                  HttpResponse* resp) {
  if (conn.keep_alive) {
    resp->SetHeader("Connection", "Keep-Alive");
  } else {
    resp->SetHeader("Connection", "close");
  }
  resp->set_version_minor(conn.minor_version);
  OutMsg* outmsg = NULL;
  OutMsg::CreateInstance(&outmsg);
  resp->Write(outmsg);
  framework_.SendMsg(conn.io_id, outmsg);
  outmsg->Release();
  if (!conn.keep_alive) {
    framework_.CloseIO(conn.io_id);
  }
  return 0;
}

} // namespace blitz.
