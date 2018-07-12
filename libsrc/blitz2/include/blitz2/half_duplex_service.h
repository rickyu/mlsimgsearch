// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ2_HALF_DUPLEX_SERVICE_H_
#define __BLITZ2_HALF_DUPLEX_SERVICE_H_ 1
#include <list>
#include <queue>
#include <blitz2/logger.h>
#include <blitz2/reactor.h>
#include <blitz2/ioaddr.h>
#include <blitz2/time.h>
#include <blitz2/stream_socket_channel.h>
namespace blitz2 {
enum EQueryResult {
  kQueryResultSuccess = 0,
  kQueryResultTimeout = -1,
  kQueryResultInternal = -2,
  kQueryResultConnectClosed = -3,
  kQueryResultFailSelectService = -4,
  kQueryResultInvalidResp = -5
  
};
template<typename Msg>  
class QueryInfo {
 public:
  typedef Msg TMsg;
  typedef std::function<void(EQueryResult, TMsg*)> TQueryCallback;
  QueryInfo() {
    timerid_  = 0;
    expire_time_ = 0;
  }
  
  ~QueryInfo() {
    Clear();
  }
  void Set(OutMsg* outmsg, const TQueryCallback& callback) {
    outmsg_ = outmsg;
    callback_ = callback;
  }
  uint32_t get_timerid() const { return timerid_;}
  void set_timerid(uint32_t timerid)  { timerid_ = timerid;}
  uint64_t get_expire_time() const { return expire_time_;}
  void set_expire_time(uint64_t expire_time) { expire_time_ = expire_time;}
  bool IsValid() const { return outmsg_.get_pointer() != NULL; }
  void Clear() {
    outmsg_.Release();
    timerid_ = 0;
  }
  OutMsg* get_outmsg() {return outmsg_;}
  void ProcessMsg(EQueryResult result, TMsg* msg) {
    callback_(result, msg);
  }
  const TQueryCallback& get_callback() const { return callback_;}
 private:
  SharedPtr<OutMsg> outmsg_;
  TQueryCallback callback_;
  uint32_t timerid_;
  uint64_t expire_time_;
};

template<typename Decoder>
class HalfDuplexConnect;
template<typename Decoder>
class HalfDuplexService :public StreamSocketChannel<Decoder>::Observer {
 public:
  typedef Decoder TDecoder;
  typedef typename TDecoder::TInitParam TDecoderInitParam;
  typedef typename TDecoder::TMsg TMsg;
  typedef QueryInfo<TMsg> TQueryInfo;
  typedef typename TQueryInfo::TQueryCallback TQueryCallback;
  typedef HalfDuplexConnect<Decoder> TConnect;
  typedef std::list<TConnect*> TConnList;
  typedef HalfDuplexService<TDecoder> TMyself;
  
 public:
  HalfDuplexService(Reactor& reactor, const IoAddr& addr,
      const TDecoderInitParam& decoder_init_param)
    :reactor_(reactor), logger_(Logger::GetLogger("blitz2::HalfDuplexService"))
     , addr_(addr), decoder_init_param_(decoder_init_param) {
      max_conn_count_ = 100;
      short_connect_ = false;
    }
  virtual ~HalfDuplexService() {
    Uninit();
  }
  int Init() { return 0; }
  void Uninit() {
    if (!conns_.empty()) {
      for (auto it = conns_.begin(); it != conns_.end(); ++it) {
        delete (*it);
        (*it) = NULL;
      }
      conns_.clear();
    }
    idle_conns_.clear();
  }
  void Query(OutMsg* outmsg, const TQueryCallback& callback, uint32_t timeout);
  void QueryWithExpire(OutMsg* outmsg, const TQueryCallback& callback,
      uint64_t expire) ;
  bool& is_short_connect() { return short_connect_;}
  bool is_short_connect() const { return short_connect_;}
  void set_max_conn_count(int max_conn) {
    max_conn_count_ = max_conn;
  }
  void OnConnectClosed(TConnect* conn);
  void OnConnectOpened(TConnect* conn) {(void)(conn);}
  /**
   * when one connect finish a query, call this.
   * @param conn [IN] query finished on this connect.
   * @param result [IN] the result of query finished.
   */
  void OnConnectQueryFinished(TConnect* conn) ;
 protected:
  TConnect* GetIdleConn();
  void ReturnConn(TConnect* conn);
  void TryQuery(TConnect* conn);
  static void TaskCallback(const TQueryCallback& cb, EQueryResult result) {
    cb(result, NULL);
    
  }
 protected:
  Reactor& reactor_;
  Logger& logger_;
  IoAddr addr_;
  int32_t id_; // service id.
  std::queue<TQueryInfo> queries_;
  TConnList idle_conns_;
  int idle_conn_count_;
  std::vector<TConnect*> conns_;
  int max_conn_count_;
  bool short_connect_;
  TDecoderInitParam decoder_init_param_;
};


template<typename Decoder>
class HalfDuplexConnect: public StreamSocketChannel<Decoder>::Observer,
                         public StreamSocketChannel<Decoder> 
{
 public:
  typedef StreamSocketChannel<Decoder> TParent;
  typedef typename Decoder::TInitParam TDecoderInitParam;
  typedef typename Decoder::TMsg TMsg;
  typedef QueryInfo<typename Decoder::TMsg> TQueryInfo;
  typedef typename TQueryInfo::TQueryCallback TQueryCallback;
  typedef HalfDuplexConnect<Decoder> TMyself;
  HalfDuplexConnect(Reactor& reactor, uint32_t id, 
      HalfDuplexService<Decoder>* service,
      const TDecoderInitParam& decoder_param)
    :StreamSocketChannel<Decoder>(reactor, decoder_param) {
      id_ = id;
      service_ = service;
      SetObserver(this);
  }
  virtual ~HalfDuplexConnect() {}
  void Query(OutMsg* outmsg, const TQueryCallback& callback, uint32_t timeout);
  uint32_t id() const { return id_;}
  virtual void OnOpened(TParent* channel) {
    LOG_DEBUG(TParent::logger_, "opened this=%p id=%u channel=%p name=%s",
        this, id_, channel, TParent::get_name());
    return service_->OnConnectOpened(this);
  }
  virtual void OnMsgArrived(TParent* channel, TMsg* msg);
  virtual void OnClosed(TParent* channel) ;
  void OnTimeout(uint32_t id);
 private:
  uint32_t id_;
  HalfDuplexService<Decoder>* service_;
  TQueryInfo query_info_;
};

template<typename Decoder>
void HalfDuplexService<Decoder>::Query(OutMsg* outmsg,
    const TQueryCallback& callback,
    uint32_t timeout) {
  TConnect*  conn = GetIdleConn();
  if (!conn) {
    // we don't have idle connect, so push query to queue.
    TQueryInfo q;
    q.Set(outmsg, callback);
    q.set_expire_time(time::GetTickMs() + timeout);
    queries_.push(q);
    return;
  }
  conn->Query(outmsg, callback, timeout);
}
template<typename Decoder>
void HalfDuplexService<Decoder>::QueryWithExpire(OutMsg* outmsg,
    const TQueryCallback& callback,
      uint64_t expire) {
    uint64_t now = time::GetTickMs();
    if (now < expire) {
      return Query(outmsg, callback, expire-now);
    } else {
      reactor_.PostTask(std::bind(TaskCallback, callback, kQueryResultTimeout));
    }
  }

template<typename Decoder>
inline void HalfDuplexService<Decoder>::OnConnectClosed(TConnect* conn) {
  TryQuery(conn);
}

template<typename Decoder>
inline void HalfDuplexService<Decoder>::OnConnectQueryFinished(TConnect* conn)  {
  if(short_connect_) {
    conn->Close(false); 
  } 
  TryQuery(conn);
}
template<typename Decoder>
typename HalfDuplexService<Decoder>::TConnect* 
HalfDuplexService<Decoder>::GetIdleConn() {
  TConnect* conn = NULL;
  if(idle_conns_.empty()) {
    if(conns_.size() < (size_t)max_conn_count_) {
      // we create new connect.
      uint32_t id = static_cast<uint32_t>(conns_.size()) + 1;
      conn = new (std::nothrow) TConnect(reactor_, id, this,
          decoder_init_param_);
      conns_.push_back(conn);
      conn->Connect(addr_);
    }
  } else {
    --idle_conn_count_;
    conn = idle_conns_.front();
    idle_conns_.pop_front();
    conn->Connect();
  }
  return conn;
}

template<typename Decoder>
void HalfDuplexService<Decoder>::ReturnConn(TConnect* conn) {
  idle_conns_.push_back(conn);
  ++idle_conn_count_;
}
template<typename Decoder>
void HalfDuplexService<Decoder>::TryQuery(TConnect* conn) {
  uint64_t cur_time = time::GetTickMs();
  bool query_issued = false;
  while(!queries_.empty()) {
    // if we have query in queue, we start the first query.
    TQueryInfo& query = queries_.front();
    if(query.get_expire_time() <= cur_time) {
      // timeout
      LOG_ERROR(logger_, "queuedQueryTimeout logid=%u:%u",
          query.get_outmsg()->logid().id1, query.get_outmsg()->logid().id2);
      query.ProcessMsg(kQueryResultTimeout, NULL);
      queries_.pop();
      continue;
    }
    conn->Connect();
    conn->Query(query.get_outmsg(), query.get_callback(), 
        query.get_expire_time()-cur_time);
    query_issued = true;
    queries_.pop();
    break;
  } 
  if(!query_issued) {
    ReturnConn(conn);
  }
}
template<typename Decoder>
void HalfDuplexConnect<Decoder>::Query(OutMsg* outmsg, 
    const TQueryCallback& callback, uint32_t timeout) {
  if (query_info_.IsValid()) {
    LogId& logid = query_info_.get_outmsg()->logid();
    LOG_FATAL(TParent::logger_, "issuedQueryOnBusyConn this=%p parent=%p"
        " id=%u name=%s logid=%u:%u timerid=%u ",
      this, (TParent*)this, id_, TParent::get_name(),
      logid.id1, logid.id2, query_info_.get_timerid());
    abort();
    return ;
  }
  uint32_t timerid = TParent::reactor_.AddTimer(timeout,
      std::bind(&TMyself::OnTimeout, this, std::placeholders::_1),
      false);
  if (timerid==0) {
    LOG_ERROR(TParent::logger_, "failed AddTimer");
    abort();
    return;
  }
  query_info_.Set(outmsg, callback);
  query_info_.set_timerid(timerid);
  LOG_DEBUG(TParent::logger_,
      "query this=%p parent=%p id=%u logid=%u:%u timerid=%u name=%s timeout=%u",
       this, (TParent*)this, id_, outmsg->logid().id1, outmsg->logid().id2,
       timerid, TParent::get_name(), timeout);

  TParent::SendMsg(outmsg);
}
template<typename Decoder>
void HalfDuplexConnect<Decoder>::OnMsgArrived(TParent* channel, TMsg* msg) {
  // if OnTimeout is called, the OnMsgArrived won't be called, 
  // so query_info_ must be valid.
  LogId& logid = query_info_.get_outmsg()->logid();
  LOG_DEBUG(TParent::logger_, "gotMsg this=%p parent=%p id=%u channel=%p"
     " name=%s logid=%u:%u timerid=%u ",
      this, (TParent*)this, id_, channel, TParent::get_name(),
      logid.id1, logid.id2, query_info_.get_timerid());
  assert(query_info_.get_timerid() > 0);
  TParent::reactor_.RemoveTimer(query_info_.get_timerid());
  query_info_.ProcessMsg(kQueryResultSuccess, msg);
  query_info_.Clear();
  return service_->OnConnectQueryFinished(this);
}
template<typename Decoder>
void HalfDuplexConnect<Decoder>::OnClosed(TParent* channel) {
  OutMsg* outmsg = query_info_.get_outmsg();
  if (outmsg) {
    LogId& logid = outmsg->logid();
    LOG_ERROR(TParent::logger_, "closed this=%p parent=%p id=%u"
      " logid=%u:%u query_timerid=%u name=%s",
      this, (TParent*)this, id_, logid.id1, logid.id2,
      query_info_.get_timerid(), TParent::get_name());
    query_info_.ProcessMsg(kQueryResultConnectClosed, NULL);
    TParent::reactor_.RemoveTimer(query_info_.get_timerid());
    query_info_.Clear();
    return service_->OnConnectClosed(this);
  } else {
    LOG_DEBUG(TParent::logger_, "idleConnectClosed this=%p parent=%p id=%u"
        " channel=%p",
        this, (TParent*)this,  id_, channel);
  }
}
template<typename Decoder>
void HalfDuplexConnect<Decoder>::OnTimeout(uint32_t id) {
  OutMsg* outmsg = query_info_.get_outmsg();
  assert(id==query_info_.get_timerid());
  // {
  //   LOG_FATAL(TParent::logger_, "timeout timer=%u this=%p id=%u"
  //     " logid=%u:%u query_timerid=%u name=%s",
  //     id, this, id_, 
  //     outmsg->logid().id1, outmsg->logid().id2,
  //     query_info_.get_timerid(), TParent::get_name());
  //   return;
  // }
  LOG_ERROR(TParent::logger_, "timeout timer=%u this=%p parent=%p id=%u"
      " logid=%u:%u query_timerid=%u name=%s",
      id, this, (TParent*)this, id_, 
      outmsg->logid().id1, outmsg->logid().id2,
      query_info_.get_timerid(), TParent::get_name());
  query_info_.ProcessMsg(kQueryResultTimeout, NULL);
  query_info_.Clear();
  TParent::Close(false);
  return service_->OnConnectQueryFinished(this);
}


} // namespace blitz2.
#endif // __BLITZ2_HALF_DUPLEX_SERVICE_H_
