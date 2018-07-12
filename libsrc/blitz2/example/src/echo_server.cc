#include "stdafx.h"
namespace po = boost::program_options;
typedef blitz2::SharedStreamSocketChannel<LineMsgDecoder> LineChannel;

void ProcessMsg(SharedLine* msg, SharedPtr<LineChannel> channel) {
  SharedPtr<OutMsg> outmsg = OutMsg::CreateInstance();
  outmsg->AppendData(MsgData(msg->buffer(), msg->offset(), msg->length()));
  channel->SendMsg(outmsg);
}
typedef void (*FnProcessMsg)(SharedLine*, SharedPtr<LineChannel>);
typedef BaseServer<LineMsgDecoder, FnProcessMsg> EchoServer;
class EchoService {
 public:
  typedef Configure TConfigure;
  EchoService()
    :logger_(Logger::GetLogger("EchoService")) {
    }
  int Init(const Configure& conf) ;
  void Uninit();
  Reactor& GetReactor() { return reactor_;}
  Acceptor::Observer* GetAcceptorObserver(const std::string& addr) {
    (void)(addr);
    return server_;
  }

 private:
  Reactor reactor_;
  EchoServer* server_;
  Logger& logger_;
};

int EchoService::Init(const Configure& conf) {
  (void)(conf);
  LoggerManager& log_mgr = LoggerManager::GetInstance();
  log_mgr.InitLogger("./logs",
     "blitz2::StreamSocketChannel,%L %T %N %f(%F:%l) - %M,echo_server.log,info");
  LOG_DEBUG(logger_, "work start");
  reactor_.Init();
  server_ = new EchoServer(reactor_);
  server_->set_decoder_init_param(128u);
  server_->processor() =  &ProcessMsg;
  return 0;
}
void EchoService::Uninit() {
  if(server_) {
    delete server_; 
    server_ = NULL;
  }
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
  blitz2::ThreadDriver<EchoService> driver;
  driver.AddAcceptor("tcp://*:3000");
  driver.Start(2, conf);
  cout<<"exited"<<endl;
}
