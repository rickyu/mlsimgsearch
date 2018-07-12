#include <string.h>
#include <beansdb_client/beansdb_client.h>


CLogger& BeansdbBucketClients::logger_ = CLogger::GetInstance(BEANSDB_CLIENT_LOGGER_NAME);
CLogger& BeansdbClient::logger_ = CLogger::GetInstance(BEANSDB_CLIENT_LOGGER_NAME);

//////////////////////////// BeansdbBucketClients ///////////////////////////////
BeansdbBucketClients::BeansdbBucketClients(int hash, int distribution)
: hash_(hash)
    , distribution_(distribution) {
    }

BeansdbBucketClients::~BeansdbBucketClients() {
  std::vector<BeansdbBucket>::iterator iter = mc_clients_.begin();
  while (iter != mc_clients_.end()) {
    BeansdbBucket bucket = *iter;
    if (bucket.clients) {
      delete []bucket.clients;
    }
    iter++;
  }
}

beansdb_client_ret_t BeansdbBucketClients::Init(const uint32_t buckets_count /*= 16*/,
                                                const uint32_t buckets_per_server /*= 3*/) {
  buckets_count_ = buckets_count;
  buckets_per_server_ = buckets_per_server;
  //mc_clients_.resize(buckets_count_);

  for (uint32_t i = 0; i < buckets_count_; i++) {
    BeansdbBucket bucket;
    bucket.clients = new MemcachedClient[buckets_per_server];
    if (NULL == bucket.clients) {
      return BCR_OUTOFMEMORY;
    }
    bucket.count = 0;
    bucket.size = buckets_per_server;
    mc_clients_.push_back(bucket);
  }

  return BCR_SUCCESS;
}

beansdb_client_ret_t BeansdbBucketClients::AddServer(const std::string& addr,
                                                     const uint32_t port,
                                                     const int buckets[],
                                                     const size_t size) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  MemcachedClient *client = NULL;

  for (size_t i = 0; i < size; i++) {
    uint32_t& count = mc_clients_[buckets[i]].count;
    client = &(mc_clients_[buckets[i]].clients[count]);
    client->Init(hash_, distribution_);
    ret = client->AddServer(addr, port);
    if (BCR_SUCCESS != ret)
      break;
    ret = client->SetAutoReset();
    if (BCR_SUCCESS != ret)
      break;
    mc_clients_[buckets[i]].addr.assign(addr);
    mc_clients_[buckets[i]].port = port;
    ++count;
  }

  return ret;
}

BeansdbBucket* BeansdbBucketClients::GetBucket(const uint32_t bucket_index) {
  if (bucket_index >= buckets_count_ && mc_clients_[bucket_index].count <= 0) {
    return NULL;
  }

  return &(mc_clients_[bucket_index]);
}

uint32_t BeansdbBucketClients::get_buckets_count() {
  return buckets_count_;
}

uint32_t BeansdbBucketClients::get_buckets_per_server() {
  return buckets_per_server_;
}


///////////////////////// BeansdbClient ///////////////////////////////
BeansdbClient::BeansdbClient(const BeansdbBucketClients* bucket_clients)
  : bucket_clients_(const_cast<BeansdbBucketClients*>(bucket_clients)){
  }

BeansdbClient::~BeansdbClient() {
  if (bucket_clients_) {
    delete bucket_clients_;
    bucket_clients_ = NULL;
  }
}

beansdb_client_ret_t BeansdbClient::Set(const std::string& key,
                                        const char *val,
                                        const size_t val_len,
                                        const long timeout) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  int succ_num = 0, index;
  BeansdbBucket* bucket = NULL;
  ret = GetBucket(key.c_str(), bucket, index);
  if (BCR_SUCCESS != ret) {
    return ret;
  }

  MemcachedClient* client = NULL;
  for (uint32_t i = 0; i < bucket->count; i++) {  
    client = &bucket->clients[i];
    ret = client->Set(key, val, val_len, timeout);
    if (BCR_SUCCESS != ret) {
      LOGGER_ERROR(logger_, "save %s failed [%s:%d]",
                   key.c_str(), bucket->addr.c_str(), bucket->port);
      continue;
    }
    succ_num++;
  }

  if (succ_num < 1) {
    return BCR_BEANSDB_SAVELESSTHAN2;
  }
  else {
    // 如果最后一个节点存储错误，会导致整个逻辑错误
    // 所以，只要有一个节点存储成功就表示成功
    // 此处在线上出现过问题
    ret = BCR_SUCCESS;
  }

  return ret;
}

beansdb_client_ret_t BeansdbClient::Get(const std::string& key,
                                        char *&val,
                                        size_t &len,
                                        const long timeout) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  BeansdbBucket* bucket = NULL;
  int index;
  val = NULL;
  ret = GetBucket(key.c_str(), bucket, index);
  if (BCR_SUCCESS != ret) {
    return ret;
  }
  MemcachedClient* client = &bucket->clients[index];
  ret = client->Get(key, val, len);
  if (BCR_SUCCESS != ret) {
    // 如果没有从选中的节点中取到，尝试其他节点
    for (uint32_t i = 0; i < bucket->count; i++) {
      if ((int)i == index)
        continue;
      if (val) {
        delete val;
        val = NULL;
      }
      client = &bucket->clients[i];
      ret = client->Get(key, val, len, timeout);
      if (BCR_SUCCESS == ret) {
        break;
      }
      else if (BCR_MEMCACHED_NOTFOUND != ret) {
        LOGGER_ERROR(logger_, "get %s failed [%s:%d]",
                     key.c_str(), bucket->addr.c_str(), bucket->port);
      }
    }
  }
  return ret;
}

beansdb_client_ret_t BeansdbClient::Delete(const std::string& key) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  int succ_num = 0, index;
  BeansdbBucket* bucket = NULL;
  ret = GetBucket(key.c_str(), bucket, index);
  if (BCR_SUCCESS != ret) {
    return ret;
  }

  MemcachedClient* client = NULL;
  for (uint32_t i = 0; i < bucket->count; i++) {  
    client = &bucket->clients[i];
    ret = client->Delete(key);
    if (BCR_SUCCESS != ret) {
      LOGGER_ERROR(logger_, "save %s failed [%s:%d]",
                   key.c_str(), bucket->addr.c_str(), bucket->port);
    }
    succ_num++;
  }

  return ret;
}

bool BeansdbClient::Exist(const std::string &key) {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  BeansdbBucket* bucket = NULL;
  int index;
  ret = GetBucket(key.c_str(), bucket, index);
  if (BCR_SUCCESS != ret) {
    return ret;
  }

  char *val = NULL;
  size_t len = 0;
  MemcachedClient* client = NULL;
  std::string exist_key("?");
  exist_key.append(key);
  for (uint32_t i = 0; i < bucket->count; i++) {  
    client = &bucket->clients[i];
    ret = client->Get(exist_key, val, len);
    switch (ret) {
      case BCR_SUCCESS:
        if (val) {
          delete val;
          val = NULL;
        }
        return true;
        break;

      case BCR_MEMCACHED_NOTFOUND:
      default:
        break;
    }
  }
  return false;
}

beansdb_client_ret_t BeansdbClient::GetBucket(const char* key,
                                              BeansdbBucket* &bucket,
                                              int& index) const {
  beansdb_client_ret_t ret = BCR_SUCCESS;
  uint32_t bucket_index = 0;
  uint64_t hash = 0;

  hash = CalcHash(key);
  bucket_index = (hash >> 28) & 0x0F;
  if (bucket_index >= bucket_clients_->get_buckets_count()) {
    return BCR_BEANSDB_OUTOFBUCKET;
  }
  bucket = const_cast<BeansdbBucket*>(bucket_clients_->GetBucket(bucket_index));
  if (NULL == bucket) {
    return BCR_BEANSDB_GETBUCKET_FAILED;
  }
  index = hash % bucket->count;
  if (index >= (int)bucket->count) {
    return BCR_BEANSDB_OUTOFCLIENTS;
  }
  return ret;
}

uint64_t BeansdbClient::CalcHash(const char* key) const {
  uint64_t prime = 0x01000193;
  uint64_t h = 0x811c9dc5;
  size_t len = strlen(key);

  for(size_t i = 0; i < len; ++i){
    h ^= key[i];
    h = (h * prime) & 0xffffffff;
  }

  return h;
}

/////////////////////////////// BeansdbClientPool ////////////////////////////

BeansdbClientPool::BeansdbClientPool(const int max_nodes /*= 0xFFFF*/)
  : ComPool<BeansdbClient>(max_nodes) {
  }

BeansdbClientPool::~BeansdbClientPool() {
}

beansdb_client_ret_t BeansdbClientPool::Set(const std::string& key,
                                            const char *val,
                                            const size_t val_len,
                                            const int wait_pool_time /*= 500*/,
                                            const long timeout /*= 0*/) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  BeansdbClient *client = const_cast<BeansdbClient*>(Borrow(wait_pool_time));
  if (client) {
    ret = client->Set(key, val, val_len, timeout);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t BeansdbClientPool::Get(const std::string& key,
                                            char *&val,
                                            size_t &len,
                                            const int wait_pool_time /*= 500*/,
                                            const long timeout /*= 0*/) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  BeansdbClient *client = const_cast<BeansdbClient*>(Borrow(wait_pool_time));
  if (client) {
    ret = client->Get(key, val, len, timeout);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t BeansdbClientPool::Delete(const std::string& key,
                                               const int wait_pool_time /*= 500*/) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  BeansdbClient *client = const_cast<BeansdbClient*>(Borrow(wait_pool_time));
  if (client) {
    ret = client->Delete(key);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t BeansdbClientPool::Exist(const std::string &key,
                                              const int wait_pool_time /*= 500*/) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  BeansdbClient *client = const_cast<BeansdbClient*>(Borrow(wait_pool_time));
  if (client) {
    if (client->Exist(key))
      ret = BCR_SUCCESS;
    else
      ret = BCR_MEMCACHED_NOTFOUND;
    Release(client);
  }
  return ret;
}
