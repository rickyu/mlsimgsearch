#ifndef UTILS_COM_POOL_H
#define UTILS_COM_POOL_H

#include <semaphore.h>
#include <vector>
#include <utils/locks.h>
#include <errno.h>

template<class TCom>
class ComPool {
 private:
  enum {
    COM_STAT_IDLE,
    COM_STAT_BUSY,
  };
  typedef struct {
    int stat;
    TCom *node;
  } com_node_t;

 public:
  ComPool(const int max_nodes = 0xFFFF) {
    max_nodes_ = max_nodes;
  }

  ~ComPool() {
    for (size_t i = 0; i < com_nodes_.size(); i++) {
      if (com_nodes_[i].node) {
        delete com_nodes_[i].node;
      }
    }
    sem_destroy(&sem_);
  }

  int Init() {
    int ret = 0;
    ret = sem_init(&sem_, 0, 0);
    if (0 != ret)
      return -1;
    return ret;
  }

  int Add(const TCom *com) {
    UniqueLock<Mutex> lock(mutex_);
    if ((int)com_nodes_.size() >= max_nodes_)
      return -1;
    com_node_t com_node;
    com_node.stat = COM_STAT_IDLE;
    com_node.node = const_cast<TCom*>(com);
    com_nodes_.push_back(com_node);
    sem_post(&sem_);
    return 0;
  }

  TCom* GetIndex(int index) {
    return com_nodes_[index].node;
  }

  TCom* Borrow(int waitms = 0) {
    int result = 0;
    if (waitms) {
      timespec to = {0};
      CalcSemTime(to, waitms);
      do {
        result = sem_timedwait(&sem_, &to);
      } while (result && EINTR == errno);
    }
    else {
      do {
        result = sem_wait(&sem_);
      } while (result && EINTR == errno);
    }
    UniqueLock<Mutex> lock(mutex_);
    TCom *com = NULL;
    for (size_t i = 0; i < com_nodes_.size(); i++) {
      if (COM_STAT_IDLE == com_nodes_[i].stat) {
        com = com_nodes_[i].node;
        com_nodes_[i].stat = COM_STAT_BUSY;
        break;
      }
    }
    return com;
  }

  void Release(const TCom *node) {
    UniqueLock<Mutex> lock(mutex_);
    for (size_t i = 0; i < com_nodes_.size(); i++) {
      if (node == com_nodes_[i].node) {
        com_nodes_[i].stat = COM_STAT_IDLE;
        sem_post(&sem_);
        break;
      }
    }
  }

 private:
  void CalcSemTime(timespec& to, uint32_t mswait) {
    clock_gettime(CLOCK_REALTIME, &to);
    to.tv_sec += (mswait / 1000);
    to.tv_nsec += (mswait % 1000) * 1000;
  }

 private:
  std::vector<com_node_t> com_nodes_;
  Mutex mutex_;
  sem_t sem_;
  int max_nodes_;
};

#endif
