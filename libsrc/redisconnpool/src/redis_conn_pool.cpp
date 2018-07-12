#include <errno.h>
#include <cstring>
#include <sys/time.h>

#include "cache/iredis_conn_pool.h"
#include "cache/redis_conn_pool.h"
#include "cache/redis_conn.h"
#include "utils/logger.h"
#include "utils/atomic.h"

namespace cache {

  const char* const LOG_LIBSRC_REDIS_POOL = "redis";

  const uint32_t US_PER_NS = 1000;
  const uint32_t MS_PER_US = 1000;
  const uint32_t MS_PER_NS = MS_PER_US * US_PER_NS;
  const uint32_t SEC_PER_MS = 1000;
  const uint32_t SEC_PER_US = SEC_PER_MS * MS_PER_US;
  const uint32_t SEC_PER_NS = SEC_PER_MS * MS_PER_US * US_PER_NS;
  const uint32_t MS100_PER_US = 100 * MS_PER_US;
  const uint32_t MS_DELETE_WAIT = 100;

  static CLogger& logger = CLogger::GetInstance(LOG_LIBSRC_REDIS_POOL, CLogger::LEVEL_DEBUG);

  const int64_t TimeDec(const timeval &left, const timeval &right) {
    timeval tvResult = { 0 };
    int64_t	n64USeconds = 0;
    int64_t n64Seconds = 0;

    n64Seconds = (int64_t)left.tv_sec - (int64_t)right.tv_sec;
    n64USeconds = (int64_t)left.tv_usec - (int64_t)right.tv_usec;

    while (n64Seconds > 0) {
      --n64Seconds;
      n64USeconds += (int64_t)SEC_PER_US;
    }

    if (n64USeconds < 0) {
      return 0;
    }
    return n64USeconds;
  }

  void GenerateSemTime(timespec& to, uint32_t ms) {
    memset( &to, 0, sizeof( to ) );
    clock_gettime(CLOCK_REALTIME, &to);
    to.tv_sec += MS_DELETE_WAIT / SEC_PER_MS;
    to.tv_nsec += (MS_DELETE_WAIT % SEC_PER_MS) * MS_PER_NS;
    if (to.tv_nsec >= SEC_PER_NS) {
      to.tv_nsec -= SEC_PER_NS;
      to.tv_sec++;
    }
  }

  int32_t SemWait(sem_t* sem, uint32_t nSem) {
    for(uint32_t i = 0; i < nSem; ++i) {
      int32_t result = -1;
      timespec to = {0};
      GenerateSemTime( to, MS_DELETE_WAIT );
      do {
        result = sem_timedwait(sem, &to);
      } while(result && EINTR == errno);

      if (0 != result) {
        LOGGER_WARN(logger, "desc=sem_wait_fail nSem=%u errno=%d done=%u",
            nSem, errno, i);
        return i;
      }
    }
    return nSem;
  }

  void SemPost(sem_t* sem, uint32_t nSem) {
    for(uint32_t i = 0; i< nSem; ++i) {
      sem_post(sem);
    }
  }

  IRedisConnPool::~IRedisConnPool() {
  }

  CRedisConnPool::CRedisConnPool() : sem_(NULL)
                                     , ms_(0)
                                     , is_avail_conn_safe_(true)
                                     , retry_worker_(this) {
                                       sem_ = new (std::nothrow) sem_t;
                                       if (NULL != sem_) {
                                         if (0 != sem_init(sem_, 0, 0)) {
                                           delete sem_;
                                           sem_ = NULL;
                                         }
                                       }

                                       Start();
                                       LOGGER_LOG(logger, "desc=construct redis_conn_pool=%p", this);
                                     }

  CRedisConnPool::~CRedisConnPool() {
    Stop();
    if (NULL != sem_) {
      sem_destroy(sem_);
      delete sem_;
    }

    std::set<std::string> to_del;
    {
      CMutexLocker lock(mutex_);
      typeof (avail_conn_.begin()) iter_begin = avail_conn_.begin();
      typeof (avail_conn_.end()) iter_end = avail_conn_.end();

      for (typeof(avail_conn_.begin()) iter = iter_begin; iter != iter_end; ++iter) {
        to_del.insert(iter->first);
      }
    }

    typeof (to_del.begin()) iter_begin = to_del.begin();
    typeof (to_del.end()) iter_end = to_del.end();

    for (typeof(iter_begin) iter = iter_begin; iter != iter_end; ++iter) {
      DelServer(*iter);
    }
    is_avail_conn_safe_ = false;

    LOGGER_LOG(logger, "desc=deconstruct redis_conn_pool=%p", this);
  }

  void CRedisConnPool::Start() {
    retry_worker_.Start();
  }

  void CRedisConnPool::Stop() {
    retry_worker_.Stop();
  }

  int32_t CRedisConnPool::AddServer(const SRedisConf& redis_conf,
      uint32_t conns) {
    std::string conf_key = redis_conf.node_key_;
    if (!is_avail_conn_safe_) {
      LOGGER_ERROR(logger, "desc=avail_conn_unsafe");
      return -1;
    }
    std::map< std::string, std::deque<IRedisConn*> >::iterator iter;
    {
      CMutexLocker lock(mutex_);
      iter = avail_conn_.find(conf_key);
    }
    if (avail_conn_.end() != iter) {
      return AddServer_(redis_conf, conns, avail_conn_[conf_key]);
    } else {
      std::deque<IRedisConn*> new_server;
      uint32_t ret;
      if (0 == (ret = AddServer_(redis_conf, conns, new_server))) {
        CMutexLocker lock(mutex_);
        avail_conn_[conf_key] = new_server;
      }
      return ret;
    }
  }

  int32_t CRedisConnPool::AddServer_(const SRedisConf& redis_conf,
      uint32_t conns,
      std::deque<IRedisConn*>& server) {
    std::string conf = redis_conf.node_key_;
    for (uint32_t i = 0; i < conns; i ++) {
      IRedisConn* conn = new (std::nothrow) CRedisConn;
      if (NULL == conn) {
        LOGGER_FATAL(logger, "desc=fail_malloc");
        return -1;
      }
      conn->Init(redis_conf);

      if (-1 == conn->Connect()) {
        LOGGER_ERROR(logger,
            "desc=fail_connect target=%s conn=%p",
            conn->GetHost().c_str(), conn);
        CMutexLocker lock(mutex_invaild_);
        ShardName2ConnPair invaild_conn_pair;
        invaild_conn_pair.first = conf;
        invaild_conn_pair.second = conn;
        invaild_conn_.push_back(invaild_conn_pair);
      }
      else {
        LOGGER_LOG(logger,
            "desc=connect_success target=%s conn=%p",
            conn->GetHost().c_str(), conn);
        CMutexLocker lock(mutex_);
        server.push_back(conn);

        sem_post(sem_);
        if (logger.CanLog(CLogger::LEVEL_INFO)) {
          int32_t value = 0;
          sem_getvalue(sem_, &value);
          LOGGER_INFO(logger, "desc=sem_post value=%d", value);
        }
      }
    }
    return 0;
  }

  int32_t CRedisConnPool::CheckOneConn(ShardName2ConnPair& conn_pair) {
    IRedisConn* conn = conn_pair.second;
    if (NULL == conn) {
      LOGGER_LOG(logger, "desc=null_conn");
      return SEC_PER_US;
    }

    /* 先判断该结点有没有从map中删除，如果删除，则直接close该连接 */
    {
      CMutexLocker lock(mutex_);
      if (avail_conn_.find(conn_pair.first) == avail_conn_.end()) {
        if (NULL != conn) {
          delete conn;
          conn = NULL;
        }
        return SEC_PER_US; 
      }
    }
    int32_t ret_ping = conn->Ping();
    if (0 != ret_ping) {
      int32_t ret_connect = conn->Connect();
      if (0 != ret_connect) {
        CMutexLocker lock(mutex_invaild_);
        invaild_conn_.push_back(conn_pair);
      } else {
        conn->setAvailable();
        CMutexLocker lock(mutex_);
        avail_conn_[conn_pair.first].push_back(conn);
        sem_post(sem_);
      }
    } else {
      conn->setAvailable();
      CMutexLocker lock(mutex_);
      avail_conn_[conn_pair.first].push_back(conn);
      sem_post(sem_);
    }
    return SEC_PER_US; 
  }

  int32_t CRedisConnPool::DelServer(const std::string& node_desc) {
    if (!is_avail_conn_safe_) {
      LOGGER_ERROR(logger, "desc=avail_conn_unsafe");
      return -1;
    }
    std::map< std::string, std::deque<IRedisConn*> >::iterator iter;
    int ret = 0;
    {
      CMutexLocker lock(mutex_);
      iter = avail_conn_.find(node_desc);
      if (avail_conn_.end() != iter) {
        ret = DelServer_(avail_conn_[node_desc]);
      }
      avail_conn_.erase(node_desc);
    }
    return ret;
  }

  int32_t CRedisConnPool::DelServer_(std::deque<IRedisConn*>& del_server) {
    IRedisConn* conn = NULL;
    while (!del_server.empty()) {
      conn = del_server.front();
      del_server.pop_front();
      if (NULL != conn) {
        delete conn;
        conn = NULL;
      }
    }
    return 0;
  }

  bool CRedisConnPool::IsAvailable() const {
    bool ret = false;
    int32_t value = 0;
    if ((NULL != sem_) && (0 == sem_getvalue(sem_, &value))) {
      ret = (value > 0);
    }
    LOGGER_DEBUG(logger, "ret=%d value=%d", ret, value);
    return ret;
  }

  uint32_t CRedisConnPool::SetTimeout(uint32_t ms) {
    uint32_t ret = ms_;
    ms_ = ms;
    return ret;
  }

  IRedisConn* CRedisConnPool::GetConn(const std::string& redis_key,
      uint32_t ms) {
    IRedisConn* ret = NULL;
    if (0 == ms) {
      ms = ms_;
    }
    std::string conf_str1 = "";
    std::string conf_str2 = "";
    GetStrConf(redis_key, conf_str1, conf_str2);
    //LOGGER_DEBUG(logger, "desc=try_get_connection timeout=%ums", ms);
    int result = 0;
    /*
     * NOTE
     *   whatever the way of get connection, the value of sem
     *   will be decreasesd if the operation succeed.
     */
    if (0 != ms) {
      timespec to = {0};
      GenerateSemTime(to, ms);

      do {
        result = sem_timedwait(sem_, &to);
      } while(result && EINTR == errno);
    }
    else {
      do {
        result = sem_wait(sem_);
      } while(result && EINTR == errno);
    }

    if (0 != result) {
      LOGGER_ERROR(logger, "desc=operation_fail errno=%d", errno);
      sem_post(sem_);
      return ret;
    }

    {
      std::string conf = "";
      CMutexLocker lock(mutex_);
      if (avail_conn_.end() != avail_conn_.find(conf_str1)) {
        conf = conf_str1;
      } else if (avail_conn_.end() != avail_conn_.find(conf_str2)) {
        conf = conf_str2;
      } else {
        LOGGER_ERROR(logger, "conf_str1 is %s, str2 is %s",
            conf_str1.c_str(), conf_str2.c_str());
        return ret;
      }
      if (!avail_conn_[conf].empty()) {
        ret = avail_conn_[conf].front();
        if (NULL != ret) {
          avail_conn_[conf].pop_front();
          LOGGER_DEBUG(logger, "avail_conn[conf] length is %d, conf is %s",
              (int)avail_conn_[conf].size(), conf.c_str());
        }
      }
    }

    /*
     * NOTE
     *   it's odd that get connection failed.
     *   however, the value of sem is decreased before,
     *   get it up anyway...
     */
    if (NULL == ret) {
      LOGGER_WARN(logger, "desc=fail_get_redis_connection");
      sem_post(sem_);
    }
    else {
      LOGGER_DEBUG(logger, "desc=get_conn_success conn=%p", ret);
    }
    return ret;
  }

  IRedisConn* CRedisConnPool::GetConnComm(const std::string& node_desc,
      uint32_t ms) {
    IRedisConn* ret = NULL;
    if (0 == ms) {
      ms = ms_;
    }
    //LOGGER_DEBUG(logger, "desc=try_get_connection timeout=%ums", ms);
    int result = 0;
    /*
     * NOTE
     *   whatever the way of get connection, the value of sem
     *   will be decreasesd if the operation succeed.
     */
    if (0 != ms) {
      timespec to = {0};
      GenerateSemTime(to, ms);

      do {
        result = sem_timedwait(sem_, &to);
      } while(result && EINTR == errno);
    }
    else {
      do {
        result = sem_wait(sem_);
      } while(result && EINTR == errno);
    }

    if (0 != result) {
      LOGGER_ERROR(logger, "desc=operation_fail errno=%d", errno);
      sem_post(sem_);
      return ret;
    }

    {
      CMutexLocker lock(mutex_);
      if (!avail_conn_[node_desc].empty()) {
        ret = avail_conn_[node_desc].front();
        if (NULL != ret) {
          avail_conn_[node_desc].pop_front();
          LOGGER_DEBUG(logger,
              "avail_conn[node_desc] length is %d, node_desc is %s",
              (int)avail_conn_[node_desc].size(), node_desc.c_str());
        }
      }
    }

    /*
     * NOTE
     *   it's odd that get connection failed.
     *   however, the value of sem is decreased before,
     *   get it up anyway...
     */
    if (NULL == ret) {
      LOGGER_WARN(logger, "desc=fail_get_redis_connection");
      sem_post(sem_);
    }
    else {
      LOGGER_DEBUG(logger, "desc=get_conn_success conn=%p", ret);
    }
    return ret;
  }

  void CRedisConnPool::FreeConn(const std::string& redis_key, IRedisConn* conn) {
    if (NULL == conn) {
      LOGGER_ERROR(logger, "free an invalid redis connection");
      return;
    }

    std::string conf_str1 = "";
    std::string conf_str2 = "";
    GetStrConf(redis_key, conf_str1, conf_str2);
    std::string conf = "";
    {
      CMutexLocker lock(mutex_);
      if (avail_conn_.end() != avail_conn_.find(conf_str1)) {
        conf = conf_str1;
      } else if (avail_conn_.end() != avail_conn_.find(conf_str2)) {
        conf = conf_str2;
      } else {
        return;
      }
    }
    if (conn->IsWorking()) {
      CMutexLocker lock(mutex_);
      avail_conn_[conf].push_back(conn);
      sem_post(sem_);
    } else {
      CMutexLocker lock(mutex_invaild_);
      ShardName2ConnPair invaild_conn_pair;
      invaild_conn_pair.first = conf;
      invaild_conn_pair.second = conn;
      invaild_conn_.push_back(invaild_conn_pair);
    }
  }

  void CRedisConnPool::FreeConnComm(const std::string& node_desc, IRedisConn* conn) {
    if (NULL == conn) {
      LOGGER_ERROR(logger, "free an invalid redis connection");
      return;
    }

    {
      CMutexLocker lock(mutex_);
      if (avail_conn_.end() == avail_conn_.find(node_desc)) {
        LOGGER_ERROR(logger, "no queue for node exists,node_desc=%s",
            node_desc.c_str());
        delete conn;
        conn = NULL;
        return;
      }
    }

    if (conn->IsWorking()) {
      CMutexLocker lock(mutex_);
      avail_conn_[node_desc].push_back(conn);
      sem_post(sem_);
    } else {
      CMutexLocker lock(mutex_invaild_);
      ShardName2ConnPair invaild_conn_pair;
      invaild_conn_pair.first = node_desc;
      invaild_conn_pair.second = conn;
      invaild_conn_.push_back(invaild_conn_pair);
    }
  }

  std::string GetConf(const SRedisConf& redis_conf) {
    char port_tmp[32] = {'\0'};
    sprintf(port_tmp, "%u", redis_conf.port_);
    return redis_conf.host_ + ":" + port_tmp;
  }

  int GetStrConf(const std::string& redis_key,
      std::string& conf_str1,
      std::string& conf_str2) {
    std::string flag = ":"; 
    std::string::size_type position = 0; 
    std::string::size_type first_pos = 0; 
    std::string::size_type sec_pos = 0; 
    int i = 0; 
    while((position = redis_key.find_first_of(flag,position)) !=
        std::string::npos && i < 2) {
      if (i == 0) {
        first_pos = position;
      } else if (i == 1) {
        sec_pos = position;
      }      
      position ++;
      i ++;
    }
    if (i == 0) {
      conf_str1 = redis_key;
      conf_str2 = redis_key;
    } else if (i == 1) {
      conf_str1 = redis_key;
      conf_str2 = redis_key.substr(0, first_pos);
    } else if (i == 2) {
      conf_str1 = redis_key.substr(0, first_pos);
      conf_str2 = redis_key.substr(0, sec_pos);
    }
    return 0;
  }

  CRedisConnPool::CRedisConnRetry::CRedisConnRetry(CRedisConnPool* pool)
    : CThread(), pool_(pool) {
    }

  CRedisConnPool::CRedisConnRetry::~CRedisConnRetry() {
  }

  bool CRedisConnPool::CRedisConnRetry::Stop() {
    bool ret = true;
    if (m_bThreadRun) {
      m_bThreadRun = false;
      {
        pthread_join(m_ThreadID, NULL);
      }
    }
    return ret;
  }

  bool CRedisConnPool::CRedisConnRetry::Run() {
    uint32_t to_sleep = 0;
    ShardName2ConnPair pair;
    pair.first = "";
    pair.second = NULL;
    {
      CMutexLocker lock(pool_->mutex_invaild_);
      if (!pool_->invaild_conn_.empty()) {
        pair = pool_->invaild_conn_.front();
        pool_->invaild_conn_.pop_front();
      }
    }
    if (pair.second != NULL) {
      to_sleep = pool_->CheckOneConn(pair);
    }
    if (m_bThreadRun) {
      usleep(to_sleep);
    }
    return true;
  }
  uint32_t CRedisConnPool::GetAvailNum() const {
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_REDIS_POOL);
    int32_t value = 0;
    sem_getvalue( sem_, &value );
    LOGGER_INFO( log, "desc=avail_num value=%d", value );
    if( value < 0 )
    {
      value = 0;
    }
    return value;
  }

} // end of namespace cache

