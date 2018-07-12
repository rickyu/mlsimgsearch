#include "blitz/framework.h"
#include "blitz/http_server.h"
#include "utils/logger_loader.h"
#include "blitz/redis_pool.h"


class HelloHandler : public blitz::HttpHandler {
 public:
  HelloHandler(blitz::Framework& framework, blitz::SimpleHttpServer& server)
    : framework_(framework), server_(server) {
  }
  virtual int Process(const std::shared_ptr<blitz::HttpRequest>& request,
                     const blitz::HttpConnection& conn) {
    blitz::HttpResponse resp;
    resp.SetHeader("Server", "Blitz::SimpleHttpServer");
    resp.SetHeader("Content-Type", "text/plain");
    resp.SetContent("this is my hello!!!!");
    server_.SendResponse(conn, &resp);
    return 0;
  }
 private:
  blitz::Framework& framework_;
  blitz::SimpleHttpServer& server_;
};
int main(int argc, char* argv[]) {
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,thriftserver.log,,"); 
  log_str1 += "30";
  // InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  // 设置service的线程数
  int thread_number = 4;
  if (argc >= 2) {
    thread_number = atoi(argv[1]);
  }
  std::list<int> a;
  a.push_back(1);
  a.push_back(2);
  std::list<int>::iterator iter;
  iter = a.begin();
  printf("iter=%d\n", *iter);
  a.push_front(3);
  printf("iter=%d\n", *iter);
  --iter;
  printf("iter=%d\n", *iter);
  
  printf("thread_number=%d\n", thread_number);
  framework.set_thread_count(thread_number);
  framework.set_max_task_execute_time(500000);
  framework.Init();
  // 初始化handler.
  blitz::SimpleHttpServer server(framework);
  HelloHandler hello_handler(framework, server);
  server.RegisterHandler("/hello", &hello_handler);
  server.Bind("tcp://*:5556");
  framework.Start();
  framework.Uninit();
  return 0;
}
