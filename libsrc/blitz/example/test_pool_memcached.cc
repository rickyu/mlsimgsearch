#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include "blitz/framework.h"
#include "blitz/service_pool.h"
#include "blitz/memcached_msg.h"
#include "utils/logger_loader.h"


#define MEMCACHED_DEFAULT_COMMAND_SIZE 350

class InitTask {
public:
  static void FillTask(Task& task, ServicePool* pool){
    task.fn_callback = TaskCallback;
    task.fn_free = NULL;
    task.arg1.ptr = pool;
    task.arg2.u64 = 0;
  }

  static void TaskCallback(const RunContext& context,TaskArg& arg1,TaskArg& arg2){
    using namespace std;

    RpcRequest request;
    char* command = NULL;
    MemcachedReqData req_data = {0};
    request.fn_callback = Callback1;
    request.callback_ctx = NULL;
    cout << "run init task"<<endl;

    const char *keys[] = { "key1", "key2", "key3" };
    size_t keys_len[3];
    keys_len[0] = 4;
    keys_len[1] = 4;
    keys_len[2] = 4;
    req_data.command = new char[MEMCACHED_MAX_BUFFER];
    const char *key = "testkey9";
    size_t key_len = strlen(key);
    const char *data = "key99 testest";
    size_t data_len = strlen(data);
    req_data.len = MemcachedReqFunction::Get(req_data.command, MEMCACHED_MAX_BUFFER, key, key_len);
    //req_data.len  = MemcachedReqFunction::Mget(req_data.command, MEMCACHED_MAX_BUFFER, keys, keys_len, 3);
    //req_data.len  = MemcachedReqFunction::Delete(req_data.command, MEMCACHED_MAX_BUFFER, "key4", 4, 0);
    //req_data.len  = MemcachedReqFunction::Decrease(req_data.command, MEMCACHED_MAX_BUFFER, "key4", 4, 1);
    //req_data.len  = MemcachedReqFunction::Storage(req_data.command, MEMCACHED_MAX_BUFFER, key, key_len, data, data_len, 0, 5, 0, SET_OP);
    printf("request is %s\n", req_data.command);
    request.request_data = &req_data;
    request.log_id = 10;
    request.timeout = 200;
    request.retry_count = 2;
    ((ServicePool*)arg1.ptr)->Query(request, key, key_len);
  }
  static void Callback1(int result,const InMsg* response,void* user_data){
    using namespace std;
    cout << "InitTask::Callback"<<endl;

    if (!response)
      return;
		
    char *str = (char*)response->data;
    if (str) {
      printf("%.*s\n", response->bytes_length, str);
    }

    if(response->fn_free){
      response->fn_free(response->data,response->free_ctx);
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

int main(){
  using namespace std;
  //  InitLogger("./","framework,%L %T %N %P:%p %f(%F:%l) - %M,framework.log,,0");

  
  Framework framework;
  framework.Init();
  vector<ServiceNode> nodes;
  ServiceNode node;
  
  MemcachedReplyDecoderFactory decoder_factory;
  ServicePool pool(&framework,&decoder_factory,NULL,NULL,MemcachedReqFunction::GetOutMsg, SERVICE_POOL_CONSISTENT_HASH);
  nodes.clear();
  node.is_short_connect = false;
  node.addr = "tcp://127.0.0.1:12111";
  nodes.push_back(node);
  node.addr = "tcp://127.0.0.1:12112";
  nodes.push_back(node);
  node.addr = "tcp://127.0.0.1:12113";
  nodes.push_back(node);
  pool.Init(nodes, false);
  Task task;
  InitTask::FillTask(task, &pool);
  framework.PostTask(task);
  //framework.PostTask(task);

  framework.Start();
  framework.Uninit();
  pool.Close();
  cout << "The instance count of OutMsg is " << OutMsg::s_instance_count_ << endl;

  return 0;
}
