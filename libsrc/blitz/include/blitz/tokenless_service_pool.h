// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_TOKENLESS_SERVICE_POOL_H_
#define __BLITZ_TOKENLESS_SERVICE_POOL_H_ 1
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include "msg.h"
#include "fd_manager.h"
#include "token_mgr.h"
#include "framework.h"
#include "rpc.h"
#include "tokenless_service.h"
#include "service_selector.h"
#include "ref_object.h"

namespace blitz {
class Framework;
class ServiceConfig {
 public:
   ServiceConfig() {
     timeout_ = 500;
     weight_ = 1;
     virtual_count_ = 1;
     hash_ = 0;
     short_connect_ = false;
   }
  void set_addr(const std::string&  addr) { addr_ = addr; }
  const std::string& get_addr() const { return addr_; }
  void set_pattern(const std::string&  pattern) { pattern_ = pattern; }
  const std::string& get_pattern() const { return pattern_; }
  int get_timeout() const { return timeout_; }
  void set_timeout(int timeout) { timeout_ = timeout; }
  int get_weight() const {return weight_;}
  void set_virtual_count(int count) {virtual_count_ = count;}
  int get_virtual_count() const { return virtual_count_;}
  bool is_short_connect()  const { return short_connect_; }
  void set_short_connect(bool b) { short_connect_ = b; }
  const PropertyMap& get_properties() const {return properties_;}
 private:
  std::string addr_;
  std::string pattern_;
  int timeout_;
  int weight_;
  int virtual_count_;
  uint32_t hash_;
  bool short_connect_;
  PropertyMap properties_;
};

/**
 * 自动重连功能,服务选择功能(一致性hash,取模).
 */
class TokenlessServicePool {
 public:
  TokenlessServicePool(Framework& framework,
	      MsgDecoderFactory* decoder_factory,
	      const std::shared_ptr<Selector>& selector);
  virtual ~TokenlessServicePool();
  int Init();
  int Uninit();
  // Service管理.
  int DivideKeysByService(const std::vector<std::string>& keys, std::map<int,std::vector<int> >& service_to_keyindex, std::map<int, std::vector<std::string> >& service_to_keys);
  int AddService(const ServiceConfig& service_config);
  int RemoveService(const std::string& addr);
  RefCountPtr<TokenlessService> GetService(const std::string& addr);
  // 异步函数, 不管成功或者失败，callback都会被调用.
  int Query(OutMsg* out_msg,
             const RpcCallback& callback,
             int log_id,
             const SelectKey* select_key,
             int timeout) {
    return RealQuery(out_msg, callback, log_id, select_key, timeout, NULL);
  }
 protected:
  TokenlessServicePool(const TokenlessServicePool&);
  TokenlessServicePool& operator=(const TokenlessServicePool&);
  void UpdateMark(int32_t service_id, int mark);
  int RealQuery(OutMsg* out_msg,
             const RpcCallback& callback,
             int log_id,
             const SelectKey* select_key,
             int timeout,
             void* extra);
  int RealQueryWithoutSelect(OutMsg* out_msg,
             const RpcCallback& callback,
             int log_id,
             int service_id,
             int timeout,
             void* extra);
  virtual void InvokeCallback(const RpcCallback& callback, int result,
                              const InMsg* msg, void* extra,
                              const IoInfo* ioinfo) {
    AVOID_UNUSED_VAR_WARN(extra);
    AVOID_UNUSED_VAR_WARN(ioinfo);
    CallRpc(callback, result, msg);
  }
  int AsyncCallback(RpcCallback* callback, void *extra);
  static void TimeoutCallback(const blitz::RunContext& context, const blitz::TaskArgs& args);
  static void TimeoutDelete(const blitz::TaskArgs& args);
  static void OnTimer(int32_t timer_id, void* arg);
  static void OnQueryCallback(int result,
                         const InMsg* msg,
                         int service_id,
                         OutMsg* out_msg,
                        int log_id,
                       uint64_t expire_time,
                       const TokenlessCallbackContext& ctx,
                       const IoInfo* ioinfo ) ;
 protected:
  typedef std::unordered_map<std::string, TokenlessService*> TAddrMap;
 protected:
  TAddrMap addr_map_;
  std::shared_ptr<Selector> selector_;
  Framework& framework_;
  MsgDecoderFactory* decoder_factory_;
  boost::mutex mutex_;
  int32_t failed_rpc_count_;
  bool inited_;
  int mark_while_fail_;
};
} // namespace blitz.
#endif // __BLITZ_SERVICE_POOL_H_
