

#include <time.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include "logger.h"
#include "wrapper.h"
#include "mutex.h"
#include <assert.h>

//#include "leakdetect.h"

/// 默认的文件名
#define UNKNOWN_FILE            ((const char *)"UnknownFile")
/// 默认的函数名
#define UNKNOWN_FUN             ((const char *)"UnknownFun")
/// 默认的行号
#define UNKNOWN_LINE            (0)


#undef __ASSERT
#define __ASSERT assert

using namespace std;
using namespace log;

// debug level
const char *CLogger::LOGNAME_IM_DEBUG = "__IM_LOGGER_DEBUG__";
// log level
const char *CLogger::LOGNAME_IM_LOG = "__IM_LOGGER_LOG__";
/// 特殊的logger，分离特殊级别的日志
const char *CLogger::LOGNAME_IM_NOTICE = "__IM_LOGGER_NOTICE__";
/// 特殊的logger，分离特殊级别的日志
const char *CLogger::LOGNAME_IM_ERROR = "__IM_LOGGER_ERROR__";
/// 特殊的logger，分离特殊级别的日志
const char *CLogger::LOGNAME_IM_WARN  = "__IM_LOGGER_WARN__";
/// 特殊的logger，分离特殊级别的日志
const char *CLogger::LOGNAME_IM_FATAL = "__IM_LOGGER_FATAL__";

// 保证getInstance在main之前被初始化，假设main之前没有启动多线程
CLogger& __gImLoggerDebug__ = CLogger::GetInstance(CLogger::LOGNAME_IM_DEBUG);
CLogger& __gImLoggerLog__ = CLogger::GetInstance(CLogger::LOGNAME_IM_LOG);
CLogger& __gImLoggerNotice__ = CLogger::GetInstance(CLogger::LOGNAME_IM_NOTICE);
CLogger& __gImLoggerError__ = CLogger::GetInstance(CLogger::LOGNAME_IM_ERROR);
CLogger& __gImLoggerWarn__  = CLogger::GetInstance(CLogger::LOGNAME_IM_WARN);
CLogger& __gImLoggerFatal__ = CLogger::GetInstance(CLogger::LOGNAME_IM_FATAL);


/// 将不定参数转换为固定参数，调用功能函数
#define SHOW(FILE, FUNC, LINE, MSG, LEVEL)    if( (!m_off) && (m_level <= LEVEL) ) \
                            { \
                                va_list args; \
                                va_start(args, format); \
                                Output(LEVEL, FILE, FUNC, LINE, MSG, format, args); \
                                va_end(args); \
                            }



CLogger::CLogger(const std::string name,int level)
: m_appenderNum(0)
, m_del(false)
, m_name(name)
, m_level(level)
, m_off(false)
, m_detail(true)
{
    for (int i = 0; i < MAXNUM_APPENDER; ++i)
    {
        m_appenders[i] = NULL;
    }
    
    Pthread_mutex_init(&m_cs, NULL);

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
#else
    m_logBuffPool = NULL;
    Pthread_mutex_init(&m_mutexLogBuffPool, NULL);
#endif
}

CLogger::~CLogger(void)
{
    Pthread_mutex_lock(&m_cs);
    //m_appenders.clear();
    Pthread_mutex_unlock(&m_cs);
    Pthread_mutex_destroy(&m_cs);

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
#else
    Pthread_mutex_lock(&m_mutexLogBuffPool);
    //del the buffer pool
    logBuff_t *pDel = NULL;
    while (m_logBuffPool != NULL)
    {
        pDel = m_logBuffPool;
        m_logBuffPool = m_logBuffPool->pNext;
        delete pDel;
        pDel = NULL;
    }
    Pthread_mutex_unlock(&m_mutexLogBuffPool);
    Pthread_mutex_destroy(&m_mutexLogBuffPool);
#endif
    for(unsigned int i = 0; i < m_appenderNum; ++ i){
      if(m_appenders[i] != NULL){
        delete m_appenders[i];
        m_appenders[i] = NULL;
      }
    }
}

class CLoggerMap {
  public:
  CLoggerMap(){
  }
  ~CLoggerMap(){
    using namespace std;
    map<string,CLogger*>::iterator iter = m_loggers.begin();
    map<string,CLogger*>::iterator iter_end = m_loggers.end();
    while(iter != iter_end){
      delete iter->second;
      ++ iter;
    }
    m_loggers.clear();
  }

  CLogger& GetLogger(const string& name,int level);
  public:
    CRWMutex m_locker;
    std::map<std::string, CLogger* > m_loggers;
};

CLogger& CLoggerMap::GetLogger(const string& name,int level){
    using namespace std;
    int ret = 0;
    ret = m_locker.wlock();
    assert(ret == 0);
    CLogger* newLogger = new CLogger(name,level);
    assert(newLogger);
    pair< map<string,CLogger*>::iterator,bool> ins_result = 
      m_loggers.insert( make_pair(name, newLogger) );
    if(!ins_result.second){
      delete newLogger;
      newLogger = ins_result.first->second;
    }
    ret = m_locker.wunlock();
    assert(newLogger);
    return *newLogger;
}


CLogger& CLogger::GetInstance(const string& name, int level)
{
    int ret = 0;
    static CLoggerMap s_logger_map;
    return s_logger_map.GetLogger(name,level);
}

CLogger& CLogger::AttachAppender(CLogAppender* appender)
{
    Pthread_mutex_lock(&m_cs);
    if(appender && m_appenderNum < MAXNUM_APPENDER)
    {
        m_appenders[m_appenderNum++] = appender;
    }
    Pthread_mutex_unlock(&m_cs);
    
    return *this;
}


void CLogger::SetName(const string& name)
{
    m_name = name;
}

CLogger& CLogger::On(void)
{
    m_off = false;
    return *this;
}

CLogger& CLogger::Off(void)
{
    m_off = true;
    return *this;
}

void CLogger::Detail(bool detailSet)
{
    m_detail = detailSet;
}

CLogger& CLogger::SetLevel(int level)
{
    m_off = false;
    m_level = level;
    return *this;
}
CLogger& CLogger::SetLevel(const char* level)
{
    if(strcasecmp(level, "all") == 0)
    {
        return SetLevel(LEVEL_ALL);
    }
    else if(strcasecmp(level, "info-") == 0)
    {
        return SetLevel(LEVEL_INFO - 1);
    }
    else if(strcasecmp(level, "info") == 0)
    {
        return SetLevel(LEVEL_INFO);
    }
    else if(strcasecmp(level, "info+") == 0)
    {
        return SetLevel(LEVEL_INFO + 1);
    }
    else if(strcasecmp(level, "debug-") == 0)
    {
        return SetLevel(LEVEL_DEBUG - 1);
    }
    else if(strcasecmp(level, "debug") == 0)
    {
        return SetLevel(LEVEL_DEBUG);
    }
    else if(strcasecmp(level, "debug+") == 0)
    {
        return SetLevel(LEVEL_DEBUG + 1);
    }
    else if(strcasecmp(level, "log-") == 0)
    {
        return SetLevel(LEVEL_LOG - 1);
    }
    else if(strcasecmp(level, "log") == 0)
    {
        return SetLevel(LEVEL_LOG);
    }
    else if(strcasecmp(level, "log+") == 0)
    {
        return SetLevel(LEVEL_LOG + 1);
    }
    else if(strcasecmp(level, "notice-") == 0)
    {
        return SetLevel(LEVEL_NOTICE - 1);
    }
    else if(strcasecmp(level, "notice") == 0)
    {
        return SetLevel(LEVEL_NOTICE);
    }
    else if(strcasecmp(level, "notice+") == 0)
    {
        return SetLevel(LEVEL_NOTICE + 1);
    }
    else if(strcasecmp(level, "error-") == 0)
    {
        return SetLevel(LEVEL_ERROR - 1);
    }
    else if(strcasecmp(level, "error") == 0)
    {
        return SetLevel(LEVEL_ERROR);
    }
    else if(strcasecmp(level, "error+") == 0)
    {
        return SetLevel(LEVEL_ERROR + 1);
    }
    else if(strcasecmp(level, "warn-") == 0)
    {
        return SetLevel(LEVEL_WARN - 1);
    }
    else if(strcasecmp(level, "warn") == 0)
    {
        return SetLevel(LEVEL_WARN);
    }
    else if(strcasecmp(level, "warn+") == 0)
    {
        return SetLevel(LEVEL_WARN + 1);
    }
    else if(strcasecmp(level, "fatal-") == 0)
    {
        return SetLevel(LEVEL_FATAL - 1);
    }
    else if(strcasecmp(level, "fatal") == 0)
    {
        return SetLevel(LEVEL_FATAL);
    }
    else if(strcasecmp(level, "fatal+") == 0)
    {
        return SetLevel(LEVEL_FATAL + 1);
    }
    else if(strcasecmp(level, "none") == 0)
    {
        return SetLevel(LEVEL_NONE);
    }
    return *this;
}

CLogger& CLogger::LogAll(void)
{
    m_level = LEVEL_ALL;
    m_detail = true;
    return *this;
}

#if (0)
void CLogger::WriteLog(const char* file, const char* func, int line, const char* prefix, 
    const char* format, ...)
{
    va_list args;
    va_start(args, format);
    output(file, func, line, prefix, format, args);
}
#endif

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)

static int              s_logger_tls_err    = 0;
static pthread_key_t    s_logger_tls_key    = PTHREAD_KEYS_MAX;
static pthread_once_t   s_logger_tls_init   = PTHREAD_ONCE_INIT;

void* logger_tls_data_malloc(size_t size)
{
    __ASSERT(size > 0);
    char *pMem = new char[size];
    return pMem;
}

void logger_tls_data_free(void *p)
{
    __ASSERT(p != NULL);
    char *pMem = (char*)p;
    delete []pMem;
}

void logger_tls_data_mk_key(void)
{
    s_logger_tls_err = pthread_key_create(&s_logger_tls_key, logger_tls_data_free);
    if (s_logger_tls_err != 0)
    {
        WRAPPER_FPRINTF(stderr, "pthread_key_create fail! err: %d\n", s_logger_tls_err);
        Fflush(stderr);
        _exit(1);
    }
}

#endif

void CLogger::Output(int level, const char* file, const char* func, int line, const char* prefix, 
    const char* format, va_list args)
{
    log_data_t use;

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
    logger_tls_data_t *store = NULL;
#else
    logBuff_t *msgStore = NULL;
    logBuff_t *logStore = NULL;
#endif

    use.level = prefix;
    use.file = (file != NULL) ? file : UNKNOWN_FILE;
    use.function = (func != NULL) ? func :UNKNOWN_FUN;
    WRAPPER_SNPRINTF(use.lineArr, MAXLEN_LINE, "%d", line);
    use.line = use.lineArr;

    use.name = m_name.c_str();

    //get a msg mem node
#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
    int onceErr = pthread_once(&s_logger_tls_init, logger_tls_data_mk_key);
    if (onceErr != 0)
    {
        WRAPPER_FPRINTF(stderr, "pthread_once fail! err: %d\n", onceErr);
        Fflush(stderr);
        _exit(1);
    }

    store = (logger_tls_data_t *)pthread_getspecific(s_logger_tls_key);
    if (store == NULL)
    {
        store = (logger_tls_data_t *)logger_tls_data_malloc(sizeof(logger_tls_data_t));
        if (store != NULL)
        {
            int setErr = pthread_setspecific(s_logger_tls_key, store);
            if (setErr != 0)
            {
                WRAPPER_FPRINTF(stderr, "pthread_setspecific fail! err: %d\n", setErr);
                Fflush(stderr);
                _exit(1);
            }
            // init TLS mem
            WRAPPER_SNPRINTF(store->pidArr, MAXLEN_PID, "%d", getpid());
            WRAPPER_SNPRINTF(store->tidArr, MAXLEN_TID, "%d", METHOD_GETTID);
            WRAPPER_SNPRINTF(store->ptidArr, MAXLEN_PTID, "%lu", pthread_self());
            memset(store->msgMem, 0, MAXLEN_MSGBUFF);
            memset(store->logMem, 0, MAXLEN_LOGBUFF);
            
            Vsnprintf(store->msgMem, MAXLEN_MSGBUFF - 1, format, args);
            use.msg = store->msgMem;
            use.pid = store->pidArr;
            use.tid = store->tidArr;
            use.ptid = store->ptidArr;
        }
        else
        {
            use.msg = "FATAL: CAN NOT ALLOC MEM FOR MSG[%M]";
            WRAPPER_SNPRINTF(use.pidArr, MAXLEN_PID, "%d", getpid());
            WRAPPER_SNPRINTF(use.tidArr, MAXLEN_TID, "%d", METHOD_GETTID);
            WRAPPER_SNPRINTF(use.ptidArr, MAXLEN_PTID, "%lu", pthread_self());
            use.pid = use.pidArr;
            use.tid = use.tidArr;
            use.ptid = use.ptidArr;
        }
    }
    else
    {
        Vsnprintf(store->msgMem, MAXLEN_MSGBUFF - 1, format, args);
        use.msg = store->msgMem;
        use.pid = store->pidArr;
        use.tid = store->tidArr;
        use.ptid = store->ptidArr;
    }
#else
    Pthread_mutex_lock(&m_mutexLogBuffPool);
    if (m_logBuffPool != NULL)
    {
        msgStore = m_logBuffPool;
        m_logBuffPool = m_logBuffPool->pNext;
        //Note the code order!
        msgStore->pNext = NULL;
    }
    else
    {
        msgStore = new logBuff_t;
    }
    Pthread_mutex_unlock(&m_mutexLogBuffPool);

    if(msgStore != NULL)
    {
        Vsnprintf(msgStore->mem, MAXLEN_LOGBUFF - 1, format, args);
        use.msg = msgStore->mem;
    }
    else
    {
        use.msg = "FATAL: CAN NOT ALLOC MEM FOR MSG[%M]";
    }
    WRAPPER_SNPRINTF(use.pidArr, MAXLEN_PID, "%d", getpid());
    WRAPPER_SNPRINTF(use.tidArr, MAXLEN_TID, "%d", METHOD_GETTID);
    WRAPPER_SNPRINTF(use.ptidArr, MAXLEN_PTID, "%lu", pthread_self());
    use.pid = use.pidArr;
    use.tid = use.tidArr;
    use.ptid = use.ptidArr;
#endif

    GetTimeString(use.timeArr);
    use.time = use.timeArr;


    //get log buffer
#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
    if (store != NULL)
    {
        use.log = store->logMem;

        Pthread_mutex_lock(&m_cs);
        for(unsigned int i = 0; i < m_appenderNum; ++i)
        {
            m_appenders[i]->GetLogData(store->logMem, use);
            m_appenders[i]->Write(store->logMem);
        }
        Pthread_mutex_unlock(&m_cs);

        // 如果级别是特殊的，则追加log到特殊的logger中
        CLogger *pSpecialLogger = NULL;
        if (LEVEL_DEBUG == level)
        {
            pSpecialLogger = &__gImLoggerDebug__;
        }
        else if (LEVEL_LOG == level)
        {
            pSpecialLogger = &__gImLoggerLog__;
        }
        else if (LEVEL_NOTICE == level)
        {
            pSpecialLogger = &__gImLoggerNotice__;
        }
        else if (LEVEL_ERROR == level)
        {
            pSpecialLogger = &__gImLoggerError__;
        }
        else if (LEVEL_WARN == level)
        {
            pSpecialLogger = &__gImLoggerWarn__;
        }
        else if (LEVEL_FATAL == level)
        {
            pSpecialLogger = &__gImLoggerFatal__;
        }

        if (NULL != pSpecialLogger)
        {
            Pthread_mutex_lock(&(pSpecialLogger->m_cs) );
            for(unsigned int i = 0; i < pSpecialLogger->m_appenderNum; ++i)
            {
                pSpecialLogger->m_appenders[i]->GetLogData(store->logMem, use);
                pSpecialLogger->m_appenders[i]->Write(store->logMem);
            }
            Pthread_mutex_unlock(&(pSpecialLogger->m_cs) );
        }
        pSpecialLogger = NULL;
    }
    else
    {
        use.log = "FATAL: CAN NOT ALLOC MEM FOR LOG!!\n";

        Pthread_mutex_lock(&m_cs);
        for(unsigned int i = 0; i < m_appenderNum; ++i)
        {
            m_appenders[i]->Write(use.log);
        }
        Pthread_mutex_unlock(&m_cs);
    }

    //不需要释放TLS内存
#else
    Pthread_mutex_lock(&m_mutexLogBuffPool);
    if (m_logBuffPool != NULL)
    {
        logStore = m_logBuffPool;
        m_logBuffPool = m_logBuffPool->pNext;
        //Note the code order!
        logStore->pNext = NULL;
    }
    else
    {
        logStore = new logBuff_t;
        // 1. Be case the new maybe return NULL!
        // 2. Here CAN NOT return;
        //logStore->pNext = NULL;
    }
    Pthread_mutex_unlock(&m_mutexLogBuffPool);

    if (logStore != NULL)
    {
        use.log = logStore->mem;

        Pthread_mutex_lock(&m_cs);
        for(unsigned int i = 0; i < m_appenderNum; ++i)
        {
            m_appenders[i]->getLogData(logStore->mem, use);
            m_appenders[i]->write(logStore->mem);
        }
        Pthread_mutex_unlock(&m_cs);
    }
    else
    {
        use.log = "FATAL: CAN NOT ALLOC MEM FOR LOG!!\n";

        Pthread_mutex_lock(&m_cs);
        for(unsigned int i = 0; i < m_appenderNum; ++i)
        {
            m_appenders[i]->write(use.log);
        }
        Pthread_mutex_unlock(&m_cs);
    }

    // put the buffer back pool
    Pthread_mutex_lock(&m_mutexLogBuffPool);
    if (msgStore != NULL)
    {
        msgStore->pNext = m_logBuffPool;
        m_logBuffPool = msgStore;
        msgStore = NULL;
    }
    if (logStore != NULL)
    {
        logStore->pNext = m_logBuffPool;
        m_logBuffPool = logStore;
        logStore = NULL;
    }
    Pthread_mutex_unlock(&m_mutexLogBuffPool);
#endif
}


void CLogger::Info(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "INFO", LEVEL_INFO);
}

void CLogger::Debug(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "DEBUG", LEVEL_DEBUG);
}

void CLogger::Log(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "LOG" , LEVEL_LOG);
}

void CLogger::Notice(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "NOTICE" , LEVEL_NOTICE);
}

void CLogger::Error(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "ERROR", LEVEL_ERROR);
}

void CLogger::Warn(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "WARN" , LEVEL_WARN);
}

void CLogger::Fatal(const char* format, ...)
{
    SHOW(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "FATAL", LEVEL_FATAL);
}

void CLogger::Info(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "INFO", LEVEL_INFO);
}

void CLogger::Debug(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "DEBUG", LEVEL_DEBUG);
}

void CLogger::Log(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "LOG" , LEVEL_LOG);
}

void CLogger::Notice(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "NOTICE" , LEVEL_NOTICE);
}

void CLogger::Error(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "ERROR", LEVEL_ERROR);
}

void CLogger::Warn(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "WARN" , LEVEL_WARN);
}

void CLogger::Fatal(const char* file, const char* func, int line, const char* format, ...)
{
    SHOW(file, func, line, "FATAL", LEVEL_FATAL);
}

#if (0)
CLogMsg::CLogMsg(const string& name, int level)
: m_name(name)
, m_level(level)
{
}

CLogMsg::~CLogMsg(void)
{
    CLogger& log = CLogger::GetInstance(m_name);
    switch(m_level)
    {
    case CLogger::LEVEL_DEBUG:
        log.Debug(m_msg.str().c_str());
        break;

    case CLogger::LEVEL_ERROR:
        log.Error(m_msg.str().c_str());
        break;

    case CLogger::LEVEL_FATAL:
        log.Fatal(m_msg.str().c_str());
        break;

    case CLogger::LEVEL_INFO:
        log.Info(m_msg.str().c_str());
        break;

    case CLogger::LEVEL_LOG:
        log.Log(m_msg.str().c_str());
        break;

    default:
        ;
    }
}

void CLogMsg::Append(const char* str, ...)
{
    char buff[4097] = {0};
    va_list args;
    va_start(args, str);
    vsnprintf(buff, 4096, str, args);
    m_msg << buff;
}

#endif



