/*
 * file: redis_conn.h
 * description: redis connection object
 * author: kailianzhang@meilishuo.com
 * date: 2012-2-1
 * Copyright (C) Meilishuo Company
 */

#ifndef __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_H_
#define __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_H_

#include "iredis_conn.h"

namespace cache {

class CRedisConn : public IRedisConn {
  public:
    CRedisConn() : redis_(NULL), is_available_(false) {
    }
    virtual ~CRedisConn();

    virtual int32_t Init(const SRedisConf& conf);
    virtual int32_t Connect();
    virtual redisContext* GetRedisConn() const {
      return redis_;
    }
    virtual void* Command(const char* format, va_list arg_list);
    virtual void* Command(const char* format);
    virtual void AppendCommand(const char* format, ...);
    virtual void* CommandArgv(int argc, const char** argv, const size_t* argvlen);
    virtual void AppendCommandArgv(int argc, const char** argv, const size_t* argvlen);
    virtual int GetReply(redisReply** reply);
    virtual std::string GetHost() const;
    virtual uint16_t GetPort() const;
    virtual bool IsWorking();
    virtual int32_t Ping();
    virtual void setAvailable();

  private:
    redisContext* redis_;
    std::string host_;
    uint16_t port_;
    uint32_t conn_timeout_;
    uint32_t opt_timeout_;
    bool is_available_;
};

} //end of namespace cache

#endif // __MIDDLEWARE_LIBSRC_REDISCONNPOOL_REDIS_CONN_H_

