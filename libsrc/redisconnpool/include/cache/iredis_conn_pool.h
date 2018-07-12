/*
 * file: iredis_conn_pool.h
 * description: redis connection pool object interface
 * author: kailianzhang@meilishuo.com
 * date: 2012-2-1
 * Copyright (C) Meilishuo Company
 */

#ifndef __MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_POOL_H_
#define __MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_POOL_H_

#include <stdint.h>
#include <string>
#include "utils/iref.h"
#include "iredis_conn.h"

namespace cache {

// implement in redis_conn_pool.cpp
std::string GetConf(const SRedisConf& redis_conf);
int GetStrConf(const std::string& redis_key, std::string& conf_str1, std::string& conf_str2);
class IRedisConnPool : public IRef {
  public:
    virtual ~IRedisConnPool() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual int32_t AddServer(const SRedisConf& redis_conf, uint32_t conns) = 0;
    virtual int32_t DelServer(const std::string& node_desc) = 0;
    virtual uint32_t SetTimeout(uint32_t ms) = 0;
    virtual bool IsAvailable() const = 0;
    virtual IRedisConn* GetConn(const std::string& redis_key, uint32_t ms) = 0;
    virtual IRedisConn* GetConnComm(const std::string& node_desc, uint32_t ms) = 0;
    virtual void FreeConn(const std::string& redis_key, IRedisConn* conn) = 0;
    virtual void FreeConnComm(const std::string& node_desc, IRedisConn* conn) = 0;
    virtual uint32_t GetAvailNum() const = 0;
};

} //end of namespace cache

#endif //__MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_POOL_H_

