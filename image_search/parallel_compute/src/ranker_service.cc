#include <common/configure.h>
#include "ranker_handler.h"
#include "thrift_exec_thread.h"
#include <common/thread_pool.h>
#include <common/http_client.h>
#include <common/constant.h>
#include <bof_common/object_selector.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <beansdb_client/beansdb_clusters.h>

CLogger& g_logger = CLogger::GetInstance("spranker");

static int _thread_pool_add_threads(ThreadPool &pool,
                                    int num,
                                    const blitz::ThriftTaskMgrServer *thrift_mgr) {
  for (int i = 0; i < num; i++) {
    Thread *thread = new ThriftExecThread(thrift_mgr);
    if (NULL == thread) {
      LOGGER_ERROR(g_logger, "act=new JobThread failed, out of mem");
      return -1;
    }
    if (CR_SUCCESS != thread->Init()) {
      LOGGER_ERROR(g_logger, "act=Thread::Init failed");
      return -2;
    }
    pool.AddThread(thread);
  }
  return 0;
}

int main(int argc, char **argv) {
  Configure::GetInstance()->Init(argv[1]);
  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    return -1;
  }

  CreateObjectFactory();
  std::vector<std::string> clusters = Configure::GetInstance()->GetStrings("beansdb_clusters");
  if (BCR_SUCCESS != BeansdbClusters::GetInstance()->AddClusters(clusters)) {
    return -2;
  }

  int thread_num = Configure::GetInstance()->GetNumber<int>("thread_num");
  std::string server_addr = Configure::GetInstance()->GetString("server_address");
  int thrift_server_timeout = Configure::GetInstance()->GetNumber<int>("thrift_timeout");

  HttpClient::GlobalInit();
  blitz::Framework framework;
  framework.set_thread_count(1);
  framework.Init();
  boost::shared_ptr<SpatialRankerMsgHandler> handler(new SpatialRankerMsgHandler());
  int ret = handler->Init();
  if (CR_SUCCESS != ret) {
    LOGGER_ERROR(g_logger, "act=ImgGenMsgHandler failed, ret=%d", ret);
    return -4;
  }
  SpatialRankerMsgProcessor processor(handler);
  blitz::ThriftTaskMgrServer thrift_mgr(framework, &processor);
  thrift_mgr.set_timeout(thrift_server_timeout);
  thrift_mgr.Bind(server_addr.c_str());

  ThreadPool thread_pool(thread_num, thread_num);
  if (!thread_pool.Init()) {
    LOGGER_ERROR(g_logger, "memcached init failed");
    return -5;
  }
  if (0 != _thread_pool_add_threads(thread_pool, thread_num, &thrift_mgr)) {
    return -6;
  }
  thread_pool.StartAll();
  framework.Start();
  framework.Uninit();
  thread_pool.KillAll();
  HttpClient::GlobalFinalize();
  DestroyObjectFactory();
  
  return 0;
}
