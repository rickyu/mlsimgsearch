#include <common/configure.h>
#include "merge_handler.h"
#include "thrift_exec_thread.h"
#include <common/thread_pool.h>
#include <common/http_client.h>
#include <common/constant.h>
#include <bof_common/object_selector.h>
#include <bof_searcher/indexer.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <beansdb_client/beansdb_clusters.h>

CLogger& g_logger = CLogger::GetInstance("merge");

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

  if (BCR_SUCCESS != BeansdbClusters::GetInstance()->AddClusters(Configure::GetInstance()->GetStrings("beansdb_clusters"))) {
    return -2;
  }

  int begin_bucket_id = Configure::GetInstance()->GetNumber<int>("begin_bucket_id");
  int end_bucket_id = Configure::GetInstance()->GetNumber<int>("end_bucket_id");
  BOFIndex::GetInstance()->SetBeginWordId(begin_bucket_id);
  //BOFIndex::GetInstance()->LoadFileData(begin_bucket_id, end_bucket_id);

  int thread_num = Configure::GetInstance()->GetNumber<int>("thread_num");
  std::string server_addr = Configure::GetInstance()->GetString("server_address");
  int thrift_server_timeout = Configure::GetInstance()->GetNumber<int>("thrift_timeout");
  std::string quantizer_name = Configure::GetInstance()->GetString("quantizer_name");
  BOFQuantizer *quantizer = BOFQuantizerSelector::GetInstance()->GenerateObject(quantizer_name);
  if (NULL == quantizer) {
    LOGGER_ERROR(g_logger, "Generate quantizer selector failed. (%s)", quantizer_name.c_str());
    return -3;
  }
  std::string vocabulary_path = Configure::GetInstance()->GetString("vocpath");
  std::string voc_node_name = Configure::GetInstance()->GetString("voc_node_name");
  quantizer->SetVocabulary(vocabulary_path, voc_node_name);
  std::string project_p_path = Configure::GetInstance()->GetString("project_p");
  std::string median_path = Configure::GetInstance()->GetString("median");
  std::string word_weight_path = Configure::GetInstance()->GetString("word_weight");
  quantizer->SetPath(median_path,project_p_path,word_weight_path);
  int ret = quantizer->Init();
  if (0 != ret) {
    BOFQuantizerSelector::GetInstance()->ReleaseObject(quantizer);
    LOGGER_ERROR(g_logger, "Quantizer init failed. (%d)", ret);
    return -5;
  }
  int K = quantizer->GetVocabularySize();
  BOFQuantizerSelector::GetInstance()->ReleaseObject(quantizer);

  std::string index_hint_path = Configure::GetInstance()->GetString("index_hint_path");
  std::string index_data_path = Configure::GetInstance()->GetString("index_data_path");
  if (0 != BOFIndex::GetInstance()->Init(K, index_hint_path, index_data_path)) {
    LOGGER_ERROR(g_logger, "Init index failed.");
    return -1;
  }

  blitz::Framework framework;
  framework.set_thread_count(1);
  framework.Init();
  boost::shared_ptr<ExecMergeMsgHandler> handler(new ExecMergeMsgHandler());
  ret = handler->Init();
  if (CR_SUCCESS != ret) {
    LOGGER_ERROR(g_logger, "act=ImgGenMsgHandler failed, ret=%d", ret);
    return -4;
  }

  ExecMergeMsgProcessor processor(handler);
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
  BOFIndex::GetInstance()->ReleaseFileData();
  DestroyObjectFactory();
  
  return 0;
}
