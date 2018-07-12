// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// logger, copied from baidu hi. 
#ifndef __BLITZ2_LOGGER_H__
#define __BLITZ2_LOGGER_H__
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/thread.hpp>


namespace blitz2 {
/*
 * 格式说明
 * %L : 输出Level，即 INFO | DEBUG | LOG | ERROR | FATAL
 * %N : 日志的名称，即getInstance()时指定的名称
 * %T : 输出 YYYY-MM-DD HH:MM:SS 格式的日志打印时间
 * %F : 输出日志打印处的文件名
 * %f : 输出日志打印处的函数名
 * %l : 输出日志打印处的行号
 * %P : 输出进程ID
 * %p : 输出pthread_self()得到的线程ID
 * %t : 输出gettid()得到的线程ID
 * %M : 输出调用日志语句中所产生的用户消息
 * %% : 输出百分号'%'
 * 其他所有的文字按照格式中所写的位置输出
 */
  


struct log_data_t;
class LogAppender;
class LoggerManager;

static const int kMaxTimeStringLen    =  32;
static const int  kMaxLineNumberLen   = 16;
static const int kMaxPIDLen  =    16;
static const int kMaxTIDLen  =   16;
static const int kMaxPTIDLen  =   32;
/// 最大的自定义log部分长度
static const int kMaxUserMsgLen  =  (10 * 1024);
/// 单条log最大长度
static const int  kMaxSingleLogLineLen = (12 * 1024);

/// To avoid the ambiguous error, put the data before the pointers.
struct log_data_t {
  char line_buf[kMaxLineNumberLen];
  char time_buf[kMaxTimeStringLen];
  char pid_buf[kMaxPIDLen];
  char tid_buf[kMaxTIDLen];
  char ptid_buf[kMaxPTIDLen];
  char msg_buf[kMaxUserMsgLen];
  char log_buf[kMaxSingleLogLineLen];
  const char *level;
  const char *name;
  const char *file;
  const char *function;
  const char *msg;
  const char *line;
  const char *time;
  const char *pid;
  const char *tid;
  const char *ptid;
  const char *log;
} ;

/// logger类
class Logger {
public:
  friend class LoggerManager;
  static Logger& GetLogger(const std::string& name);
  static const int kLevelTrace = 10;
  static const int kLevelDebug = 20;
  static const int kLevelInfo = 30;
  static const int kLevelWarn = 40;
  static const int kLevelError = 50;
  static const int kLevelFatal = 60;

  static const int LEVEL_ALL = 0;
  static const int LEVEL_NONE = 100;

public:
  void Trace(const char* format, ...)__attribute__((format(printf,2,3)));
  void Debug(const char* format, ...)__attribute__((format(printf,2,3)));
  void Info(const char* format, ...)__attribute__((format(printf,2,3)));
  void Warn(const char* format, ...)__attribute__((format(printf,2,3)));
  void Error(const char* format, ...)__attribute__((format(printf,2,3)));
  void Fatal(const char* format, ...)__attribute__((format(printf,2,3)));

  void Trace(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void Debug(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void Info(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void Warn(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void Error(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void Fatal(const char* file, const char* func, int line,
      const char* format, ...)__attribute__((format(printf,5,6)));
  void InitLogData();
  int AddAppender(LogAppender* appender);
  Logger& SetLevel(int level);
  Logger& SetLevel(const char* level);
  Logger& LogAll(void);
  bool CanLog(int level) const {
    return (level_ <= level);
  }

protected:
  void Output(int level,
      const char* file,
      const char* func,
      int line,
      const char* prefix, 
      const char* format,
      va_list args);
private:
  Logger(const std::string& name,int level = kLevelError);
  ~Logger(void);
  Logger(Logger& log);
  Logger(const Logger& log);
  Logger& operator = (Logger& log);
  Logger& operator = (const Logger& log);
private:
  typedef std::unordered_set<LogAppender*> TAppenderSet;
 private:
  static const size_t kAppenderMaxCount = 30;
  std::string  name_; // name for output and GetLogger.
  LoggerManager& manager_;
  int  level_;
  log_data_t  log_data_;
  TAppenderSet appender_set_; // avoid add appenders repeatedly.
  LogAppender* appenders_[kAppenderMaxCount];
  size_t  appender_count_;
};
  

void GetTimeString(char *);

/// logappender信息节点    
typedef struct {
  char  *name;
  ssize_t offset;
} logappender_format_t;

        

/// log appender接口
class LogAppender {
 public:
  LogAppender(const std::string& format);
  void GetLogData(char *log_buff, const log_data_t& data);
  virtual ~LogAppender(void);
  virtual void Write(const char* msg) = 0;
  virtual void Rotate() {}
  void GetData(logappender_format_t& format);
 protected:
  LogAppender(const LogAppender&);
  LogAppender& operator=(const LogAppender&);
 protected:
  void ParseFormat(const std::string& format);
  void ClearFormat();
 protected:
  //user defined output format
  logappender_format_t* formats_;
  int format_count_;
  //LogAppender's format filter table
  static const logappender_format_t kFormatFilters[];
  static const size_t kFormatFiltersCount;
};
class LoggerManager {
 public:
  static LoggerManager& GetInstance();
 private:
  LoggerManager() {error_appender_ = NULL;}
  ~LoggerManager();
  LoggerManager(const LoggerManager&);
  LoggerManager& operator=(const LoggerManager&);
  static void DeleteFunctor(LoggerManager* obj) {delete obj;}
 public:
  LogAppender* GetAppender(const std::string& name) ;
  Logger& GetLogger(const std::string& name) ;
  /**
   * @param log_dir [IN] - the directory that log files will be placed.
   * @param  conf_str [IN] - the config strings for log.
   *     the format is : log_name,log_format,file_name,log_level
   *     for example : framework,%L %T %N %P:%p %f(%F:%l) - %M,main.log,30
   * @return 0 - success, other fail.
   */
  int InitLogger(const std::string& pre_path, const std::string& conf);
  /**
   * @param log_dir [IN] - the directory that log files will be placed.
   * @param  log_conf_strs [IN] - the config strings for log.
   *     the format is : log_name,log_format,file_name,log_level
   *     for example : framework,%L %T %N %P:%p %f(%F:%l) - %M,main.log,,
   */
  int InitLoggers(const std::string& log_dir,
      const std::vector<std::string>& log_conf_strs);
  
  /**
   * logger 
   * {
   *   dir ./logs
   *   appenders 
   *   {
   *     appender  "framework,%L %T %N %P:%p %f(%F:%l) - %M,main.log,trace"
   *     appender  "xxx,%L %T %N %P:%p %f(%F:%l) - %M,main.log,trace"
   *   }
   * }
   */
  int InitFromLoggerConf( const std::string& path );
  void AfterFork();
  void RotateLogFiles() {
    for(auto iter=appenders_.begin(); iter!=appenders_.end(); ++iter) {
      iter->second->Rotate();
    }
  }
  const std::string& last_error() const { return last_error_;}
  LogAppender* error_appender() { return error_appender_;}
 private:
  typedef std::unordered_map<std::string, Logger*> TLoggerMap;
  typedef std::unordered_map<std::string, LogAppender*> TAppenderMap;
  TLoggerMap loggers_;
  TAppenderMap appenders_;
  std::string last_error_;
  LogAppender* error_appender_;
};
inline LoggerManager& LoggerManager::GetInstance() {
  // every thread own one LoggerManager Instance.
  static boost::thread_specific_ptr<LoggerManager> instance(DeleteFunctor);
  LoggerManager* log_manager = instance.get();
  if(!log_manager) {
    log_manager = new LoggerManager();
    instance.reset(log_manager);
  }
  return *log_manager;
}
inline LogAppender* LoggerManager::GetAppender(const std::string& name) {
  auto iter = appenders_.find(name);
  if(iter == appenders_.end()) {
    return NULL;
  } else {
    return iter->second;
  }
}
inline Logger& LoggerManager::GetLogger(const std::string& name) {
  auto iter = loggers_.find(name);
  if(iter == loggers_.end()) {
    Logger* logger = new Logger(name);
    loggers_[name] = logger;
    return *logger;
  } else {
    return *(iter->second);
  }
}

/// 终端
class ConsoleAppender : public LogAppender
{
public:
    ConsoleAppender(const std::string& format);
    ~ConsoleAppender(void);
    void Write(const char* msg);
};


/// 文件
class FileAppender : public LogAppender {
 public:
  FileAppender(const std::string& file, const std::string& format);
  ~FileAppender(void);
  void Write(const char* msg);
  void Rotate();
 private:
  std::string path_;
  int  fd_;
  std::string cur_path_;
  /// 文件的打开模式 
  static const  int kFileOpenFlag = (O_CREAT | O_WRONLY | O_APPEND);
  /// 新文件的创建模式
  static const unsigned int  kFileCreateMode   = (0644);
};

inline Logger& Logger::GetLogger(const std::string& name) {
  return LoggerManager::GetInstance().GetLogger(name);
}


/// 默认的文件名
static const char*  UNKNOWN_FILE  = "UnknownFile";
/// 默认的函数名
static const char*  UNKNOWN_FUN =  "UnknownFun";
/// 默认的行号
static const int  UNKNOWN_LINE    = 0;


#define __LoggerShow(FILE, FUNC, LINE, MSG, LEVEL) \
              if ( level_ <= LEVEL ) \
              { \
                  va_list args; \
                  va_start(args, format); \
                  Output(LEVEL, FILE, FUNC, LINE, MSG, format, args); \
                  va_end(args); \
              }


inline void Logger::Trace(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "TRACE" , kLevelTrace);
}
inline void Logger::Debug(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "DEBUG", kLevelDebug);
}
inline void Logger::Info(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "INFO", kLevelInfo);
}
inline void Logger::Warn(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "WARN" , kLevelWarn);
}

inline void Logger::Error(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "ERROR", kLevelError);
}

inline void Logger::Fatal(const char* format, ...) {
  __LoggerShow(UNKNOWN_FILE, UNKNOWN_FUN, UNKNOWN_LINE, "FATAL", kLevelFatal);
}

inline void Logger::Trace(const char* file, const char* func, int line,
    const char* format, ...) {
    __LoggerShow(file, func, line, "TRACE" , kLevelTrace);
}

inline void Logger::Debug(const char* file, const char* func, int line,
    const char* format, ...) {
  __LoggerShow(file, func, line, "DEBUG", kLevelDebug);
}

inline void Logger::Info(const char* file, const char* func, int line,
    const char* format, ...) {
  __LoggerShow(file, func, line, "INFO", kLevelInfo);
}
inline void Logger::Warn(const char* file, const char* func, int line,
    const char* format, ...) { 
  __LoggerShow(file, func, line, "WARN" , kLevelWarn);
}


inline void Logger::Error(const char* file, const char* func, int line,
    const char* format, ...) {
  __LoggerShow(file, func, line, "ERROR", kLevelError);
}


inline void Logger::Fatal(const char* file, const char* func, int line,
    const char* format, ...) {
  __LoggerShow(file, func, line, "FATAL", kLevelFatal);
}

/// 简化书写的宏
#define ASSI_INFO __FILE__, __FUNCTION__, __LINE__

/// 辅助log，检测是否可以log
#define LOG_TRACE(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(Logger::kLevelTrace) )\
        {\
            (logRef).Trace(  __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)
/// 辅助log，检测是否可以log
#define LOG_DEBUG(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(Logger::kLevelDebug) )\
        {\
            (logRef).Debug(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

/// 辅助log，检测是否可以log
#define LOG_INFO(logRef,fmt, ...)\
     do\
     {\
         if( (logRef).CanLog(Logger::kLevelInfo) ) \
         {\
             (logRef).Info( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
         }\
     }\
     while(0)

/// 辅助log，检测是否可以log
#define LOG_WARN(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(Logger::kLevelWarn) )\
        {\
            (logRef).Warn(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)


/// 辅助log，检测是否可以log
#define LOG_ERROR(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(Logger::kLevelError) )\
        {\
            (logRef).Error(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)


/// 辅助log，检测是否可以log
#define LOG_FATAL(logRef,fmt, ...)\
    do\
    {\
        if( (logRef).CanLog(Logger::kLevelFatal) )\
        {\
            (logRef).Fatal(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);\
        }\
    }\
    while(0)

} // namespace blitz2
#endif // __BLITZ2_LOGGER_H__
