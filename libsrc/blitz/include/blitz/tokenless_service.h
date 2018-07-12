// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_TOKENLESS_SERVICE_H_
#define __BLITZ_TOKENLESS_SERVICE_H_ 1
#include <string>
#include <queue>
#include <boost/thread.hpp>
#include "rpc.h"
#include "msg.h"
#include "fd_manager.h"
#include "token_mgr.h"
// #include "service.h"
#include "ref_object.h"
#include "utils/time_util.h"

namespace blitz {
class TokenlessServicePool;
struct TokenlessCallbackContext {
  int try_count;
  TokenlessServicePool* pool;
  RpcCallback rpc;
  void* extra_ptr;
};
class TokenlessConn;
class DelayReq;
struct DelayReqArgs {
  std::shared_ptr<TokenlessConn> conn;
  std::shared_ptr<DelayReq> req ;
};

typedef void (*FnTokenlessCallback) (
    int result,
    const InMsg* msg,
    int service_id,
    OutMsg* out_msg,
    int log_id,
    uint64_t expire_time,
    const TokenlessCallbackContext& ctx,
    const IoInfo* ioinfo);

struct TokenlessCallback {
  FnTokenlessCallback fn_action;
  TokenlessCallbackContext context;
};
struct TokenlessRpc {
  OutMsg* out_msg;
  TokenlessCallback callback;
  int log_id;
  uint64_t expire_time;
};
class TokenlessService;
class TokenlessConn : public MsgProcessor,
                      public IoStatusObserver {
 public:
  typedef std::list< std::shared_ptr<TokenlessConn> >::iterator TConnListIter;
  TokenlessConn(TokenlessService& owner_service)
    : owner_service_(owner_service) {
    id_ = -1;
    io_id_ = 0;
    // expire_time_ = 0;
    status_ = kClosed;
    memset(&rpc_, 0, sizeof(rpc_));
  }
  void set_id(int id) { id_ = id; }
  int Query(const TokenlessRpc& rpc);
  int Close();
  int ProcessMsg(const IoInfo& ioinfo,const InMsg& msg);
  virtual int OnOpened(const IoInfo& ioinfo);
  virtual int OnClosed(const IoInfo& ioinfo);
  void set_iter(const TConnListIter& iter) {iter_ = iter; }
  const TConnListIter& get_iter() const { return iter_; }
  int SendRequest();
  int get_id() const { return id_; }
  static void OnTimer(int32_t id, void* arg);
 protected:
  void Reset(int err);
 public:
  void ClearRpc() {
    memset(&rpc_, 0, sizeof(rpc_));
    // expire_time_ = 0;
  }
 private:
  enum Status {
    kClosed = 0, 
    kConnecting = 1,
    kConnected = 2,
    kSending = 3, 
    kUninited = 4 
  };
 private:
  TokenlessService& owner_service_;
  int id_;
  uint64_t io_id_;
  // uint64_t expire_time_;
  TokenlessRpc rpc_; // 正在执行的请求.
  int status_;
  boost::mutex mutex_;
  TConnListIter iter_;
  int32_t timer_id_;
};
class DelayReq {
  public:
    typedef std::list< std::shared_ptr<DelayReq> >::iterator TReqListIter;
    typedef std::list< std::shared_ptr<DelayReq> > TReqList;
    DelayReq(TokenlessRpc& rpc, TokenlessService& owner_service)
      : rpc_(rpc),owner_service_(owner_service) {}
    void set_iter(const TReqListIter& iter) {iter_ = iter; }
    const TReqListIter& get_iter() const { return iter_; }
    void set_expire(uint64_t expire_time) {expire_time_ = expire_time; }
    void ClearRpc() {
      memset(&rpc_, 0, sizeof(rpc_));
    }
  public:
    TokenlessRpc rpc_;
    TokenlessService& owner_service_;
    uint64_t expire_time_;
    boost::mutex mutex_;
  private:
    TReqListIter iter_;
};
class TokenlessService: public RefCountImpl {
 public:
  friend class TokenlessConn;
  friend class DelayReq;
  typedef std::list< std::shared_ptr<TokenlessConn> > TConnList;
  typedef std::list< std::shared_ptr<DelayReq> > TReqList;
  typedef TConnList::iterator TConnListIter;
  typedef TConnList::const_iterator TConnListConstIter;
  typedef TReqList::iterator TReqListIter;

  static int get_obj_count() { return s_obj_count; }
  static void ExecDelayReqCallback(const blitz::RunContext& context,const blitz::TaskArgs& args);
  static void DelayReqArgsDelete(const blitz::TaskArgs& args) ;
  TokenlessService(Framework& framework, 
                  const std::string& addr,
                  MsgDecoderFactory* decoder_factory) 
    
    : framework_(framework),  addr_(addr) {
    id_ = -1;
    timeout_ = 1000;
    decoder_factory_ = decoder_factory;
    ++s_obj_count;
    inited_ = false;
    short_connect_ = false;
    idle_conn_count_ = 0;
    busy_conn_count_ = 0;
    next_conn_id_ = 0;
  }
  ~TokenlessService() {
    Uninit();
    --s_obj_count;
  }
  int Init();
  int Uninit();
  void set_id(int32_t id) { id_=id;}
  int32_t get_id() const  {return id_;}
  void set_timeout(int timeout) {assert(timeout>0);timeout_=timeout;}
  void set_short_connect(bool b) { short_connect_ = b; }
  int Query(OutMsg* out_msg, int log_id, uint64_t expire_time,
             const TokenlessCallback& callback);
  // 取没有超时的请求.
  int PopRequest(TokenlessRpc* rpc);
 private:
  std::shared_ptr<TokenlessConn> GetIdleConn();
  std::shared_ptr<DelayReq> GetDelayReq();
  void ReturnIdleConn(const TConnListIter& iter);
 private:
 private:
  Framework& framework_;
  std::string addr_;
  int32_t id_; // service id.
  int timeout_;
  boost::mutex mutex_;
  MsgDecoderFactory* decoder_factory_;
  TConnList idle_conns_;
  int idle_conn_count_;
  TConnList busy_conns_;
  TReqList delay_reqs_;
  int busy_conn_count_;
  int next_conn_id_;
  bool inited_;
  bool short_connect_;
  static int s_obj_count;
};
} // namespace blitz.
#endif // __BLITZ_TOKENLESS_SERVICE_H_
