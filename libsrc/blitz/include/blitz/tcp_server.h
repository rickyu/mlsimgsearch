#pragma once
#include <string.h>
#include <queue>
#include <iostream>
#include <boost/thread.hpp>
#include <event2/event.h>
#include "runnable.h"
#include "msg.h"
#include "fd_manager.h"

namespace blitz {
class TcpConn;
class Framework;
class MsgProcessor;


class TcpServer : public IoHandler {
 public:
  TcpServer(Framework* framework,MsgDecoderFactory* factory,
      MsgProcessor* processor, IoStatusObserver* observer) {
    framework_ = framework;
    msg_parser_factory_ = factory;
    processor_ = processor;
    observer_ = observer;
    fd_ = -1;
  }
  ~TcpServer() {
    Close();
  }
  int Bind(const char* name,uint16_t port);
  int fd() const { return fd_; }
  void set_id( uint64_t id) { my_id_ = id; }
  uint64_t get_id() const { return my_id_; }
  // ****************IoHandler 接口.****************//
  virtual int OnRead();
  virtual int OnWrite();
  virtual int OnError();
  virtual int Close();
  virtual int SendMsg(OutMsg* out_msg);
 private:
  TcpServer(const TcpServer&);
  TcpServer& operator=(const TcpServer&);
 private:
  Framework* framework_;
  MsgDecoderFactory* msg_parser_factory_;
  MsgProcessor* processor_;
  IoStatusObserver* observer_;
  int fd_;
  uint64_t my_id_;
};

} // namespace blitz.

