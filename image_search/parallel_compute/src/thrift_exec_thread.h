#ifndef IMAGE_SEARCH_SEARCHD_THRIFT_EXEC_THREAD_H_
#define IMAGE_SEARCH_SEARCHD_THRIFT_EXEC_THREAD_H_

#include <blitz/thrift_server.h>
#include <common/thread.h>

class ThriftExecThread : public Thread {
 public:
  ThriftExecThread(const blitz::ThriftTaskMgrServer *thrift_mgr);
  ~ThriftExecThread();

  int Init();
  
 private:
  int Process();

 private:
  blitz::ThriftTaskMgrServer *thrift_mgr_;
};

#endif
