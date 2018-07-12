#include "stdafx.h"

const char* REQUEST_LINE="1234567890\n";
const size_t REQUEST_LINE_LEN = 11;
static void EmptyDelete(uint8_t*) {
}
typedef StressTester<LineMsgDecoder> EchoStressTester;

SharedPtr<OutMsg> GenNextRequest(size_t idx) {
  (void)(idx);
  SharedPtr<OutMsg> outmsg = OutMsg::CreateInstance();
  outmsg->logid().set(idx, 0);

  
  outmsg->AppendData(const_cast<char*>(REQUEST_LINE),
      REQUEST_LINE_LEN, EmptyDelete);
  return outmsg;
}
static size_t g_reported_count = 0;
bool ReportProgress(EchoStressTester* tester, size_t idx) {
  (void)(tester);
  (void)(idx);
  ++g_reported_count;
  return true;
}
int main(int argc, char* argv[]) {
  size_t concurrency;
  size_t req_count;
  po::options_description options_desc("supported command line");
  options_desc.add_options()
    ("help,h","help message")
    ("count,n",po::value<size_t>(&req_count)->default_value(1000), "request count")
    ("concurrency,c",po::value<size_t>(&concurrency)->default_value(10), "concurrency")
    ("addr,a",po::value<string>(), "target address");
  po::variables_map conf_map;
  po::store(po::parse_command_line(argc,argv,options_desc),conf_map);
  po::notify(conf_map);
  if (conf_map.count("help")) {
    cout << options_desc << endl;
    return 1;
  }
  if (!conf_map.count("addr")) {
    cout << "must specify addr" <<endl;
    cout << options_desc << endl;
    return 2;
  }
  string addr = conf_map["addr"].as<string>();
  cout<<"using addr:"<<addr<<endl;
  cout<<"concurrency:"<<concurrency<<endl;
  

  EchoStressTester tester;
  tester.set_progress_reporter(ReportProgress);
  tester.set_req_count(req_count);
  tester.set_concurrency(concurrency);
  LoggerManager& log_manager = LoggerManager::GetInstance();
  log_manager.InitLogger("./logs",
    "blitz2::HalfDuplexService,%L %T %N %f(%F:%l) - %M,echo_ab.log,info");
   log_manager.InitLogger("./logs",
     "blitz2::StreamSocketChannel,%L %T %N %f(%F:%l) - %M,echo_ab.log,info");
  tester.Start(addr, GenNextRequest, 128u);
  tester.Print(cout);
}
