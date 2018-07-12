// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ2_CONTROLCHANNEL_H_
#define __BLITZ2_CONTROLCHANNEL_H_ 1
#include <queue>
#include <functional>
#include <blitz2/logger.h>
#include <blitz2/ioobject.h>
#include <blitz2/reactor.h>
namespace blitz2 {
/**
 * class ConotrolChannel is a pipe used send fd between processes.
 * this class is used internally.
 */
class ControlChannel: public IoObject {
 public:
  typedef std::function<void(ControlChannel*,int)> TClosedCallback;
  typedef std::function<void(ControlChannel*,int,int32_t)> TRecvFdCallback;
  struct TransferedFd {
    int fd;
    int32_t type;
  };
  typedef std::queue<TransferedFd> TFdQueue;
  static void OnClosed(ControlChannel*,int) {}
  static void OnRecvFd(ControlChannel*, int fd, int32_t) {::close(fd);}
  ControlChannel(Reactor& reactor)
    :reactor_(reactor), logger_(Logger::GetLogger("blitz2::ControlChannel")) {
      closed_callback_ = OnClosed;
      recv_fd_callback_ = OnRecvFd;
  }
  ~ControlChannel() {
      closed_callback_ = OnClosed;
      recv_fd_callback_ = OnRecvFd;
      RealClose(0);
  }
  int Init(int fd) {
    fd_ = fd;
    evutil_make_socket_nonblocking(fd_);
    reactor_.RegisterIoObject(this);
    return 0;
  }
  virtual void ProcessEvent(const IoEvent& event);
  void SendFd(int fd_to_send, int32_t type);
  void Close() {reactor_.PostTask(std::bind(&ControlChannel::OnCloseTask, this));}
  /**
   * force close, don't notify
   */
  void ForceClose() {
    RealClose(0, false);
  }
  void SetRecvFdCallback(const TRecvFdCallback& cb) {recv_fd_callback_=cb;}
  void SetClosedCallback(const TClosedCallback& cb) {closed_callback_=cb;}
  void ResetAllCallback() {
    closed_callback_ = OnClosed;
    recv_fd_callback_ = OnRecvFd;
  }

 protected:
  void OnRead();
  void OnWrite();
  int WriteFd(int fd_to_send, int32_t type);
  /*
   *
   * @return 0, succeed.
   *         1, receive EOF.
   *         2. NO_DATA_AVAILABLE.
   *         <0. error.
   */
  int ReadFd(int *fd, int32_t* type);
  void RealClose(int reason, bool notify=true) {
    while(!fds_.empty()) {
      TransferedFd& tfd = fds_.front();
      ::close(tfd.fd);
      fds_.pop();
    }
    reactor_.RemoveIoObject(this);
    ::close(fd_);
    fd_=-1;
    if(notify) {
      closed_callback_(this, reason);
    }
  }
  void OnCloseTask() {
    RealClose(0);
  }
 protected:
  Reactor& reactor_;
  Logger& logger_;
  TFdQueue fds_;
  TClosedCallback closed_callback_;
  TRecvFdCallback recv_fd_callback_;
  // Observer* observer_;
};

inline void ControlChannel::ProcessEvent(const IoEvent& event) {
  if (event.haveRead()) {
    OnRead();
  }
  if (fd_!=-1 && event.haveWrite()) {
    OnWrite();
  }
}

} // namespace blitz2.
#endif // __BLITZ2_CONTROLCHANNEL_H_
