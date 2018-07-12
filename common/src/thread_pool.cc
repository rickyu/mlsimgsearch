#include <common/thread_pool.h>
#include <stdint.h>

const uint32_t DEFAULT_EACH_APPEND_NUM = 3;

ThreadPool::ThreadPool(int init_thread_num,
                       int max_thread_num)
  : max_thread_num_(max_thread_num)
  , init_thread_num_(init_thread_num)
  , each_append_thread_num_(DEFAULT_EACH_APPEND_NUM)
  , kDefaultEachAppendNum(3) {
}

ThreadPool::~ThreadPool() {
  KillAll();
}

void ThreadPool::set_init_thread_num(int num) {
  init_thread_num_ = num;
}

void ThreadPool::set_max_thread_num(int num) {
  max_thread_num_ = num;
}

void ThreadPool::set_each_append_thread_num(int num) {
  each_append_thread_num_ = num;
}

const int ThreadPool::get_max_thread_num() const {
  return max_thread_num_;
}

const int ThreadPool::get_cur_thread_num() const {
  return threads_.size();
}

const int ThreadPool::GetCurThreadNumWithState(const int state) {
  int num = 0;
  std::list<Thread*>::iterator it = threads_.begin();
  while (it != threads_.end()) {
    Thread *thread = *it;
    if (thread->get_state() == state)
      num++;
    ++it;
  }
  return num;
}

const bool ThreadPool::Init() {
  return true;
}

const int ThreadPool::StartAll() {
  Thread *pcur = NULL;
  int started = 0;
  std::list<Thread*>::iterator it_cur = threads_.begin();
  while (it_cur != threads_.end()) {
    pcur = *it_cur;
    if (pcur->Start())
      started++;
    ++it_cur;
  }
  return started;
}

const bool ThreadPool::KillAll() {
  Thread *pcur = NULL;
  std::list<Thread*>::iterator it_cur = threads_.begin();
  while (it_cur != threads_.end()) {
    pcur = *it_cur++;
    threads_.pop_front();
    if (pcur) {
      pcur->Terminate();
      delete pcur;
      pcur = NULL;
    }
  }
  return true;
}

const int ThreadPool::AddThread(const Thread *thread) {
  if (!thread || (int)threads_.size() >= max_thread_num_)
    return 0;

  UniqueLock<Mutex> lock(threads_mutex_);
  threads_.push_back(const_cast<Thread*>(thread));
  return 1;
}

const int ThreadPool::RemoveThreads(const int num) {
  int removed_num = 0;
  if (threads_.size() == 0)
    return removed_num;

  std::list<Thread*>::iterator it_cur = threads_.begin();
  while (it_cur != threads_.end()) {
    Thread *pthread = *it_cur;
    std::list<Thread*>::iterator it_tmp = it_cur++;
    if (THREAD_STATE_IDLE == pthread->get_state()) {
      threads_.erase(it_tmp);
      pthread->Terminate();
      delete pthread;
      pthread = NULL;

      removed_num++;
    }
  }

  return removed_num;
}

const int ThreadPool::SuspendThreads(const int num) {
  if (num > max_thread_num_
      || num <= 0
      || (int)threads_.size() < num)
    return 0;
  
  int suspend_num = 0;
  std::list<Thread*>::iterator it_cur = threads_.begin();
  while (it_cur != threads_.end()) {
    Thread *thread = *it_cur;
    thread->Suspend();
    suspend_num++;
  }
  return suspend_num;
}

void ThreadPool::WaitForAll() {
  std::list<Thread*>::iterator it_cur = threads_.begin();
  while (it_cur != threads_.end()) {
    Thread *pthread = *it_cur++;
    pthread->Join();
  }

  KillAll();
}
