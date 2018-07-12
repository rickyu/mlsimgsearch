#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include "logappender.h"
/// 一个logger可以拥有的logappender的数目上限
#define MAXNUM_APPENDER			(10)

/// 获取tid的方法
#define METHOD_GETTID 			((pid_t)syscall(SYS_gettid))

/// 重要: 这个选择被禁止时对应的代码已经不再被维护，请不要禁止这个开关
#define LOGBUFF_USE_TLS			(1)

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)

/// tls属性
typedef struct _logger_tls_data_t
{
    char 				pidArr[MAXLEN_PID];
	char 				tidArr[MAXLEN_TID];
    char 				ptidArr[MAXLEN_PTID];
	char				msgMem[MAXLEN_MSGBUFF];
	char				logMem[MAXLEN_LOGBUFF];
} logger_tls_data_t;

#endif

/// logger接口类
class CLogger
{
public:
    static const int LEVEL_ALL = 0;
    static const int LEVEL_INFO = 10;
    static const int LEVEL_DEBUG = 20;
    static const int LEVEL_LOG = 30;
    static const int LEVEL_NOTICE = 35;
    static const int LEVEL_ERROR = 40;
    static const int LEVEL_WARN = 45;
    static const int LEVEL_FATAL = 50;
    static const int LEVEL_NONE = 100;

    static const char *LOGNAME_IM_DEBUG;
    static const char *LOGNAME_IM_LOG;
    static const char *LOGNAME_IM_NOTICE;
    static const char *LOGNAME_IM_ERROR;
    static const char *LOGNAME_IM_WARN;
    static const char *LOGNAME_IM_FATAL;

private:
    CLogAppender* 	m_appenders[MAXNUM_APPENDER];
	unsigned int	m_appenderNum;
    bool            m_del;
    std::string     m_name;
    int             m_level;
    bool            m_off;
    bool            m_detail;

    pthread_mutex_t m_cs;

#if (defined(LOGBUFF_USE_TLS) && LOGBUFF_USE_TLS != 0)
#else
    logBuff_t       *m_logBuffPool;
    pthread_mutex_t m_mutexLogBuffPool;
#endif

public:
    CLogger(const std::string name,int level = LEVEL_ERROR);
    virtual ~CLogger(void);
    static CLogger& GetInstance(const std::string& name, int level = LEVEL_ERROR);
    CLogger& AttachAppender(CLogAppender* appender);

    CLogger& On(void);
    CLogger& Off(void);
    void Detail(bool detail);
    CLogger& SetLevel(int level);
    CLogger& SetLevel(const char* level);
    CLogger& LogAll(void);
    bool CanLog(int level) const
	{
    	return ((!m_off) && (m_level <= level));
	}
    virtual void Info(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Debug(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Log(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Notice(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Error(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Warn(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Fatal(const char* format, ...)__attribute__((format(printf,2,3)));
    virtual void Info(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Debug(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Log(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Notice(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Error(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Warn(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    virtual void Fatal(const char* file, const char* func, int line, const char* format, ...)__attribute__((format(printf,5,6)));
    /*
    void write_log(const char* file, const char* func, int line, const char* prefix, 
        const char* format, ...);
    */

protected:
    void Output(int level, const char* file, const char* func, int line, const char* prefix, 
        const char* format, va_list args);
    const char* FormatTime(void) const;
    void SetName(const std::string& name);
private:
    CLogger(CLogger& log);
    CLogger(const CLogger& log);
    CLogger& operator = (CLogger& log);
    CLogger& operator = (const CLogger& log);
};

#if (0)
class CLogMsg
{
private:
    std::stringstream m_msg;
    std::string m_name;
    int m_level;

public:
    CLogMsg(const std::string& name = "Log", int level = CLogger::LEVEL_DEBUG);
    ~CLogMsg(void);
    void Append(const char* str, ...);
};

#endif

/// 简化书写的宏
#define ASSI_INFO __FILE__, __FUNCTION__, __LINE__

/// 辅助函数，使用指针打log
#define PTRLOGGER_INFO(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_INFO) ) \
    { \
	    (_inner_logger_ptr)->Info( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_DEBUG(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_DEBUG) ) \
    { \
	    (_inner_logger_ptr)->Debug( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_LOG(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_LOG) ) \
    { \
	    (_inner_logger_ptr)->Log( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_NOTICE(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_NOTICE) ) \
    { \
	    (_inner_logger_ptr)->Notice( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_ERROR(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_ERROR) ) \
    { \
	    (_inner_logger_ptr)->Error( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_WARN(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_WARN) ) \
    { \
	    (_inner_logger_ptr)->Warn( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助函数，使用指针打log
#define PTRLOGGER_FATAL(logPtr, fmt, ...) \
do \
{ \
    CLogger *_inner_logger_ptr = (logPtr); \
	if( (_inner_logger_ptr) && (_inner_logger_ptr)->CanLog(CLogger::LEVEL_FATAL) ) \
    { \
	    (_inner_logger_ptr)->Fatal( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } \
} while(0)

/// 辅助log，检测是否可以log
#define LOGGER_INFO(logRef,fmt, ...)\
     do\
     {\
         if( (logRef).CanLog(CLogger::LEVEL_INFO) ) \
         {\
             (logRef).Info( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
         }\
     }\
     while(0)

/// 辅助log，检测是否可以log
#define LOGGER_DEBUG(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_DEBUG) )\
        {\
            (logRef).Debug(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOGGER_LOG(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_LOG) )\
        {\
            (logRef).Log(  __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOGGER_NOTICE(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_NOTICE) )\
        {\
            (logRef).Notice(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOGGER_ERROR(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_ERROR) )\
        {\
            (logRef).Error(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOGGER_WARN(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_WARN) )\
        {\
            (logRef).Warn(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOGGER_FATAL(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(CLogger::LEVEL_FATAL) )\
        {\
            (logRef).Fatal(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)


#endif // __LOGGER_H__
