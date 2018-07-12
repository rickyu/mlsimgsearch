#ifndef IMAGE_SERVICE_COMMON_THREAD_H_
#define IMAGE_SERVICE_COMMON_THREAD_H_

#include <pthread.h>
#include <string>
#include <utils/locks.h>
#include <common/constant.h>

enum {
  THREAD_STATE_IDLE,
  THREAD_STATE_READY,
  THREAD_STATE_RUNNING,
  THREAD_STATE_SUSPEND,
  THREAD_STATE_ENDING,
};


class Thread {
 public:
  Thread(const bool is_detached = false);
  virtual ~Thread();

  virtual int Init() = 0;

  // running mode
  bool Detach();
  bool Join();

  // state control
  bool Start();
  bool Terminate();
  bool Wakeup();
  bool Suspend();

  // property
  int get_state() const;
  pthread_t GetId() const;
  unsigned int GetIdInt() const;
  std::string GetIdString() const;

 protected:
  // main process of task
  virtual int Process() = 0;

 private:
  // thread start entrance
  static void* ThreadProc(void *param);

 protected:
  pthread_t pthread_;
  pthread_attr_t pthread_attr_;
  Mutex mutex_;
  ConditionVariable cond_;
  int state_;
  bool is_detached_;
  int finished_task_count_;
};

#endif
