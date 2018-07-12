#ifndef UTILS_LOCKS_H_
#define UTILS_LOCKS_H_

#include <pthread.h>

template<typename TMutex>
class UniqueLock {
 public:
 UniqueLock(TMutex &mutex)
   : mutex_(&mutex)
    , is_locked_(false) {
    is_locked_ = true;
    mutex_->Lock();
  }
  ~UniqueLock() {
    mutex_->Unlock();
  }

  TMutex* GetMutex()  {
    return mutex_;
  }

 private:
  TMutex *mutex_;
  bool is_locked_;
};



class Mutex  {
 public:
  Mutex();
  ~Mutex();

  bool Lock();
  bool TryLock();
  bool Unlock();
  const pthread_mutex_t* Handle() const;

 private:
  pthread_mutex_t mutex_;
};


class ConditionVariable {
 public:
  ConditionVariable();
  ~ConditionVariable();

  void Wait(UniqueLock<Mutex> &mutex);
  //  template<typename lock_type> void wait(lock_type &mutex);
  bool TimedWait(UniqueLock<Mutex> &mutex, const timespec &time);
  //  template<typename lock_type> bool timed_wait(lock_type &mutex, const timespec &time);
  void NotifyOne();
  void NotifyAll();

 private:
  pthread_cond_t cond_;
};


#endif
