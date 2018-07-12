// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_FRAMEWORK_H_
#define __BLITZ_FRAMEWORK_H_ 1

#include <boost/thread.hpp>

#include <queue>
#include <string>
#include <map>
#include <utils/compile_aux.h>
#include "runnable.h"
#include "fd_manager.h"
#include "msg.h"
#include "token_mgr.h"

#include "framework_thread.h"

namespace blitz {
enum ReturnCode {
  kInvalidParam = -5000,
  kObjectUninit = -5001,
  kInvalidAddr = -5002,
  kNotSupportAddr = -5003,
  kSocketError = -5004
};
class Framework {
 public:
   struct TlsData {
    int thread_number;
  };

  Framework();
  ~Framework();
  // 状态和属性操作.
  int Init();
  void Uninit();
  int a() { return 10; }
  int Start();
  int Stop();
  bool IsReady() const;
  int GetCurrentThreadNumber();
  int set_thread_count(int count);
  int get_thread_count() const { return thread_count_; }
  int set_max_fd_count(int count);
  int get_max_fd_count() const { return fd_manager_.get_max_count(); }
  void set_max_task_execute_time(int val) {
    assert(val>=0);
    max_task_execute_time_ = val;
  }
  pthread_key_t get_tls_key() const { return tls_key_; }
  // *****************io操作相关**********************.
  void AssignFdIoId(int fd, uint64_t* io_id);
  // 建立Io连接.
  // @param addr: [IN],目标地址,当前仅支持tcp协议.如tcp://127.0.0.1:9090
  // @param decoder_factory:[IN] 如果对数据流进行decoder.
  // @param io_id: [OUT] io唯一标识, 在返回0时有效.
  // @return 1: 连接正在进行，将通过status_observer->OnOpened进行通知.
  //         - 0 连接成功, OnOpened不会被调用.
  //         - 其他 失败, OnOpened不会被调用.
  int Connect(const std::string& addr,
      MsgDecoderFactory* decoder_factory,
      MsgProcessor* msg_processor,
      IoStatusObserver* status_observer,
      uint64_t* io_id);
  int Bind(const std::string& addr,
      MsgDecoderFactory* decoder_factory,
      MsgProcessor* msg_processor,
      uint64_t* io_id,
      IoStatusObserver* observer=NULL);
  int SendMsg(uint64_t ioid, OutMsg* msg);
  // 异步关闭IO句柄.
  int CloseIO(uint64_t io_id);
  int AddIO(uint64_t io_id,
            IoHandler* handler,
            FnHandlerFree fn_free,
            void* free_context);
  int AddAcceptTcp(uint64_t io_id,
            IoHandler* handler,
            FnHandlerFree fn_free,
            void* free_context);
  // 重新激活IO,在ET模式下可能存车不能马上读完数据的情况，则重新激活.
  int ReactivateIO(uint64_t io_id);
  // task相关.
  int PostTask(const Task& task);
  int PostTask(int thread_number, const Task& task);
  // 利用线程本地存储做的，只有blitz起的线程才能调用.
  int32_t SetTimer(uint32_t ms, FnTimerCallback callback, void* arg);
  // 外部起的线程调用.
  int SetExternalTimer(uint32_t ms, FnTimerCallback callback, void* arg);
  int GetThreadNumber() const;
 private:
  Framework(const Framework&);
  Framework& operator=(const Framework&);
  int Run();
  int RealSendMsg(uint64_t io_id, OutMsg* msg);
  int RealCloseIO(uint64_t io_id);
  int RealAddFd(uint64_t io_id,
                IoHandler* handler,
                FnHandlerFree fn_free,
                void* free_ctx);
  int RealReaddFd(int fd);
  int RealDelFd(int fd);
  int TlsInit();
  int TlsUninit();
  void CheckThreads();

  // lievent 回调函数.
  static void LibeventCallback(evutil_socket_t fd,
                        short event, void* arg);  // NOLINT(runtime/int)
  static void LibeventSigHup(evutil_socket_t fd,
                             short event, void* arg);   // NOLINT(runtime/int)

  // 自身task回调函数.
  static void SendMsgTaskCallback(const RunContext& context,
                                  const TaskArgs& args);
  static void SendMsgTaskArgFree(const TaskArgs& args) {
      reinterpret_cast<OutMsg*>(args.arg2.ptr)->Release();
  }
  static void CloseIOTaskCallback(const RunContext& context,
                                  const TaskArgs& args);
  static void AddIOTaskCallback(const RunContext& context,
                                  const TaskArgs& args);
  static void AddAcceptTcpCallback(const RunContext& context,
                                  const TaskArgs& args);
  static void ReactivateIOTaskCallback(const RunContext& context,
                                  const TaskArgs& args);
  static void AddTcpConnTaskCallback(const RunContext& context,
                                     const TaskArgs& args);
  static void AddTcpConnTaskDelete(const TaskArgs& args);
  static void AddTcpServTaskCallback(const RunContext& context,
                                     const TaskArgs& args);
  static void AddTcpServTaskDelete(const TaskArgs& args);
  static void SetTimerTaskCallback(const RunContext& context,
                                   const TaskArgs& args);
  static void TimerCheckThreadsCallback(int32_t timer_id, void* user_arg);
  static void OnSigUsr1(int signo);
 private:
  FdManager fd_manager_;
  struct event* hup_event_;
  FrameworkThread *threads_;
  FrameworkThread thread_myself_;
  int thread_count_;
  int thread_iter_;
  enum Status {
    kUninit = 0,
    kInit,
    kStart
  } status_;
  bool tls_inited_;
  pthread_key_t tls_key_;
  int max_task_execute_time_; // 允许的task最长执行时间.
};
inline  bool Framework::IsReady() const { 
  return status_ != kUninit; 
}
inline  void Framework::AssignFdIoId(int fd, uint64_t* io_id) {
  *io_id = IoId::IoIdFromFd(fd, fd_manager_.NextSalt(fd));
}
inline void Framework::TimerCheckThreadsCallback(int32_t id,
                                                 void* user_arg) {
  AVOID_UNUSED_VAR_WARN(id);
  reinterpret_cast<Framework*>(user_arg)->CheckThreads();
}
inline int Framework::GetThreadNumber() const {
  TlsData* tls_data = 
    reinterpret_cast<TlsData*>(pthread_getspecific(get_tls_key()));
  if (tls_data) {
    return tls_data->thread_number;
  }
  return -2;
}
} // namespace blitz.
#endif  //  __FRAMEWORK_H_
