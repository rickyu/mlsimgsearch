// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#include "utils/logger.h"
#include "blitz/framework.h"
#include "blitz/http_pool.h"
namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("framework");
HttpPool::HttpPool(Framework& framework,
                       const std::shared_ptr<Selector>& selector)
  : TokenlessServicePool(framework, NULL, selector) {
  decoder_factory_ = &http_resp_decoder_factory_;
}
int HttpPool::Request(const HttpRequest& req, int log_id, int timeout, 
                       const HttpRpcCallback& callback) {
  OutMsg* outmsg = NULL;
  OutMsg::CreateInstance(&outmsg);
  if (!outmsg) {
    return -1;
  }
  req.Write(outmsg);
  int ret = SendOutmsg(outmsg, log_id, timeout, callback);
  outmsg->Release();
  return ret;
}
int HttpPool::SendOutmsg(OutMsg* outmsg,
                       int log_id,
                       int timeout,
                       const HttpRpcCallback& callback) {
  // Query(out_msg, callback, log_id, select_key, 200);
  assert(sizeof(callback.fn_action) <= sizeof(void*));
  void* extra_ptr = reinterpret_cast<void*>(callback.fn_action);
  RpcCallback mycallback;
  mycallback.fn_action = NULL;
  mycallback.user_data = callback.args;
  return RealQuery(outmsg, mycallback, log_id, NULL, timeout, extra_ptr);
}
void HttpPool::InvokeCallback(const RpcCallback& rpc, int result,
                              const InMsg* msg, void* extra_ptr,
                              const IoInfo* ioinfo) {
  HttpResponse* reply = NULL;
  if (msg) {
    reply = reinterpret_cast<HttpResponse*>(msg->data);
    const char* str = reply->GetHeader("Connection");
    // uint64_t ioid = 0;
    if (str && strcasecmp(str, "close") == 0) {
      if (ioinfo) {
        LOGGER_DEBUG(g_log_framework, "CloseHttpConn Connection=close ioid=0x%lx",
                     ioinfo->get_id());
       framework_.CloseIO(ioinfo->get_id());
      }
    }
  }
  FnHttpRpcCallback fn_action = reinterpret_cast<FnHttpRpcCallback>(extra_ptr);
  fn_action(result, reply, rpc.user_data);
}

} // namespace blitz.
