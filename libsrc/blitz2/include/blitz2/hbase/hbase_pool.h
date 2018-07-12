// Copyright 2012 meilishuo Inc.
// 访问Hbase相关的类和函数，包括
// HbasePool - Hbase连接池
// HbaseEventFire - 状态机Event包装，访问HbasePool得到应答后回调状态机

#ifndef __BLITZ2_HBASE_POOL_H_
#define __BLITZ2_HBASE_POOL_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/zookeeper.h>
#include <transport/TBufferTransports.h>
#include <blitz2/thrift.h>
#include <blitz2/hbase/thrift_gen/Hbase_constants.h>
#include <blitz2/hbase/thrift_gen/Hbase_types.h>
#include <blitz2/hbase/thrift_gen/Hbase.h>
#include <blitz2/state_machine.h>
#include <blitz2/outmsg.h>
#include <blitz2/blitz2.h>
#include <blitz2/configure.h>

namespace blitz2
{
using std::function;
using std::string;
using std::vector;
using std::map;
using std::unordered_map;
using boost::shared_ptr;
using ::apache::thrift::protocol::TBinaryProtocol;
using ::apache::thrift::protocol::TProtocol;
using ::apache::thrift::transport::TTransport;
using ::apache::thrift::transport::TFramedTransport;
using namespace apache::hadoop::hbase::thrift;

typedef SharedObject<BytesMsg> ThriftMsg;
typedef function<void(EQueryResult, vector<TCell>*)> HbaseGetCallback;
typedef function<void(EQueryResult)> HbaseSetCallback;
typedef HalfDuplexService<ThriftFramedMsgDecoder> RegionServer;
class Hbase;
class RegionServerInfo;
class TableRegions;
typedef unordered_map<string, shared_ptr<TableRegions>> TableMap;
typedef Hbase HbasePool;
class Hbase {
 public:
  friend struct Func;
  Hbase(Reactor& reactor);
  ~Hbase();
  int Init(const Configure& conf);
  // zookeeper watcher
  static void ZookeeperWatcher(zhandle_t* zh, int type,
    int state, const char* path, void* watcher_ctx);
  // Get one row.
  void Get(const Text& table,
           const Text& row,
           const Text& column,
           const HbaseGetCallback& callback,
           const LogId& logid,
           uint32_t timeout,
           int retry_count);
  // Set one row.
  void Set(const Text& table, 
           const Text& row, 
           const std::vector<Mutation> & mutations,
           const HbaseSetCallback& callback,
           const LogId& logid,
           uint32_t timeout,
           int retry_count);
 private:
  // auxiliary method for Get.
  void GetInvoke(const Text& table,
            const Text& row,
            SharedPtr<OutMsg> outmsg,
            const HbaseGetCallback& callback,
            uint64_t expire,
            int retry_count);
  void SetInvoke(const Text& table,
            const Text& row,
            SharedPtr<OutMsg> outmsg,
            const HbaseSetCallback& callback,
            uint64_t expire,
            int retry_count);

  void GetSendReq(RegionServer* server,
                SharedPtr<OutMsg> outmsg,
                const HbaseGetCallback& callback,
                const Text& table,
                const Text& row,
                uint64_t expire_time,
                int retry_count);

   void SetSendReq(RegionServer* server,
                SharedPtr<OutMsg> outmsg,
                const HbaseSetCallback& callback,
                const Text& table,
                const Text& row,
                uint64_t expire_time,
                int retry_count);
  // 处理从region server过来的get 应答.
  void RegionServerOnGet(EQueryResult result, 
                         ThriftMsg* msg,
                         SharedPtr<OutMsg> outmsg,
                        uint64_t expire,
                        int retry_count,
                         const HbaseGetCallback& cb,
                         const Text& table,
                         const Text& row);
  // 处理从region server过来的set 应答.
  void RegionServerOnSet(EQueryResult result, 
                         ThriftMsg* msg,
                         SharedPtr<OutMsg> outmsg,
                        uint64_t expire,
                        int retry_count,
                         const HbaseSetCallback& cb,
                         const Text& table,
                         const Text& row);

  void LoadRegionInfo(const string& table, const string& row, 
      const function<void(RegionServer*)>& cb, uint64_t expire);
  // 请求zookeeper获取root server.
  void ZookeeperGetRootServer(const function<void()>& cb);
  // 请求zookeeper获取region servers.
  void ZookeeperGetRegionServers(const function<void()>& cb);
  static void ZookeeperGetRootServerCompletion(int rc, const char* value,
      int value_len, const struct Stat* stat, const void* data);
  static void ZookeeperGetRegionServersCompletion(int rc, 
    const struct String_vector *strings, const void* data);
  void RootGetRegionInfo(const string& table, const string& row, 
      const function<void(RegionServer*)>& cb,
      uint64_t expire_time);
  void RootOnGetRegionInfo(EQueryResult result, ThriftMsg* msg,
                          const function<void(RegionServer*)>& cb,
                          const string& table, const string& row,
                          uint64_t expire_time);
  RegionServer* FindRegionServer(const string& table, const string& row );
  RegionServer* FindRegionServerByHost(const Text& host) {
    if (region_servers_.find(host) != region_servers_.end()) {
      //return region_servers_[host];
      return region_servers_[host];
    }
    return NULL;
  }
  // create OutMsg from transport.
  SharedPtr<OutMsg> CreateOutMsg(shared_ptr<ThriftMemoryOutTransport> trans);
 private:
  Logger& logger_;
  Reactor& reactor_;
  ZookeeperPool* zookeeper_pool_;
  RegionServer* root_server_;
  map<string, RegionServer*> region_servers_;
  TableMap regions_;
  uint32_t num_regions_;
  int32_t thrift_port_;
  char* hbase_rootserver_; 
  char* hbase_regionservers_;
};
class RegionServerInfo {
 public:
  RegionServerInfo(const TRegionInfo& r, int region_num) :
      start_row(r.startKey), end_row(r.endKey), host_valid(true),
      host(r.serverName), port(r.port), region_num(region_num)
    { }

  const string start_row;
  const string end_row;
  const bool host_valid;
  const string host;
  const int port;
  const int region_num;
 private:
};
// map: first_row -> regionserver
class TableRegions {
 public:
  /**
   * Add the region to map. If the region exist in the map, just return the
   * exist data
   * @param region for creating the RegionServerInfo
   * @param num_regions current num of region in current TableMap
   * @returns RegionServerInfo either created or in the map
   */
  shared_ptr<const RegionServerInfo> AddRegionServerInfo(
    const TRegionInfo& region,
    uint32_t* num_regions);

  /**
   * Find this key in the map
   * @param  key key to find the RegionServerInfo
   * @returns RegionServerInfo, otherwise NULL if not found
   */
  shared_ptr<const RegionServerInfo> FindRegionServerInfoInCache(
    const string& key) const;

  /**
   * Remove the region server info for the specified start key.
   */
  void RemoveRegionServerInfo(const string& start_key);

 private:
  map<const string, shared_ptr<const RegionServerInfo> > tableRegions_;
};

// Record RegionServerInfo
inline shared_ptr<const RegionServerInfo>
TableRegions::AddRegionServerInfo(const TRegionInfo& region,
                                  uint32_t* num_regions) {
  auto ret = tableRegions_[region.startKey];
  if (ret.get() == NULL) {
    // Not in the table, add it.
    ret.reset(new RegionServerInfo(region, ++*num_regions));
    tableRegions_[region.startKey] = ret;
  }
  return ret;
}

// Find TableRegions in the map
inline shared_ptr<const RegionServerInfo>
TableRegions::FindRegionServerInfoInCache(const string& key) const {
  // Maybe we got lucky and hit the exact row; return it.
  auto iter = tableRegions_.lower_bound(key);
  if (iter != tableRegions_.end()) {
    if (key >= iter->second->start_row &&
        (key <= iter->second->end_row || iter->second->end_row.empty())) {
      return iter->second;
    }
  }

  // Otherwise the entry we found was the first one after the row we
  // sought; so, move the iterator back, unless we're already at the
  // first row, which means the row isn't in the table in question/
  if (iter == tableRegions_.begin()) {
    return shared_ptr<const RegionServerInfo>();
  }
  --iter;
  if (key >= iter->second->start_row &&
      (key <= iter->second->end_row || iter->second->end_row.empty())) {
    return iter->second;
  }
  return shared_ptr<const RegionServerInfo>();
}

inline void TableRegions::RemoveRegionServerInfo(const string& key) {
  tableRegions_.erase(key);
}

inline SharedPtr<OutMsg> Hbase::CreateOutMsg(shared_ptr<ThriftMemoryOutTransport> trans) {
  SharedPtr<OutMsg>  out_msg = OutMsg::CreateInstance();
  if (!out_msg ) {
    abort();
  }
  uint8_t* reply_data = NULL;
  uint32_t reply_data_len  = 0;
  trans->Detach(&reply_data, &reply_data_len);
  if (reply_data != NULL && reply_data_len != 0) {
    SharedPtr<SharedBuffer> shared_buffer = SharedBuffer::CreateInstance();
    shared_buffer->Attach(reply_data, reply_data_len, Buffer::DestroyFreeImpl);
    out_msg->AppendData(MsgData(shared_buffer, 0, reply_data_len));
  }
  return out_msg;
}

inline RegionServer* Hbase::FindRegionServer(const string& table, const string& row) {
  TableMap::const_iterator it = regions_.find(table);
  TableRegions* table_entry = NULL;
  if (it == regions_.end()) {
    regions_[table].reset(new TableRegions);
    table_entry = regions_[table].get();
  } else {
    table_entry = it->second.get();
  }
  assert(table_entry);
  shared_ptr<const RegionServerInfo> ret =
    table_entry->FindRegionServerInfoInCache(row);

  if (ret.get()) {
    return FindRegionServerByHost(ret->host);
  }
  return NULL;
}
class HbaseGetEvent: public Event {
 public:
  HbaseGetEvent(int type, EQueryResult result,
      vector<TCell>* reply, void* param)
    :Event(type), result_(result), reply_(reply) {
      param_ = param;
  }
  EQueryResult result() const { return result_; }
  vector<TCell>* reply() { return reply_; }
  void* param() const { return param_; }
 private:
  EQueryResult result_;
  vector<TCell>* reply_;
  void* param_;
};
class HbaseSetEvent: public Event {
 public:
  HbaseSetEvent(int type, EQueryResult result, void* param)
    :Event(type), result_(result){
      param_ = param;
  }
  EQueryResult result() const { return result_; }
  void* param() const { return param_; }
 private:
  EQueryResult result_;
  void* param_;
};

template<typename StateMachine, typename Hbase>
class HbaseEventFire {
 public:
  typedef StateMachine TStateMachine;
  typedef Hbase THbasePool;
  typedef HbaseEventFire<StateMachine, Hbase> TMyself;
  HbaseEventFire(THbasePool* pool, int event_type) {
    pool_ = pool;
    event_type_ = event_type;
  }
  THbasePool* get_pool() { return pool_; }
  void FireGet(const Text& table,
               const Text& row,
               const Text& column,
               uint32_t context_id, 
               TStateMachine* machine, 
               void* myparam, 
               int timeout, 
               const LogId& logid,
               int retry) {
    pool_->Get(table, row, column, 
        std::bind(OnHbaseGetResp, machine, context_id, event_type_, myparam,
        std::placeholders::_1, std::placeholders::_2),
        logid, timeout, retry);
  }
  void FireSet(const Text& table,
               const Text& row,
               const vector<Mutation>& mutations,
               uint32_t context_id,
               TStateMachine* machine, 
               void* myparam, 
               int timeout,
               const LogId& logid,
               int retry) {
    pool_->Set(table, row, mutations,
        std::bind(OnHbaseSetResp, machine, context_id, event_type_, myparam,
        std::placeholders::_1),
        logid, timeout, retry);
  }
 private:
  static void OnHbaseGetResp(TStateMachine* machine, 
                          uint32_t ctxid, 
                          int event_type, 
                          void* param, 
                          EQueryResult result, 
                          vector<TCell>* reply) {
    HbaseGetEvent event(event_type, result, reply, param);
    machine->ProcessEvent(ctxid, &event);
  }
  static void OnHbaseSetResp(TStateMachine* machine, 
                          uint32_t ctxid, 
                          int event_type, 
                          void* param, 
                          EQueryResult result) {
    HbaseSetEvent event(event_type, result, param);
    machine->ProcessEvent(ctxid, &event);
  }

 private:
  THbasePool* pool_;
  int event_type_;
};
} // namespace blitz2

#endif // __BLITZ2_HBASE_POOL_H_

