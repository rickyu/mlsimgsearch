// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// 访问Http相关的类和函数,包括
//  HttpPool - Http连接池
//  HttpEventFire - 状态机Event包装,访问RedisPool得到应答后回调状态机

#ifndef __BLITZ_HTTP_POOL_H_
#define __BLITZ_HTTP_POOL_H_ 1
#include "tokenless_service_pool.h"
#include "utils/compile_aux.h"
#include "http_common.h"
#include "state_machine.h"
namespace blitz {
// RedisPool回调函数.
typedef void (*FnHttpRpcCallback)(int result, HttpResponse* reply,
                                  void* args);
struct HttpRpcCallback {
    FnHttpRpcCallback fn_action;
    void* args;
};

// Http连接池,继承自TokenlessServicePool，主要增加请求的编码.
class HttpPool : public TokenlessServicePool {
 public:
  HttpPool(Framework& framework, const std::shared_ptr<Selector>& selector);
  // 给http服务器发送请求。异步函数，pool保证callback被调用到.
  int Request(const HttpRequest& req, int log_id, int timeout, 
               const HttpRpcCallback& callback);
  int SendOutmsg(OutMsg* outmsg, 
               int log_id, 
               int timeout,
               const HttpRpcCallback& callback);
 protected:
  virtual void InvokeCallback(const RpcCallback& rpc, int result,
                              const InMsg* msg, void* extra_ptr,
                              const IoInfo* ioinfo) ;
 private:
  SimpleMsgDecoderFactory<HttpRespDecoder> http_resp_decoder_factory_;
};

// StateMachine支持，Event::data指向该结构体.
struct HttpRespEventData {
   int result;
   HttpResponse* response;
   void* param;
 };

template <typename TStateMachine>
class HttpEventFire {
 public:
  explicit HttpEventFire(HttpPool* pool) {
    event_type_ = -1;
    pool_ = pool;
  }
  HttpEventFire(HttpPool* pool, int event_type) {
    pool_ = pool;
    event_type_ = event_type;
  }
  HttpPool* get_pool()  { return pool_; }
  struct HttpEventCallbackContext {
    TStateMachine* state_machine;
    int event_type;
    int context_id;
    void* param;
  };
  // 返回发送msg_id
  int FireGet(const std::string& method,
              const std::string& url,
              const std::string* body, 
              std::map<std::string, std::string>*  headers,
              int context_id,
              TStateMachine* machine,
              void* myparam,
              int timeout) {
    int msgid = -1;
    HttpRpcCallback callback;
    HttpEventCallbackContext* ctx  = new HttpEventCallbackContext; 
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.fn_action = OnHttpResp;
    callback.args = ctx;
    OutMsg* outmsg = NULL;
    OutMsg::CreateInstance(&outmsg);
    if (!outmsg) {
      return -1;
    }
    msgid = outmsg->get_msgid();
    HttpRequest::Encode(outmsg, method, url, body, headers);
    pool_->SendOutmsg(outmsg, context_id, timeout, callback);
    outmsg->Release();
    return msgid;
  }

 private:
  static void OnHttpResp(int result, HttpResponse* response,
                         void* args) {
    HttpEventCallbackContext* ctx = 
      reinterpret_cast<HttpEventCallbackContext*>(args);
    HttpRespEventData event_data;
    event_data.result = result;
    event_data.response = response;
    event_data.param = ctx->param;
    Event event;
    event.type = ctx->event_type;
    event.data = &event_data;;
    int ret = ctx->state_machine->ProcessEvent(ctx->context_id, &event);
    if (ret != 0) {
      if (response) {
        delete response;
      }
    }
    delete ctx;
   }
 private:
  HttpPool* pool_;
  int event_type_;
};

}
#endif // __BLITZ_REDIS_POOL_H_
