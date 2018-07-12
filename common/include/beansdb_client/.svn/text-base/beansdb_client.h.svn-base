#ifndef IMAGE_STORAGE_CLIENT_BEANSDB_CLIENT_BEANSDB_CLIENT_H_
#define IMAGE_STORAGE_CLIENT_BEANSDB_CLIENT_BEANSDB_CLIENT_H_

#include <stdint.h>
#include <vector>
#include <utils/com_pool.h>
#include <beansdb_client/memcached_client.h>
#include <utils/logger.h>

typedef struct {
  std::string addr;
  uint32_t port;
  int buckets[BEANSDB_MAX_BUCKETS];
  int buckets_count;
} server_addr_t;


typedef struct {
  MemcachedClient *clients;
  uint32_t size;
  uint32_t count;
  std::string addr;
  uint32_t port;
} BeansdbBucket;

/**
 * Beansdb bucket管理器，考虑到libmemcached在节点失效后会自动重连，
 * 所以将此各个客户端设计成单件模式，方便统一管理。
 */
class BeansdbBucketClients {
 public:
  BeansdbBucketClients(int hash, int distribution);
  ~BeansdbBucketClients();

  beansdb_client_ret_t Init(const uint32_t buckets_count = 16,
                            const uint32_t buckets_per_server = 3);
  beansdb_client_ret_t AddServer(const std::string& addr, const uint32_t port,
                                 const int buckets[], const size_t count);
  BeansdbBucket* GetBucket(const uint32_t bucket_index);

  uint32_t get_buckets_count();
  uint32_t get_buckets_per_server();

 private:
  static CLogger &logger_;
  std::vector<BeansdbBucket> mc_clients_;
  uint32_t buckets_count_;
  uint32_t buckets_per_server_;
  int hash_;
  int distribution_;
};

/**
 * Beansdb存储管理接口的实现
 */
class BeansdbClient {
 public:
  BeansdbClient(const BeansdbBucketClients* bucket_clients);
  ~BeansdbClient();

  beansdb_client_ret_t Set(const std::string& key,
                           const char *val,
                           const size_t val_len,
                           const long time = 0);
  beansdb_client_ret_t Get(const std::string& key,
                           char *&val,
                           size_t &len,
                           const long timeout = 0);
  beansdb_client_ret_t Delete(const std::string& key);
  bool Exist(const std::string &key);
  BeansdbBucketClients* GetBucketClients() const { return bucket_clients_; }

 private:
  uint64_t CalcHash(const char* key) const;
  beansdb_client_ret_t GetBucket(const char* key,
                                 BeansdbBucket* &bucket,
                                 int& index) const;

 private:
  BeansdbBucketClients* bucket_clients_;
  static CLogger &logger_;
};

class BeansdbClientPool : public ComPool<BeansdbClient> {
 public:
  BeansdbClientPool(const int max_nodes = 0xFFFF);
  ~BeansdbClientPool();

  beansdb_client_ret_t Set(const std::string& key,
                           const char *val,
                           const size_t val_len,
                           const int wait_pool_time = 0,
                           const long timeout = 0);
  beansdb_client_ret_t Get(const std::string& key,
                           char *&val,
                           size_t &len,
                           const int wait_pool_time = 0,
                           const long timeout = 0);
  beansdb_client_ret_t Delete(const std::string& key,
                              const int wait_pool_time = 0);
  beansdb_client_ret_t Exist(const std::string &key,
                             const int wait_pool_time = 0);
};

#endif
