#include <common/thread.h>
#include <stdint.h>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>

Thread::Thread(const bool is_detached)
  : state_(THREAD_STATE_IDLE)
  , is_detached_(is_detached)
  , finished_task_count_(0) {
}

Thread::~Thread() {
  Terminate();
}

bool Thread::Detach() {
  int ret = pthread_detach(pthread_);
  if (ret) {
    // TODO: ERR_LOG("pthread_detech failed, error=%d", ret);
    return false;
  }

  return true;
}

bool Thread::Join() {
  int ret = 0;
  ret = pthread_join(pthread_, NULL);
  if (ret) {
    // TODO: ERR_LOG("pthread_join failed, error=%d", err);
    return false;
  }

  return true;
}

bool Thread::Start() {
  int err = pthread_create(&pthread_, NULL, ThreadProc, this);
  if (err) {
    // TODO: ERR_LOG("pthread_create failed, error=%d", err);
    return false;
  }

  state_ = THREAD_STATE_IDLE;
  
  return true;
}

bool Thread::Terminate() {
  state_ = THREAD_STATE_ENDING;
  Join();

  return true;
}

bool Thread::Wakeup() {
  UniqueLock<Mutex> lock(mutex_);
  cond_.NotifyOne();

  state_ = THREAD_STATE_READY;
  return true;
}

bool Thread::Suspend() {
  state_ = THREAD_STATE_SUSPEND;
  UniqueLock<Mutex> lock(mutex_);
  cond_.Wait(lock);

  return true;
}

int Thread::get_state() const {
  return state_;
}

pthread_t Thread::GetId() const {
  return pthread_self();
}

unsigned int Thread::GetIdInt() const {
  return boost::lexical_cast<unsigned int>(GetId());
}

std::string Thread::GetIdString() const {
  return boost::lexical_cast<std::string>(GetId());
}

void* Thread::ThreadProc(void *param) {
  Thread *pthis = static_cast<Thread*>(param);
  pthis->Process();//处理过程
  return NULL;
}
