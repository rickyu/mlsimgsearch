#ifndef __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_
#define __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_ 1

#include "stdafx.h"
#include "Test.h"
// 功能: 收到key,同时 发出key:1, key:2, key:3请求redis,
//   收到后合并返回.
class GetOneHandler {
 public:
   // 状态机相关数据结构的前置声明.
  class Context;
  typedef blitz2::StateMachine<Context> StateMachine;
  typedef blitz2::RedisEventFire<StateMachine, RedisPool> TRedisEventFire;


  // public函数.
  GetOneHandler(RedisPool* redis_pool, Logger& logger)
    :logger_(logger), state_machine_("GetOne") {
      redis_pool_ = redis_pool;
      InitStateMachine();
  }
  StateMachine& get_state_machine() { return state_machine_; }

  // ThriftHandler接口实现.
  int Process(::apache::thrift::protocol::TProtocol* iprot, 
                      int seqid, blitz2::ThriftServer::TSharedChannel* channel);
  // 状态机接口回调函数.
  static int OnStateMachineEnd(GetOneHandler::Context* context, int seqid,
      SharedPtr<ThriftServer::TSharedChannel> channel);
 protected:
  void InitStateMachine();

 // 状态机实现.
 public:
  enum {
      kStateEnter = 1,
      kStateWaitRedis = 2,
      kStateEnd = 3
  };
  enum {
      kEventRedisResp = 1
  };

  class Context {
   public:
    Context(Logger& logger, const std::string& key, RedisPool* redis_pool)
      :logger_(logger), key_(key) {
      redis_req_count_ = 0;
      redis_resp_count_ = 0;
      redis_pool_ = redis_pool;
    }
    int get_result() const { return 0; }
    const std::string& get_key() const { return key_; }
    const std::string& get_value() const { return value_; }
    action_result_t OnEnter(uint32_t context_id, StateMachine* machine);
    action_result_t OnWaitRedisMeetRedisResp(blitz2::Event* event, uint32_t context_id,
                                 StateMachine* machine);
    action_result_t OnEnd(uint32_t context_id, StateMachine* machine);
   private:
    Logger& logger_;
    std::string key_;
    std::string value_;
    RedisPool* redis_pool_;
    int redis_req_count_;
    int redis_resp_count_;
    std::vector< std::pair<std::string, std::string> > redis_reply_;
  };
 private:
  Logger& logger_;
  StateMachine state_machine_;
  RedisPool* redis_pool_;
};

#endif // __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_
