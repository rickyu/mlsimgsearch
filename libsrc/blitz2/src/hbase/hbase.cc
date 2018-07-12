#include <netinet/in.h>
#include <zookeeper/zookeeper.h>
#include <blitz2/hbase.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <exception>
#include <transport/TTransportUtils.h>

namespace blitz2 {
static bool ParseHostPort(const std::string& hostport, std::string* host_out,
                          int* port_out) {
  // hbase 89 uses some magic "gunk" around the data it reads and
  // writes to zookeeper; the indicator of this gunk is the first byte
  // being the magic byte, 0xff.  If we encounter this signifier, we
  // parse as per removeMetaData.  If we do not, we treat it directly
  // as host:port (consistent with hbase 90).
  const char RECOVERABLE_ZOOKEEPER_MAGIC = 0xff;
  std::string real_hostport = hostport;
  if (real_hostport[0] == RECOVERABLE_ZOOKEEPER_MAGIC) {
    const int32_t MAGIC_OFFSET = sizeof(RECOVERABLE_ZOOKEEPER_MAGIC);
    const int32_t ID_OFFSET = sizeof(int32_t);
    const int32_t id_length =
      ntohl(*reinterpret_cast<const int32_t*>(hostport.data() +
                                                    MAGIC_OFFSET));
    const int32_t data_length =
      hostport.size() - MAGIC_OFFSET - ID_OFFSET - id_length;
    const int32_t data_offset = MAGIC_OFFSET + ID_OFFSET + id_length;
    real_hostport = std::string(hostport.data() + data_offset, data_length);
  }
  std::vector<std::string> pieces;
  // Format: hbase 89 and 90 use a colon-separates 2-tuple of
  // host:port.  hbase 92 uses a 3-tuple of host,port,timestamp.
  boost::algorithm::split(pieces, real_hostport, boost::algorithm::is_any_of(","));
  if (pieces.size() == 1) {
    // No commas, that means hbase 89/90 that is just host:port.
    pieces.clear();
    boost::algorithm::split(pieces, real_hostport, boost::algorithm::is_any_of(","));
  }
  if (pieces.size() < 2) {
    return false;
  }
  *host_out = pieces[0];
  *port_out = boost::lexical_cast<int>(pieces[1]);
  return true;
}

Hbase::Hbase(Reactor& reactor)
  :logger_(Logger::GetLogger("blitz2::hbase")),
   reactor_(reactor) {
    zookeeper_pool_ = NULL;
    root_server_ = NULL;
    thrift_port_ = 0;
    num_regions_ = 0;
    hbase_regionservers_ = NULL;
    hbase_rootserver_ = NULL;
}
Hbase::~Hbase() {
  LOG_DEBUG(logger_, "Enter");
  if (zookeeper_pool_ != NULL) {
    delete zookeeper_pool_;
    zookeeper_pool_ = NULL;
  }
  if (!region_servers_.empty()) {
    for (auto it = region_servers_.begin(); it != region_servers_.end(); ++it) {
      if (it->second != NULL) {
        delete it->second;
        it->second = NULL;
      }
    }
    region_servers_.clear();
  }
  if (root_server_ != NULL) {
    root_server_ = NULL;
  }
}
void Hbase::ZookeeperWatcher(zhandle_t* zh, int type,
    int state, const char* path, void* watcher_ctx) {
  (void)(zh);
  Hbase* myself = (Hbase*)(watcher_ctx);
  //判断是不是session事件，重新建立连接时捕捉这个,重新加上回调
  LOG_DEBUG(myself->logger_, "Enter, type=%d, state=%d", type, state);
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_EXPIRED_SESSION_STATE) {
      LOG_DEBUG(myself->logger_, "ZOO_EXPIRED_SESSION_STATE");
    } else if (state == ZOO_CONNECTING_STATE) {
      LOG_DEBUG(myself->logger_, "ZOO_CONNECTING_STATE");
    } else if (state == ZOO_CONNECTED_STATE) {
      LOG_DEBUG(myself->logger_, "ZOO_CONNECTED_STATE");
      myself->ZookeeperGetRegionServers(NULL);
      myself->ZookeeperGetRootServer(NULL);
    }
    return;
  }
  
  // 判断path的不同
  if (0 == strncmp(path, myself->hbase_rootserver_, 
        strlen(myself->hbase_rootserver_))) {
    LOG_DEBUG(myself->logger_, "root-region-server changed, type=%d", type);
    // rootserver改变时，regionservers一定同时改变，但是regionservers的改变事件
    // 有可能后来捕捉到，这样regionservers列表中找不到rootserver
    // 因此，先重新拉取一下regionservers列表
    myself->ZookeeperGetRegionServers(NULL);
    myself->ZookeeperGetRootServer(NULL);
  } else if (0 == strncmp(path, myself->hbase_regionservers_,
        strlen(myself->hbase_regionservers_))) {
    if (type == ZOO_CHILD_EVENT) {
      LOG_DEBUG(myself->logger_, "/hbase/rs child changed");
      myself->ZookeeperGetRegionServers(NULL);
    }
  }
}
int Hbase::Init(const Configure& conf) {
  LOG_DEBUG(logger_, "Enter");
  thrift_port_ = conf.get<int>("thrift_port", 9091);
  hbase_rootserver_ = const_cast<char*>(conf.get<string>("root-regionserver", 
      "/hbase/root-region-server").data());
  hbase_regionservers_ = const_cast<char*>(conf.get<string>("regionservers", 
      "/hbase/rs").data());
  //建立root-server和regionservers
  zookeeper_pool_ = new ZookeeperPool(reactor_);
  zookeeper_pool_->Init(conf.get<string>("zookeeper.host").data(), 
      ZookeeperWatcher,
      this, // this作为context传过去
      conf.get<int>("zookeeper.flags", 0),
      conf.get<int>("zookeeper.recvtimeout", 1000), 
      conf.get<int>("zookeeper.connectcount", 1));
  LOG_DEBUG(logger_, "recvtimeout=%d", conf.get<int>("zookeeper.recvtimeout", 300000));
  return 0;
}
void Hbase::Get(const Text& table,
            const Text& row,
            const Text& column,
            //const std::map<Text,Text>& attributes,
            const HbaseGetCallback& callback,
            const LogId& logid,
            uint32_t timeout,
            int retry_count) {
  uint64_t expire_time = time::GetTickMs() + timeout;
  shared_ptr<ThriftMemoryOutTransport> out_transport(new ThriftMemoryOutTransport());
  shared_ptr<TProtocol> oprot(new TBinaryProtocol(out_transport));
  HbaseClient client(oprot);
  const std::map<Text,Text> attributes;
  client.send_get(table, row, column, attributes);
  SharedPtr<OutMsg> outmsg = CreateOutMsg(out_transport);
  outmsg->logid() = logid;
  LOG_DEBUG(logger_, "Enter");
  GetInvoke(table, row, outmsg, callback, expire_time, retry_count);
}
void Hbase::Set(const Text& table,
            const Text& row,
            const std::vector<Mutation> & mutations,
            const HbaseSetCallback& callback,
            const LogId& logid,
            uint32_t timeout,
            int retry_count) {
  LOG_DEBUG(logger_, "Enter");
  uint64_t expire_time = time::GetTickMs() + timeout;
  shared_ptr<ThriftMemoryOutTransport> out_transport(new ThriftMemoryOutTransport());
  shared_ptr<TProtocol> oprot(new TBinaryProtocol(out_transport));
  HbaseClient client(oprot);
  const std::map<Text,Text> attributes;
  client.send_mutateRow(table, row, mutations, attributes);
  SharedPtr<OutMsg> outmsg = CreateOutMsg(out_transport);
  outmsg->logid() = logid;
  SetInvoke(table, row, outmsg, callback, expire_time, retry_count);
}

void Hbase::GetInvoke(const Text& table,
            const Text& row,
            SharedPtr<OutMsg> outmsg,
            const HbaseGetCallback& callback,
            uint64_t expire,
            int retry_count) {
  RegionServer* region_server = FindRegionServer(table, row);
  if (region_server) {
    LOG_DEBUG(logger_, "region_server is not null");
    GetSendReq(region_server, outmsg, callback, table, row,
        expire, retry_count);
  } else {
    // refresh region info.
    LOG_DEBUG(logger_, "region_server is null, load regioninfo");
    LoadRegionInfo(table, row, 
        bind(&Hbase::GetSendReq, this, std::placeholders::_1, 
          outmsg, callback, table, row, expire, retry_count),
        expire);
  }
}
void Hbase::SetInvoke(const Text& table,
            const Text& row,
            SharedPtr<OutMsg> outmsg,
            const HbaseSetCallback& callback,
            uint64_t expire,
            int retry_count) {
  LOG_DEBUG(logger_, "Enter");
  RegionServer* region_server = FindRegionServer(table, row);
  if (region_server) {
    LOG_DEBUG(logger_, "region_server is not null");
    SetSendReq(region_server, outmsg, callback, table, row,
        expire, retry_count);
  } else {
    // refresh region info.
    LOG_DEBUG(logger_, "region_server is null, load regioninfo");
    LoadRegionInfo(table, row, 
        bind(&Hbase::SetSendReq, this, std::placeholders::_1, 
          outmsg, callback, table, row, expire, retry_count),
        expire);
  }
}

void Hbase::GetSendReq(
            RegionServer* region, 
            SharedPtr<OutMsg> outmsg,
            const HbaseGetCallback& callback,
            const Text& table,
            const Text& row,
            uint64_t expire_time, 
            int retry_count) {
  if (!region) {
    LOG_DEBUG(logger_, "region=null");
    callback(kQueryResultInternal, NULL);
    return;
  } 
  region->QueryWithExpire(outmsg, 
             bind(&Hbase::RegionServerOnGet, this, std::placeholders::_1,
               std::placeholders::_2, SharedPtr<OutMsg>(outmsg),
               expire_time, retry_count, callback, table, row),
             expire_time);
}
void Hbase::SetSendReq(
            RegionServer* region, 
            SharedPtr<OutMsg> outmsg,
            const HbaseSetCallback& callback,
            const Text& table,
            const Text& row,
            uint64_t expire_time, 
            int retry_count) {
  LOG_DEBUG(logger_, "Enter");
  if (!region) {
    LOG_DEBUG(logger_, "region=null");
    callback(kQueryResultInternal);
    return;
  }
  LOG_DEBUG(logger_, "region not null");
  region->QueryWithExpire(outmsg, 
             bind(&Hbase::RegionServerOnSet, this, std::placeholders::_1,
               std::placeholders::_2, SharedPtr<OutMsg>(outmsg),
               expire_time, retry_count, callback, table, row),
             expire_time);
}

void Hbase::RegionServerOnGet(
                         EQueryResult result,
                         ThriftMsg* msg,
                         SharedPtr<OutMsg> outmsg,
                         uint64_t expire,
                         int retry_count,
                         const HbaseGetCallback& callback,
                         const Text& table,
                         const Text& row) {
  LOG_DEBUG(logger_, "Enter");
  if (result == kQueryResultSuccess) {
    LOG_DEBUG(logger_, "kQueryResultSuccess");
    LOG_DEBUG(logger_, "msg->data_len=%d", msg->data_len());
    shared_ptr<TTransport> transport(new ThriftMemoryInTransport(msg->data(), 
          msg->data_len()));
    shared_ptr<TTransport> framed(new TFramedTransport(transport));
    shared_ptr<TBinaryProtocol> proto(new TBinaryProtocol(framed));
    HbaseClient client(proto);
    std::vector<TCell> cells;
    try {
      client.recv_get(cells);
      LOG_DEBUG(logger_, "RegionServerOnGet --> ok");
      callback(result, &cells);
      return;
    } catch(...) {
      callback(kQueryResultInvalidResp, NULL);
      return;
    }
  } else if (result == kQueryResultTimeout) {
    LOG_DEBUG(logger_, "kQueryResultTimeout");
    callback(result, NULL);
    return;
  }
  // retry
  if (retry_count > 0) {
    uint64_t now = time::GetTickMs();
    if (expire > now) {
      return GetInvoke(table, row, outmsg, callback, expire, --retry_count);
    }
  }
  callback(result, NULL);
}
void Hbase::RegionServerOnSet(
                         EQueryResult result,
                         ThriftMsg* msg,
                         SharedPtr<OutMsg> outmsg,
                         uint64_t expire,
                         int retry_count,
                         const HbaseSetCallback& callback,
                         const Text& table,
                         const Text& row) {
  LOG_DEBUG(logger_, "Enter");
  if (result == kQueryResultSuccess) {
  LOG_DEBUG(logger_, "result == kQueryResultSuccess");
    shared_ptr<TTransport> transport(new ThriftMemoryInTransport(msg->data(), 
          msg->data_len()));
    shared_ptr<TTransport> framed(new TFramedTransport(transport));
    shared_ptr<TBinaryProtocol> proto(new TBinaryProtocol(framed));
    HbaseClient client(proto);
    try {
      client.recv_mutateRow();
      callback(result);
      return;
    } catch(...) {
      callback(kQueryResultInvalidResp);
      return;
    }
  } else if (result == kQueryResultTimeout) {
    LOG_DEBUG(logger_, "kQueryResultTimeout");
    callback(result);
    return;
  }
  // retry
  if (retry_count > 0) {
    LOG_DEBUG(logger_, "retry , result=%d, not kQueryResultSuccess", result);
    uint64_t now = time::GetTickMs();
    if (expire > now) {
      return SetInvoke(table, row, outmsg, callback, expire, --retry_count);
    }
  }
  callback(result);
}


struct ZookeeperCompletionData {
  Hbase* hbase;
  function<void()> cb;
};


void Hbase::LoadRegionInfo(const string& table, const string& row, 
    const function<void(RegionServer*)>& cb, uint64_t expire_time) {
  LOG_DEBUG(logger_, "Enter");
  if (!root_server_) {
    LOG_DEBUG(logger_, "root_server=null");
    ZookeeperGetRootServer(
        bind(&Hbase::RootGetRegionInfo, this, table, row, cb, expire_time));
  } else {
    RootGetRegionInfo(table, row, cb, expire_time);
  }
}

void Hbase::RootGetRegionInfo(const string& table, const string& row, 
    const function<void(RegionServer*)>& cb, uint64_t expire_time) {
  LOG_DEBUG(logger_, "Enter");
  //确保root_server_不为空
  if (root_server_ == NULL) {
    LOG_ERROR(logger_, "root_server_ == NULL, no root_server_ info in zookeeper");
    if (cb) {
      cb(NULL);
      return;
    }
  } else {
    LOG_DEBUG(logger_, "root_server_ not NULL");
  }

  shared_ptr<ThriftMemoryOutTransport> out_transport(new ThriftMemoryOutTransport());
  shared_ptr<TProtocol> oprot(new TBinaryProtocol(out_transport));
  HbaseClient client(oprot);
  static const string kNines = "99999999999999"; // from HConstats.java
  string meta_row_key = table + "," + row + "," + kNines;
  client.send_getRegionInfo(meta_row_key);
  SharedPtr<OutMsg> outmsg = CreateOutMsg(out_transport);
     
  uint64_t cur_time = time::GetTickMs();
  if (expire_time > cur_time) {
    root_server_->Query(outmsg, 
        bind(&Hbase::RootOnGetRegionInfo, this, std::placeholders::_1,
          std::placeholders::_2, cb, table, row, expire_time), 
          expire_time-cur_time);
  } else {
    LOG_INFO(logger_, "Timeout when get regioninfo.Innomal");
    if (cb) {
      cb(NULL);
    }
  }
}

void Hbase::RootOnGetRegionInfo(EQueryResult result, ThriftMsg* msg,
                          const function<void(RegionServer*)>& cb,
                          const string& table, const string& row,
                          uint64_t expire_time) {
  LOG_DEBUG(logger_, "Enter");
  if (result != kQueryResultSuccess) {
    LOG_DEBUG(logger_, "result != kQueryResultSuccess");
    if (cb) {
      cb(NULL);
    }
    return;
  }
  uint64_t now = time::GetTickMs();
  if (now > expire_time) {
    LOG_INFO(logger_, "result == kQueryTimeout.Innomal");
    if (cb) {
      cb(NULL);
    }
    return;
  }
  LOG_DEBUG(logger_, "result == kQueryResultSuccess");
  shared_ptr<TTransport> transport(new ThriftMemoryInTransport(msg->data(), msg->data_len()));
  shared_ptr<TTransport> framedtransport(new TFramedTransport(transport));
  shared_ptr<TBinaryProtocol> proto(new TBinaryProtocol(framedtransport));
  HbaseClient client(proto);
  TRegionInfo region_info;
  client.recv_getRegionInfo(region_info);
  assert(row >= region_info.startKey);
  TableRegions* table_entry = regions_[table].get();
  if (row <= region_info.endKey || region_info.endKey.empty()) {
     shared_ptr<const RegionServerInfo> ret =
       table_entry->FindRegionServerInfoInCache(row);
    if (ret.get()) {
      table_entry->RemoveRegionServerInfo(ret->start_row);
    }
    ret = table_entry->AddRegionServerInfo(region_info, &num_regions_);
  }
  RegionServer* region_server = FindRegionServerByHost(region_info.serverName);
  if (cb) {
    cb(region_server);
  }
}
void Hbase::ZookeeperGetRootServer(const function<void()>& cb) {
  LOG_DEBUG(logger_, "Enter, time=%ld", time::GetTickMs());
  ZookeeperCompletionData* data = new ZookeeperCompletionData();
  data->hbase = this;
  data->cb = cb;
  zookeeper_pool_->Get(hbase_rootserver_, 1, ZookeeperGetRootServerCompletion, 
      data);
}
void Hbase::ZookeeperGetRootServerCompletion(int rc, const char* value,
      int value_len, const struct Stat* stat, const void* data) {
  (void)(stat);
  ZookeeperCompletionData* mydata = 
    reinterpret_cast<ZookeeperCompletionData*>((void*)data);
  Hbase* myself = mydata->hbase;
  LOG_DEBUG(myself->logger_, "Enter, time=%ld", time::GetTickMs());
  if (!myself->regions_.empty()) {   
    LOG_DEBUG(myself->logger_, "myself->regions_ not empty");
    for (auto it = myself->regions_.begin(); it != myself->regions_.end(); ++it) {
      it->second.reset(new TableRegions);
    }
  } else {
    LOG_DEBUG(myself->logger_, "myself->regions_ is empty");
  }
  if (value == NULL) { 
    LOG_DEBUG(myself->logger_, "root-server is null");
    myself->root_server_ = NULL;
  } else {
    std::string host;
    int port;
    if (ParseHostPort(std::string(value, value_len), &host, &port)) {
      myself->root_server_ = myself->FindRegionServerByHost(host);
      LOG_DEBUG(myself->logger_, "root-server is initialized");
    }
  }
  LOG_DEBUG(myself->logger_, "rc=%d", rc);
  //if (rc == ZOK) {
    if (mydata->cb) {
      mydata->cb();
    }
  //}
  // 释放data指针
  if (mydata) {
    delete mydata;
    mydata = NULL;
  }
}
void Hbase::ZookeeperGetRegionServers(const function<void()>& cb) {
  ZookeeperCompletionData* data = new ZookeeperCompletionData();
  data->hbase = this;
  data->cb = cb;
  zookeeper_pool_->GetChildren(hbase_regionservers_, 1, ZookeeperGetRegionServersCompletion, 
      data);
}
void Hbase::ZookeeperGetRegionServersCompletion(int rc, 
    const struct String_vector *strings, const void* data) {
  ZookeeperCompletionData* mydata = 
    reinterpret_cast<ZookeeperCompletionData*>((void*)data);
  Hbase* myself = mydata->hbase;
  LOG_DEBUG(myself->logger_, "Enter");
  if (!myself->regions_.empty()) {   
    LOG_DEBUG(myself->logger_, "myself->regions_ not empty");
    for (auto it = myself->regions_.begin(); it != myself->regions_.end(); ++it) {
      it->second.reset(new TableRegions);
    }
  } else {
    LOG_DEBUG(myself->logger_, "myself->regions_ is empty");
  }
  if (rc == ZOK) {
    if (mydata->cb) {
      mydata->cb();
    }
  }
  if (strings) {
    LOG_DEBUG(myself->logger_, "strings != NULL");
    int i;
    for (i = 0; i < strings->count; ++i) {
      std::vector<std::string> pieces;
      boost::algorithm::split(pieces, strings->data[i], boost::algorithm::is_any_of(","));
      if (pieces.size() == 1) {
        pieces.clear();
        boost::algorithm::split(pieces, strings->data[i], boost::algorithm::is_any_of(","));
      }
      if (pieces.size() < 2) {
        return;
      }
      auto iter = myself->region_servers_.find(pieces[0]);
      if (iter != myself->region_servers_.end()) {
        LOG_DEBUG(myself->logger_, "region_servers_ find %s", pieces[0].data());
        continue;
      }
      std::string hostport("tcp://");
      char port[32] = { '0' }; 
      struct hostent *ht;
      sprintf(port, "%d", myself->thrift_port_);
      if((ht=gethostbyname(pieces[0].data())) == NULL) {
        LOG_DEBUG(myself->logger_, "gethostbyname, name=%s", pieces[0].data());
      }
      hostport += inet_ntoa(*((struct in_addr *)ht->h_addr));
      hostport += ":";
      hostport += port;
      LOG_DEBUG(myself->logger_, "hostport=%s", hostport.data());
      IoAddr addr;
      addr.Parse(hostport);
      ThriftFramedMsgDecoder::TInitParam param; 
      RegionServer *regionserver = new RegionServer(myself->reactor_, addr, param);

      myself->region_servers_[pieces[0]] = regionserver;
    }
  }
  //释放data指针 
  if (mydata) {
    delete mydata;
    mydata = NULL;
  }
}
} // namespace blitz2.
