#include "thrift_exec_thread.h"

ThriftExecThread::ThriftExecThread(const blitz::ThriftTaskMgrServer *thrift_mgr) {
  thrift_mgr_ = const_cast<blitz::ThriftTaskMgrServer*>(thrift_mgr);
}

ThriftExecThread::~ThriftExecThread() {
}

int ThriftExecThread::Init() {
  return 0;
}
  
int ThriftExecThread::Process() {
  blitz::Task task;
  int ret = 0;
  blitz::RunContext run_context;
  while (THREAD_STATE_ENDING != state_) {
    ret = thrift_mgr_->PopValidTask(task);
    if (1 == ret) {
      task.fn_callback(run_context, task.args);
    }
    else {
      usleep(10);
    }
  }
  return 0;
}
