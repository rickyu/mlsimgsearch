#ifndef __BLITZ3_REDIS_POOL_HPP
#define __BLITZ3_REDIS_POOL_HPP

class RedisService {
 public:
  RedisClient* GetClient();
  void ReturnClient(RedisClient* client, bool succeed);
  list<RedisClient*> idle_lists_; /// 空闲列表
  string ip_;
  uint16_t port_;
  int err_count_;
  int req_count_;
  int resp_count_;
};
class RedisPool {
  public:
   shared_ptr<RedisClient> Select(const char* key);
   // 选择多个
   int Select(const char* key, shared_ptr<RedisClient>* clients, int count);
   RedisClient* SelectBatch(const vector<const char*>& keys);
   void Return(RedisClient* client, bool fail);

};
#endif 

