// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// Acceptor. 
#ifndef __BLITZ2_ACCEPTOR_H_
#define __BLITZ2_ACCEPTOR_H_ 1

#include <blitz2/ioobject.h>
#include <blitz2/ioaddr.h>
#include <blitz2/logger.h>
#include <blitz2/reactor.h>
namespace blitz2 {
/**
 * socket for listen.
 */
class Acceptor:public IoObject {
 public:
  class Observer {
   public:
    virtual void OnConnectAccepted(Acceptor* acceptor, int fd) {
      (void)(acceptor);
      ::close(fd);
    }
    virtual void OnClosed(Acceptor* acceptor) { (void)(acceptor); }
    virtual ~Observer() {}
  };
  Acceptor(Reactor& reactor)
    :logger_(Logger::GetLogger("blitz2::Acceptor")) , reactor_(reactor) {
    observer_ = &s_default_observer_;
  }
  ~Acceptor() { Close();}
  static int BindAndListen(const IoAddr& addr);
  int Bind(const std::string& addr);
  int Bind(const IoAddr& addr) { 
    if(fd_!=-1) { return 1; }
    addr_ = addr; 
    return Bind(); 
  }
  int Bind();
  void Close();
  const IoAddr& addr() const { return addr_;}
  Observer* observer() { return observer_;}
  void SetObserver(Observer* ob) {
    if(ob) {
      observer_ = ob;
    } else {
      observer_ = &s_default_observer_;
    }
  }
  virtual void ProcessEvent(const IoEvent& ioevent);
 protected:
  Logger& logger_;
  Reactor& reactor_;
  Observer *observer_;
  IoAddr addr_;
  static Observer s_default_observer_;
};
inline void Acceptor::Close() {
  if(fd_>=0) {
    reactor_.RemoveIoObject(this);
    close(fd_);
    fd_ = -1;
  }
}

} // namespace blitz2.
#endif // __BLITZ2_ACCEPTOR_H_

