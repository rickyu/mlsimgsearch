
#include <string.h>
#include "logger_loader.h"
#include "configure.h"
#include "textconfig.h"


/// trace to srdout
#undef TRACE
#define TRACE(fmt, ...)  printf("place=%s:%s:%d "fmt"\n", \
    __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
//#define TRACE(fmt, ...)  do {} while(0)
//#define TRACE(fmt, ...)  (void)0

//-------------------------------------------------------------------------------

/// 如果count不等于0，则最多将字符串截断为count个，否则截取完整，如果不够count个则不管
std::vector<std::string> _logger_config_split(const std::string& src, char det, 
    unsigned int count)
{
  std::vector<std::string> ret;
  std::string::size_type pos = 0;
  std::string::size_type use;
  use = src.find(det);
  while(use != std::string::npos)
  {
    if((count != 0) && (count == (ret.size() + 1)))
    {
      break;
    }
    ret.push_back(src.substr(pos, use - pos));
    pos = use + 1;
    use = src.find(det, pos);
  }
  ret.push_back(src.substr(pos));
  return ret;
}

/// inner use
std::vector<std::string> _logger_config_split(const std::string& src, const std::string& det, 
    unsigned int count)
{
  std::vector<std::string> ret;
  std::string::size_type pos = 0;
  std::string::size_type use;
  std::string::size_type add = det.length();
  use = src.find(det);
  while(use != std::string::npos)
  {
    if((count != 0) && (count == (ret.size() + 1)))
    {
      break;
    }
    ret.push_back(src.substr(pos, use - pos));
    pos = use + add;
    use = src.find(det, pos);
  }
  ret.push_back(src.substr(pos));
  return ret;
}


int InitLogger(const std::string& pre_path,const std::string& conf) {
  std::string my_pre_path = pre_path;
  if (pre_path.length() > 0 && pre_path[pre_path.length() - 1] != '/') {
    my_pre_path += "/";
  }

   std::vector<std::string> part = _logger_config_split(conf, ",");
    // name<0>, fmt<1>, path[2] = CONSOLE, size[3] = 0, level[4] = 0
    if(part.size() < 5)
    {
      return -1;
    }

    // level[4] = 0
    int32_t level = 0;
    if(part[4].length() > 0)
    {
      std::stringstream use(part[4]);
      use >> level;
    }

    // size[3] = 0
    unsigned int divtime = 0;
    if(part[3].length() > 0)
    {
      std::stringstream use(part[3]);
      use >> divtime;
    }

    // name<0>
    CLogger& log = CLogger::GetInstance(part[0]);
    log.SetLevel(level);

    // path[2] = CONSOLE
    CLogAppender* appender = NULL;
    if((part[2].length() == 0) || (strncmp(part[2].c_str(), "CONSOLE", 7) == 0))
    {
      appender = new CConsoleAppender(part[1]);
    }
    else
    {
      {
        std::string logFileName;
        // 如果有前缀，且文件名前面不是以"."或者"/"开头，则加上前缀
        if (my_pre_path.length() != 0 && part[2][0] != '.' && part[2][0] != '/')
        {
          logFileName = my_pre_path + part[2];
        }
        else
        {
          logFileName = part[2];
        }
        //TRACE("act=addFileAppender log=%s file=%s", 
        //    part[0].c_str(), logFileName.c_str());
        appender = new CFileAppender(logFileName, part[1], 0, divtime);
      }
    }
    if(appender)
    {
      log.AttachAppender(appender);
    }
  return 0;

}
/// IMPORTANT: 这里使用不了log
int InitLoggers(const std::string& confFile)
{
  CConfigure<CTextConfig> conf(new CTextConfig(confFile));

  static std::map<std::string, CLogAppender* > log_files;

  std::string logPrePath = conf.GetString("logpath.pre");
  if (logPrePath.length() > 0 && logPrePath[logPrePath.length() - 1] != '/')
  {
    logPrePath += "/";
  }
  //TRACE("act=get_logPrePath prePath=%s", logPrePath.c_str());

  std::vector<std::string>  items = conf.GetStrings("log.config");
  for(size_t i = 0; i < items.size(); ++ i)
  {
    std::vector<std::string> part = _logger_config_split(items[i], ",");
    // name<0>, fmt<1>, path[2] = CONSOLE, size[3] = 0, level[4] = 0
    if(part.size() < 5)
    {
      TRACE("init act=init_logger part=%u value=%s", 
          (uint32_t)part.size(), items[i].c_str());
      return -1;
    }

    // level[4] = 0
    int32_t level = 0;
    if(part[4].length() > 0)
    {
      std::stringstream use(part[4]);
      use >> level;
    }

    // size[3] = 0
    unsigned int  divtime = 0;
    if(part[3].length() > 0)
    {
      std::stringstream use(part[3]);
      use >> divtime;
    }

    // name<0>
    CLogger& log = CLogger::GetInstance(part[0]);
    log.SetLevel(level);

    // path[2] = CONSOLE
    CLogAppender* appender = NULL;
    if((part[2].length() == 0) || (strncmp(part[2].c_str(), "CONSOLE", 7) == 0))
    {
      appender = new CConsoleAppender(part[1]);
    }
    else
    {
      std::map<std::string, CLogAppender* >::iterator iter = log_files.find(part[2]);
      if(iter == log_files.end())
      {
        std::string logFileName;
        // 如果有前缀，且文件名前面不是以"."或者"/"开头，则加上前缀
        if (logPrePath.length() != 0 && part[2][0] != '.' && part[2][0] != '/')
        {
          logFileName = logPrePath + part[2];
        }
        else
        {
          logFileName = part[2];
        }
	//    TRACE("act=addFileAppender log=%s file=%s", 
        //    part[0].c_str(), logFileName.c_str());
        appender = new CFileAppender(logFileName, part[1], 0, divtime);
        log_files.insert(make_pair(part[2], appender));
      }
      else
      {
        appender = iter->second;
      }
    }
    if(appender)
    {
      log.AttachAppender(appender);
    }
  }
  return 0;
}

int ReinitLoggers(const std::string& conf_path) {
  using std::vector;
  using std::string;
  CConfigure<CTextConfig> conf(new CTextConfig(conf_path));
  std::vector<std::string>  items = conf.GetStrings("log.config");
  for(size_t i = 0; i < items.size(); ++ i)
  {
    vector<string> part = _logger_config_split(items[i], ",");
    if(part.size() == 5)
    {
      int32_t level = 0;
      if(part[4].length() > 0)
      {
        std::stringstream use(part[4]);
        use >> level;
      }
      CLogger& log = CLogger::GetInstance(part[0]);
      log.SetLevel(level);
    }
    else
    {
      return -1;
    }

  }
  return 0;
}
