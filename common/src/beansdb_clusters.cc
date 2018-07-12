#include <beansdb_client/beansdb_clusters.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/crc.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#define ORIGIN_MARK "_o"

CLogger& BeansdbClusters::logger_ = CLogger::GetInstance(BEANSDB_CLIENT_LOGGER_NAME);

BeansdbClusters* BeansdbClusters::instance_ = NULL;

BeansdbClusters::BeansdbClusters() {
}

BeansdbClusters::~BeansdbClusters() {
  BeansdbClustersNode::iterator it_node = clusters_.begin();
  while (it_node != clusters_.end()) {
    BeansdbSingleCluster::iterator it_cluster = it_node->second->begin();
    while (it_cluster != it_node->second->end()) {
      if (it_cluster->second) {
        delete it_cluster->second;
        it_cluster->second = NULL;
      }
      ++it_cluster;
    }
    if (it_node->second) {
      delete it_node->second;
      it_node->second = NULL;
    }
    ++it_node;
  }
}

BeansdbClusters* BeansdbClusters::GetInstance() {
  if (NULL == instance_) {
    instance_ = new BeansdbClusters;
  }
  return instance_;
}

void BeansdbClusters::Finalize() {
  if (instance_) {
    delete instance_;
    instance_ = NULL;
  }
}

beansdb_client_ret_t BeansdbClusters::AddCluster(const std::string &conf) {
  ClusterInfo cluster_info;
  beansdb_client_ret_t ret = ParseConf(conf, cluster_info);
  if (BCR_SUCCESS != ret) {
    return ret;
  }
  //添加集群分配信息初始化操作
  ClusterScale cluster_scale;
  int cluster_location_flag = 1;
  for (size_t i = 0; i < cluster_location_.size(); i++) {
    if (cluster_location_[i].kind == cluster_info.pic_type 
        && cluster_location_[i].flag == cluster_info.cluster_flag)
      cluster_location_flag = 0;
  }
  if (cluster_location_flag) {
    cluster_scale.kind = cluster_info.pic_type; 
    cluster_scale.flag = cluster_info.cluster_flag;
    cluster_scale.scale = cluster_info.cluster_scale;
    cluster_location_.push_back(cluster_scale);
  }
  enum {
    POOL_EXIST_NONE,
    POOL_EXIST_TYPE,
    POOL_EXIST_CLUSTER,
  };
  int pool_exist = POOL_EXIST_NONE;
  BeansdbClientPool *pool = NULL;
  BeansdbClustersNode::iterator it_type = clusters_.find(cluster_info.pic_type);
  if (it_type != clusters_.end()) {
    pool_exist = POOL_EXIST_TYPE;
    BeansdbSingleCluster::iterator it_cluster = it_type->second->find(cluster_info.cluster_flag);
    if (it_cluster != it_type->second->end()) {
      pool = it_cluster->second;
      pool_exist = POOL_EXIST_CLUSTER;
    }
  }
  if (NULL == pool) {
    pool = new BeansdbClientPool(cluster_info.pool_size);
  }
  for (int pindex = 0; pindex < cluster_info.pool_size; pindex++) {
    BeansdbBucketClients *bucket = NULL;
    if (POOL_EXIST_CLUSTER != pool_exist) {
      bucket = new BeansdbBucketClients(1, 1);
      ret = bucket->Init(cluster_info.bucket_count, cluster_info.servers.size());
    }
    else {
      bucket = const_cast<BeansdbBucketClients*>(pool->GetIndex(pindex)->GetBucketClients());
    }
    if (BCR_SUCCESS != ret) {
      delete bucket;
      LOGGER_ERROR(logger_, "init bucket failed, count=%d, dup_num=%ld",
                   cluster_info.bucket_count, cluster_info.servers.size());
      return ret;
    }
    for (size_t bindex = 0; bindex < cluster_info.servers.size(); bindex++) {
      ret = bucket->AddServer(cluster_info.servers[bindex].ip,
                              cluster_info.servers[bindex].port,
                              cluster_info.buckets.data(),
                              cluster_info.buckets.size());
      if (BCR_SUCCESS != ret) {
        LOGGER_ERROR(logger_, "bucket add server failed");
        delete bucket;
        return ret;
      }
    }
    if (POOL_EXIST_CLUSTER != pool_exist) {
      BeansdbClient *client = new BeansdbClient(bucket);
      if (NULL == client) {
        delete bucket;
        return BCR_OUTOFMEMORY;
      }
      pool->Add(client);
    }
  }

  // check type exist
  if (POOL_EXIST_NONE == pool_exist) {
    BeansdbSingleCluster *cluster = new BeansdbSingleCluster;
    cluster->insert(std::pair<std::string, BeansdbClientPool*>(cluster_info.cluster_flag, pool));
    clusters_.insert(std::pair<std::string, BeansdbSingleCluster*>(cluster_info.pic_type, cluster));
  }
  else if (POOL_EXIST_TYPE == pool_exist) {
    it_type->second->insert(std::pair<std::string, BeansdbClientPool*>(cluster_info.cluster_flag, pool));
  }

  return BCR_SUCCESS;
}

beansdb_client_ret_t BeansdbClusters::AddClusters(const std::vector<std::string> &confs) {
  for (size_t i = 0; i < confs.size(); i++) {
    AddCluster(confs[i]);
  }
  return BCR_SUCCESS;
}

beansdb_client_ret_t BeansdbClusters::Set(const std::string& key,
                                          const char *val,
                                          const size_t val_size,
                                          const long time /*= 0*/) {
  BeansdbClientPool *pool = NULL;
  beansdb_client_ret_t ret = GetCluster(key, pool);
  if (BCR_SUCCESS != ret) {
    return ret;
  }
  return pool->Set(key, val, val_size, time);
}

beansdb_client_ret_t BeansdbClusters::Get(const std::string& key,
                                          char *&val,
                                          size_t &val_size,
                                          long timeout /*= 0*/) {
  BeansdbClientPool *pool = NULL;
  beansdb_client_ret_t ret = GetCluster(key, pool);
  if (BCR_SUCCESS != ret) {
    return ret;
  }
  return pool->Get(key, val, val_size, timeout);
}

// type=picdetail;cluster_flag=c1;cluster_scale=10;pool_size=5;servers=172.16.0.61:7900,172.16.0.62:7900;buckets_num=16;buckets=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
beansdb_client_ret_t BeansdbClusters::ParseConf(const std::string &conf, ClusterInfo &cluster) {
  try {
    std::vector<std::string> segments;
    boost::algorithm::split(segments, conf, boost::algorithm::is_any_of(";"));
    std::vector<std::string> kv;
    std::string key, value;
    for (size_t i = 0; i < segments.size(); i++) {
      boost::algorithm::split(kv, segments[i], boost::algorithm::is_any_of("="));
      if (2 != kv.size()) {
        throw 1;
      }
      key = kv[0];
      value = kv[1];
      boost::algorithm::trim(key);
      boost::algorithm::trim(value);
      if ("type" == key) {
        cluster.pic_type = value;
      }
      else if ("cluster_flag" == key) {
        cluster.cluster_flag = value;
      }
      else if ("cluster_scale" == key) {
        cluster.cluster_scale = boost::lexical_cast<size_t>(value);
      }
      else if ("pool_size" == key) {
        cluster.pool_size = boost::lexical_cast<int>(value);
      }
      else if ("servers" == key) {
        std::vector<std::string> servers;
        boost::algorithm::split(servers, value, boost::algorithm::is_any_of(","));
        for (size_t si = 0; si < servers.size(); si++) {
          std::vector<std::string> server_info;
          boost::algorithm::split(server_info, servers[si], boost::algorithm::is_any_of(":"));
          ServerAddress sa;
          sa.ip = server_info[0];
          sa.port = server_info[1].empty() ? 7900 : boost::lexical_cast<int>(server_info[1]);
          cluster.servers.push_back(sa);
        }
      }
      else if ("buckets_num" == key) {
        cluster.bucket_count = boost::lexical_cast<int>(value);
      }
      else if ("buckets" == key) {
        std::vector<std::string> buckets;
        boost::algorithm::split(buckets, value, boost::algorithm::is_any_of(","));
        for (size_t bi = 0; bi < buckets.size(); bi++) {
          boost::algorithm::trim(buckets[bi]);
          cluster.buckets.push_back(boost::lexical_cast<int>(buckets[bi]));
        }
      }
      else {
        throw 2;
      }
    }
  }
  catch (...) {
    LOGGER_ERROR(logger_, "parse conf failed, conf=%s", conf.c_str());
    return BCR_CLUSTER_CONF_FAILED;
  }
  return BCR_SUCCESS;
}

beansdb_client_ret_t BeansdbClusters::ParsePicKey(const std::string &key,
                                                  PicKeyInfo &key_info) {
  try {
    size_t type_index = key.find_first_of("/");
    if (std::string::npos == type_index) {
      throw 1;
    }
    key_info.type.assign(key.c_str(), type_index);

    size_t cluster_begin = key.find_first_of(".");
    size_t cluster_end = key.find_last_of(".");
    if (std::string::npos == cluster_end) {
      throw 2;
    }
    if (std::string::npos == cluster_begin || cluster_begin == cluster_end) {
      key_info.cluster.assign(DEFAULT_CLUSTER_NAME);
    }
    else {
      key_info.cluster.assign(key.c_str()+cluster_begin+1, cluster_end-cluster_begin-1);
    }
  }
  catch (...) {
    LOGGER_ERROR(logger_, "wrong pic key (%s)", key.c_str());
    return BCR_PARSE_PICKEY_FAILED;
  }
  return BCR_SUCCESS;
}

beansdb_client_ret_t BeansdbClusters::GetCluster(const std::string &key, BeansdbClientPool *&pool) {
  PicKeyInfo key_info;
  beansdb_client_ret_t ret = BCR_SUCCESS;

  ret = ParsePicKey(key, key_info);
  if (BCR_SUCCESS != ret) {
    return ret;
  }

  BeansdbClustersNode::iterator it_node = clusters_.find(key_info.type);
  if (clusters_.end() == it_node) {
    it_node = clusters_.find(DEFAULT_PIC_TYPE_NAME);
    if (clusters_.end() == it_node) {
      LOGGER_ERROR(logger_, "can't find any type cluster");
      return BCR_NOCLUSTER_FOUND;
    }
  }
  BeansdbSingleCluster::iterator it_cluster = it_node->second->find(key_info.cluster);
  if (it_node->second->end() == it_cluster) {
    LOGGER_ERROR(logger_, "can't find any cluster");
    return BCR_NOCLUSTER_FOUND;
  }

  pool = it_cluster->second;
  return BCR_SUCCESS;
}

//用于生成存储图片的key,所有同一类型集群下配额相加不允许大于100
beansdb_client_ret_t BeansdbClusters::GenClusterLocate(const std::string &bytes_md5,
                                                       const std::string &kind,
                                                       std::string &cluster_locate) {
  int sum = 0, current = 0;
  std::string allocation[100];
  std::string cluster_kind;
  cluster_kind.assign(kind);
  if (cluster_kind != "picdetail") {
    cluster_kind.assign(DEFAULT_CLUSTER_NAME);
  }
  for (size_t i = 0;i < cluster_location_.size(); i++ ) {
    if (cluster_location_[i].kind == cluster_kind) {
      sum += cluster_location_[i].scale; 
      for (size_t j = 0;j < cluster_location_[i].scale; j ++) {
        allocation[current] = cluster_location_[i].flag;
        current ++;
      }
    }
  }
  if (0 == sum) {
    LOGGER_ERROR(logger_, "can't find any location info");
    return BCR_NOCLUSTER_FOUND;
  } 
  boost::crc_32_type result;
  result.process_bytes(bytes_md5.c_str(), bytes_md5.length());
  int remain = result.checksum() % sum;
  remain = abs(remain);
  cluster_locate = allocation[remain];
  return BCR_SUCCESS; 
}
//{new_orig_picid} = {kind}/_o/{middle}/{base}_{width}_{height}.{cluster}.{ext}
//{default_orig_picid} = {kind}/_o/{middle}/{base}_{width}_{height}.{ext}
beansdb_client_ret_t BeansdbClusters::GenerateOriginPicId(const std::string &bytes_md5,
                                                          const std::string &kind,
                                                          const std::string &ext,
                                                          const int width,
                                                          const int height,
                                                          std::string &orig_picid) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  std::string cluster_locate; 
  ret = GenClusterLocate(bytes_md5, kind, cluster_locate);
  if (ret != BCR_SUCCESS) {
    LOGGER_ERROR(logger_, "GenClusterLocate failed");
    return ret;
  }
  orig_picid.assign(kind);
  orig_picid.append("/"ORIGIN_MARK"/");
  orig_picid.append(bytes_md5.c_str(), 2);
  orig_picid.append("/");
  orig_picid.append(bytes_md5.c_str()+2, 2);
  orig_picid.append("/");
  orig_picid.append(bytes_md5.c_str()+4);
  orig_picid.append("_");
  orig_picid.append(boost::lexical_cast<std::string>(width));
  orig_picid.append("_");
  orig_picid.append(boost::lexical_cast<std::string>(height));
  orig_picid.append(".");
  if ("default" != cluster_locate) {
    orig_picid.append(cluster_locate.c_str());
    orig_picid.append(".");
  }
  orig_picid.append(ext.c_str());
  return ret;
}

