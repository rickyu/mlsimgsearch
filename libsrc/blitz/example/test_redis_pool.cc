#include <iostream>
#include <boost/shared_ptr.hpp>
#include "blitz/framework.h"
#include "blitz/thrift_frame_msg.h"
#include <protocol/TBinaryProtocol.h>
#include "utils/logger_loader.h"
#include "blitz/buffer.h"
#include "blitz/redis_pool.h"

class Runner {
 public:
  Runner(blitz::Framework& framework) : framework_(framework) {
    redis_pool_ = NULL;
    log_id_ = 0;
  }
  int Init() {
    std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
    redis_pool_ = new blitz::RedisPool(framework_, selector);
    blitz::ServiceConfig service_config;
    service_config.set_connect_count(5);
    service_config.set_addr("tcp://127.0.0.1:6379");
    int ret = redis_pool_->AddService(service_config);
    if (ret != 0) {
      printf("add service fail\n");
      return 1;
    }
    PostTask();
    return 0;
  }
  void Uninit() {
    delete redis_pool_;
    redis_pool_ = NULL;
  }
  void PostTask() {
    blitz::Task task;
    task.fn_callback   = MyTaskCallback;
    task.fn_delete = NULL;
    task.args.arg1.ptr = this;
    task.args.arg2.u64 = 0;
    usleep(10000);
    int ret = framework_.PostTask(task);
    if (ret != 0) {
      printf("Post Task Fail\n");
    }
  }
  struct MyContext {
    Runner* runner;
  };
  void IssueGet() {
    blitz::RedisRpcCallback callback;
    MyContext* my_context = blitz::CallbackContextTo<MyContext>(
                                                    callback.args);
    my_context->runner = this;
    callback.fn_action = OnRedisGetCallback;
    redis_pool_->Get("key_123", ++log_id_, callback);

    // redis_pool.Get();
  }
  void IssueSet() {
    blitz::RedisRpcCallback callback;
    MyContext* my_context = blitz::CallbackContextTo<MyContext>(
                                                    callback.args);
    my_context->runner = this;
    callback.fn_action = OnRedisSetCallback;
    redis_pool_->Set("key_123", "value_234", ++log_id_, callback);
  }
  static void OnRedisSetCallback(int result, redisReply* reply,
                                 const blitz::CallbackContext& context_args) {
    printf("OnRedisSetCallback result=%d\n", result);
    const MyContext* my_context = 
                       blitz::CallbackContextTo<MyContext>(context_args);
    if (reply) {
      if (reply->type == REDIS_REPLY_STATUS) {
        printf("redis reply %s\n", reply->str);
      }
      freeReplyObject(reply);
    }
    if (result == 0) {
      my_context->runner->IssueGet();
    } else {
      my_context->runner->PostTask();
    }

  }
  static void OnRedisGetCallback(int result, redisReply* reply,
                                 const blitz::CallbackContext& context_args) {
     printf("OnRedisGetCallback result=%d\n", result);
    const MyContext* my_context = blitz::CallbackContextTo<MyContext>(context_args);
    if (reply) {
      if (reply->type == REDIS_REPLY_STRING) {
        printf("redis reply %s\n", reply->str);
      }
      freeReplyObject(reply);
    }
    if (result == 0) {
      my_context->runner->IssueGet();
    } else {
      my_context->runner->PostTask();
    }
  }

 private:
  static void MyTaskCallback( const blitz::RunContext& run_context, 
                      const blitz::TaskArgs& args) {
      Runner* self = reinterpret_cast<Runner*>(args.arg1.ptr);
      printf("task callback\n");
      self->IssueSet();
   }
 
 private:
  blitz::Framework& framework_;
  blitz::RedisPool* redis_pool_;
  int log_id_;
};

int main(int argc,char* argv[]) {
  srand(time(NULL));
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,test_redis_pool.log,,"); 
  log_str1 += "10";
  InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  framework.Init();
  framework.set_max_task_execute_time(100000);
  Runner runner(framework);
  runner.Init();
  framework.Start();
  framework.Uninit();
  runner.Uninit();
  return 0;
}
