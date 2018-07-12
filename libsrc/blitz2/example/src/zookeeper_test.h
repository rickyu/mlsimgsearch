// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo.
// zookeeper tester.
#include "stdafx.h"
typedef blitz2::SharedStreamSocketChannel<LineMsgDecoder> LineChannel;
typedef std::function<void(SharedLine*, SharedPtr<LineChannel>)> MsgProcessor;
typedef BaseServer<LineMsgDecoder, MsgProcessor> LineServer;
class ZookeeperService;
class CreateContext {
 public:
  static const state_id_t  kStateEnter = 1;
  static const state_id_t kStateWaitZookeeper = 2;
  static const state_id_t kStateEnd = 3;
  static const int kEventZookeeperResp = 1;
  static const action_result_t kResultSuccess = 0;
  static const action_result_t kResultFail = 1;
  CreateContext(ZookeeperService* service, const char* path,
                SharedPtr<LineChannel> channel) {
    service_ = service;
    path_ = path;
    channel_ = channel;
  }
  static int InitStateMachine(StateMachine<CreateContext>* fsm);
  action_result_t OnEnter(uint32_t context_id, StateMachine<CreateContext>* fsm);
  action_result_t OnEnd(uint32_t context_id, StateMachine<CreateContext>* fsm);
  action_result_t ProcessZookeeper(Event* event, uint32_t context_id, 
      StateMachine<CreateContext>* fsm);
 private:
  ZookeeperService* service_;
  string path_;
  SharedPtr<LineChannel> channel_;
};
class LsContext {
};
class ZookeeperService {
 public:
  typedef Configure TConfigure;
  ZookeeperService()
    :logger_(Logger::GetLogger("ZookeeperService")) , 
     fsm_create_("create"),  fsm_ls_("ls") {
      server_ = NULL;
      zookeeper_pool_ = NULL;
    }
  void ProcessMsg(SharedLine* line, SharedPtr<LineChannel> channel);
  int Init(const Configure& conf) ;
  void Uninit();
  Reactor& GetReactor() { return reactor_;}
  Acceptor::Observer* GetAcceptorObserver(const std::string& addr) {
    (void)(addr);
    return server_;
  }
  Logger& logger() { return logger_; }
  ZookeeperPool* zookeeper_pool() { return zookeeper_pool_; }
  static void ZookeeperWatch(zhandle_t* zh, int type, int state, 
      const char* path, void *watcher_ctx);
  

 private:
  Reactor reactor_;
  LineServer* server_;
  Logger& logger_;
  ZookeeperPool* zookeeper_pool_;
  StateMachine<CreateContext> fsm_create_;
  StateMachine<LsContext> fsm_ls_;
};

