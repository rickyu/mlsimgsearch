#include <set>
#include "utils/time_util.h"
#include "utils/logger.h"
// #include "service_pool.h"
#include "blitz/tcp_connect.h"
#include "blitz/framework.h"
// #include "service.h"
#include "blitz/tokenless_service_pool.h"

namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("framework");

TokenlessServicePool::TokenlessServicePool(Framework& framework,
			 MsgDecoderFactory* decoder_factory,
			 const std::shared_ptr<Selector>& selector)
  :framework_(framework) {
  decoder_factory_ = decoder_factory;
  selector_ = selector;
  failed_rpc_count_ = 0;
  inited_ = false;
  mark_while_fail_ = -10;
}
TokenlessServicePool::~TokenlessServicePool() {
  Uninit();
}
int TokenlessServicePool::Init() {
  if (!selector_)  {
    return -1;
  }
  int32_t timer_id = framework_.SetTimer(5000, OnTimer, this);
  if (timer_id > 0 ) {
    inited_ = true;
    return 0;
  }
  return -2;
}
int TokenlessServicePool::Uninit() {
  boost::mutex::scoped_lock guard(mutex_);
  TAddrMap::iterator iter =  addr_map_.begin();
  TAddrMap::iterator end =  addr_map_.end();
  for (; iter!=end; ++iter) {
    selector_->RemoveItem(iter->second->get_id());
    iter->second->Uninit();
    iter->second->Release();
  }
  addr_map_.clear();
  inited_ = false;
  return 0;
}

int TokenlessServicePool::DivideKeysByService(const std::vector<std::string>& keys, std::map<int, std::vector<int> >& service_to_keyindex, std::map<int, std::vector<std::string> >& service_to_keys) {
  std::vector<std::string>::const_iterator iter = keys.begin();
  for (int i = 0; iter != keys.end(); iter ++, i ++) {
    SelectKey select_key;
    int service_id = -1;
    select_key.main_key = iter->c_str();
    select_key.sub_key = 0;
    RefCountPtr<TokenlessService> service; 
    {
      boost::mutex::scoped_lock guard(mutex_);
      SelectNode selected;
      if (selector_->Select(select_key, &selected) == 0) {
        service = reinterpret_cast<TokenlessService*>(selected.get_data());
        if (service) {
          service_id = service->get_id();
        } else {
          LOGGER_ERROR(g_log_framework, "divide key can't find service,key=%s",
               iter->c_str());
          return -1;
        }
      }
    }
    if (service_to_keys.find(service_id) != service_to_keys.end()) {
      service_to_keys[service_id].push_back(*iter);
      service_to_keyindex[service_id].push_back(i);
    } else {
      service_to_keys.insert(std::pair<int, std::vector<std::string> >(service_id, std::vector<std::string>(1, *iter)));
      service_to_keyindex.insert(std::pair<int, std::vector<int> >(service_id, std::vector<int>(1, i)));
    }
  }
  return 0;
}


int TokenlessServicePool::AddService(const ServiceConfig& service_config) {
  if (!selector_ || !decoder_factory_) {
    return -1;
  }
  if (service_config.get_addr().empty()) {
    return -2;
  }
  boost::mutex::scoped_lock guard(mutex_);
  TAddrMap::iterator iter =  addr_map_.find(service_config.get_addr());
  if (iter != addr_map_.end()) {
    // 当前如果service已经存在，则忽略.
    // TODO(YuZuo): 可能需要刷新配置项，也就是先删除，再重新加入.
    LOGGER_NOTICE(g_log_framework, "ServiceExist addr=%s",
                 service_config.get_addr().c_str());
    return 1;
  }
  RefCountPtr<TokenlessService> service;
  service.Attach(new TokenlessService(framework_, service_config.get_addr(),
                 decoder_factory_));
  if (!service) { abort(); }
  service->set_timeout(service_config.get_timeout());
  service->set_short_connect(service_config.is_short_connect());
  int32_t service_id = 0;
  int ret = selector_->AddItem(service.get_pointer(), service_config.get_weight(),
      service_config.get_properties(), service_config.get_pattern(), service_config.get_addr(), service_config.get_virtual_count(), &service_id);

  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, "Selector::AddItem addr=%s ret=%d",
                  service_config.get_addr().c_str(), ret);
    return -1;
  }
  service->set_id(service_id);
  std::pair<TAddrMap::iterator, bool> insert_result
    = addr_map_.insert(
        std::make_pair(service_config.get_addr(), service.get_pointer()));
  if (!insert_result.second) { 
    LOGGER_ERROR(g_log_framework, "map::insert addr=%s",
                  service_config.get_addr().c_str());
    selector_->RemoveItem(service_id);
    return -1;
  }
  service->Init();
  service.Detach();
  LOGGER_DEBUG(g_log_framework, "AddService addr=%s",
                service_config.get_addr().c_str());
  return 0;
}
  
int TokenlessServicePool::RemoveService(const std::string& addr) {
  TokenlessService* service = NULL;
  {
    boost::mutex::scoped_lock guard(mutex_);
    TAddrMap::iterator iter =  addr_map_.find(addr);
    if (iter == addr_map_.end()) {
      // 没找到.
      return 1;
    }
    service = iter->second;
    selector_->RemoveItem(service->get_id());
    addr_map_.erase(iter);
  }
  if (service) {
     service->Uninit();
    service->Release();
  }
  return 0;
}
        
RefCountPtr<TokenlessService> TokenlessServicePool::GetService(
                                         const std::string& addr) {
  boost::mutex::scoped_lock guard(mutex_);
  TAddrMap::iterator iter =  addr_map_.find(addr);
  if (iter == addr_map_.end()) {
    // 没找到.
    return RefCountPtr<TokenlessService>();
  }
  return RefCountPtr<TokenlessService>(iter->second);
}

void TokenlessServicePool::TimeoutCallback(const blitz::RunContext& context, const blitz::TaskArgs& args) {
  reinterpret_cast<TokenlessServicePool*>(args.arg3.ptr)->InvokeCallback(*reinterpret_cast<RpcCallback*>(args.arg1.ptr), -111, NULL, args.arg2.ptr, NULL);
  delete reinterpret_cast<RpcCallback*>(args.arg1.ptr);
}

void TokenlessServicePool::TimeoutDelete(const blitz::TaskArgs& args) {
  delete reinterpret_cast<RpcCallback*>(args.arg1.ptr);
}

int TokenlessServicePool::AsyncCallback(RpcCallback* callback, void *extra) {
  blitz::Task task;
  blitz::TaskUtil::ZeroTask(&task);
  task.fn_callback = TimeoutCallback;
  task.fn_delete = TimeoutDelete;
  task.args.arg1.ptr = (void *)(callback);
  task.args.arg2.ptr = extra;
  task.args.arg3.ptr = (void *)this;
  framework_.PostTask(task);
  return 0;
}
int TokenlessServicePool::RealQuery(
             OutMsg* out_msg,
             const RpcCallback& callback,
             int log_id,
             const SelectKey* select_key,
             int timeout, 
             void* extra) {
  // 异步函数内绝对不能调用回调.
  assert(timeout>=0);
  if (!out_msg) {
    LOGGER_ERROR(g_log_framework, " out msg  is gone");
    return -1;
  }
  RefCountPtr<TokenlessService> service; 
  {
    // 注意锁的范围，调用Query和CallRpc前都释放锁.
    boost::mutex::scoped_lock guard(mutex_);
    SelectNode selected;
    if (selector_->Select(*select_key, &selected) == 0) {
      service = reinterpret_cast<TokenlessService*>(selected.get_data());
      if (service) {
        LOGGER_INFO(g_log_framework, "selected sid=%d mark=%d",
                    service->get_id(), selected.get_mark());
      }
    }
  }
  if (service) {
    TokenlessCallback my_callback;
    my_callback.context.try_count = 1;
    my_callback.context.pool = this;
    my_callback.context.rpc = callback;
    my_callback.context.extra_ptr = extra;
    my_callback.fn_action = OnQueryCallback;
    if (timeout <= 0) {
      timeout = 1000;
    }
    uint64_t expire_time = TimeUtil::GetTickMs() + timeout;
    out_msg->AddRef();
    if (service->Query(out_msg, log_id, expire_time,  my_callback) != 0)  {
      // 可以考虑重试.
      RpcCallback* rpc_callback = new RpcCallback();
      *rpc_callback = callback;
      AsyncCallback(rpc_callback, extra);
      out_msg->Release();
      return -3;
    }
  } else {
    // 没有可用服务.
    // InvokeCallback(callback, -2, NULL, extra);
    LOGGER_ERROR(g_log_framework, " no service available");
    return -2;
  }
  return 0;
}

int TokenlessServicePool::RealQueryWithoutSelect(
             OutMsg* out_msg,
             const RpcCallback& callback,
             int log_id,
             int service_id,
             int timeout, 
             void* extra) {
  // 异步函数内绝对不能调用回调.
  assert(timeout>=0);
  if (!out_msg) {
    LOGGER_ERROR(g_log_framework, " out msg  is gone");
    return -1;
  }
  RefCountPtr<TokenlessService> service; 
  {
    // 注意锁的范围，调用Query和CallRpc前都释放锁.
    boost::mutex::scoped_lock guard(mutex_);
    SelectNode* selected = selector_->GetItem(service_id);
    if (selected != NULL) {
      service = reinterpret_cast<TokenlessService*>(selected->get_data());
      if (service) {
        LOGGER_INFO(g_log_framework, "selected sid=%d mark=%d",
                    service->get_id(), selected->get_mark());
      }
    }
  }
  if (service) {
    TokenlessCallback my_callback;
    my_callback.context.try_count = 1;
    my_callback.context.pool = this;
    my_callback.context.rpc = callback;
    my_callback.context.extra_ptr = extra;
    my_callback.fn_action = OnQueryCallback;
    if (timeout <= 0) {
      timeout = 1000;
    }
    uint64_t expire_time = TimeUtil::GetTickMs() + timeout;
    out_msg->AddRef();
    if (service->Query(out_msg, log_id, expire_time,  my_callback) != 0)  {
      // 可以考虑重试.
      RpcCallback* rpc_callback = new RpcCallback();
      *rpc_callback = callback;
      AsyncCallback(rpc_callback, extra);
      out_msg->Release();
      return -3;
    }
  } else {
    // 没有可用服务.
    // InvokeCallback(callback, -2, NULL, extra);
    LOGGER_ERROR(g_log_framework, " no service available");
    return -2;
  }
  return 0;
}

void TokenlessServicePool::UpdateMark(int32_t service_id, int mark) {
  boost::mutex::scoped_lock guard(mutex_);
  selector_->UpdateMark(service_id, mark);
}
void TokenlessServicePool::OnTimer(int32_t id, void* arg){
  AVOID_UNUSED_VAR_WARN(id);
  // 定时刷新所有服务的mark.
  TokenlessServicePool* pool = (TokenlessServicePool*)arg;
  LOGGER_INFO(g_log_framework, "ServicePool UpdateAllMark inited=%s",
            pool->inited_ ? "true" : "false");
  boost::mutex::scoped_lock guard(pool->mutex_);
  if (pool->inited_) {
    pool->selector_->UpdateAllMark(5);
    pool->framework_.SetTimer(5000, OnTimer, pool);
  }
}
void TokenlessServicePool::OnQueryCallback(int result,
                         const InMsg* msg,
                         int service_id,
                         OutMsg* out_msg,
                         int log_id,
                         uint64_t expire_time,
                         const TokenlessCallbackContext& ctx,
                         const IoInfo* ioinfo )  {
  uint64_t time1 = TimeUtil::GetTickMs();
  if (result != 0) {
    // 给service_id降级.
    LOGGER_NOTICE(g_log_framework,
                  "QueryFail minusMark result=%d sid=%d logid=%d msgid=%d",
                  result, service_id, log_id, out_msg->get_msgid());
    ctx.pool->UpdateMark(service_id, ctx.pool->mark_while_fail_);
    uint64_t cur_time = TimeUtil::GetTickMs();
    bool retry = false;
    if (!retry ||  cur_time >= expire_time) {
      // 不重试或者超时
      out_msg->Release();
      ctx.pool->InvokeCallback(ctx.rpc, result, NULL, ctx.extra_ptr, ioinfo);
      //ctx.pool->InvokeCallback(ctx.rpc, result, NULL, ctx.extra_ptr);
      return;
    }
    //目前不支持重试, 重试的话需要将select_key也保存起来.
  } else {
    uint64_t time2 = TimeUtil::GetTickMs();
    out_msg->Release();
    uint64_t time3 = TimeUtil::GetTickMs();
    ctx.pool->InvokeCallback(ctx.rpc, result, msg, ctx.extra_ptr, ioinfo);
    LOGGER_DEBUG(g_log_framework, "OnQueryCallback end, time_cost1=%d, time_cost2=%d, time_cost3=%d, ioid=0x%lx", time2-time1, time3-time2,TimeUtil::GetTickMs()-time3,ioinfo->get_id()); 
    //ctx.pool->InvokeCallback(ctx.rpc, result, msg, ctx.extra_ptr);
  }
}
} // namespace blitz.
