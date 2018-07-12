#include <unistd.h>
//#define _POSIX_C_SOURCE 199309 
#include <time.h>
#include "utils/logger.h"
#include "utils/time_util.h"
#include "utils/compile_aux.h"
#include "blitz/framework_thread.h"
#include "blitz/framework.h"

namespace blitz {
struct TimerItem {
  FrameworkThread* thread;
  int32_t timer_id;
  int timeout;
  uint64_t time_start;
  bool repeated;
  FnTimerCallback  fn_callback;
  void* arg;
  struct event ev;
};

static CLogger& g_log_framework = CLogger::GetInstance("framework");

void* FrameworkThread::FrameworkThreadRoutine(void* arg){
  FrameworkThread* frame_thread = reinterpret_cast<FrameworkThread*>(arg);
  frame_thread->Loop();
  return NULL;
}
int FrameworkThread::next_timer_id_ = 0;
int FrameworkThread::Start(){
  running_ = 1;
  pthread_create(&run_context_.thread_id,NULL,FrameworkThreadRoutine,this);
  return 0;
}
int FrameworkThread::Init(int thread_number, Framework* framework){
  run_context_.thread_number = thread_number;
  run_context_.framework = framework;
  if(libevent_){
    return 1;
  }
  libevent_ = event_base_new();
  return 0;
}
int FrameworkThread::Uninit(){
  while(!task_queue_.empty()){
    Task& task = task_queue_.front();
    if(task.fn_delete){
      task.fn_delete(task.args);
    }
    task_queue_.pop();
  } 
  
  if(libevent_){
    event_base_free(libevent_);
    libevent_ = NULL;
  }

  return 0;
}
bool FrameworkThread::IsTaskExecuteTimeout(int timeout) const {
  if (task_start_time_ != 0 && time(NULL) >= task_start_time_ + timeout && task_start_time_ > 0) {
    return true;
  }
  return false;
}
int FrameworkThread::Loop(){
  struct timespec time_to_sleep = {0,0};
  time_to_sleep.tv_nsec = 10000;
  Framework::TlsData* tls_data = reinterpret_cast<Framework::TlsData*>(
      pthread_getspecific(run_context_.framework->get_tls_key()));
  if (tls_data == NULL) {
    LOGGER_DEBUG(g_log_framework, "InitTls no=%d", run_context_.thread_number);
    tls_data = reinterpret_cast<Framework::TlsData*>(malloc(sizeof(Framework::TlsData)));
    if (tls_data == NULL) {
      LOGGER_FATAL(g_log_framework, "OutOfMemory");
      abort();
    }
    tls_data->thread_number = run_context_.thread_number;
    if (pthread_setspecific(run_context_.framework->get_tls_key(), tls_data) != 0) {
      int err = errno;
      char str[80];
      str[0] = '\0';
      const char* returned_str = strerror_r(err, str, sizeof(str));
      LOGGER_FATAL(g_log_framework, "SetTls errno=%d err=%s",
          err, returned_str );
      abort();
    }
  }
  task_start_time_ = 0;
  while(running_) {
    int ret;
   // uint64_t t1 = TimeUtil::GetTickUs();
    ret = event_base_loop(libevent_,EVLOOP_NONBLOCK);
  //  uint64_t t2 = TimeUtil::GetTickUs();
    if (ret != 0) {
      // break;
    }
    if (task_queue_running_.empty())  {
      boost::mutex::scoped_lock guard(task_queue_mutex_);
      task_queue_running_.swap(task_queue_);
    }
    while (!task_queue_running_.empty()) {
      Task& task = task_queue_running_.front();
      if (task.expire_time > 0) {
          uint64_t cur_time = TimeUtil::GetTickMs();
          if (cur_time > task.expire_time) {
            task.fn_delete(task.args);
            task_queue_running_.pop();
            LOGGER_ERROR(g_log_framework, "TaskTimeout");
            continue;
          }
      }
      task_start_time_ = time(NULL);
    // 如果线程被pthread_exit退出,则task占用的资源不会被释放， 会有内存泄漏.
      task.fn_callback(run_context_, task.args);
      task_start_time_ = 0;
      task_queue_running_.pop();
    }
  //  LOGGER_NOTICE(g_log_framework, "Loop timecost, cost1=%lu, cost2 = %lu,all_cost=%lu,", t2-t1, TimeUtil::GetTickUs()-t2, TimeUtil::GetTickUs()-t1);
    nanosleep(&time_to_sleep,NULL);
  }
  pthread_setspecific(run_context_.framework->get_tls_key(), NULL);
  free(tls_data);
  return 0;
}
void FrameworkThread::LibeventTimerCallback(int fd, short event, void* arg) {
  using namespace std;
  AVOID_UNUSED_VAR_WARN(fd);
  AVOID_UNUSED_VAR_WARN(event);
  assert(arg!=NULL);
  uint64_t time_end = TimeUtil::GetTickMs();
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  TimerItem* item = reinterpret_cast<TimerItem*>(arg);
  FrameworkThread* self = item->thread;
  if (item->fn_callback && item->timer_id > 0) {
    item->fn_callback(item->timer_id, item->arg);
  } else {
    LOGGER_ERROR(g_log_framework,
              "act=timer:callback InvalidFunc timer_id=%d thread_no=%d item=%p",
               item->timer_id, self->run_context_.thread_number, item);
  }
  LOGGER_INFO(g_log_framework,
               "act=timer:callback NotRepeat timer_id=%d thread_no=%d item=%p,timeout = %d, time_cost = %lu, second = %ld, usecond = %ld, now_sec=%ld, now_usec=%ld",
               item->timer_id, self->run_context_.thread_number, item, item->timeout, time_end - item->time_start, item->ev.ev_timeout.tv_sec, item->ev.ev_timeout.tv_usec,
                    ts.tv_sec, ts.tv_nsec/1000);
  delete item;
  item = NULL;
}

int32_t FrameworkThread::SetTimer(uint32_t timeout, FnTimerCallback callback, 
                                  void* arg) {
  using namespace std;
  if(!callback || timeout == 0) {
    return -1;
  }
  TimerItem* item = new TimerItem;
  if (!item) {
    LOGGER_FATAL(g_log_framework,
              "act=timer:set fail(NewItem) thread_no=%d",
                run_context_.thread_number);
    return -101;
  }
  item->thread = this;
  item->timeout = timeout;
  item->time_start = TimeUtil::GetTickMs();
  item->fn_callback = callback;
  item->arg = arg;
  item->timer_id = NextTimerId();
  int ret;
  ret = event_assign(&item->ev, libevent_, -1, 0, LibeventTimerCallback, item);
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework,
                 "act=timer:set fail(event_assign) timer_id=0x%x thread_no=%d"
                 " item=%p",
                 item->timer_id, run_context_.thread_number, item);
    delete item;
    item = NULL;
    return -102;
  }
  timeval tv;
  tv.tv_sec = timeout /1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  ret = event_add(&item->ev, &tv);
  LOGGER_INFO(g_log_framework,
               "act=timer:set timer_id=%d thread_no=%d item=%p, second=%ld, usecond=%ld",
                item->timer_id, run_context_.thread_number, item, item->ev.ev_timeout.tv_sec, item->ev.ev_timeout.tv_usec);

  if (ret != 0) {
    LOGGER_ERROR(g_log_framework,
                 "act=timer:set fail(event_add) ret=%d timer_id=%d thread_no=%d"
                 " item=%p",
                 ret, item->timer_id, run_context_.thread_number, item);
    delete item;
    item = NULL;
    return -103;
  }
  return item->timer_id;
}

} // namespace blitz.
