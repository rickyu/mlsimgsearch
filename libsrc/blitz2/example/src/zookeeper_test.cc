#include "zookeeper_test.h"

namespace po = boost::program_options;


const char* UNKNOWN_CMD = "unknown cmd\n";
const size_t UNKNOWN_CMD_LEN = 12;
static void DestroyDoNothing(uint8_t* ) {
}
#define ARRAY_COUNT(array) sizeof(array)/sizeof(array[0])
int CreateContext::InitStateMachine(StateMachine<CreateContext>* fsm) {
  typedef StateMachine<CreateContext>::State State;
  typedef StateMachine<CreateContext>::Transition Transition; 
  static const State kStates[] = {
    State(kStateEnter, &CreateContext::OnEnter,
          ActionArcs()
          (kResultSuccess, kStateWaitZookeeper)
          (kResultFail, kStateEnd)),

    State(kStateWaitZookeeper, NULL, ActionArcs()),
    State(kStateEnd, &CreateContext::OnEnd, ActionArcs())
  };
  int rc = fsm->SetStates(kStates, ARRAY_COUNT(kStates), kStateEnter, kStateEnd);
  static const Transition kTransitions[] = {
    Transition(kStateWaitZookeeper, kEventZookeeperResp,
        &CreateContext::ProcessZookeeper,
        ActionArcs()(kResultSuccess, kStateEnd)(kResultFail, kStateEnd))
  };
  rc = rc!=0?rc:fsm->SetTransitions(kTransitions, ARRAY_COUNT(kTransitions));
  return rc;
}
action_result_t CreateContext::OnEnter(uint32_t context_id,
    StateMachine<CreateContext>* fsm) { 
 LOG_DEBUG(service_->logger(), "Enter context_id=%u", context_id);
 ZookeeperEventFire<StateMachine<CreateContext> > fire(service_->zookeeper_pool());
 fire.FireCreate(path_.c_str(), "new", 3, &ZOO_OPEN_ACL_UNSAFE, 0, context_id, fsm,
     kEventZookeeperResp);
 return kResultSuccess;
}
action_result_t CreateContext::ProcessZookeeper(Event* event, uint32_t context_id, 
      StateMachine<CreateContext>* fsm) {
  (void)(context_id);
  (void)(fsm);
  ZookeeperStringEvent* zevent = dynamic_cast<ZookeeperStringEvent*>(event);
  SharedPtr<OutMsg> outmsg = OutMsg::CreateInstance();
  if (zevent->rc != ZOK) {
    LOG_ERROR(service_->logger(), "recved_resp ctxid=%u rc=%d", 
        context_id, zevent->rc);
    outmsg->AppendConstString("Fail\r\n", 6);
    channel_->SendMsg(outmsg);
  } else {
    LOG_DEBUG(service_->logger(), "recved_resp ctxid=%u", context_id);
    outmsg->AppendConstString("OK\r\n", 4);
    channel_->SendMsg(outmsg);
  }
  return kResultSuccess;
}
action_result_t CreateContext::OnEnd(uint32_t context_id,
    StateMachine<CreateContext>* fsm) {
 (void)(context_id);
 (void)(fsm);
 LOG_DEBUG(service_->logger(), "end ctxid=%u", context_id);
 return kResultSuccess;
}

void ZookeeperService::ProcessMsg(SharedLine* msg,
    SharedPtr<LineChannel> channel) {
  string line(reinterpret_cast<char*>(msg->buffer()->mem() + msg->offset()), msg->length());
  vector<string> parts;
  boost::trim(line);
  boost::split(parts, line, boost::is_any_of(" "));
  if (parts[0] == "create") {
    CreateContext* context = new CreateContext(this, parts[1].c_str(), channel);
                                              
    fsm_create_.StartOneContext(context, StateMachine<CreateContext>::ContextDeleter());
    return;
  } else if(parts[0] == "ls") {
  } else {
    SharedPtr<OutMsg> outmsg = OutMsg::CreateInstance();
    outmsg->AppendData(const_cast<char*>(UNKNOWN_CMD), UNKNOWN_CMD_LEN, 
        DestroyDoNothing);
    channel->SendMsg(outmsg);
  }
  
}
int ZookeeperService::Init(const Configure& conf) {
  using namespace std;
  (void)(conf);
  LoggerManager& log_mgr = LoggerManager::GetInstance();
  log_mgr.InitLogger("./logs",
     "blitz2::StreamSocketChannel,%L %T %N %f(%F:%l) - %M,zookeeper_test.log,debug");
  log_mgr.InitLogger("./logs",
     "ZookeeperService,%L %T %N %f(%F:%l) - %M,zookeeper_test.log,debug");
  log_mgr.InitLogger("./logs",
     "zookeeper,%L %T %N %f(%F:%l) - %M,zookeeper_test.log,debug");
  log_mgr.InitLogger("./logs",
     "blitz2::ThreadDriver,%L %T %N %f(%F:%l) - %M,zookeeper_test.log,debug");
  log_mgr.InitLogger("./logs",
     "blitz2::Acceptor,%L %T %N %f(%F:%l) - %M,zookeeper_test.log,debug");
  LOG_DEBUG(logger_, "work start");
  reactor_.Init();
  zookeeper_pool_ = new ZookeeperPool(reactor_);
  zookeeper_pool_->Init("127.0.0.1:2181", ZookeeperWatch, this, 0, 
      20000, 1);
  CreateContext::InitStateMachine(&fsm_create_);
  server_ = new LineServer(reactor_);
  server_->set_decoder_init_param(128u);
  server_->processor() =  std::bind(&ZookeeperService::ProcessMsg, this, 
      placeholders::_1, placeholders::_2);

  return 0;
}
void ZookeeperService::Uninit() {
  if(server_) {
    delete server_; 
    server_ = NULL;
  }
  if (zookeeper_pool_) {
    delete zookeeper_pool_;
    zookeeper_pool_ = NULL;
  }
}
void ZookeeperService::ZookeeperWatch(zhandle_t* zh, int type, int state, 
      const char* path, void *watcher_ctx) {
  (void)(zh);
  (void)(type);
  (void)(state);
  (void)(path);
  (void)(watcher_ctx);
}
int main(int argc, char* argv[]) {
  po::options_description options_desc("supported command line");
  options_desc.add_options()
    ("help,h","help message")
    ("name,n",po::value<string>(), "process name");
  po::variables_map conf_map;
  po::store(po::parse_command_line(argc,argv,options_desc),conf_map);
  po::notify(conf_map);
  if (conf_map.count("help")) {
    cout << options_desc << endl;
    return 1;
  }
  Configure conf;
  blitz2::ThreadDriver<ZookeeperService> driver;
  driver.AddAcceptor("tcp://*:3000");
  driver.Start(1, conf);
  cout<<"exited"<<endl;
}
