// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// ZooHandle. 
#ifndef __BLITZ2_ZOOKEEPER_ZOOKEEPER_HANDLE_H_
#define __BLITZ2_ZOOKEEPER_ZOOKEEPER_HANDLE_H_ 1
#include <zookeeper/zookeeper.h>
#include <blitz2/ioobject.h>
#include <blitz2/reactor.h>
#include <blitz2/logger.h>
#include <blitz2/state_machine.h>
#include <blitz2/time.h>
namespace blitz2 {
class ZookeeperHandle : public IoObject {
 public:
  ZookeeperHandle(Reactor& reactor);
  virtual ~ZookeeperHandle() {
    Close();
  }
  int Init(const char* host, watcher_fn fn, int recv_timeout, 
      void* context, int flags);
  void Close();
  zhandle_t* zhandle() { return zhandle_; }
  virtual void ProcessEvent(const IoEvent& event);
 protected:
  int Reopen();
  int Open();
  int Activate();
  void Ping(int32_t); 

 private:
  Reactor& reactor_;
  Logger& logger_;
  zhandle_t* zhandle_;
  clientid_t clientid_;
  uint32_t myid_;
  std::string host_;
  watcher_fn watcher_;
  int recv_timeout_;
  void* context_;
  int flags_;
};

class ZookeeperPool {
 public:
  ZookeeperPool(Reactor& reactor);
  ~ZookeeperPool();
  int Init(const char* host, watcher_fn watcher, void* context,
      int flags, int recv_timeout, size_t connect_count);
  void Create(const char* path, const char* value, int valuelen, 
      const struct ACL_vector* acl, int flags, 
      string_completion_t completion, const void* data);
  void Delete(const char* path, int version,
      void_completion_t completion, const void* data);
  void Get(const char* path, int watch, data_completion_t completion,
      const void* data);
  void GetChildren(const char* path, int watch, strings_completion_t completion,
      const void* data);
 protected:
  ZookeeperHandle* GetConnect();
 private:
  Reactor& reactor_;
  std::vector<ZookeeperHandle*> connects_;
  size_t connect_index_;
  std::string host_;
  watcher_fn watcher_;
  void* context_;
  int flags_;
  int recv_timeout_;
  Logger& logger_;
};

class ZookeeperStringEvent : public Event {
 public:
  ZookeeperStringEvent(int rc, const char* value, int event_type) 
    : Event(event_type) {
    this->rc = rc;
    this->value = value;
  }
  int rc;
  const char* value;
};

template<typename StateMachine>
class ZookeeperEventFire {
 public:
  ZookeeperEventFire(ZookeeperPool* pool) {pool_ = pool;}
  void FireCreate(const char* path, const char* value, int valuelen, 
      const struct ACL_vector* acl, int flags, 
      uint32_t context_id,  StateMachine* fsm, int event_type) ;
 private:
  static void StringCompletion(int rc, const char* value, const void* data);
  struct CreateData {
    CreateData(StateMachine* fsm, uint32_t context_id, int event_type) {
      this->fsm = fsm;
      this->context_id = context_id;
      this->event_type = event_type;
    }
    StateMachine* fsm;
    uint32_t context_id;
    int event_type;
  };
 private:
  ZookeeperPool* pool_;
};

template<typename StateMachine>
inline void ZookeeperEventFire<StateMachine>::FireCreate(
    const char* path, const char* value, int valuelen, 
    const struct ACL_vector* acl, int flags, 
    uint32_t context_id,  StateMachine* fsm, int event_type) {
  CreateData* data = new CreateData(fsm, context_id, event_type);
  pool_->Create(path, value, valuelen, acl, flags, 
      StringCompletion, data);
}
template<typename StateMachine>
inline void ZookeeperEventFire<StateMachine>::StringCompletion(
    int rc, const char* value, const void* data) {
  CreateData* create_data = reinterpret_cast<CreateData*>((void*)data);
  ZookeeperStringEvent event(rc, value, create_data->event_type);
  create_data->fsm->ProcessEvent(create_data->context_id, &event);
  if (create_data) {
    delete create_data;
    create_data = NULL;
  }
}


inline int ZookeeperHandle::Reopen() {
  LOG_INFO(logger_, "Reopen");
  if (is_unrecoverable(zhandle_)) {
    LOG_INFO(logger_, "unrecoverable");
    Close();
    return Open();
  } else {
    reactor_.RemoveIoObject(this);
    return Activate();
  }
}
inline ZookeeperHandle* ZookeeperPool::GetConnect() {
  size_t i = connect_index_;
  connect_index_ = (++connect_index_) % connects_.size();
  ZookeeperHandle* conn = connects_[i];
  return conn;
}
inline void ZookeeperPool::Create(const char* path, const char* value,
    int valuelen, const struct ACL_vector* acl, int flags, 
    string_completion_t completion, const void* data) {
  ZookeeperHandle* conn = GetConnect();
  int rc = zoo_acreate(conn->zhandle(), path, value, valuelen, acl, flags,
      completion, data);
  if (rc != ZOK) {
    LOG_ERROR(logger_, "rc=%d handle=%p myh=%p", rc, conn->zhandle(), conn);
  }
}
inline void ZookeeperPool::Delete(const char* path, int version, 
    void_completion_t completion, const void* data) {
  ZookeeperHandle* conn = GetConnect();
  zoo_adelete(conn->zhandle(), path, version, completion, data);
}
inline void ZookeeperPool::Get(const char* path, int watch, 
    data_completion_t completion, const void* data) {
  LOG_DEBUG(logger_, "Enter, time=%ld", time::GetTickMs());
  ZookeeperHandle* conn = GetConnect();
  zoo_aget(conn->zhandle(), path, watch, completion, data);
  LOG_DEBUG(logger_, "LEAVE, time=%ld", time::GetTickMs());
}
inline void ZookeeperPool::GetChildren(const char* path, int watch, 
    strings_completion_t completion, const void* data) {
  ZookeeperHandle* conn = GetConnect();
  zoo_aget_children(conn->zhandle(), path, watch, completion, data);
}
} // namespace blitz2.
#endif // __BLITZ2_ZOOKEEPER_ZOOKEEPER_HANDLE_H_

