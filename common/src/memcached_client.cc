#include "beansdb_client/memcached_client.h"
#include <zlib.h>



CLogger& MemcachedClient::logger_ = CLogger::GetInstance(BEANSDB_CLIENT_LOGGER_NAME);
CLogger& MemcachedClientPool::logger_ = CLogger::GetInstance(BEANSDB_CLIENT_LOGGER_NAME);

#define MEMCACHE_COMPRESS_FLAG (2)

//在连接失败n次之后，暂时移除这个server 
#define BEANSDB_REMOVE_FAILED_COUNT 3
//移除后重新尝试的间隔，单位秒
#define BEANSDB_REMOVED_RETRY_INTERVAL 30

int ref = 0;

////////////////////////////////// MemcacheClient //////////////////////////////
MemcachedClient::MemcachedClient()
  : mc_(NULL) {
  }

MemcachedClient::~MemcachedClient() {
  if (mc_) {
    memcached_free(mc_);
    mc_ = NULL;
  }
}

beansdb_client_ret_t MemcachedClient::Init(int hash, int distribution) {
  mc_ = memcached_create(NULL);
  if (!mc_) {
    LOGGER_ERROR(logger_, "memcached_create failed");
    return BCR_MEMCACHED_CREATE_FAILED;
  }

  hash_ = hash;
  distribution_ = distribution;

  return BCR_SUCCESS;
}

beansdb_client_ret_t MemcachedClient::SetAutoReset() {
  memcached_return_t ret;

  ret = memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS, BEANSDB_REMOVE_FAILED_COUNT);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_behavior failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_SET_BEHAVIOR_FAILED;
  }
  ret = memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, BEANSDB_REMOVED_RETRY_INTERVAL);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_behavior failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_SET_BEHAVIOR_FAILED;
  }
 
  return BCR_SUCCESS;
}

beansdb_client_ret_t MemcachedClient::AddServer(const std::string &addr,
                                                const uint32_t port) {
  memcached_return_t ret;
  memcached_server_st *server = NULL;

  server = memcached_server_list_append(NULL, addr.c_str(), port, &ret);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_server_list_append failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_APPENDSERVERLIST_FAILED;
  }
  ret = memcached_server_push(mc_, server);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_server_push failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_PUSHSERVER_FAILED;
  }
  memcached_server_list_free(server);

  ret = memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_HASH, hash_);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_behavior failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_SET_BEHAVIOR_FAILED;
  }
  ret = memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_DISTRIBUTION, distribution_);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_behavior failed, ret=%d, error=%s",
                 ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_SET_BEHAVIOR_FAILED;
  }
  memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, 1);
  return BCR_SUCCESS;
}

beansdb_client_ret_t MemcachedClient::Get(const std::string &key,
                                          char *&val,
                                          size_t &len,
                                          long timeout /* = 0 */) {
  memcached_return_t ret = MEMCACHED_SUCCESS;
  uint32_t flag = 0;
  char *get_val = NULL;
  size_t get_len = 0;
  val = NULL;
  len = 0;

  if (timeout) {
    memcached_behavior_set(mc_, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, timeout);
  }

  get_val = memcached_get(mc_, key.c_str(), key.length(), &get_len, &flag, &ret);
  if (MEMCACHED_SUCCESS == ret) {
    if (MEMCACHE_COMPRESS_FLAG == flag) {
      int status, factor = 1;
      do {
        len = get_len * (1 << factor++);
        val = (char *)realloc(val, len + 1);
        status = uncompress((unsigned char *)val, &len, (unsigned const char *)get_val, get_len);
      } while (status == Z_BUF_ERROR && factor < 16);

      if (get_val) {
        free(get_val);
        get_val = NULL;
      }
      if (status == Z_OK) {
        LOGGER_LOG(logger_, "detect compress flag, uncompress success, "
                   "compressed_len=%ld, uncompressed_len=%ld", get_len, len);
        return BCR_SUCCESS;
      }
      if (val) {
        free(val);
        val = NULL;
        len = 0;
      }
      LOGGER_ERROR(logger_, "detect compress flag, but uncompress failed");
      return BCR_MEMCACHE_UNCOMPRESS_FAILED;
    }
    val = get_val;
    len = get_len;
    return BCR_SUCCESS;
  }
  else if (MEMCACHED_NOTFOUND == ret) {
    return BCR_MEMCACHED_NOTFOUND;
  }
  else {
    LOGGER_ERROR(logger_, "memcached_get key %s failed, ret=%d, error=%s",
                 key.c_str(), ret, memcached_strerror(mc_, ret)); 
    return BCR_MEMCACHED_GET_FAILED;
  }

  // never here
  return BCR_SUCCESS;
}

beansdb_client_ret_t MemcachedClient::Set(const std::string& key,
                                          const char *val,
                                          const size_t val_len,
                                          const long timeout /*=0*/) {
  memcached_return_t ret;

  ret = memcached_set(mc_, key.c_str(), key.length(),
                      val, val_len,
                      timeout, 0);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached set %s failed, ret=%d, error_str=%s",
                 key.c_str(), ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_SET_FAILED;
  }

  return BCR_SUCCESS;
}

beansdb_client_ret_t MemcachedClient::Delete(const std::string& key) {
  memcached_return_t ret;

  ret = memcached_delete(mc_, key.c_str(), key.length(), 0);
  if (MEMCACHED_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "memcached_delete %s failed, ret=%d, error=%s",
                 key.c_str(), ret, memcached_strerror(mc_, ret));
    return BCR_MEMCACHED_DELETE_FAILED;
  }

  return BCR_SUCCESS;
}

bool MemcachedClient::Exist(const std::string &key) {
  return MEMCACHED_SUCCESS == memcached_exist(mc_, key.c_str(), key.length());
}

////////////////////// MemcacheClientPool ///////////////////////////////////
MemcachedClientPool::MemcachedClientPool(const int max_nodes /*= 0xFFFF*/)
  : ComPool<MemcachedClient>(max_nodes) {
  }

MemcachedClientPool::~MemcachedClientPool() {
}

beansdb_client_ret_t MemcachedClientPool::Set(const std::string& key,
                                              const char *val,
                                              const size_t val_size,
                                              const long time /*= 0*/) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  MemcachedClient *client = const_cast<MemcachedClient*>(Borrow());
  if (client) {
    ret = client->Set(key, val, val_size, time);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t MemcachedClientPool::Get(const std::string& key,
                                              char *&val,
                                              size_t &val_size,
                                              long timeout) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  MemcachedClient *client = const_cast<MemcachedClient*>(Borrow());
  if (client) {
    ret = client->Get(key, val, val_size, timeout);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t MemcachedClientPool::Delete(const std::string& key) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  MemcachedClient *client = const_cast<MemcachedClient*>(Borrow());
  if (client) {
    ret = client->Delete(key);
    Release(client);
  }
  return ret;
}

beansdb_client_ret_t MemcachedClientPool::Exist(const std::string &key) {
  beansdb_client_ret_t ret = BCR_POOL_UNAVAILABLE_SOURCE;
  MemcachedClient *client = const_cast<MemcachedClient*>(Borrow());
  if (client) {
    if (client->Exist(key))
      ret = BCR_SUCCESS;
    else
      ret = BCR_MEMCACHED_NOTFOUND;
    Release(client);
  }
  return ret;
}
