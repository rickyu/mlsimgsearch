#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <blitz2/logger.h>
#include <blitz2/configure.h>

namespace blitz2
{
/// 获取tid的方法
#define METHOD_GETTID                   ((pid_t)syscall(SYS_gettid))

#define OFFSET(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define GETMEM(S, OFF) (((char *)(S)) + OFF)
/// 格式定义起始    
#ifndef LOG_FMT_BEGIN
#define LOG_FMT_BEGIN() \
  const logappender_format_t LogAppender::kFormatFilters[] = { \
    {const_cast<char*>(""), -1} 
#endif 

/// 定义格式项
#ifndef LOG_FMT_MAP
#define LOG_FMT_MAP(FMT, MEMBER)\
    , {const_cast<char*>(FMT), OFFSET(log_data_t, MEMBER)}
#endif 

/// 格式定义结束
#ifndef LOG_FMT_END
#define LOG_FMT_END() }; \
    const size_t LogAppender::kFormatFiltersCount = \
    sizeof(LogAppender::kFormatFilters) / sizeof(LogAppender::kFormatFilters[0]);
#endif  


using namespace std;
LOG_FMT_BEGIN()
      LOG_FMT_MAP("%L", level)
      LOG_FMT_MAP("%N", name)
      LOG_FMT_MAP("%T", time)
      LOG_FMT_MAP("%F", file)
      LOG_FMT_MAP("%f", function)
      LOG_FMT_MAP("%l", line)
      LOG_FMT_MAP("%P", pid)
      LOG_FMT_MAP("%t", tid)
      LOG_FMT_MAP("%p", ptid)
      LOG_FMT_MAP("%M", msg)
LOG_FMT_END()
LoggerManager::~LoggerManager() {
  for(auto iter=loggers_.begin(); iter!=loggers_.end(); ++iter) {
    delete iter->second;
  }
  loggers_.clear();
  for(auto iter=appenders_.begin(); iter!=appenders_.end(); ++iter) {
    delete iter->second;
  }
  appenders_.clear();
}

LogAppender::LogAppender(const string& format) {
  formats_ = NULL;
  format_count_ = 0;
  ParseFormat(format);
}

LogAppender::~LogAppender(void) {
  ClearFormat();
}

void LogAppender::ClearFormat() {
  if (formats_ != NULL) {
    for (int i = 0; i < format_count_; ++i) {
        delete []formats_[i].name;
    }
    format_count_ = 0;
    delete []formats_;
    formats_ = NULL;
  }
}
void LogAppender::ParseFormat(const string& format) {
  //clear m_format
  ClearFormat();
  vector<string> use;
  boost::split(use, format, boost::is_any_of("%"));
  size_t split_count = use.size();

#define INSERT_FMT_ITEM(STR) do \
    { \
        formats_[format_count_].name = new char[strlen(STR) + 1]; \
        strcpy(formats_[format_count_].name, STR); \
        formats_[format_count_].offset = -1; \
        ++format_count_; \
    } while(0)
    
  if (split_count > 0) {
    //malloc the space
    format_count_ = 0;
    formats_ = new logappender_format_t[split_count * 2 + 1];
    //Insert the string before the first '%'
    INSERT_FMT_ITEM(use[0].c_str());
    for (size_t i = 1; i < split_count; ++i) {
      stringstream tmp;
      if (use[i].length() == 0) {
        INSERT_FMT_ITEM("%");
        i++;
        if(i < split_count) {
          INSERT_FMT_ITEM(use[i].c_str());
        }
      } else {
        tmp << '%' << use[i].at(0);     // [doubhor] one char format control string
        INSERT_FMT_ITEM(tmp.str().c_str());
        INSERT_FMT_ITEM(use[i].substr(1).c_str());
      }
    }
    for (int i = 0; i < format_count_; ++i) {
      GetData(formats_[i]);
    }
    // 整理为交替格式
    logappender_format_t    *temp_formats = formats_;
    int                     temp_formats_count = format_count_;
    std::string             temp_div_string;
    format_count_ = 0;
    formats_ = new logappender_format_t[split_count * 2 + 1];
    for (int i = 0; i < temp_formats_count; ++i) {
      if (temp_formats[i].offset > 0) {
        // insert div string
        formats_[format_count_].name = new char[temp_div_string.length() + 1];
        strcpy(formats_[format_count_].name, temp_div_string.c_str());
        formats_[format_count_].offset = -1 * (int)temp_div_string.length();
        format_count_++;
        //insert format string
        formats_[format_count_].name = new char[strlen(temp_formats[i].name) + 1];
        strcpy(formats_[format_count_].name, temp_formats[i].name);
        formats_[format_count_].offset = temp_formats[i].offset;
        format_count_++;
        //prepare next step
        temp_div_string.clear();
      }
      else {
        temp_div_string += temp_formats[i].name;
      }
    }
    // insert div string last
    formats_[format_count_].name = new char[temp_div_string.length() + 1];
    strcpy(formats_[format_count_].name, temp_div_string.c_str());
    formats_[format_count_].offset = -1 * (int)temp_div_string.length();
    format_count_++;
    //free used mem
    for (int i = 0; i < temp_formats_count; ++i) {
      delete []temp_formats[i].name;
    }
    delete []temp_formats;
    temp_formats = NULL;
  }
}
void LogAppender::GetData(logappender_format_t& format) {
  size_t i = 0;
  for (i = 1; i < kFormatFiltersCount; i++) {
    if (strcmp(format.name, kFormatFilters[i].name) == 0) {
        format.offset = kFormatFilters[i].offset;
        break;
    }
  }
  if (i >= kFormatFiltersCount) {
    format.offset = (-1) * (int)(strlen(format.name));
  }
}

void GetTimeString(char *time_buff) {
  struct timeval tmval;
  gettimeofday(&tmval, NULL);
  //time_t now = time(NULL);
  time_t now = tmval.tv_sec;
  struct tm new_time_obj;
  struct tm *new_time = localtime_r(&now, &new_time_obj);
  // 2007-12-25 01:45:32:123456
  snprintf(time_buff, kMaxTimeStringLen, "%4d-%02d-%02d %02d:%02d:%02d:%06ld"
      , new_time->tm_year + 1900, new_time->tm_mon + 1
      , new_time->tm_mday, new_time->tm_hour
      , new_time->tm_min, new_time->tm_sec, (long int)tmval.tv_usec);
  return;
}

void LogAppender::GetLogData(char *log_buff, const log_data_t& data) {
  int i = 0;
  char *log_ptr = log_buff;
  char *str = NULL;
  long buff_len = kMaxSingleLogLineLen - 2;
  long str_len = 0;
  log_buff[kMaxSingleLogLineLen - 2] = '\n';
  log_buff[kMaxSingleLogLineLen - 1] = '\0';
  for (i = 0; i + 1 < format_count_; i += 2) {
    str = formats_[i].name;
    str_len = -formats_[i].offset;
    str_len = (str_len < buff_len ? str_len : buff_len);
    assert(str_len >= 0);
    memcpy(log_ptr, str, (size_t)str_len );
    buff_len -= str_len;
    log_ptr += str_len;
    str = *(char **)GETMEM(&data, formats_[i+1].offset);
    str_len = (int)strlen(str);
    str_len = (str_len < buff_len ? str_len : buff_len);
    assert(str_len >= 0);
    memcpy(log_ptr, str, (size_t)str_len );
    buff_len -= str_len;
    log_ptr += str_len;
  }
  str = formats_[i].name;
  str_len = -formats_[i].offset;
  str_len = (str_len < buff_len ? str_len : buff_len);
  assert(str_len >= 0);
  memcpy(log_ptr, str, (size_t)str_len );
  buff_len -= str_len;
  log_ptr += str_len;
  // 如果输出的长度较短，按照实际的长度加入字符串结束标记
  if (buff_len > 0) {
    log_ptr[0] = '\n';
    log_ptr[1] = '\0';
  }
}


ConsoleAppender::ConsoleAppender(const  std::string& format)
: LogAppender(format) {
}

ConsoleAppender::~ConsoleAppender(void) {
}

void ConsoleAppender::Write(const char* msg) {
 static boost::mutex console_mutex;
 boost::mutex::scoped_lock locker(console_mutex);
 cout << msg << std::flush;
}

FileAppender::FileAppender(const std::string& file, const std::string& format)
: LogAppender(format) { 
  path_ = file;
  fd_ = -1;
  Rotate();
}

FileAppender::~FileAppender(void) {
  ::close(fd_);
  fd_ = -1;
}

//write() is a public interface function, so need mutex
void FileAppender::Write(const char* msg) {
  ::write(fd_, msg, strlen(msg));
}

void FileAppender::Rotate() {
  //struct timeval tmval;
  //gettimeofday(&tmval, NULL);
  struct tm tm;
  time_t now = time(NULL);
  localtime_r(&now, &tm);
  char str[16];
  snprintf(str, 16, ".%4d%02d%02d%02d", tm.tm_year+1900, tm.tm_mon+1,
      tm.tm_mday, tm.tm_hour);
  cur_path_ = path_ + str;
  if(fd_!=-1) {
    ::close(fd_);
    fd_ = -1;
  }
  fd_ = ::open(cur_path_.c_str(), kFileOpenFlag, kFileCreateMode);
}


Logger::Logger(const std::string& name,int level)
  : name_(name), manager_(LoggerManager::GetInstance()) {
  level_ = level;
  for (size_t i = 0; i < kAppenderMaxCount; ++i) {
    appenders_[i] = NULL;
  }
  appender_count_ = 0;
  InitLogData();
}

Logger::~Logger(void) {
  //appenders_.clear();
  for (size_t i = 0; i < appender_count_; ++ i) {
    if (appenders_[i] != NULL) {
      appenders_[i] = NULL;
    }
  }
}

int Logger::AddAppender(LogAppender* appender) {
  if(appender_set_.find(appender) != appender_set_.end()) {
    return 1;
  }
  if(appender && appender_count_ < kAppenderMaxCount) {
    appenders_[appender_count_++] = appender;
    appender_set_.insert(appender);
    return 0;
  }
  return -1;
}


Logger& Logger::SetLevel(int level) {
    level_ = level;
    return *this;
}
Logger& Logger::SetLevel(const char* level)
{
  if(strcasecmp(level, "all") == 0) {
    return SetLevel(LEVEL_ALL);
  } else if(strcasecmp(level, "trace") == 0) {
    return SetLevel(kLevelTrace);
  } else if(strcasecmp(level, "debug") == 0) {
    return SetLevel(kLevelDebug);
  } else if(strcasecmp(level, "info") == 0) {
    return SetLevel(kLevelInfo);
  } else if(strcasecmp(level, "warn") == 0) {
    return SetLevel(kLevelWarn);
  } else if(strcasecmp(level, "error") == 0) {
    return SetLevel(kLevelError);
  } else if(strcasecmp(level, "fatal") == 0) {
    return SetLevel(kLevelFatal);
  } else if(strcasecmp(level, "none") == 0) {
    return SetLevel(LEVEL_NONE);
  }
  return *this;
}

Logger& Logger::LogAll(void)
{
    level_ = LEVEL_ALL;
    return *this;
}

void Logger::InitLogData() {
  memset(&log_data_, 0, sizeof(log_data_));
  snprintf(log_data_.pid_buf, kMaxPIDLen, "%d", getpid());
  snprintf(log_data_.tid_buf, kMaxTIDLen, "%d", METHOD_GETTID);
  snprintf(log_data_.ptid_buf, kMaxPTIDLen, "%lu", pthread_self());
}
void Logger::Output(int level,
    const char* file,
    const char* func,
    int line,
    const char* prefix, 
    const char* format,
    va_list args) {
  log_data_.level = prefix;
  log_data_.file = (file != NULL) ? file : UNKNOWN_FILE;
  log_data_.function = (func != NULL) ? func :UNKNOWN_FUN;
  snprintf(log_data_.line_buf, kMaxLineNumberLen, "%d", line);
  log_data_.line = log_data_.line_buf;
  log_data_.name = name_.c_str();

  // format user message.好像格式化早了, 可以直接格式化到log buffer中.
  // 对于一条消息，如果只打印一次，可以只格式化一次且不用拷贝.
  vsnprintf(log_data_.msg_buf, kMaxUserMsgLen - 1, format, args);
  log_data_.msg = log_data_.msg_buf;
  log_data_.pid = log_data_.pid_buf;
  log_data_.tid = log_data_.tid_buf;
  log_data_.ptid = log_data_.ptid_buf;
  GetTimeString(log_data_.time_buf);
  log_data_.time = log_data_.time_buf;
  log_data_.log = log_data_.log_buf;
  // 是不是有重复格式化的嫌疑
  for(unsigned int i = 0; i < appender_count_; ++i) {
    appenders_[i]->GetLogData(log_data_.log_buf, log_data_);
    appenders_[i]->Write(log_data_.log_buf);
  }
  LogAppender* error_appender = manager_.error_appender();
  if(level > kLevelInfo && error_appender) {
    error_appender->GetLogData(log_data_.log_buf, log_data_);
    error_appender->Write(log_data_.log_buf);
  }
}

int LoggerManager::InitLogger(const std::string& pre_path,
    const std::string& conf) {
  std::string my_pre_path = pre_path;
  if (pre_path.length() > 0 && pre_path[pre_path.length() - 1] != '/') {
    my_pre_path += "/";
  }
  std::vector<std::string> part;
  boost::split(part, conf, boost::is_any_of(","));
  if (part.size() != 4) {
    last_error_ = "the count of substring after splitted by , must be 4, conf="
      + conf;
    return -1;
  }
  // level[4] = 0
  std::string level = part[3];
  // path[2] = CONSOLE
  std::string appender_name = part[2];
  if (appender_name.empty()) {
    last_error_ = "appender is null conf=" + conf;
    return -2;
  }
  if (appender_name != "CONSOLE") {
    // 如果有前缀，且文件名前面不是以"."或者"/"开头，则加上前缀
    if(my_pre_path.length() != 0 && appender_name[0] != '.' 
        && appender_name[0] != '/') {
      appender_name = my_pre_path + appender_name;
    }
  }
  LogAppender* appender = GetAppender(appender_name);
  if (appender==NULL) {
    if(appender_name ==  "CONSOLE") {
      appender = new ConsoleAppender(part[1]);
    } else {
      appender = new FileAppender(appender_name, part[1]);
    }
    appenders_[appender_name] = appender;
  }
  if (appender) {
    if (part[0] == "__ERROR__") {
      error_appender_ = appender;
    } else {
      // name<0>
      Logger& log = GetLogger(part[0]);
      log.SetLevel(level.c_str());
      log.AddAppender(appender);
    }
  }
  return 0;
}
int LoggerManager::InitLoggers(const std::string& log_dir,
      const std::vector<std::string>& log_conf_strs) {
  for (auto it = log_conf_strs.begin(); it != log_conf_strs.end(); ++it) {
    int ret = InitLogger(log_dir, *it);
    if (ret != 0) {
      return ret;
    }
  }
  return 0;
}
int LoggerManager::InitFromLoggerConf( const string& path ) {
  Configure conf;
  ConfigureLoader loader;
  if ( !loader.LoadFile( conf, path ) ) {
    return -1;
  }
  string log_dir = conf.get( "logger.dir", "./logs" );
  const Configure& subconf = conf.get_child( "logger.appenders", Configure() );
  auto range = subconf.equal_range( "appender" );
  for ( auto it = range.first; it != range.second; ++it ) {
    InitLogger( log_dir, it->second.data() );
  }
  return 0;
}
void LoggerManager::AfterFork() {
  for(auto iter=loggers_.begin(); iter!=loggers_.end(); ++iter) {
    iter->second->InitLogData();
  }

}
} // namespace blitz.

