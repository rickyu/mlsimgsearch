#include "blitz/framework.h"
#include "blitz/thrift_server.h"
#include "utils/logger_loader.h"
#include "get_one_handler.h"

class MyThreadData {
 public:
  int Init() {
    std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
    blitz::ServiceConfig service_config;
    service_config.set_addr("tcp://127.0.0.1:6379");
    redis_pool.AddService(service_config);
    return 0;
  }
  int Uninit() {
    return 0;
  }
 private:
  blitz::RedisPool redis_pool(framework, selector);
};
class MsgProcessor {
 public:
  Process(ProcessorData* data, ioinfo io, msg msg);
};
int main() {
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,thriftserver.log,,"); 
  log_str1 += "0";
  InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  // 设置service的线程数
  framework.set_thread_count(2);
  framework.set_max_task_execute_time(500000);
  framework.Init();
  // 初始化RedisPool
  // 初始化handler.
  GetOneHandler get_one_handler(framework, &redis_pool);
  blitz::ThriftServer server(framework);
  server.RegisterHandler("GetOne", &get_one_handler);
  server.Bind("tcp://127.0.0.1:5556");
  // TcpServer server2(decoder_factory, processor_factory);
  framework.Start();
  framework.Uninit();
  return 0;
}
