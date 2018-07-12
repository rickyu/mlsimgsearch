#ifndef __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_
#define __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_ 1

#include <vector>
#include <string>
#include "blitz/state_machine.h"
#include "blitz/fd_manager.h"
#include "blitz/redis_pool.h"
#include "blitz/thrift_server.h"
#include "Test.h"
// 功能: 收到key,同时 发出key:1, key:2, key:3请求redis,
//   收到后合并返回.
class GetOneHandler : public blitz::ThriftHandler {
 public:
   // 状态机相关数据结构的前置声明.
  struct Input;
  class Context;
  struct Output;
  typedef blitz::StateMachine<Input, Context, Output> StateMachine;

  struct ContextEndParam {
    GetOneHandler* self;
    uint64_t io_id;
    int seqid;
  };

  // public函数.
  GetOneHandler(blitz::Framework& framework, blitz::RedisPool* redis_pool)
    :state_machine_("GetOne"), framework_(framework) {
      redis_pool_ = redis_pool;
      InitStateMachine();
  }
  StateMachine& get_state_machine() { return state_machine_; }

  // ThriftHandler接口实现.
  virtual int Process(::apache::thrift::protocol::TProtocol* iprot, 
                      int seqid, const blitz::IoInfo& ioinfo);
  // 状态机接口回调函数.
  static int OnStateMachineEnd(int res,
                               const GetOneHandler::Output* output,
                               void* user_params);
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

  struct Input {
    std::string key;
    blitz::RedisPool* redis_pool;
  };
  struct Output {
    int result;
    std::string key;
    std::string value;
  };
  class Context {
   public:
    Context() {
      redis_req_count_ = 0;
      redis_resp_count_ = 0;
      redis_pool_ = NULL;
    }
    void SetInput(const Input& input) {
      redis_pool_ = input.redis_pool;
      key_ = input.key;
    }
    void GetOutput(Output* output) { 
      output->result = 0;
      output->key = key_;
      for (int i=0; i<static_cast<int>(redis_reply_.size()); ++i) {
        if (i>0) {
          output->value += ',';
        }
        output->value += redis_reply_[i].first + ':' + redis_reply_[i].second;
      }
    }
    int OnEnter(int context_id, StateMachine* machine);
    int OnWaitRedisMeetRedisResp(blitz::Event* event, int context_id,
                                 StateMachine* machine);
    int OnEnd(int context_id, StateMachine* machine);
   private:
    std::string key_;
    blitz::RedisPool* redis_pool_;
    int redis_req_count_;
    int redis_resp_count_;
    std::vector< std::pair<std::string, std::string> > redis_reply_;
  };
 private:
   StateMachine state_machine_;
   blitz::Framework& framework_;
   blitz::RedisPool* redis_pool_;
};

#endif // __BLITZ_EXAMPLE_THRIFT_SERVER_GET_ONE_HANDLER_H_
