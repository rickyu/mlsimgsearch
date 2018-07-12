#pragma once
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <utils/compile_aux.h>
#include "runnable.h"
#include "msg.h"

namespace blitz {
class OutMsg;
class IoId {
 public:
  enum IoType {
    kUnknown = 0,
    kFd = 1,
  };
  static uint64_t IoIdFromFd(int fd,uint32_t salt){
    return (uint64_t)salt<<32 | fd | kFd << 24;
  }
  static int GetType(uint64_t id) {
    return (id>>24) & 0x0FF;
  }

  static int GetFd(uint64_t id) {
    return id & 0x0FFFFFF;
  }
  static uint32_t GetSalt(uint64_t id ) {
    return id >> 32;
  }
  static bool IsValid(uint64_t id)  {
    return GetType(id) == kFd;
  }


  IoId() { id_ = 0; }
  explicit IoId(uint64_t id) { id_ = id; }
  uint64_t get_id() const { return id_; }
  void SetFd(int fd, uint32_t salt){
    id_ = IoIdFromFd(fd,salt);
  }
  bool IsValid() const {
    return IsValid(id_);
  }
  int GetType() const{
    return GetType(id_);
  }
  int GetFd() const {
    return GetFd(id_);
  }
  uint32_t GetSalt() const {
    return GetSalt(id_);
  }
  public:
    uint64_t id_;
};

class IoHandler {
  public:
    virtual int OnRead() = 0;
    virtual int OnWrite() = 0;
    virtual int OnError() = 0;
    virtual int Close()  = 0;
    virtual int SendMsg(OutMsg* msg)  = 0;
    virtual ~IoHandler() {}
};

class IoAddr {
 public:
  enum {
    kTcpIpv4 = 1
  };
  int Parse(const std::string& addr) {
    AVOID_UNUSED_VAR_WARN(addr);
    return 0;
  }
  void SetTcpIpv4(const struct sockaddr_in& ipv4) {
    type_ = kTcpIpv4;
    ipv4_ = ipv4;
  }
  uint32_t get_ipv4() const {
    if (type_ == kTcpIpv4) {
      return ipv4_.sin_addr.s_addr;
    }
    return 0;
  }
  int get_ipv4_str(std::string* str) const {
    if (type_ != kTcpIpv4) {
      return -1;
    }
    char buf[32];
    const char* s = inet_ntop(AF_INET, &ipv4_.sin_addr, buf, 32);
    *str = s;
    return 0;
  }
  void ToString(std::string* str) const {
    switch(type_) {
      case kTcpIpv4:
        {
          char buf[32];
          const char* s = inet_ntop(AF_INET, &ipv4_.sin_addr, buf, 32);
         *str  = s;
         sprintf(buf, ":%d", ntohs(ipv4_.sin_port));
         *str += buf;
        }
        break;
      default:
        break;
    }
  }
 public:
  int type_;
  union {
    struct sockaddr_in ipv4_;
  };
};
class IoInfo {
 public:
  enum {
    kEof = 0x01, // 连接被对端关闭.
    kError = 0x02 // 出现某种错误.
  };
  IoInfo() {
    id_ = 0;
    status_ = 0;
  }
  IoInfo(uint64_t id, uint32_t status) {
    id_ = id;
    status_ = status;
  }
  uint64_t get_id() const { return id_; }
  void set_id(uint64_t id) { id_ = id; }
  bool IsEof() const { return status_ & kEof; }
  bool IsEofOrError() const { return status_ != 0; }
  void SetEof() { status_ |= kEof; }
  void SetError() { status_ |= kError; }
  bool IsError() const { return status_ & kError; }
  uint32_t get_status() const { return status_; }
  const IoAddr& get_remote_addr()  const { return addr_remote_; }
  IoAddr& get_remote_addr() { return addr_remote_; }
 protected:
  uint64_t id_;
  uint32_t status_;
  IoAddr addr_local_;
  IoAddr addr_remote_;
};
class MsgProcessor {
  public:
    virtual int ProcessMsg(const IoInfo& ioinfo, const InMsg& msg) = 0;
    virtual ~MsgProcessor() {}
};
class IoStatusObserver {
  public:
    virtual int OnOpened(const IoInfo& ioinfo) = 0;
    virtual int OnClosed(const IoInfo& ioinfo) = 0;
    virtual ~IoStatusObserver() {}
};


class Framework;
typedef void (*FnHandlerFree)(IoHandler* handler,void* ctx);
struct FdDescriptor {
  int fd;
  uint64_t io_id;
  IoHandler* handler;
  FnHandlerFree fn_free;
  void* free_ctx;
  struct event* ev;
  Framework* framework;
  RunContext run_context;
};
// TODO(YuZuo) 将已经使用的fd连成链表.
class FdManager {
  public:
    FdManager() {
      max_count_ = 0;
      // count_ = 0;
      items_ = NULL;
    }
    ~FdManager(){
      Uninit();
    }
    int Init(int max_fd_count);
    int Uninit();    
    int get_max_count() const { return max_count_; }
    FdDescriptor* Use(int fd){
      assert(items_);
      if(fd >= max_count_ || fd < 0){
        return NULL;
      }
      if(items_[fd].in_use){
        return NULL;
      }
      items_[fd].in_use = true;
      return &items_[fd].descriptor;
    }

    FdDescriptor* GetByIoId(uint64_t io_id){
      IoId id;
      id.id_ = io_id;
      int fd = id.GetFd();
      // uint32_t salt = id.GetSalt();
      if(fd < 0 || fd >= max_count_){
        return NULL;
      }
      if(items_[fd].in_use == false){
        return NULL;
      }
      if(items_[fd].descriptor.io_id != io_id){
        return NULL;
      }
      return &items_[fd].descriptor;
    }
    FdDescriptor* GetFd(int fd){
      assert(items_);
      if(!items_ ){
        return NULL;
      }
      return &items_[fd].descriptor;
    }
    int FreeFd(int fd){
      if(fd >= max_count_){
        return -1;
      }
      items_[fd].in_use = false;
      memset(&items_[fd].descriptor ,0,sizeof(FdDescriptor));
      return 0;
    }
    uint32_t NextSalt(int fd) {
      return ++ items_[fd].next_salt ;
    }
 private:
  FdManager(const FdManager&);
  FdManager& operator=(const FdManager&);
 private:
  struct Item {
    uint32_t next_salt;
    bool in_use;
    FdDescriptor descriptor;
  };
  int max_count_;
  Item* items_;
};
} // namespace blitz.
