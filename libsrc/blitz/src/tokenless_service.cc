// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#include "blitz/framework.h"
#include "blitz/tokenless_service.h"
#include "blitz/tokenless_service_pool.h"
#include "utils/logger.h"

namespace blitz {

static CLogger& g_log_framework = CLogger::GetInstance("framework");
static void Callback(const TokenlessRpc& task,
                       int result, const InMsg* msg, int32_t service_id,
                       const IoInfo* ioinfo) {
    task.callback.fn_action(result, msg, service_id,
                           const_cast<OutMsg*>(task.out_msg), task.log_id,
                           task.expire_time, task.callback.context, ioinfo);
  }

///// =============== TokenlessConn =================//////
int TokenlessConn::Query(const TokenlessRpc& rpc) {
  // 当前不能有查询在执行.
  boost::mutex::scoped_lock guard(mutex_);
  assert(rpc_.out_msg == NULL);
  assert(rpc_.callback.fn_action == NULL);
  assert(rpc.out_msg != NULL);
  uint64_t cur_time = TimeUtil::GetTickMs();
  if (cur_time >= rpc.expire_time) {
    LOGGER_ERROR(g_log_framework, "Query rpc_timeout logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc.log_id, rpc.out_msg->get_msgid(),
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);
    return -100;
  }
  rpc_ = rpc;
  int timeout = rpc.expire_time - cur_time;
  int ret = 0;
  if (status_ == kClosed) {
    // 通道没打开.
    assert(io_id_ == 0);
    status_ = kConnecting;
    // TODO(YuZuo) 对io_id_的修改没有锁保护，会不会有问题？
    // 将IoStatusObserver改成回调形式，因为只发生一次.Connect-> ConnectCallback,
    // 在ConnectCallback中设定ConnectCloseCallback回调，可以不设置.
    // MsgProcessor则不一样，因为会多次调用.
    ret = owner_service_.framework_.Connect(owner_service_.addr_,
        owner_service_.decoder_factory_, this, this, &io_id_);
    if (ret == 1) {
      // 连接在进行，等候OnOpened回调.
      LOGGER_INFO(g_log_framework, "Query Connecting logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc.log_id, rpc.out_msg->get_msgid(),
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);
      timer_id_ = owner_service_.framework_.SetTimer(timeout, OnTimer,
                                                     this);
      LOGGER_DEBUG(g_log_framework,"Onopen timer_id=%d, timeout=%d, conn=%p", timer_id_, timeout, this);
      return 0;
    } else if (ret == 0) {
      // 连接完成, 发送消息.
      status_ = kConnected;
      LOGGER_INFO(g_log_framework, "Query Connected logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc.log_id, rpc.out_msg->get_msgid(),
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);
      if (SendRequest() == 0) {
        timer_id_ = owner_service_.framework_.SetTimer(timeout, OnTimer,
                                                       this);
        LOGGER_DEBUG(g_log_framework,"connect1 timer_id=%d, timeout=%d, conn=%p", timer_id_, timeout, this);
        return 0;
      } else {
        LOGGER_ERROR(g_log_framework, "Query SendFail logid=%d msgid=%d"
                  " conn=%d:%d:%s ioid=0x%lx status=%d,conn=%p", 
                  rpc.log_id, rpc.out_msg->get_msgid(),
                  owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_,
                  status_, this);
      }
    } else {
      // 连接失败.
      LOGGER_ERROR(g_log_framework, "Query ConnectFail logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc.log_id, rpc.out_msg->get_msgid(),
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);
    }
  } else if (status_ == kConnected) {
    LOGGER_INFO(g_log_framework, "Query Connected logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc.log_id, rpc.out_msg->get_msgid(),
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);

    if ( SendRequest() == 0) { 
      timer_id_ = owner_service_.framework_.SetTimer(timeout, OnTimer,
                                                     this);
        LOGGER_DEBUG(g_log_framework,"Connent2 timer_id=%d, timeout=%d, conn=%p,ioid=0x%lx, cost_time=%lu", timer_id_, timeout, this, io_id_, TimeUtil::GetTickMs() - cur_time);
      return 0;
    } else {
      LOGGER_ERROR(g_log_framework, "Query SendFail logid=%d msgid=%d"
        " conn=%d:%d:%s ioid=0x%lx status=%d", 
        rpc.log_id, rpc.out_msg->get_msgid(),
        owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_,
        status_);
    }
  } else {
    // 状态错误.
    LOGGER_ERROR(g_log_framework, "Query invalidStatus logid=%d msgid=%d"
        " conn=%d:%d:%s ioid=0x%lx status=%d", 
        rpc.log_id, rpc.out_msg->get_msgid(),
        owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_,
        status_);
  }
  ClearRpc();
  if (io_id_ != 0) {
    owner_service_.framework_.CloseIO(io_id_);
  }
  io_id_ = 0;
  status_ = kClosed;
  // owner_service_.ReturnIdleConn(iter_);
  return -1;
}
int TokenlessConn::Close() {
  Reset(-__LINE__);
  mutex_.lock();
  status_ = kUninited;
  mutex_.unlock();
  return 0;
}
int TokenlessConn::OnOpened(const IoInfo& ioinfo) {
  {
    LOGGER_DEBUG(g_log_framework, "Opened "
               " conn=%d:%d:%s ioid=0x%lx param_ioid=0x%lx", 
               owner_service_.id_, id_, owner_service_.addr_.c_str(),
               io_id_, ioinfo.get_id());
    boost::mutex::scoped_lock guard(mutex_);
    if (timer_id_ == 0)
      return 0;
    if (ioinfo.get_id() == io_id_) {
      status_ = kConnected;
      int ret = 0;
      if (rpc_.out_msg) {
        // 发送请求.
        ret = SendRequest();
        if (ret == 0) {
          return 0;
        }
      }
    } else {
      LOGGER_ERROR(g_log_framework, "Opened IoIoNotMatch "
                   " conn=%d:%d:%s ioid=0x%lx param_ioid=0x%lx", 
                   owner_service_.id_, id_, owner_service_.addr_.c_str(),
                   io_id_, ioinfo.get_id());
    }
  }
  Reset(-__LINE__);
  return -1;
}
int TokenlessConn::OnClosed(const IoInfo& ioinfo) {
  LOGGER_DEBUG(g_log_framework, "Closed conn=%d:%d:%s ioid=0x%lx param_ioid=0x%lx", 
               owner_service_.id_, id_, owner_service_.addr_.c_str(),
               io_id_, ioinfo.get_id());
  mutex_.lock();
  // bool active_closed = (status_ == kUninited);
  bool need_reset = (ioinfo.get_id() == io_id_);
  mutex_.unlock();
  
  if( need_reset) {
    Reset(-__LINE__);
  }
  return 0;
}
int TokenlessConn::ProcessMsg(const IoInfo& ioinfo, const InMsg& msg) {
 bool success = false;
  // 处理响应包,状态必须是kSending, ioid要匹配, rpc中的out_msg和fn_action必须为NULL. 
  LOGGER_DEBUG(g_log_framework, "into processmsg, ioid=0x%lx", io_id_);
  mutex_.lock();
  if (ioinfo.get_id() == io_id_ && status_ == kSending
     && rpc_.out_msg != NULL && rpc_.callback.fn_action != NULL) {
    success = true;
  } else {
    LOGGER_ERROR(g_log_framework, "act=rpc:gotmsg CheckError the conn=%d:%d:%s ioid=0x%lx"
                " param_ioid=0x%lx logid=%d msgid=%d status=%d", 
               owner_service_.id_, id_, owner_service_.addr_.c_str(),
               io_id_, ioinfo.get_id(), rpc_.log_id,
               rpc_.out_msg ? rpc_.out_msg->get_msgid() : 0, status_);

  }
  if (timer_id_ == 0){
    io_id_ = 0;
    status_ = kClosed;
    mutex_.unlock();
    msg.fn_free(msg.data, msg.free_ctx);
    return 0;
  }
  TokenlessRpc rpc_old = rpc_;
  status_ = kConnected;
  ClearRpc();
  timer_id_ = 0;
  mutex_.unlock();
  LOGGER_DEBUG(g_log_framework, "timerid reset to 0, ioid=0x%lx", io_id_);
  if (success) {
    LOGGER_DEBUG(g_log_framework, "GotMsg conn=%d:%d:%s ioid=0x%lx"
                " logid=%d msgid=%d iostatus=0x%x", 
               owner_service_.id_, id_, owner_service_.addr_.c_str(),
               io_id_, rpc_old.log_id, rpc_old.out_msg?rpc_old.out_msg->get_msgid():0,
               ioinfo.get_status());
    uint64_t cur = TimeUtil::GetTickMs();
    Callback(rpc_old, 0, &msg, owner_service_.id_, &ioinfo);
    LOGGER_DEBUG(g_log_framework, "Processmsg callback, ioid=0x%lx, time=%d", io_id_, TimeUtil::GetTickMs() - cur);
  } else {
    if (rpc_old.callback.fn_action) {
      Callback(rpc_old, -1, NULL, owner_service_.id_, NULL);
      msg.fn_free(msg.data, msg.free_ctx);
    } else {
      LOGGER_ERROR(g_log_framework, "the msg may leak");
    }
  }
  if (owner_service_.short_connect_ || ioinfo.IsEofOrError() || !success) {
    // 短连接或者eof/error关闭连接.
    mutex_.lock();
    uint64_t io_id = io_id_;
    io_id_ = 0;
    status_ = kClosed;
    mutex_.unlock();
    owner_service_.framework_.CloseIO(io_id);
  }
  // 查询service队列是否有请求，继续执行；如果没有请求，将自己放入空闲队列.
  uint64_t cur = TimeUtil::GetTickMs();
  owner_service_.ReturnIdleConn(iter_);
  LOGGER_DEBUG(g_log_framework, "processmsg end, ioid=0x%lx, time=%d", io_id_, TimeUtil::GetTickMs() - cur);
  return 0;
}
void TokenlessConn::Reset(int err) {
  mutex_.lock();
  TokenlessRpc rpc_old = rpc_;
  ClearRpc();
  uint64_t io_id = io_id_;
  io_id_ = 0;
  status_ = kClosed;
  timer_id_ = 0;
  mutex_.unlock();
  // 调用callback前释放锁.
  if (rpc_old.out_msg) {
    LOGGER_ERROR(g_log_framework, "reset err=%d conn=%d:%d:%s logid=%d"
                " msgid=%d ioid=0x%lx",
                 err, owner_service_.id_, id_, owner_service_.addr_.c_str(), 
                 rpc_old.log_id, rpc_old.out_msg->get_msgid(), io_id);
    Callback(rpc_old, -1, NULL, owner_service_.id_, NULL);
  }
  if (io_id != 0) {
    owner_service_.framework_.CloseIO(io_id);
  }
  LOGGER_DEBUG(g_log_framework, "Reset err=%d,conn=%p",err, this);
  owner_service_.ReturnIdleConn(iter_);
}
int TokenlessConn::SendRequest() {
  status_ = kSending;
  OutMsg* out_msg = rpc_.out_msg;
  out_msg->AddRef();
  int ret = owner_service_.framework_.SendMsg(io_id_, out_msg);
  int msgid = out_msg->get_msgid();
  out_msg->Release();
  if (ret != 0) {
    // 发送失败.
    LOGGER_ERROR(g_log_framework, "SendRequestFail logid=%d msgid=%d"
          " conn=%d:%d:%s ioid=0x%lx", 
          rpc_.log_id, msgid,
          owner_service_.id_, id_, owner_service_.addr_.c_str(), io_id_);
  }
  return ret;
}
void TokenlessConn::OnTimer(int32_t id, void* arg) {
  TokenlessConn* self = reinterpret_cast<TokenlessConn*>(arg);
  self->mutex_.lock();
  if (self->timer_id_ != id) {
    self->mutex_.unlock();
    return;
  }
  self->timer_id_ = 0;
  self->mutex_.unlock();
  // 超时了, 调用出错回调，关闭连接..
  //
  LOGGER_ERROR(g_log_framework, "Timeout logid=%d conn=%d:%d:%s ioid=0x%lx, timer_id=%d",
               self->rpc_.log_id, self->owner_service_.id_, self->id_,
               self->owner_service_.addr_.c_str(), self->io_id_, id);
  LOGGER_ERROR(g_log_framework, "ON timer, conn=%p", self);
  self->Reset(-__LINE__);
}
///// TokenlessService /////////////////////////
int TokenlessService::s_obj_count = 0;
int TokenlessService::Init() {
  boost::mutex::scoped_lock guard(mutex_);
  inited_ = true;
  return 0;
}
int TokenlessService::Uninit() {
  mutex_.lock();
  if (!inited_) {
    mutex_.unlock();
    return 1;
  }
  TConnList all_conns;
  inited_ = false;
  all_conns.insert(all_conns.end(), busy_conns_.begin(), busy_conns_.end());
  all_conns.insert(all_conns.end(), idle_conns_.begin(), idle_conns_.end());
  mutex_.unlock();
  TConnListIter end = all_conns.end();
  for (TConnListIter iter=all_conns.begin(); iter!=end; ++iter) {
    (*iter)->Close();
  }
  mutex_.lock();
  idle_conns_.clear();
  busy_conns_.clear();
  mutex_.unlock();
  return 0;
}
int TokenlessService::Query(OutMsg* out_msg, int log_id, uint64_t expire_time,
                             const TokenlessCallback& callback) {
  assert(out_msg); 
  assert(callback.fn_action);
  TokenlessRpc rpc;
  rpc.out_msg = out_msg;
  rpc.callback = callback;
  rpc.log_id = log_id;
  rpc.expire_time = expire_time;
  std::shared_ptr<TokenlessConn>  conn = GetIdleConn();
  if (!conn && !inited_) {
    return -1;
  } else if (!conn && inited_) {
    std::shared_ptr<DelayReq> req = std::shared_ptr<DelayReq>(new DelayReq(rpc, *this));
    uint64_t cur_time = TimeUtil::GetTickMs();
    if (cur_time >= rpc.expire_time) {
      LOGGER_ERROR(g_log_framework, "Query rpc_timeout ");
      return -100;
    }
    //DelayReqArgs* arg = new DelayReqArgs();
    //arg->req = req;
    int expire_time = rpc.expire_time;
    //int timer_id = framework_.SetTimer(35, DelayReq::OnTimer, (void*)arg);
    req->set_expire(expire_time);
    TReqListIter iter = delay_reqs_.insert(delay_reqs_.end(), req);
    req->set_iter(iter);
    return 0;
  }
  int ret = conn->Query(rpc);
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, "FailQuery logid=%d msgid=%d",
                log_id, out_msg->get_msgid());
    ReturnIdleConn(conn->get_iter());
    return ret;
  }
  return 0;
}

std::shared_ptr<TokenlessConn> TokenlessService::GetIdleConn() {
  boost::mutex::scoped_lock guard(mutex_);
  std::shared_ptr<TokenlessConn> conn;
  if (!inited_) {
    return conn;
  }
  if (idle_conns_.empty() && next_conn_id_ > 400) {
    return conn;
  }
  if (idle_conns_.empty()) {
    conn = std::shared_ptr<TokenlessConn>(new TokenlessConn(*this));
    conn->set_id(++next_conn_id_);
    conn->set_iter(busy_conns_.end());
  } else {
    --idle_conn_count_;
    conn = idle_conns_.front();
    idle_conns_.pop_front();
  }
  ++busy_conn_count_;
  TConnListIter iter =  busy_conns_.insert(busy_conns_.end(), conn);
  conn->set_iter(iter);
  if (idle_conn_count_+busy_conn_count_ != next_conn_id_) {
    LOGGER_ERROR(g_log_framework, "act=conn::fetch conn=%d:%d idle_count=%d"
               " busy_count=%d next_id=%d",
               id_, conn->get_id(), idle_conn_count_, busy_conn_count_,
               next_conn_id_);
  } else {
    LOGGER_DEBUG(g_log_framework,
               "act=conn::fetch conn=%d:%d idle_count=%d busy_count=%d"
               " next_id=%d",
               id_, conn->get_id(), idle_conn_count_, busy_conn_count_,
               next_conn_id_);
  }
  return conn;
}

std::shared_ptr<DelayReq> TokenlessService::GetDelayReq() {
  //boost::mutex::scoped_lock guard(mutex_);
  std::shared_ptr<DelayReq> req;
  if (!inited_ || delay_reqs_.empty()) {
    return req;
  }
  req = delay_reqs_.front();
  delay_reqs_.pop_front();
  req->set_iter(delay_reqs_.end());
  return req;
}

void TokenlessService::ReturnIdleConn(const TConnListIter& conn_pos) {
  std::shared_ptr<DelayReq> req;
  std::shared_ptr<TokenlessConn> conn;
  {
  boost::mutex::scoped_lock guard(mutex_);
  if (conn_pos == busy_conns_.end()) {
    return ;
  }
  conn = *conn_pos;
  req = GetDelayReq();
  }
  if (!req) {
    boost::mutex::scoped_lock guard(mutex_);
    --busy_conn_count_;
    ++idle_conn_count_;
    if (idle_conn_count_+busy_conn_count_ != next_conn_id_) {
      LOGGER_ERROR(g_log_framework, "act=conn::return conn=%d:%d idle_count=%d"
                 " busy_count=%d next_id=%d",
                 id_, conn->get_id(), idle_conn_count_, busy_conn_count_,
                 next_conn_id_);
    } else {
      LOGGER_DEBUG(g_log_framework,
                 "act=conn::return conn=%d:%d idle_count=%d busy_count=%d"
                 " next_id=%d",
                 id_, conn->get_id(), idle_conn_count_, busy_conn_count_,
                 next_conn_id_);
    }
    idle_conns_.push_back(conn);
    busy_conns_.erase(conn_pos);
    conn->set_iter(busy_conns_.end());
  } else {
    uint64_t cur_time = TimeUtil::GetTickMs();
    if (cur_time >= req->expire_time_) {
      req->mutex_.lock();
      TokenlessRpc rpc_old = req->rpc_;
      req->ClearRpc();
      req->set_iter(req->owner_service_.delay_reqs_.end());
      req->mutex_.unlock();
      LOGGER_ERROR(g_log_framework, "time is up,query timeout, idle_count=%d, busy_count=%d ", idle_conn_count_, busy_conn_count_);
      // 调用callback前释放锁.
      Callback(rpc_old, -1, NULL, req->owner_service_.id_, NULL);
      {
      boost::mutex::scoped_lock guard(mutex_);
      --busy_conn_count_;
      ++idle_conn_count_;
      idle_conns_.push_back(conn);
      busy_conns_.erase(conn_pos);
      conn->set_iter(busy_conns_.end());
      }
      return;
    }
    blitz::Task task;
    blitz::TaskUtil::ZeroTask(&task);
    task.fn_callback = ExecDelayReqCallback;
    task.fn_delete = DelayReqArgsDelete;
    DelayReqArgs* arg =  new DelayReqArgs();
    arg->conn = conn;
    arg->req = req;
    task.args.arg1.ptr = (void*)arg;
    int ret = framework_.PostTask(task);
    if (ret < 0)
      delete arg;
  }
}

void TokenlessService::ExecDelayReqCallback(const blitz::RunContext& context, const blitz::TaskArgs& args) {
  DelayReqArgs* arg = reinterpret_cast<DelayReqArgs*>(args.arg1.ptr);
  TokenlessRpc rpc_old = arg->req->rpc_;
  int ret = arg->conn->Query(arg->req->rpc_);
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, "FailQuery");
    arg->conn->ClearRpc();
    Callback(rpc_old, -1, NULL, arg->req->owner_service_.id_, NULL);
    arg->req->owner_service_.ReturnIdleConn(arg->conn->get_iter());
  }
  delete arg;
  return ;
}

void TokenlessService::DelayReqArgsDelete(const blitz::TaskArgs& args) {
  DelayReqArgs* arg = reinterpret_cast<DelayReqArgs*>(args.arg1.ptr);
  delete arg;
}


/*void DelayReq::OnTimer(int32_t id, void* arg){
  std::shared_ptr<DelayReq> self = reinterpret_cast<DelayReqArgs*>(arg)->req;
  boost::mutex::scoped_lock guard(self->owner_service_.mutex_);
  self->mutex_.lock();
  TokenlessRpc rpc_old = self->rpc_;
  self->ClearRpc();
  self->timer_id_ = 0;
  self->owner_service_.delay_reqs_.erase(self->iter_);
  self->set_iter(self->owner_service_.delay_reqs_.end());
  self->mutex_.unlock();
  LOGGER_ERROR(g_log_framework, "time is up,query timeout ");
  // 调用callback前释放锁.
  Callback(rpc_old, -1, NULL, self->owner_service_.id_, NULL);
  delete reinterpret_cast<DelayReqArgs*>(arg);
}*/

} // namespace blitz.
