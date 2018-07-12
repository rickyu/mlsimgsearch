#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include "blitz/framework.h"
#include "blitz/service_pool.h"
#include "blitz/redis_msg.h"
#include "utils/logger_loader.h"
#include "blitz/mysql_pool.h"

class InitTask {
  public:
    static void FillTask(Task& task,ServicePool* redis_pool){
      task.fn_callback = TaskCallback;
      task.fn_free = NULL;
      task.arg1.ptr = redis_pool;
      task.arg2.u64 = 0;
    }
    static void TaskCallback(const RunContext& context,TaskArg& arg1,TaskArg& arg2){
      using namespace std;

      RpcRequest request;
      char* command = NULL;
      RedisReqData req_data = {0};
      request.fn_callback = Callback1;
      request.callback_ctx = NULL;

      req_data.len  = redisFormatCommand(&req_data.command,"set mykey hello");
      request.request_data = &req_data;
      request.log_id = 10;
      request.timeout = 200;
      request.retry_count = 2;
      cout << "issue query"<<endl;
      ((ServicePool*)arg1.ptr)->Query(request);
    }
    static void Callback1(int result,const InMsg* response,void* user_data){
      using namespace std;
      
      if(result == 0){
        redisReply* reply = (redisReply*)response->data;
        if(reply->type == REDIS_REPLY_STATUS){
          cout << reply->str<<endl;
        }
        else {
          cout << "wrong reply"<<endl;
        }
        if(response->fn_free){
          response->fn_free(response->data,response->free_ctx);
        }
      }
      else {
        cout << "result="<<result<<endl;
      }
    }
};

class App {
  public:
    static App& GetInstance() {
      static App app;
      return app;
    }
};
void TimerCallback(void* user_arg){
  using namespace std;
  static int log_id = 0;
  ServicePool* pool = (ServicePool*)user_arg;
  RpcRequest request;
  char* command = NULL;
  RedisReqData req_data = {0};
  request.fn_callback = InitTask::Callback1;
  request.callback_ctx = NULL;

  req_data.len  = redisFormatCommand(&req_data.command,"set mykey hello");
  request.request_data = &req_data;
  request.log_id = ++ log_id;
  request.timeout = 200;
  request.retry_count = 2;
  cout << "issue query"<<endl;
  pool->Query(request);
}
int main(){
  using namespace std;

  
  InitLogger("./","framework,%L %T %N %P:%p %f(%F:%l) - %M,framework.log,,0");
  Framework framework;
  framework.Init();
  vector<ServiceNode> nodes;
  ServiceNode node;
  MysqlConnect mysql_connect(framework);
  MysqlNode mysql_node;
  mysql_node.addr = "tcp://127.0.0.1:3306";
  mysql_node.user = "root";
  mysql_node.db = "dolphin";
  mysql_node.passwd="";
  mysql_connect.Init(mysql_node);
  
  framework.Start();
  framework.Uninit();
  cout << "The instance count of OutMsg is " << OutMsg::s_instance_count_ << endl;
  return 0;
}
