#include "stdafx.h"
#include "get_one_handler.h"
class Worker {
 public:
  typedef Configure TConfigure;
  Worker()  {
  }
  int Init(const TConfigure& conf) {
    int d = conf.get("abc", 1);
    cout << "abc="<<d<<endl;
    LoggerManager& log_manager = LoggerManager::GetInstance();
    std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,fra.log,debug"); 
    log_manager.InitLogger("./logs", log_str1.c_str());
    Logger& log = log_manager.GetLogger("framework");
    LOG_DEBUG(log, "work start");
    reactor_.Init();
    redis_pool_ = new RedisPool(reactor_, RedisPool::TDecoderInitParam());
    redis_pool_->AddService("tcp://127.0.0.1:6379");
    // 初始化handler.
    GetOneHandler*  get_one_handler = new GetOneHandler(redis_pool_, log);
    server_ = new blitz2::ThriftServer(reactor_);
    server_->processor().RegisterHandler("GetOne",
        std::bind(&GetOneHandler::Process, get_one_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    return 0;
  }
  Reactor& GetReactor() {
    return reactor_;
  }
  Acceptor::Observer* GetAcceptorObserver(const std::string& addr) {
    (void)(addr);
    return server_;
  }
  void Uninit() {
    if(server_) {
      delete server_; 
      server_ = NULL;}
    if(redis_pool_) {
      redis_pool_->Uninit();
      delete redis_pool_;
      redis_pool_ = NULL;
    }
  }
  ~Worker() {
    Uninit();
  }
 private:
  Reactor reactor_;
  blitz2::ThriftServer* server_;
  RedisPool* redis_pool_;
};
int main(int argc, char* argv[]) {
  (void)(argc);
  (void)(argv);
  LoggerManager& logger_manager = LoggerManager::GetInstance();
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,main.log,trace"); 
  logger_manager.InitLogger("./logs", log_str1);
  Logger& logger = logger_manager.GetLogger("framework");
  blitz2::ThreadDriver<Worker> driver;
  driver.AddAcceptor("tcp://127.0.0.1:2000");
  driver.AddAcceptor("tcp://127.0.0.1:2001");
  LOG_DEBUG(logger, "start");
  Configure conf;
  conf.put("abc", 123);
  driver.Start(2, conf);
}
