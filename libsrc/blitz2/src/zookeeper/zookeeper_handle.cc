#include <blitz2/zookeeper/zookeeper_handle.h>
namespace blitz2 {
ZookeeperPool::ZookeeperPool(Reactor& reactor)
  :reactor_(reactor) , logger_(Logger::GetLogger("zookeeper")) {
  connect_index_ = 0;
  watcher_ = watcher_fn(0);
  context_ = NULL;
  flags_ = 0;
  recv_timeout_ = 30000;
}
ZookeeperPool::~ZookeeperPool() {
 if (!connects_.empty()) {
   for (auto it = connects_.begin(); it != connects_.end(); ++it) {
     if ((*it) != NULL) {
       delete (*it);
       (*it) = NULL;
     }
   }
   connects_.clear();
 }
 if (context_ != NULL) {
   context_ = NULL;
 }
}
int ZookeeperPool::Init(const char* host, watcher_fn watcher, void* context,
    int flags, int recv_timeout, size_t connect_count) {
  assert(connect_count>0);
  host_ = host;
  watcher_ = watcher;
  context_ = context;
  flags_ = flags;
  recv_timeout_ = recv_timeout;
  connects_.reserve(connect_count);
  for (size_t i=0; i<connect_count; ++i) {
    ZookeeperHandle* conn = new ZookeeperHandle(reactor_);
    conn->Init(host_.c_str(), watcher_, recv_timeout_,  context_, flags);
    connects_.push_back(conn);
  }
  return 0;
}

int ZookeeperHandle::Init(const char* host, watcher_fn fn,
    int recv_timeout, void* context, int flags) {
  host_ = host;
  watcher_ = fn;
  recv_timeout_ = recv_timeout;
  context_ = context;
  flags_ = flags;
  if (0 == Open()) {
    LOG_DEBUG(logger_, "AddTimer");
    uint32_t timeid = reactor_.AddTimer(1000, std::bind(
          &ZookeeperHandle::Ping, this, std::placeholders::_1),
          true);
    if (timeid==0) {
      LOG_FATAL(logger_, "AddTimer fail");
      abort();
      return -1;
    }
  }
  return 0;
}
ZookeeperHandle::ZookeeperHandle(Reactor& reactor)
  :reactor_(reactor), 
   logger_(Logger::GetLogger("zookeeper")) {
  zhandle_ = NULL;
  clientid_.client_id = 0;
  clientid_.passwd[0] = '0';
  myid_ = 0;
  watcher_ = watcher_fn(0);
  recv_timeout_ = 1000;
  context_ = NULL;
  flags_ = 0;
}

int ZookeeperHandle::Open() {
  zhandle_ = zookeeper_init(host_.c_str(), watcher_, recv_timeout_,
      &clientid_, context_, flags_);
  if (!zhandle_) {
    LOG_ERROR(logger_, "zookeeper_init fail");
    return -1;
  }
  LOG_INFO(logger_, "clientid=clientid_.client_id=%ld", clientid_.client_id);
  return Activate();
}

void ZookeeperHandle::ProcessEvent(const IoEvent& event) {
   
  LOG_INFO(logger_, "Enter, time=%ld", time::GetTickMs());
  int rc = ZOK;
  if (event.haveWrite()) {
    // write 没有循环是因为zookeeper 内部会循环发送直到EAGAIN
    int previous_state = zoo_state(zhandle_);
    rc = zookeeper_process(zhandle_, ZOOKEEPER_WRITE);
    if (rc == ZOK && previous_state != ZOO_CONNECTED_STATE) {
      // 连接刚刚建立，发送请求.
      rc = zookeeper_process(zhandle_, ZOOKEEPER_WRITE);
    }
  }
  LOG_INFO(logger_, "before while, time=%ld", time::GetTickMs());
  if ( rc == ZOK && event.haveRead() ) {
    do {
      rc = zookeeper_process( zhandle_, ZOOKEEPER_READ );
    } while ( rc == ZOK );
  }
  LOG_INFO(logger_, "after while, time=%ld", time::GetTickMs());

  int state = zoo_state(zhandle_);
  if (state == ZOO_CONNECTED_STATE) {
    const clientid_t *id = zoo_client_id(zhandle_);
    if (clientid_.client_id == 0 || clientid_.client_id != id->client_id) {
      LOG_INFO(logger_, "state=ZOO_CONNECTED_STATE, clientid=%ld, current id=%ld",
               clientid_.client_id, id->client_id);
      clientid_ = *id; 
    }
  }
  if (rc != ZOK && rc != ZNOTHING) { 
    if (rc == ZCONNECTIONLOSS) {
      LOG_ERROR(logger_, "bad after state=%d handle=%p rc=ZCONNECTIONLOSS",
        zoo_state(zhandle_), zhandle_ );
    } else if (rc == ZSESSIONEXPIRED || rc == ZINVALIDSTATE) {
      LOG_ERROR(logger_, "bad after state=%d handle=%p rc=ZSESSIONEXPIRED or\
          rc == ZINVALIDSTATE", zoo_state(zhandle_), zhandle_ );
      clientid_.client_id = 0;
      clientid_.passwd[0] = '0';
    }
    rc = Reopen();
    if (rc != 0) {
      LOG_ERROR(logger_, "reopenFail rc=%d this=%p", rc, this);
    }
  } else {
    LOG_DEBUG(logger_, "after state=%d handle=%p rc=%d",
             zoo_state(zhandle_), zhandle_, rc);
  }
  LOG_INFO(logger_, "Leave, time=%ld", time::GetTickMs());
}
void ZookeeperHandle::Close() {
  LOG_DEBUG(logger_, "Close state=%d handle=%p",
        zoo_state(zhandle_), zhandle_ );
  if (zhandle_) {
    reactor_.RemoveIoObject(this);
    zookeeper_close(zhandle_);
    zhandle_ = NULL;
  }
  fd_ = -1;
}
void ZookeeperHandle::Ping(int32_t) {
  int interest = 0;
  struct timeval tv = {0, 0};
  int rc = zookeeper_interest(zhandle_, &fd_, &interest, &tv);
  if (rc != ZOK) {
    LOG_ERROR(logger_, "zookeeper_interest handle=%p rc=%d",
      zhandle_, rc);
  }
}
int ZookeeperHandle::Activate() {
  int interest = 0;
  struct timeval tv = {0, 0};
  int rc = zookeeper_interest(zhandle_, &fd_, &interest, &tv);
  if (fd_ < 0) {
    rc = zookeeper_interest(zhandle_, &fd_, &interest, &tv);
  }
  if (rc != ZOK ) {
    LOG_ERROR(logger_, "zookeeper_interest handle=%p rc=%d",
      zhandle_, rc);
    return -2;
  }
  if (fd_ < 0) {
    return -3;
  }
  reactor_.RegisterIoObject(this);
  return 0;
}
} // namespace blitz2.
