/*
 * file: iredis_conn.h
 * description: redis connection object interface
 * author: kailianzhang@meilishuo.com
 * date: 2012-2-1
 * Copyright (C) Meilishuo Company
 */

#ifndef __MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_H_
#define __MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_H_

#include <stdint.h>
#include <string>
#include "utils/iref.h"
#include "hiredis.h"

namespace cache {
/*
 * description: a structure to describe redis config
 */
struct SRedisConf {
  std::string host_;
  uint16_t port_;
  uint32_t conn_timeout_;
  uint32_t opt_timeout_;
  std::string node_key_;
};

class IRedisConn : public IRef {
  public:
    virtual ~IRedisConn() = 0;
    virtual int32_t Init(const SRedisConf& conf) = 0;
    virtual int32_t Connect() = 0;
    virtual redisContext* GetRedisConn() const = 0;
    virtual void* Command(const char* format, va_list arg_list) = 0;
    virtual void* Command(const char* format) = 0;
    virtual void AppendCommand(const char* format, ...) = 0;
    virtual void* CommandArgv(int argc, const char** argv, const size_t* argvlen) = 0;
    virtual void AppendCommandArgv(int argc, const char** argv, const size_t* argvlen) = 0;
    virtual int GetReply(redisReply** reply) = 0;
    virtual std::string GetHost() const = 0;
    virtual uint16_t GetPort() const = 0;
    virtual bool IsWorking() = 0;
    virtual int32_t Ping() = 0;
    //设置连接可用
    virtual void setAvailable() = 0;
};

}//end of namespace cache

#endif //__MIDDLEWARE_LIBSRC_REDISCONNPOOL_IREDIS_CONN_H_

