// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_RWLOCK_H_
#define __BLITZ_RWLOCK_H_ 1

#include <stdlib.h>
#include <pthread.h>

namespace blitz {
class RWLock {
 public:
  RWLock();
  ~RWLock();
  void LockRead();
  void LockWrite();
  void Unlock();
  class ReadGuard {
   public:
    ReadGuard(RWLock& lock):lock_(lock) {
      lock_.LockRead();
    }
    ~ReadGuard() { lock_.Unlock(); }
   private:
    RWLock& lock_;
  };
  class WriteGuard {
   public:
    WriteGuard(RWLock& lock):lock_(lock) {
      lock_.LockWrite();
    }
    ~WriteGuard() { lock_.Unlock(); }
   private:
    RWLock& lock_;
  };
 private:
  RWLock(const RWLock&);
  RWLock& operator=(const RWLock&);
 private:
  pthread_rwlock_t rwlock_;
};
inline RWLock::RWLock() {
  int ret = pthread_rwlock_init(&rwlock_, NULL);
  if (ret != 0) { abort(); }
}
inline RWLock::~RWLock() {
  int ret = pthread_rwlock_destroy(&rwlock_);
  if (ret != 0) { abort(); }
}
inline void RWLock::LockRead() {
  int ret = pthread_rwlock_rdlock(&rwlock_);
  if (ret != 0) { abort(); }
}
inline void RWLock::LockWrite() {
  int ret = pthread_rwlock_wrlock(&rwlock_);
  if (ret != 0) { abort(); }
}
inline void RWLock::Unlock() {
  int ret = pthread_rwlock_unlock(&rwlock_);
  if (ret != 0) { abort(); }
}

} // namespace blitz.
#endif // __BLITZ_RWLOCK_H_
