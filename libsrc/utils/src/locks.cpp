#include "locks.h"
#include <errno.h>


Mutex::Mutex()
{
  if (0 != pthread_mutex_init(&mutex_, NULL)) {
    // TODO: ERR_LOG("pthread_mutex_init failed!");
    return;
  }
}

Mutex::~Mutex()
{
  if (0 != pthread_mutex_destroy(&mutex_)) {
    // TODO: ERR_LOG("pthread_mutex_destory failed");
  }
}

bool Mutex::Lock()
{
  return pthread_mutex_lock(&mutex_)?false:true;
}

bool Mutex::TryLock()
{
  return pthread_mutex_trylock(&mutex_)?false:true;
}

bool Mutex::Unlock()
{
  return pthread_mutex_unlock(&mutex_)?false:true;
}

const pthread_mutex_t* Mutex::Handle() const
{
  return &mutex_;
}


ConditionVariable::ConditionVariable()
{
  pthread_cond_init(&cond_, NULL);
}

ConditionVariable::~ConditionVariable()
{
  pthread_cond_destroy(&cond_);
}

void ConditionVariable::Wait(UniqueLock<Mutex> &m)
{
  pthread_cond_wait(&cond_, const_cast<pthread_mutex_t*>(m.GetMutex()->Handle()));
}

bool ConditionVariable::TimedWait(UniqueLock<Mutex> &m, const timespec &time)
{
  int cond_res = pthread_cond_timedwait(&cond_,
                                        const_cast<pthread_mutex_t*>(m.GetMutex()->Handle()),
                                        &time);
  return cond_res!=ETIMEDOUT;
}

void ConditionVariable::NotifyOne()
{
  pthread_cond_signal(&cond_);
}

void ConditionVariable::NotifyAll()
{
  pthread_cond_broadcast(&cond_);
}
