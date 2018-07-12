#ifndef IMAGE_STORAGE_CLIENT_BEANSDB_CLIENT_MEMCACHED_CLIENT_H_
#define IMAGE_STORAGE_CLIENT_BEANSDB_CLIENT_MEMCACHED_CLIENT_H_

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <libmemcached/memcached.h>
#include <utils/com_pool.h>
#include <beansdb_client/constant.h>
#include <utils/logger.h>

class MemcachedClient {
 public:
  MemcachedClient();
  ~MemcachedClient();

  beansdb_client_ret_t Init(int hash, int distribution);
  beansdb_client_ret_t AddServer(const std::string &addr,
                                 const uint32_t port);
  //设置beansdb的自动失败摘除和自动重试
  beansdb_client_ret_t SetAutoReset();
  beansdb_client_ret_t Set(const std::string& key,
                           const char *val,
                           const size_t val_size,
                           const long time = 0);
  beansdb_client_ret_t Get(const std::string& key,
                           char *&val,
                           size_t &val_size,
                           long timeout = 0/* us */);
  beansdb_client_ret_t Delete(const std::string& key);
  bool Exist(const std::string &key);

 private:
  static CLogger &logger_;
  memcached_st* mc_;
  int hash_;
  int distribution_;
};

//typedef ComPool<MemcachedClient> MemcachedClientPool;

class MemcachedClientPool : public ComPool<MemcachedClient> {
 public:
  MemcachedClientPool(const int max_nodes = 0xFFFF);
  ~MemcachedClientPool();

  beansdb_client_ret_t Set(const std::string& key,
                           const char *val,
                           const size_t val_size,
                           const long time = 0);
  beansdb_client_ret_t Get(const std::string& key,
                           char *&val,
                           size_t &val_size,
                           long timeout = 0);
  beansdb_client_ret_t Delete(const std::string& key);
  beansdb_client_ret_t Exist(const std::string &key);

 private:
  static CLogger &logger_;
};

#endif
