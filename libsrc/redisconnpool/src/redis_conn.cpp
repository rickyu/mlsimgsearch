#include <cstring>
#include "utils/logger.h"
#include "utils/time_util.h"
#include "cache/redis_conn.h"

using std::string;

namespace cache {

const char* const LOG_LIBSRC_REDIS = "redis";

IRedisConn::~IRedisConn() {
}

CRedisConn::~CRedisConn() {
  if (NULL != redis_) {
    redisFree(redis_);
    redis_ = NULL;
  }
}

int32_t CRedisConn::Init(const SRedisConf& conf) {
  host_ = conf.host_;
  port_ = conf.port_;
  conn_timeout_ = conf.conn_timeout_;
  opt_timeout_ = conf.opt_timeout_;
  return 0;
}

int32_t CRedisConn::Connect() {
	static CLogger& logger = CLogger::GetInstance(LOG_LIBSRC_REDIS);
  if (this->IsWorking()) {
    return 0;
  } else {
    if (NULL != redis_) {
      redisFree(redis_);
      redis_ = NULL;
    }
  }

  /* 防止锁被长期占用, 连接最多重试3次 */
  int try_times = 0;
  uint64_t start_conn = TimeUtil::GetTickUs();
  struct timeval timeout = {conn_timeout_/1000000, conn_timeout_%1000000};
  do {
    redis_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);
    if (NULL == redis_) {
      LOGGER_ERROR(logger, "[attention=redis] [host=%s] [port=%d] [error=fail to connect redis server, return null]", host_.c_str(), port_);
    } else if (redis_->err != 0) {
      LOGGER_ERROR(logger, "[attention=redis] [host=%s] [port=%d] [error=%s]", host_.c_str(), port_, redis_->errstr);
      redisFree(redis_);
      redis_ = NULL;
    } else {
      //设置操作超时时间
      struct timeval timeout2 = {opt_timeout_/1000000, opt_timeout_%1000000};
      redisSetTimeout(redis_, timeout2);
      // 需要先设置超时，否则在redis-server宕机的情况下Ping()会卡住不返回
      if (this->Ping() == -1) {
        redisFree(redis_);
        redis_ = NULL;
        LOGGER_ERROR(logger,"[attention=redis] [host=%s] [port=%d] [error=fail to ping redis server]", host_.c_str(), port_);
      } else {
        is_available_ = true;
        break;
      }
    }
    try_times ++;
  } while(try_times < 4);
  if (is_available_) {
    LOGGER_DEBUG(logger,
        "note=success to connect redis server,host=%s,port=%d,timeconsume=%lu",
        host_.c_str(), port_, TimeUtil::GetTickUs() - start_conn);
    return 0;
  } else {
    return -1;
  }
}

void* CRedisConn::Command(const char* format, va_list arg_list) {
  if (NULL == redis_) {
    is_available_ = false;
    return NULL;
  }
  void* reply = redisvCommand(redis_, format, arg_list);
  if (NULL == reply || REDIS_REPLY_ERROR == ((redisReply*)reply)->type) {
    is_available_ = false;
  }
  return reply;
}

void* CRedisConn::Command(const char* format) {
  if (NULL == redis_) {
    is_available_ = false;
    return NULL;
  }
  void* reply = redisCommand(redis_, format);
  if (NULL == reply || REDIS_REPLY_ERROR == ((redisReply*)reply)->type) {
    is_available_ = false;
  }
  return reply;
}

void CRedisConn::AppendCommand(const char* format, ...) {
  va_list arg_list;
  if (NULL == redis_) {
    is_available_ = false;
    return;
  }
  va_start(arg_list, format);
  redisvAppendCommand(redis_, format, arg_list);
  va_end(arg_list);
}

void* CRedisConn::CommandArgv(int argc,
    const char** argv,
    const size_t* argvlen) {
  if (NULL == redis_) {
    is_available_ = false;
    return NULL;
  }
  void* reply = redisCommandArgv(redis_, argc, argv, argvlen);
  if (NULL == reply || REDIS_REPLY_ERROR == ((redisReply*)reply)->type) {
    is_available_ = false;
  }
  return reply;
}

void CRedisConn::AppendCommandArgv(int argc,
    const char** argv,
    const size_t* argvlen) {
  if (NULL == redis_) {
    is_available_ = false;
    return;
  }
  redisAppendCommandArgv(redis_, argc, argv, argvlen);
}

int CRedisConn::GetReply(redisReply** reply) {
  if (NULL == redis_) {
    is_available_ = false;
    return REDIS_ERR;
  }
  int ret = redisGetReply(redis_, (void**)reply);
  if (REDIS_OK != ret) {
    is_available_ = false;
  }
  return ret;
}

std::string CRedisConn::GetHost() const {
  return host_;
}

uint16_t CRedisConn::GetPort() const {
  return port_;
}

void CRedisConn::setAvailable() {
  is_available_ = true;
}

bool CRedisConn::IsWorking() {
  if (NULL == redis_ || !is_available_) {
    return false;
  }
  return true;
}

int32_t CRedisConn::Ping() {
  if (NULL == redis_) {
    return -1;
  }
  redisReply* reply = (redisReply*)redisCommand(redis_, "PING");
  int ret = 0;
  if (NULL == reply ||
      REDIS_REPLY_STATUS != reply->type ||
      0 != strcmp(reply->str, "PONG")) {
    ret = -1;
  }
  if (NULL != reply) {
    freeReplyObject(reply);
  }
  return ret;
}

} //end of namespace cache

