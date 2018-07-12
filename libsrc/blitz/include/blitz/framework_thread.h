#pragma once
#include <stdint.h>
#include <pthread.h>
#include <queue>
#include <boost/thread.hpp>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <utils/atomic.h>
#include "runnable.h"
#include "token_mgr.h"
#include "timer.h"

namespace blitz {
class Framework;
class FrameworkThread {
  public:
    static void* FrameworkThreadRoutine(void* arg);
    FrameworkThread() {
      run_context_.thread_number = 0;
      run_context_.thread_id = 0;
      run_context_.framework = NULL;
      running_ = 0;
      libevent_ = NULL;
      task_start_time_ = 0;
     // next_timer_id_ = 0;
    }
    ~FrameworkThread() {
      Uninit();
    }
    int Init(int thread_idx, Framework* framework);
    int Start();
    int Uninit();
    time_t get_task_start_time() const { return task_start_time_; }
    bool IsTaskExecuteTimeout(int timeout) const;

    int Loop();
    int PostTask(const Task& task) {
      boost::mutex::scoped_lock guard(task_queue_mutex_);
      task_queue_.push(task);
      return 0;
    }
    static int32_t NextTimerId() { 
      return InterlockedIncrement(&next_timer_id_);
    }
    // 设置定时器，返回定时器id.
    // > 0 成功，定时器ID, <=0失败.
    int32_t SetTimer(uint32_t timeout, FnTimerCallback callback, void* arg);
    static void LibeventTimerCallback(int fd,short event,void* arg);
 public:
  public:
    RunContext run_context_;
    uint32_t running_;
    struct event_base* libevent_;
    boost::mutex libevent_mutex_;
    std::queue<Task> task_queue_;
    boost::mutex task_queue_mutex_;
    time_t task_start_time_;
    std::queue<Task> task_queue_running_;
    static int32_t next_timer_id_ ;
};
} // namespace blitz.
