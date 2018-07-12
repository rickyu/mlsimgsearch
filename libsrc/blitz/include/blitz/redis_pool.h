// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// 访问Redis相关的类和函数,包括
//  RedisPool - Redis连接池
//  RedisEventFire - 状态机Event包装,访问RedisPool得到应答后回调状态机

#ifndef __BLITZ_REDIS_POOL_H_
#define __BLITZ_REDIS_POOL_H_ 1
#include "tokenless_service_pool.h"
#include "redis_msg.h"
#include "utils/compile_aux.h"
#include "utils/logger.h"
#include "state_machine.h"
namespace blitz {
// RedisPool回调函数.
typedef void (*FnRedisRpcCallback)(int result, redisReply* reply,
                                   void* user_data);
struct RedisRpcCallback {
    FnRedisRpcCallback fn_action;
    void* user_data;
};

// Redis连接池,继承自TokenlessServicePool，主要增加请求的编码.
class RedisPool : public TokenlessServicePool {
 public:
  RedisPool(Framework& framework, const std::shared_ptr<Selector>& selector);
  // 给Redis发送Get请求。异步函数，pool保证callback被调用到.
  int DivideKeys(const std::vector<std::string>& keys, std::map<int, std::vector<int> >& service_to_keyindex, std::map<int, std::vector<std::string> >& service_to_keys);
  int Get(const char* key, int log_id, const RedisRpcCallback& callback,
          int timout);
  // 给Redis发送Set请求。异步函数，pool保证callback被调用到.
  int Set(const char* key, const char* value, int log_id,
            const RedisRpcCallback& callback, int timeout);
  int Incrby(const char* key, int offset, int log_id, 
            const RedisRpcCallback& callback, int timeout);
  int CommandArgvWithoutSelect(int service_id, int argc, const char **argv, const size_t *argvlen,
            int log_id, const RedisRpcCallback& callback, int timeout);
  int CommandArgv(const char* key, int argc, const char **argv, const size_t *argvlen,
            int log_id, const RedisRpcCallback& callback, int timeout);
  int Setex(const char* key, uint32_t expire_time, const char* value, int log_id, const RedisRpcCallback& callback, int timeout);
  int Expire(const char* key, uint32_t expire_time, int log_id, const RedisRpcCallback& callback, int timeout);
  int Exists(const char* key, int log_id, const RedisRpcCallback& callback, int timeout);
  int ZrangeByScore(const char* key, int start, int limit, int direction, int log_id, const RedisRpcCallback& callback, int timeout);
  int Sadd(const char* set, const char* key, int log_id, const RedisRpcCallback& callback, int timeout);
  int Zadd(const char* zset, int32_t score, const char* key, int log_id, const RedisRpcCallback& callback, int timeout);
  int Command(char* command, // 需要由pool来释放.
               int len,
               int log_id, 
               const RedisRpcCallback& callback,
               const SelectKey* select_key,
               FnMsgContentFree fn_free, // 用于释放command.
               void* free_ctx, 
               int timeout);
  int CommandWithoutSelect(char* command, // 需要由pool来释放.
               int len,
               int log_id, 
               const RedisRpcCallback& callback,
               int service_id,
               FnMsgContentFree fn_free, // 用于释放command.
               void* free_ctx, 
               int timeout);
  static void FreeRedisCommand(void* data, void* ctx) {
    AVOID_UNUSED_VAR_WARN(ctx);
    free(data);
  }
 protected:
  virtual void InvokeCallback(const RpcCallback& rpc, int result, const InMsg* msg, void* extra_ptr, const IoInfo* ioinfo) {
    redisReply* reply = NULL;
    if (msg) {
      reply = reinterpret_cast<redisReply*>(msg->data);
    }
    FnRedisRpcCallback fn_action = reinterpret_cast<FnRedisRpcCallback>(extra_ptr);
    fn_action(result, reply, rpc.user_data);

  }
 private:
  RedisReplyDecoderFactory redis_decoder_factory_;
};

// StateMachine支持，Event::data指向该结构体.
struct RedisEventData {
   int result;
   redisReply* reply;
   void* param;
 };

template <typename TStateMachine>
class RedisEventFire {
 public:
  explicit RedisEventFire(RedisPool* pool) {
    event_type_ = -1;
    pool_ = pool;
  }
  RedisEventFire(RedisPool* pool, int event_type) {
    pool_ = pool;
    event_type_ = event_type;
  }
  RedisPool* get_pool()  { return pool_; }
  struct RedisEventCallbackContext {
    TStateMachine* state_machine;
    int event_type;
    int context_id;
    void* param;
  };
  int FireDivideKeys(const std::vector<std::string>& keys,std::map<int, std::vector<int> >& service_to_keyindex, std::map<int, std::vector<std::string> >& service_to_keys) {
    return pool_->DivideKeys(keys, service_to_keyindex, service_to_keys);
  }
  int FireGet(const char* key, int context_id, TStateMachine* machine,
               void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Get(key, context_id,  callback, timeout);
  }
  int FireSet(const char* key, const char* value, int context_id,
             TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Set(key, value, context_id,  callback, timeout);
  }
  int FireIncrby(const char* key, int offset, int context_id,
             TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Incrby(key, offset, context_id,  callback, timeout);
  }
  int FireSetex(const char* key, uint32_t expire_time, const char* value,
               int context_id, TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Setex(key, expire_time, value, context_id, callback, timeout);
  }
  int FireExists(const char* key, int context_id, 
               TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Exists(key, context_id, callback, timeout);
  }
  int FireExpire(const char* key, uint32_t expire_time,int context_id, 
               TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Expire(key, expire_time, context_id, callback, timeout);
  }
  int FireSadd(const char* set, const char* key, int context_id, TStateMachine* machine, 
               void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Sadd(set, key, context_id, callback, timeout);
  }
  int FireZadd(const char* zset, int32_t score, const char* key, int context_id,
              TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->Zadd(zset, score, key, context_id, callback, timeout);
  }

  int FireMGet(int service_id, int argc, const char **argv, const size_t *argvlen, int context_id,
               TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->CommandArgvWithoutSelect(service_id, argc, argv, argvlen, context_id, callback, timeout);
  }

  int FireCommandArgv(const char* key, int argc, const char **argv, const size_t *argvlen,
              int context_id, TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->CommandArgv(key, argc, argv, argvlen, context_id, callback, timeout);
  }

  int FireZrangeByScore(const char* key, int start, int limit, int direction, int context_id,
            TStateMachine* machine, void* myparam, int timeout) {
    RedisRpcCallback callback;
    RedisEventCallbackContext* ctx = new RedisEventCallbackContext;
    ctx->state_machine = machine;
    ctx->event_type = event_type_;
    ctx->context_id = context_id;
    ctx->param = myparam;
    callback.user_data = ctx;
    callback.fn_action = OnRedisResp;
    return pool_->ZrangeByScore(key, start, limit, direction, context_id, callback, timeout);
  }



 private:
  static void OnRedisResp(int result, redisReply* reply,
                         void* user_data) {
    static CLogger& g_log_framework = CLogger::GetInstance("framework");
    uint64_t time1 = TimeUtil::GetTickMs();
    RedisEventCallbackContext* ctx = 
      reinterpret_cast<RedisEventCallbackContext*>(user_data);
    RedisEventData event_data;
    event_data.result = result;
    event_data.reply = reply;
    event_data.param = ctx->param;
    Event event;
    event.type = ctx->event_type;
    event.data = &event_data;
    uint64_t time2 = TimeUtil::GetTickMs();
    int ret = ctx->state_machine->ProcessEvent(ctx->context_id, &event);
    uint64_t time3 = TimeUtil::GetTickMs();
    if (ret != 0 && event_data.reply != NULL) {
      freeReplyObject(event_data.reply);
      event_data.reply = NULL;
    }
    delete ctx;
    LOGGER_DEBUG(g_log_framework, "On RedisResp End, time_cost1=%d, time_cost2=%d, time_cost3=%d", time2-time1, time3-time2, TimeUtil::GetTickMs()-time3);
  }
 private:
  RedisPool* pool_;
  int event_type_;
};

}
#endif // __BLITZ_REDIS_POOL_H_
