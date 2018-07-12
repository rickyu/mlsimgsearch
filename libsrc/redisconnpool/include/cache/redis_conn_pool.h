/*
 * file: redis_conn_pool.h
 * description: redis connection pool object
 * author: kailianzhang@meilishuo.com
 * date: 2012-2-1
 * Copyright (C) Meilishuo Company
 */

#ifndef __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_POOL_H_
#define __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_POOL_H_

#include <semaphore.h>
#include <deque>
#include <map>
#include <utility>
#include <set>

#include "utils/mutex.h"
#include "utils/thread.h"

#include "cache/iredis_conn.h"
#include "cache/iredis_conn_pool.h"

namespace cache {
class CRedisConnPool : public IRedisConnPool {
  public:
    typedef std::pair< std::string, IRedisConn* > ShardName2ConnPair; 

    CRedisConnPool();
    virtual ~CRedisConnPool();

    virtual void Start();
    virtual void Stop();
    virtual int32_t AddServer(const SRedisConf& redis_conf, uint32_t conns);
    virtual int32_t DelServer(const std::string& node_desc);
    virtual uint32_t SetTimeout(uint32_t ms);
    virtual bool IsAvailable() const;
    //一次提供两个redis key用冒号分隔,优先选取第一个redis key,如果获取失败选取第二个,第二个参数为等待信号量超时时间
    virtual IRedisConn* GetConn(const std::string& redis_key, uint32_t ms);
    //第二个参数为等待信号量超时时间
    virtual IRedisConn* GetConnComm(const std::string& node_desc, uint32_t ms);
    virtual void FreeConn(const std::string& redis_key, IRedisConn* conn);
    virtual void FreeConnComm(const std::string& node_desc, IRedisConn* conn);
    virtual uint32_t GetAvailNum() const;
  private:
    int32_t AddServer_(const SRedisConf& redis_conf, uint32_t conns, std::deque<IRedisConn*>& server);
    int32_t DelServer_(std::deque<IRedisConn*>& del_server);
    int32_t CheckOneConn(ShardName2ConnPair& conn_pair);
  private:
    sem_t* sem_;
    uint32_t ms_;
    CMutex mutex_;
    CMutex mutex_invaild_;

    std::map< std::string, std::deque<IRedisConn*> > avail_conn_;
    std::deque< ShardName2ConnPair > invaild_conn_;
    bool is_avail_conn_safe_;

    class CRedisConnRetry : public CThread {
      public:
        CRedisConnRetry(CRedisConnPool* pool);
        ~CRedisConnRetry();
        bool Run();
        bool Stop();
      private:
        CRedisConnPool* pool_;
    };
    CRedisConnRetry retry_worker_;
};

} //end of namespace cache

#endif // __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_POOL_H_

