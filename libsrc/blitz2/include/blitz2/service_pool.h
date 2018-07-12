// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// ServicePool. 
#ifndef __BLITZ2_SERVICE_POOL_H_
#define __BLITZ2_SERVICE_POOL_H_ 1
#include <string>
#include <blitz2/half_duplex_service.h>
#include <blitz2/configure.h>
#include <blitz2/time.h>
namespace blitz2 {
class ServiceConfig {
 public:
   ServiceConfig() {
     timeout_ = 500;
     weight_ = 1;
     hash_ = 0;
     short_connect_ = false;
   }
  void set_addr(const std::string&  addr) { addr_ = addr; }
  const std::string& addr() const { return addr_; }
  int timeout() const { return timeout_; }
  void set_timeout(int timeout) { timeout_ = timeout; }
  int weight() const {return weight_;}
  bool is_short_connect()  const { return short_connect_; }
  void set_short_connect(bool b) { short_connect_ = b; }
  const Configure& conf() const {return conf_;}
 private:
  std::string addr_;
  int timeout_;
  int weight_;
  uint32_t hash_;
  bool short_connect_;
  Configure conf_;
};

/**
 * template parameter Selector is also a template class 
 */
template<typename SelectKey, template<typename Key, typename Item> class Selector, typename Decoder>
class ServicePool {
 public:
  typedef Decoder TDecoder;
  typedef typename TDecoder::TInitParam TDecoderInitParam;
  typedef HalfDuplexService<Decoder> TService;
  typedef Selector<SelectKey, TService*> TSelector;
  typedef ServicePool<SelectKey,Selector,Decoder> TMyself;
  typedef typename TSelector::TKey TSelectKey;
  typedef typename Decoder::TMsg TMsg;
  typedef typename TService::TQueryCallback TQueryCallback;
  typedef std::unordered_map<std::string, TService*> TAddrMap;
  ServicePool(Reactor& reactor, const TDecoderInitParam& decoder_init_param)
    :reactor_(reactor), logger_(Logger::GetLogger("blitz2::ServicePool")),
    decoder_init_param_(decoder_init_param) {
  }
  virtual ~ServicePool() {
    Uninit();
  }
  void Uninit();
  int AddService(const char* addr) {
    ServiceConfig cfg;
    cfg.set_addr(addr);
    return AddService(cfg);
  }
  int AddService(const ServiceConfig& service_config);
  void Query(OutMsg* outmsg, const TQueryCallback& callback,
            const TSelectKey& key, uint32_t timeout, int retry_count);
  void QueryByService(int service_id, OutMsg* outmsg,
      const TQueryCallback& callback, uint32_t timeout, int retry_count);

  
  /**
   * this method for multi key query. 
   *
   * for multi keys query, we first select service, then issue query.
   * for example:
   * int uids[] = {1, 3, 5, 7};
   * BatchSelectResult select_result;
   * pool.SelectBatch(uids, 4, &select_result);
   * for(size_t i=0; i<select_result.keys_selected.size(); ++i) {
   *   OutMsg* outmsg = CreateOutmsg(select_result.keys_selected[i].keys);
   *   pool.Query2(outmsg, callback, 1000, select_result.keys_selected[i].items);
   * }
   */
  void Query2(OutMsg* outmsg, const TQueryCallback& callback, uint32_t timeout,
      const std::vector<int>& try_services);
  const std::vector<int>& GetAllServiceId() const { return service_ids_; }
  void CancelAllQueries();
  int GetQueuedQueryCount();
 protected:

  static void ErrorQueryCallback(const TQueryCallback& callback,
                                EQueryResult result) {
    callback(result, NULL);
  }
  void MyQueryCallback(const TQueryCallback& callback, 
      OutMsg* outmsg, 
      const TSelectKey& key, 
      uint64_t expire_time, 
      int retry_count, 
      EQueryResult result, 
      TMsg* msg) ;
  void MyQueryByServiceCallback(const TQueryCallback& callback, 
      OutMsg* outmsg, 
      int service_id,
      uint64_t expire_time, 
      int retry_count, 
      EQueryResult result, 
      TMsg* msg) ;

 protected:
  Reactor& reactor_;
  Logger& logger_;
  TSelector selector_;
  TAddrMap addr_map_;
  TDecoderInitParam decoder_init_param_;
  std::vector<int> service_ids_;
};

template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
int ServicePool<SelectKey, Selector,Decoder>::AddService(
                            const ServiceConfig& service_config) {
  if (service_config.addr().empty()) {
    return -1;
  }
  auto iter =  addr_map_.find(service_config.addr());
  if (iter != addr_map_.end()) {
    return 1;
  }
  IoAddr addr;
  addr.Parse(service_config.addr());
  TService* service = new TService(reactor_, addr, decoder_init_param_);
  if (!service) { abort(); }
  service->Init();
  service->is_short_connect() = service_config.is_short_connect();
  auto insert_result = addr_map_.insert(
        std::make_pair(service_config.addr(), service));
  if (!insert_result.second) { 
    delete service;
    service = NULL;
    return -1;
  }

  int32_t service_id  = selector_.AddItem(service, service_config.weight(),
      service_config.conf());

  if (service_id < 0) {
    addr_map_.erase(insert_result.first);
    delete service;
    service = NULL;
    return -1;
  }
  service_ids_.push_back(service_id);
  return 0;
}
  

template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
void ServicePool<SelectKey, Selector,Decoder>::Query(OutMsg* outmsg,
    const TQueryCallback& callback,
    const TSelectKey& select_key, uint32_t timeout, int retry_count) {
  using namespace std;
  OneSelectResult<TService*> selected;
  selector_.Select(select_key, &selected);
  if(!selected.item) {
    reactor_.PostTask(
        bind(ErrorQueryCallback, callback, kQueryResultFailSelectService));
    return;
  }
  TService* service = selected.item;
  uint64_t expire_time = time::GetTickMs() + timeout;
  outmsg->AddRef();
  service->Query(outmsg, 
      bind(&TMyself::MyQueryCallback, this,callback,  outmsg, select_key,
        expire_time, retry_count, placeholders::_1, placeholders::_2),
      timeout);
}
template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
void ServicePool<SelectKey, Selector,Decoder>::QueryByService( 
                                          int service_id,
                                          OutMsg* outmsg,
                                          const TQueryCallback& callback,
                                          uint32_t timeout, 
                                          int retry_count) {
  using namespace std;
  typename TSelector::TOneResult selected;
  selector_.SelectById(service_id, &selected);
  if(!selected.item) {
    reactor_.PostTask(
        bind(ErrorQueryCallback, callback, kQueryResultFailSelectService));
    return;
  }
  TService* service = selected.item;
  uint64_t expire_time = time::GetTickMs() + timeout;
  outmsg->AddRef();
  service->Query(outmsg, 
      bind(&TMyself::MyQueryByServiceCallback, this, callback, outmsg,
        service_id, expire_time, retry_count,
        placeholders::_1, placeholders::_2),
      timeout);
}
template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
void ServicePool<SelectKey, Selector,Decoder>::Uninit() {
  for ( auto it = addr_map_.begin(); it != addr_map_.end(); ++it ) {
    delete it->second;
  }
  addr_map_.clear();
  addr_map_.clear();
  service_ids_.clear();
  //selector_.Uninit();
}

template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
void ServicePool<SelectKey,Selector, Decoder>::MyQueryCallback(
    const TQueryCallback& callback, 
      OutMsg* outmsg, 
      const TSelectKey& key,
      uint64_t expire_time, 
      int retry_count, 
      EQueryResult result, 
      TMsg* msg) {
  if(result == kQueryResultSuccess) {
    outmsg->Release();
    callback(result, msg);
  } else {
    uint64_t cur_time = time::GetTickMs();
    if(expire_time > cur_time && retry_count > 0 ) {
      /**
       * if select key is a char*, it is invalid here.
       * so we must have methods to copy a key. do not use const char* as key.
       * use string.
       */
      LOG_ERROR(logger_, "query fail, retry");
      // // the query didn't expire, so we try to issue new query.
      Query(outmsg, callback, key, expire_time-cur_time,
           --retry_count);
      outmsg->Release();
    } else {
      outmsg->Release();
      callback(result, msg);
    }
  }
}
template<typename SelectKey, template<typename T, typename T2> class Selector, typename Decoder>
void ServicePool<SelectKey,Selector, Decoder>::MyQueryByServiceCallback(
     const TQueryCallback& callback, 
      OutMsg* outmsg, 
      int service_id,
      uint64_t expire_time, 
      int retry_count, 
      EQueryResult result, 
      TMsg* msg)  {
  if(result == kQueryResultSuccess) {
    outmsg->Release();
    callback(result, msg);
  } else {
    uint64_t cur_time = time::GetTickMs();
    if(expire_time > cur_time && retry_count > 0 ) {
      /**
       * if select key is a char*, it is invalid here.
       * so we must have methods to copy a key. do not use const char* as key.
       * use string.
       */
      LOG_ERROR(logger_, "query fail, retry");
      // // the query didn't expire, so we try to issue new query.
      QueryByService(service_id, outmsg, callback, expire_time-cur_time,
           --retry_count);
      outmsg->Release();
    } else {
      outmsg->Release();
      callback(result, msg);
    }
  }

}

} // namespace blitz2.
#endif // __BLITZ2_SERVICE_POOL_H_ 
