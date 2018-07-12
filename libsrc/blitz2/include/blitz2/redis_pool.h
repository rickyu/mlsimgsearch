// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// 访问Redis相关的类和函数,包括
//  RedisPool - Redis连接池
//  RedisEventFire - 状态机Event包装,访问RedisPool得到应答后回调状态机

#ifndef __BLITZ2_REDIS_POOL_H_
#define __BLITZ2_REDIS_POOL_H_ 1
#include <functional>
#include <boost/lexical_cast.hpp>
#include <blitz2/redis_msg.h>
#include <blitz2/state_machine.h>
#include <blitz2/outmsg.h>
#include <blitz2/blitz2.h>
 // namespace blitz2
namespace blitz2 
{
// Redis连接池,继承自ServicePool，主要增加请求的编码.
static void FreeRedisCommand(void* data) {
  free(data);
}
template<template<typename T, typename T2> class Selector>
class RedisPool : public ServicePool<std::string, Selector, RedisReplyDecoder> {
 public:
  typedef ServicePool<std::string, Selector, RedisReplyDecoder> TParent;
  typedef RedisPool<Selector> TMyself;
  typedef typename TParent::TQueryCallback TQueryCallback;
  typedef RedisReplyDecoder::TInitParam TDecoderInitParam;

  RedisPool(Reactor& reactor, const TDecoderInitParam& decoder_param)
    :TParent(reactor, decoder_param)  {
    }

  // 给Redis发送Get请求。异步函数，pool保证callback被调用到.
  void Get(const char* key, const LogId& logid,
      const TQueryCallback& callback,
          uint32_t timeout) {
    char* cmd = NULL;
    int len = redisFormatCommand(&cmd, "GET %s", key);
    return Command(cmd, len, logid, callback, key, timeout);

  }
  
  
  // 给Redis发送Set请求。异步函数，pool保证callback被调用到.
  int Set(const char* key, const char* value, const LogId& log_id,
            const TQueryCallback& callback, uint32_t timeout) {
    char* cmd = NULL;
    int len = redisFormatCommand(&cmd, "SET %s %s", key, value);
    return Command(cmd, len, log_id, callback, key, timeout);
  }

  void Command(char* command, // 需要由pool来释放.
               int len,
               const LogId& log_id, 
               const TQueryCallback& callback,
               const char* select_key,
               uint32_t timeout);
};
template<template<typename T, typename T2> class Selector>
void RedisPool<Selector>::Command(char* command,
                        int len,
                        const LogId& log_id,
                        const TQueryCallback& callback,
                        const char* select_key,
                        uint32_t timeout) {
  SharedPtr<OutMsg> out_msg =  OutMsg::CreateInstance();
  assert(out_msg.get_pointer() != NULL);
  out_msg->AppendData(command, len,  FreeRedisCommand);
  out_msg->logid() = log_id;
  Query(out_msg, callback, select_key, timeout, 1);
  out_msg->Release();
}
// StateMachine支持，Event::data指向该结构体.
class RedisEvent: public Event {
 public:
  RedisEvent(int type, EQueryResult result,
      RedisReplyDecoder::TMsg* reply, void* param)
    :Event(type), result_(result), reply_(reply) {
      param_ = param;
  }
  EQueryResult result() const { return result_; }
  RedisReplyDecoder::TMsg* reply() { return reply_;}
  void* param() const { return param_;}
 private:
  EQueryResult result_;
  SharedPtr<RedisReplyDecoder::TMsg> reply_;
  void* param_;
 };

template <typename StateMachine, typename Redis>
class RedisEventFire {
 public:
  typedef StateMachine TStateMachine;
  typedef Redis TRedisPool;
  typedef typename TRedisPool::TQueryCallback TQueryCallback;
  typedef RedisEventFire<StateMachine,Redis> TMyself;
  RedisEventFire(TRedisPool* pool, int event_type) {
    pool_ = pool;
    event_type_ = event_type;
  }
  TRedisPool* get_pool()  { return pool_; }
  void FireGet(const char* key, uint32_t context_id, TStateMachine* machine,
               void* myparam, int timeout, const LogId& logid) {
    return pool_->Get(key, logid,
        std::bind(OnRedisResp, machine, context_id, event_type_, myparam,
          std::placeholders::_1, std::placeholders::_2),
        timeout);
  }
  void FireSet(const char* key, const char* value, uint32_t context_id,
             TStateMachine* machine, void* myparam, int timeout,
             const LogId& logid) {
    pool_->Set(key, value, logid, 
        std::bind(OnRedisResp, machine, context_id, event_type_, myparam,
          std::placeholders::_1, std::placeholders::_2),
        timeout);
  }
 private:
  static void OnRedisResp(TStateMachine* machine, uint32_t ctxid, int event_type,
      void* param, EQueryResult result,  RedisReplyDecoder::TMsg* reply) {
    RedisEvent event(event_type, result, reply, param);
    machine->ProcessEvent(ctxid, &event);
   }
 private:
  TRedisPool* pool_;
  int event_type_;
};
} // namespace blitz2

#endif // __BLITZ2_REDIS_POOL_H_
