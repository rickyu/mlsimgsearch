#include "parallel_request.h"
#include <common/thread.h>
#include <common/thread_pool.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>

extern CLogger& g_logger;

class ParallelRequestThread : public Thread {
 public:
  ParallelRequestThread(parallel_request_callback_fn request_fn,
                        void *request_context,
                        int index) 
    : request_fn_(request_fn)
    , request_context_(request_context)
    , index_(index) {
  }
  int Init() {
    return 0;
  }

 private:
  int Process() {
    if (NULL == request_fn_) {
      return -1;
    }
    if (0 != request_fn_(request_context_, index_)) {
      return -2;
    }
    return 0;
  }

 private:
  parallel_request_callback_fn request_fn_;
  void *request_context_;
  int index_;
};

ParallelRequester::ParallelRequester(int requests_num)
  : request_fn_(NULL)
  , request_context_(NULL)
  , requests_num_(requests_num) {
}

ParallelRequester::~ParallelRequester() {
}

void ParallelRequester::SetRequestCallbackFn(parallel_request_callback_fn fn,
                                             void *context) {
  request_fn_ = fn;
  request_context_ = context;
}

int ParallelRequester::Run() {
  ThreadPool pool(requests_num_, requests_num_);
  for (int i = 0; i < requests_num_; i++) {
    Thread *thread = new ParallelRequestThread(request_fn_, request_context_, i);
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
  pool.StartAll();
  pool.KillAll();

  return 0;
}
