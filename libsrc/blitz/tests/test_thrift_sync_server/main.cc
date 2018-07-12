#include <stdio.h>
#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <blitz/framework.h>
#include <blitz/thrift_server.h>
#include <utils/logger_loader.h>
#include "TestTime.h"
namespace po=boost::program_options;
CLogger& g_logger = CLogger::GetInstance("HANDLER");
class TestTimeHandler : virtual public TestTimeIf {
 public:
  TestTimeHandler() {
    // Your initialization goes here
  }

  int32_t Wait(const int32_t time) {
    LOGGER_LOG(g_logger, "sleep %d ms", time);
    usleep(time*1000);
    return 0;
  }

};

int main(int argc, char* argv[]) {
  std::string log_str1("HANDLER,%L %T %N %P:%p %f(%F:%l) - %M,thriftserver.log,,"); 
  log_str1 += "0";
  InitLogger("./", log_str1.c_str());
  std::string log_str2("framework,%L %T %N %P:%p %f(%F:%l) - %M,thriftserver.log,,"); 
  log_str2 += "30";
  InitLogger("./", log_str2.c_str());
  po::options_description options_desc("command arg");
  options_desc.add_options()
    ("help,h", "help message")
    ("thread_count,t", po::value<int>(), "thread count")
    ("task_timeout,k", po::value<int>(), "task timeout")
    ("bind,b", po::value<std::string>(), "bind addr");
  po::variables_map conf_map;
  po::store(po::parse_command_line(argc,argv,options_desc), conf_map);
  po::notify(conf_map); 
  if(conf_map.count("help")) {
    std::cout << options_desc << std::endl;
    return 0;
  }
  int thread_count = 1;
  if(conf_map.count("thread_count")) {
    thread_count = conf_map["thread_count"].as<int>();
  }
  if(!conf_map.count("bind")) {
    std::cout << "must specify bind" << std::endl;
    std::cout << options_desc << std::endl;
    return 1;
  }
  std::string bind_addr = conf_map["bind"].as<std::string>();
  int timeout = 1000;
  if(conf_map.count("task_timeout")) { 
    timeout = conf_map["task_timeout"].as<int>();
  }
  std::cout << "bind:" << bind_addr 
            << " thread:" << thread_count 
            << " timeout:" << timeout
            << std::endl;
  blitz::Framework framework;
  // 设置service的线程数
  framework.set_thread_count(2);
  framework.set_max_task_execute_time(500000);
  framework.Init();
  boost::shared_ptr<TestTimeHandler> handler(new TestTimeHandler());
  TestTimeProcessor processor(handler);
  blitz::ThriftSyncServer server(framework, &processor);
  server.Bind(bind_addr.c_str());
  server.set_timeout(1000);
  server.Start(thread_count);
  server.set_timeout(timeout);
  framework.Start();
  server.Stop();
  framework.Uninit();
  return 0;
}
