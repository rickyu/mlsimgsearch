// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#include "blitz/framework.h"
#include "blitz/redis_pool.h"
namespace blitz {
  RedisPool::RedisPool(Framework& framework,
                       const std::shared_ptr<Selector>& selector)
  : TokenlessServicePool(framework, NULL, selector) {
  decoder_factory_ = &redis_decoder_factory_;
}
int RedisPool::DivideKeys(const std::vector<std::string>& keys, std::map<int,std::vector<int> >& service_to_keyindex, std::map<int, std::vector<std::string> >& service_to_keys) {
  return DivideKeysByService(keys, service_to_keyindex, service_to_keys);
}
int RedisPool::Get(const char* key, int log_id, const RedisRpcCallback& callback, 
                   int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "GET %s", key);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Set(const char* key, const char* value,int log_id, 
    const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "SET %s %s", key, value);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Incrby(const char* key, int offset, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "INCRBY %s %d", key, offset);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand, NULL, timeout);
}
int RedisPool::Setex(const char* key, uint32_t expire_time, const char* value, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "SETEX %s %u %s", key, expire_time, value);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Exists(const char* key, int log_id, 
    const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "EXISTS %s", key);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Expire(const char* key, uint32_t expire_time, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "EXPIRE %s %u", key, expire_time);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Sadd(const char* set, const char* key, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "SADD %s %s", set, key);
  SelectKey select_key;
  select_key.main_key = set;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}
int RedisPool::Zadd(const char* zset, int32_t score, const char* key, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommand(&cmd, "ZADD %s %d %s", zset, score, key);
  SelectKey select_key;
  select_key.main_key = zset;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}


int RedisPool::ZrangeByScore(const char* key, int start, int limit, int direction, int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len;
  if (direction == 0)
    len = redisFormatCommand(&cmd, "ZRANGEBYSCORE %s 0 +inf LIMIT %d %d", key, start, limit);
  else
    len = redisFormatCommand(&cmd, "ZREVRANGEBYSCORE %s +inf 0 LIMIT %d %d", key, start, limit);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}

int RedisPool::CommandArgv(const char* key, int argc, const char** argv, const size_t* argvlen, 
    int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommandArgv(&cmd,argc,argv,argvlen);
  SelectKey select_key;
  select_key.main_key = key;
  select_key.sub_key  = 0;
  for (int i = 0; i < argc; i ++) {
    if (argv[i] != NULL) {
      delete[] argv[i];
      argv[i] = NULL;
    }
  }
  delete[] argv;
  return Command(cmd, len, log_id, callback, &select_key, FreeRedisCommand,
                 NULL, timeout);
}

int RedisPool::CommandArgvWithoutSelect(int service_id, int argc, const char** argv, const size_t* argvlen, 
    int log_id, const RedisRpcCallback& callback, int timeout) {
  char* cmd = NULL;
  int len = redisFormatCommandArgv(&cmd,argc,argv,argvlen);
  for (int i = 0; i < argc; i ++) {
    if (argv[i] != NULL) {
      delete[] argv[i];
      argv[i] = NULL;
    }
  }
  delete[] argv;
  return CommandWithoutSelect(cmd, len, log_id, callback, service_id, FreeRedisCommand,
                 NULL, timeout);
}

int RedisPool::Command(char* command,
                        int len,
                        int log_id,
                        const RedisRpcCallback& callback,
                        const SelectKey* select_key,
                        FnMsgContentFree fn_free,
                        void* free_ctx,
                        int timeout) {
  OutMsg *out_msg = NULL;
  OutMsg::CreateInstance(&out_msg);
  assert(out_msg != NULL);
  out_msg->AppendData(command, len,  fn_free, free_ctx);
  // Query(out_msg, callback, log_id, select_key, 200);
  assert(sizeof(callback.fn_action) <= sizeof(void*));
  void* extra_ptr = reinterpret_cast<void*>(callback.fn_action);
  RpcCallback mycallback;
  mycallback.fn_action = NULL;
  mycallback.user_data = callback.user_data;
  int ret =  RealQuery(out_msg, mycallback, log_id, select_key, timeout,
                       extra_ptr);
  out_msg->Release();
  return ret;
}

int RedisPool::CommandWithoutSelect(char* command,
                        int len,
                        int log_id,
                        const RedisRpcCallback& callback,
                        int service_id,
                        FnMsgContentFree fn_free,
                        void* free_ctx,
                        int timeout) {
  OutMsg *out_msg = NULL;
  OutMsg::CreateInstance(&out_msg);
  assert(out_msg != NULL);
  out_msg->AppendData(command, len,  fn_free, free_ctx);
  // Query(out_msg, callback, log_id, select_key, 200);
  assert(sizeof(callback.fn_action) <= sizeof(void*));
  void* extra_ptr = reinterpret_cast<void*>(callback.fn_action);
  RpcCallback mycallback;
  mycallback.fn_action = NULL;
  mycallback.user_data = callback.user_data;
  int ret =  RealQueryWithoutSelect(out_msg, mycallback, log_id, service_id, timeout,
                       extra_ptr);
  out_msg->Release();
  return ret;
}
  
} // namespace blitz.
