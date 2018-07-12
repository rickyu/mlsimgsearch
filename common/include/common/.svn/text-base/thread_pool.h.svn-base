#ifndef IMAGE_SERVICE_COMMON_THREAD_POOL_H
#define IMAGE_SERVICE_COMMON_THREAD_POOL_H

#include <list>
#include <common/thread.h>
#include <stdint.h>


class ThreadPool {
 public:
 ThreadPool(int init_thread_num,
            int max_thread_num);
  virtual ~ThreadPool();

  void set_init_thread_num(int num);
  void set_max_thread_num(int num);
  void set_each_append_thread_num(int num);
  const int get_max_thread_num() const;
  const int get_cur_thread_num() const;
  const int GetCurThreadNumWithState(const int state);
  const bool Init();
  const int StartAll();
  const bool KillAll();
  void WaitForAll();
  /**
   * @return success number
   */
  const int AddThread(const Thread *thread);
  /**
   * @brief only remove idle threads
   * @return removed number
   */
  const int RemoveThreads(const int num);
  /**
   * @brief suspend the threads when it's idle
   * @return the suspended threads number
   */
  const int SuspendThreads(const int num);
  
 private:
  int max_thread_num_;
  int init_thread_num_;
  int each_append_thread_num_;
  std::list<Thread*> threads_;
  Mutex threads_mutex_;
  const uint32_t kDefaultEachAppendNum;
};

#endif
