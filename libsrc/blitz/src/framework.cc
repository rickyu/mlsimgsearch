// Copyright 2012 meilishuo Inc.
// Author: 余佐
//
// The implementation of Framework .
#include <signal.h>
#include <event2/event.h>
#include <utils/logger.h>
#include "blitz/framework.h"
#include "blitz/tcp_server.h"
#include "blitz/tcp_connect.h"

namespace blitz {

static CLogger& g_log_framework = CLogger::GetInstance("framework");
static void FreeFdHandler(IoHandler* handler, void* /* context */) {
    delete handler;
}


Framework::Framework() {
  hup_event_ = NULL;
  threads_ = NULL;
  thread_count_ = 0;
  thread_iter_ = 0;
  status_ = kUninit; 
  tls_inited_ = false;
  tls_key_ =  0;
  max_task_execute_time_ = 5;
}
Framework::~Framework() {
  Uninit();
}
int Framework::Init() {
  if (status_ != kUninit) { return 1; }
  if ( TlsInit() != 0) abort();
  struct sigaction act ;
  memset(&act, 0, sizeof(act));
  act.sa_handler = OnSigUsr1;
  int ret = sigaction(SIGUSR1, &act, NULL);
  assert(ret == 0);
  
  if (fd_manager_.get_max_count() == 0) {
    if (fd_manager_.Init(65535) != 0) {
      abort();
    }
  }
  thread_myself_.Init(-1, this);
  if (thread_count_ == 0) {
    // TODO(YuZuo) set applicable thread count according to the cpu num.
    set_thread_count(1);
  }
  status_ = kInit;
  return 0;
}
void Framework::Uninit() {
  assert(status_ != kStart);
  // 释放掉所有句柄.
  fd_manager_.Uninit();
  if (hup_event_) {
    event_free(hup_event_);
    hup_event_ = NULL;
  }
  // 停止线程.
  thread_myself_.Uninit();
  if (threads_) {
    for (int i = 0; i < thread_count_; ++i) {
      threads_[i].Uninit();
    }
    delete [] threads_;
    threads_ = NULL;
    thread_count_ = 0;
  }
  TlsUninit();
  status_ = kUninit;
}


int Framework::Start() {
  if (status_ != kInit) { return 1; }
  if (thread_count_ == 0) { 
    set_thread_count(1);
  }
  for (int i = 0; i < thread_count_; ++i) {
    threads_[i].Start();
  }
  hup_event_ = evsignal_new(thread_myself_.libevent_, SIGINT,
      LibeventSigHup, this);
  event_add(hup_event_, NULL);
  status_ = kStart;
  thread_myself_.running_ = 1;
  return Run();
}
int Framework::Stop() {
  if (status_ != kStart) { return 1; }
  if(!threads_) {
    status_ = kInit;
    return 2;
  }
  for (int i = 0; i < thread_count_; ++i) {
    threads_[i].running_ = 0;
  }
  for (int i = 0; i < thread_count_; ++i) {
    pthread_join(threads_[i].run_context_.thread_id, NULL);
  }
  thread_myself_.running_  = 0;
  status_ = kInit;
  return 0;
}
int Framework::GetCurrentThreadNumber() {
  void* ptr = pthread_getspecific(tls_key_);
  TlsData* tls_data = reinterpret_cast<TlsData*>(ptr);
  return tls_data->thread_number;
}
int Framework::set_thread_count(int count) {
  assert(count > 0);
  if (status_ == kStart) { return 1; }
  if (threads_) {
    delete [] threads_;
    threads_ = NULL;
  }
  thread_count_ = count;
  threads_ = new FrameworkThread[thread_count_ ];
  if (threads_ == NULL) { abort(); }
  for (int i = 0; i < thread_count_; ++i) {
    threads_[i].Init(i, this);
  }
  return 0;
}
int Framework::set_max_fd_count(int count) {
  return fd_manager_.Init(count);
}

int Framework::Connect(const std::string& addr,
                      MsgDecoderFactory* decoder_factory,
                      MsgProcessor* msg_processor,
                      IoStatusObserver* status_observer,
                      uint64_t* io_id) {
  if (!decoder_factory) { return kInvalidParam; }
  if (!msg_processor) { return kInvalidParam; }
  if (status_ == kUninit) { return kObjectUninit; }
  size_t pos = addr.find(':');
  if (pos == std::string::npos) { return kInvalidAddr; }
  std::string protocol = addr.substr(0, pos);
  if (protocol == "tcp") {
    // process tcp protocol.
    size_t pos2 = addr.find(':', pos + 1);
    if (pos2 == std::string::npos) { return kInvalidAddr; }
    std::string ip = addr.substr(pos+3, pos2-pos-3);
    std::string port = addr.substr(pos2+1);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      int the_errno = errno;
      char str[80];
      str[0] = '\0';
      char* err_str = strerror_r(the_errno, str, sizeof(str));
      LOGGER_ERROR(g_log_framework, "Connect fail socket addr=%s"
                  " errno=%d err_str=%s", 
                  addr.c_str(), the_errno, err_str);
      return kSocketError;
    }
    TcpConn * conn = new TcpConn(this, msg_processor, decoder_factory,
                                 status_observer);
    assert(conn != NULL);
    conn->fd_ = fd;
    uint64_t ioid = 0;
    AssignFdIoId(fd, &ioid);
    if (io_id) { *io_id = ioid; }
    conn->addr_ = addr;
    conn->ioinfo_.set_id(ioid);
    int conn_ret = conn->Connect(ip.c_str(), atoi(port.c_str()));
    if (conn_ret != 0 && conn_ret != 1) { 
      delete conn;
      return -1; 
    }
    Task task;
    TaskUtil::ZeroTask(&task);
    task.fn_callback = AddTcpConnTaskCallback;
    task.fn_delete = AddTcpConnTaskDelete;
    task.args.arg1.ptr = conn;
    int thread_number = fd % thread_count_;
    int ret = PostTask(thread_number, task);
    if (ret != 0) {
      LOGGER_NOTICE(g_log_framework, "Connect fail PostTask ret=%d addr=%s",
                 ret, addr.c_str());
      delete conn;
      return -1;
    }
    return conn_ret;

  } else {
    LOGGER_NOTICE(g_log_framework, "Connect fail addrNotSupport addr=%s",
                 addr.c_str());
    return kNotSupportAddr; 
  }
  return 0;
}
int Framework::Bind(const std::string& addr,
                    MsgDecoderFactory* factory,
                    MsgProcessor* processor,
                    uint64_t* io_id,
                    IoStatusObserver* observer) {
  if (!factory || !processor) { 
    return kInvalidParam; 
  }
  if (status_ == kUninit) {
    return kObjectUninit; 
  }
  size_t pos = addr.find(':');
  if (pos == std::string::npos) { return kInvalidAddr; }
  std::string protocol = addr.substr(0, pos);
  if (protocol == "tcp") { 
    size_t pos2 = addr.find(':', pos + 1);
    if (pos2 == std::string::npos) { return kInvalidAddr; }
    std::string ip = addr.substr(pos+3, pos2-pos-3);
    std::string port = addr.substr(pos2+1);
    TcpServer* serv = new TcpServer(this, factory, processor, observer);
    int ret = serv->Bind(ip.c_str(), static_cast<uint16_t>(atoi(port.c_str())));
    if (ret != 0) {
      LOGGER_ERROR(g_log_framework, "Bind fail network ret=%d addr=%s",
                  ret, addr.c_str());
      delete serv;
      serv = NULL;
      return ret;
    }
    uint64_t id = 0;
    AssignFdIoId(serv->fd(), &id);
    serv->set_id(id);
    if (io_id) { *io_id = serv->get_id(); }
    Task task ;
    TaskUtil::ZeroTask(&task);
    task.fn_callback = AddTcpServTaskCallback;
    task.fn_delete = AddTcpServTaskDelete;
    task.args.arg1.ptr = serv;
    int thread_number = serv->fd() % thread_count_;
    ret = PostTask(thread_number, task);
    if (ret != 0) {
      LOGGER_ERROR(g_log_framework, "Bind fail PostTask ret=%d addr=%s",
                  ret, addr.c_str());
      delete serv;
      serv = NULL;
      return ret;
    }
    return 0;
  }
  else { 
    LOGGER_NOTICE(g_log_framework, "Bind fail addrNotSupport addr=%s",
                  addr.c_str());
    return kNotSupportAddr; 
  }
  return 0;
}
int Framework::SendMsg(uint64_t io_id, OutMsg* out_msg) {
  if (!out_msg || !IoId::IsValid(io_id)) { 
    return kInvalidParam;
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int fd = IoId::GetFd(io_id);
  int thread_number = fd % thread_count_;
  if (thread_number == GetThreadNumber()) {
    out_msg->AddRef();
    return RealSendMsg(io_id, out_msg);
  }
  Task task;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = SendMsgTaskCallback;
  task.fn_delete = SendMsgTaskArgFree;
  task.args.arg1.u64 = io_id;
  task.args.arg2.ptr  = out_msg;
  out_msg->AddRef();
  int ret = PostTask(thread_number, task);
  if (ret != 0) {
    out_msg->Release();
    return ret;
  }
  return 0;
}

int Framework::CloseIO(uint64_t  io_id) {
  if (!IoId::IsValid(io_id)) {
    return kInvalidParam;
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int fd = IoId::GetFd(io_id);
  int thread_number = fd % thread_count_;
  Task task ;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = CloseIOTaskCallback;
  task.args.arg1.u64 = io_id;
  return PostTask(thread_number, task);
}
int Framework::AddIO(uint64_t io_id,
                     IoHandler* handler,
                     FnHandlerFree fn_free,
                     void* free_context)
                      {
  if (!IoId::IsValid(io_id) || !handler) {
    return kInvalidParam; 
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int fd = IoId::GetFd(io_id);
  int thread_number = fd % thread_count_;
  Task task;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = AddIOTaskCallback;
  task.args.arg1.u64 = io_id;
  task.args.arg2.ptr = handler;
  task.args.arg3.ptr = reinterpret_cast<void*>(fn_free);
  task.args.arg4.ptr = free_context;
  return PostTask(thread_number, task);
}

int Framework::AddAcceptTcp(uint64_t io_id,
                            IoHandler* handler,
                            FnHandlerFree fn_free,
                            void* free_context) {
  if (!IoId::IsValid(io_id) || !handler) {
    return kInvalidParam; 
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int fd = IoId::GetFd(io_id);
  int thread_number = fd % thread_count_;
  Task task;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = AddAcceptTcpCallback;
  task.args.arg1.u64 = io_id;
  task.args.arg2.ptr = handler;
  task.args.arg3.ptr = reinterpret_cast<void*>(fn_free);
  task.args.arg4.ptr = free_context;
  return PostTask(thread_number, task);
}

int Framework::ReactivateIO(uint64_t io_id) {
  if (!IoId::IsValid(io_id)) {
    return kInvalidParam;
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int fd = IoId::GetFd(io_id);
  int thread_number = fd % thread_count_;
  Task task;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = ReactivateIOTaskCallback;
  task.args.arg1.u64 = io_id;
  return PostTask(thread_number, task);

}
int Framework::PostTask(const Task& task) {
  if (!task.fn_callback) { 
    return kInvalidParam; 
  }
  if (status_ == kUninit) {
    return kObjectUninit;
  }
  int thread_number = thread_iter_ % thread_count_;
  ++thread_iter_;
  return threads_[thread_number].PostTask(task);
}
int Framework::PostTask(int thread_number, const Task& task) {
  assert(thread_number >= 0 && thread_number < thread_count_);
  if (!task.fn_callback) { 
    return kInvalidParam; 
  }
  if (status_ == kUninit) { 
    return kObjectUninit; 
  }
   // TODO(YUZUO) 检查run_context的正确性,thread_idx和thread_id是否匹配.
  return threads_[thread_number].PostTask(task);
}
int32_t Framework::SetTimer(
    uint32_t timeout_ms,
    FnTimerCallback callback,
    void* arg) {
  if (status_ == kUninit) {
    return -1 ;
  }

  if (callback == NULL) {
    return -1;
  }
  // 调用本线程的SetTimer, 因为libevent线程不安全.
  TlsData* tls_data = reinterpret_cast<TlsData*>(pthread_getspecific(tls_key_));
  assert(tls_data != NULL);
  int thread_number = tls_data->thread_number;
  if (thread_number >= 0 && thread_number < thread_count_) {
    return threads_[thread_number].SetTimer(timeout_ms, callback, arg);
  } else if (thread_number == -1) {
    return thread_myself_.SetTimer(timeout_ms, callback, arg);
  } else {
    return -5;
  }
}

int Framework::SetExternalTimer(uint32_t ms, FnTimerCallback callback,
                                void* arg) {
  Task task ;
  TaskUtil::ZeroTask(&task);
  task.fn_callback = SetTimerTaskCallback;
  task.args.arg1.u64 = ms;
  task.args.arg2.ptr = reinterpret_cast<void*>(callback);
  task.args.arg3.ptr = arg;
  task.args.arg4.ptr = this;
  return PostTask(task);

}
int Framework::Run() {
  thread_myself_.run_context_.thread_id = pthread_self();
  thread_myself_.SetTimer(5000, TimerCheckThreadsCallback, this);
  return thread_myself_.Loop();
}

int Framework::RealSendMsg(uint64_t io_id, OutMsg* out_msg) {
  // 这个函数只在fd所属的线程中被调用.
  IoId the_id;
  the_id.id_ = io_id;
  int fd = the_id.GetFd();
  bool want_to_del = false;
  FdDescriptor* descriptor = fd_manager_.GetByIoId(io_id);
  if (!descriptor) {
    out_msg->Release();
    return -1;
  }
  assert(descriptor->handler);
  if (descriptor->handler->SendMsg(out_msg) != 0) {
    want_to_del = true;
  }
  if (want_to_del) {
    RealDelFd(fd);
  }
  return 0;
}
int Framework::RealCloseIO(uint64_t io_id) {
  if (!IoId::IsValid(io_id)) {
    return kInvalidParam;
  }
  FdDescriptor* descriptor = fd_manager_.GetByIoId(io_id);
  if (!descriptor) {
    LOGGER_ERROR(g_log_framework, "closeIO FailToGetDescriptor ioid=0x%lu",
                 io_id );
    return -1;
  } 
  if (descriptor->ev) {
    event_del(descriptor->ev);
    event_free(descriptor->ev);
  }
  if (descriptor->handler && descriptor->fn_free) {
    descriptor->fn_free(descriptor->handler, descriptor->free_ctx);
  }
  fd_manager_.FreeFd(IoId::GetFd(io_id));
  return 0;
}
int Framework::RealAddFd(uint64_t io_id,
                     IoHandler* handler,
                     FnHandlerFree fn_free,
                     void* free_ctx) {
  if (!IoId::IsValid(io_id)  || !handler ) {
    return kInvalidParam;
  }
  int fd = IoId::GetFd(io_id);
  int ret = 0;
  int thread_number = fd % thread_count_;
  FdDescriptor *descriptor  = fd_manager_.Use(fd);
  if (!descriptor) {
    return -2;
  }
  descriptor->io_id = io_id;
  descriptor->handler = handler;
  descriptor->fn_free = fn_free;
  descriptor->free_ctx = free_ctx;
  descriptor->fd = fd;
  descriptor->run_context.thread_number = thread_number;
  descriptor->run_context.thread_id =
    threads_[thread_number].run_context_.thread_id;
  descriptor->run_context.framework = this;
  descriptor->framework = this;
  descriptor->ev = event_new(threads_[thread_number].libevent_, fd,
      EV_READ|EV_WRITE|EV_PERSIST|EV_ET, LibeventCallback, descriptor);
  if (!descriptor->ev) {
    abort();
  }
  ret = event_add(descriptor->ev, NULL) ;
  if (ret != 0) {
    RealDelFd(fd);
    return -3;
  }
  return 0;
}

int Framework::RealReaddFd(int fd) {
  if (fd <= 0) {
    return -1;
  }
  FdDescriptor *descriptor  = fd_manager_.GetFd(fd);
  event_del(descriptor->ev);
  event_add(descriptor->ev, NULL);
  return 0;
}

int Framework::RealDelFd(int fd) {
  FdDescriptor* descriptor = fd_manager_.GetFd(fd);
  if (!descriptor) {
    return -1;
  } 
  if (descriptor->ev) {
    event_del(descriptor->ev);
    event_free(descriptor->ev);
  }
  if (descriptor->handler && descriptor->fn_free) {
    descriptor->fn_free(descriptor->handler, descriptor->free_ctx);
  }
  fd_manager_.FreeFd(fd);
  return 0;
}
int Framework::TlsInit() {
  if (tls_inited_) { return 1; }
  int ret = pthread_key_create(&tls_key_, NULL);
  if (ret == 0) {
    TlsData* tls_data = reinterpret_cast<TlsData*>(malloc(sizeof(TlsData)));
    if (!tls_data) {
      LOGGER_ERROR(g_log_framework, "act=tls fail(malloc)");
      pthread_key_delete(tls_key_);
      return -501;
    }
    tls_data->thread_number = -1;
    if (pthread_setspecific(get_tls_key(), tls_data) != 0) {
      LOGGER_ERROR(g_log_framework, "act=tls fail(pthread_setspecific)");
      free(tls_data);
      pthread_key_delete(tls_key_);
      return -502;
    }
    tls_inited_ = true;
    return 0;
  } else {
    LOGGER_ERROR(g_log_framework, "pthread_key_create ret=%d", ret);
    return -1;
  }
}
int Framework::TlsUninit() {
  if (!tls_inited_) { return 1; }
  TlsData* tls_data = reinterpret_cast<TlsData*>(pthread_getspecific(tls_key_)); 
  if (tls_data) {
    free(tls_data);
    tls_data = NULL;
  }
  pthread_key_delete(tls_key_);
  tls_inited_ = false;
  return 0;
}
void Framework::CheckThreads() {
  for (int i = 0; i < thread_count_; ++i) {
    if (threads_[i].IsTaskExecuteTimeout(max_task_execute_time_)) {
      threads_[i].running_ = 0;
      LOGGER_ERROR(g_log_framework, "taskExecuteTimeout thread_number=%d", i);
      //pthread_kill(threads_[i].run_context_.thread_id, SIGUSR1);
      //pthread_join(threads_[i].run_context_.thread_id, NULL);
      //threads_[i].Start();
      abort();
    }
  }
  thread_myself_.SetTimer(5000, TimerCheckThreadsCallback, this);
}


void Framework::LibeventSigHup(evutil_socket_t /* fd */,
    short /* event */, void* arg) {  // NOLINT(runtime/int)
  Framework* framework = reinterpret_cast<Framework*>(arg);
  framework->Stop();
}
void Framework::LibeventCallback(evutil_socket_t fd,
    short event, void* arg) {  // NOLINT(runtime/int)
  FdDescriptor* descriptor = reinterpret_cast<FdDescriptor*>(arg);
  IoHandler* handler = descriptor->handler;
  if (EV_READ & event) {
    uint64_t t1 = TimeUtil::GetTickMs();
    if (handler->OnRead() != 0) {
      descriptor->framework->RealDelFd(fd);
      return;
    }
    LOGGER_DEBUG(g_log_framework, "LibEventCallback, read_cost=%lu", TimeUtil::GetTickMs() - t1);
  }
  if (EV_WRITE & event) {
    uint64_t t1 = TimeUtil::GetTickMs();
    if (handler->OnWrite() != 0) {
      descriptor->framework->RealDelFd(fd);
    }
    LOGGER_DEBUG(g_log_framework, "LibEventCallback, write_cost=%lu", TimeUtil::GetTickMs() - t1);
  }
}

void Framework::SendMsgTaskCallback(const RunContext& context,
                                    const TaskArgs& args) {
  context.framework->RealSendMsg(
      args.arg1.u64, reinterpret_cast<OutMsg*>(args.arg2.ptr));
}

void Framework::CloseIOTaskCallback(const RunContext& context,
                                    const TaskArgs& args) {
  IoId the_id;
  the_id.id_ = args.arg1.u64;
  int fd = the_id.GetFd();
  context.framework->RealDelFd(fd);
}
void Framework::AddIOTaskCallback(const RunContext& context,
                                  const TaskArgs& args) {
  uint64_t  io_id = args.arg1.u64;
  int ret = context.framework->RealAddFd(io_id, 
      reinterpret_cast<IoHandler*>(args.arg2.ptr),
      reinterpret_cast<FnHandlerFree>(args.arg3.ptr),
      args.arg4.ptr);
  if (ret != 0) {
    IoHandler* handler = reinterpret_cast<IoHandler*>(args.arg2.ptr);
    FnHandlerFree fn_free = reinterpret_cast<FnHandlerFree>(args.arg3.ptr);
    if (handler && fn_free) {
      fn_free(handler, args.arg4.ptr);
    }
  }
}
void Framework::AddAcceptTcpCallback(const RunContext& context,
                                  const TaskArgs& args) {
  uint64_t io_id = args.arg1.u64;
  int ret = context.framework->RealAddFd(io_id, 
      reinterpret_cast<IoHandler*>(args.arg2.ptr),
      reinterpret_cast<FnHandlerFree>(args.arg3.ptr),
      args.arg4.ptr);
  int fd = IoId::GetFd(io_id);
  reinterpret_cast<TcpConn*>(args.arg2.ptr)->PassiveConnected(fd);
  if (ret != 0) {
    IoHandler* handler = reinterpret_cast<IoHandler*>(args.arg2.ptr);
    FnHandlerFree fn_free = reinterpret_cast<FnHandlerFree>(args.arg3.ptr);
    if (handler && fn_free) {
      fn_free(handler, args.arg4.ptr);
    }
  }
}
void Framework::ReactivateIOTaskCallback(const RunContext& context,
                                    const TaskArgs& args) {
  uint64_t io_id = args.arg1.u64;
  int fd = IoId::GetFd(io_id);
  context.framework->RealReaddFd(fd);
}



void Framework::AddTcpConnTaskCallback(const RunContext& run_context,
                           const TaskArgs& args ) {
  TcpConn* conn = reinterpret_cast<TcpConn*>(args.arg1.ptr);
  int ret = run_context.framework->RealAddFd(conn->ioinfo_.get_id(), conn, FreeFdHandler,
                              NULL);
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, "AddFd fail ret=%d fd=%d name=%s-%s",
        ret, conn->fd(), conn->get_local_name(), conn->get_remote_name());
    delete conn;
  }
}
void Framework::AddTcpConnTaskDelete(const TaskArgs& args ) {
  TcpConn* conn = reinterpret_cast<TcpConn*>(args.arg1.ptr);
  delete conn;
}
void Framework::AddTcpServTaskCallback(const RunContext& run_context,
                           const TaskArgs& args ) {
  TcpServer* serv = reinterpret_cast<TcpServer*>(args.arg1.ptr);
  int ret = run_context.framework->RealAddFd(serv->get_id(), serv, FreeFdHandler,
                              NULL); 
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, "AddFd fail ret=%d fd=%d",
        ret, serv->fd());
    delete serv;
  }
}
void Framework::AddTcpServTaskDelete(const TaskArgs& args ) {
  TcpServer* serv = reinterpret_cast<TcpServer*>(args.arg1.ptr);
  delete serv;
}
void Framework::SetTimerTaskCallback(const RunContext& context,
                                     const TaskArgs& args) {
  AVOID_UNUSED_VAR_WARN(context);
  Framework* self = reinterpret_cast<Framework*>(args.arg4.ptr);
  int32_t id = self->SetTimer(static_cast<uint32_t>(args.arg1.u64),
          reinterpret_cast<FnTimerCallback>(args.arg2.ptr), args.arg3.ptr);
  if (id <= 0) {
    LOGGER_ERROR(g_log_framework, "act=timer:set fail ret=%d",
                 id);
  }
}
void Framework::OnSigUsr1(int /* signo */) {
  LOGGER_ERROR(g_log_framework, "thread_timerout");
  abort();
}
} // namespace blitz.

