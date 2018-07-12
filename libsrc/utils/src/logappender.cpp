
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <assert.h>


#include <logappender.h>
//#include "misc.h"

#include "mutex.h"


//#include "commondef.h"
#include "wrapper.h"
#include "strutil.h"


#undef __ASSERT
#define __ASSERT assert

using namespace std;
using namespace log;



#if (__WORDSIZE == 64)
    #define OFFSET(S, M) ((long int)(&(((S *)0)->M)))
#else
    #define OFFSET(S, M) ((int)(&(((S *)0)->M)))
#endif
#define GETMEM(S, OFF) (((char *)(S)) + OFF)
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
    
CLogAppender::CLogAppender(const string& format)
{
    m_format = NULL;
    m_formatNum = 0;
    ParseFormat(format);
    GetTimeString(m_timeArr);
}

CLogAppender::~CLogAppender(void)
{
    //clear m_format
    if (m_format != NULL)
    {
        for (int i = 0; i < m_formatNum; i++)
        {
            delete []m_format[i].fmtName;
        }
        m_formatNum = 0;
        delete []m_format;
        m_format = NULL;
    }
}

void CLogAppender::ParseFormat(const string& format)
{
    //clear m_format
    if (m_format != NULL)
    {
        for (int i = 0; i < m_formatNum; i++)
        {
            delete []m_format[i].fmtName;
        }
        m_formatNum = 0;
        delete []m_format;
        m_format = NULL;
    }
    
    vector<string> use = split(format, '%');
    size_t countFmt = use.size();

#define INSERT_FMT_ITEM(STR) do \
    { \
        m_format[m_formatNum].fmtName = new char[strlen(STR) + 1]; \
        strcpy(m_format[m_formatNum].fmtName, STR); \
        m_format[m_formatNum].offset = -1; \
        m_formatNum++; \
    } while(0)
    
    if(countFmt > 0)
    {
        //malloc the space
        m_formatNum = 0;
        m_format = new logAppender_format_t[countFmt * 2 + 1];
 
        //Insert the string before the first '%'
        INSERT_FMT_ITEM(use[0].c_str());

        for(size_t i = 1; i < countFmt; i++)
        {
            stringstream tmp;
            if(use[i].length() == 0)
            {
                INSERT_FMT_ITEM("%");
                i++;
                if(i < countFmt)
                {
                    INSERT_FMT_ITEM(use[i].c_str());
                }
            }
            else
            {
                tmp << '%' << use[i].at(0);     // [doubhor] one char format control string
                INSERT_FMT_ITEM(tmp.str().c_str());
                INSERT_FMT_ITEM(use[i].substr(1).c_str());
            }
        }

        for (int i = 0; i < m_formatNum; i++)
        {
            GetData(m_format[i]);
        }

        // 整理为交替格式
        logAppender_format_t    *pTmpFmt = m_format;
        int                     tmpFmtNum = m_formatNum;
        std::string             tmpDivString;
        
        m_formatNum = 0;
        m_format = new logAppender_format_t[countFmt * 2 + 1];
        for (int i = 0; i < tmpFmtNum; ++i)
        {
            if (pTmpFmt[i].offset > 0)
            {
                // insert div string
                m_format[m_formatNum].fmtName = new char[tmpDivString.length() + 1];
                strcpy(m_format[m_formatNum].fmtName, tmpDivString.c_str());
                m_format[m_formatNum].offset = -1 * (int)tmpDivString.length();
                m_formatNum++;

                //insert format string
                m_format[m_formatNum].fmtName = new char[strlen(pTmpFmt[i].fmtName) + 1];
                strcpy(m_format[m_formatNum].fmtName, pTmpFmt[i].fmtName);
                m_format[m_formatNum].offset = pTmpFmt[i].offset;
                m_formatNum++;

                //prepare next step
                tmpDivString.clear();
            }
            else
            {
                tmpDivString += pTmpFmt[i].fmtName;
            }
        }
        
        // insert div string last
        m_format[m_formatNum].fmtName = new char[tmpDivString.length() + 1];
        strcpy(m_format[m_formatNum].fmtName, tmpDivString.c_str());
        m_format[m_formatNum].offset = -1 * (int)tmpDivString.length();
        m_formatNum++;

        //free used mem
        for (int i = 0; i < tmpFmtNum; ++i)
        {
            delete []pTmpFmt[i].fmtName;
        }
        delete []pTmpFmt;
        pTmpFmt = NULL;
    }
}
int CLogAppender::GetData(logAppender_format_t& outFmt)
{
    int ret = 0;
    int i = 0;

    for (i = 1; i < m_fmtNum; i++)
    {
        if (strcmp(outFmt.fmtName, m_fmtFilter[i].fmtName) == 0)
        {
            outFmt.offset = m_fmtFilter[i].offset;
            break;
        }
    }
    if (i >= m_fmtNum)
    {
        outFmt.offset = (-1) * (int)(strlen(outFmt.fmtName));
    }

return ret;
}

void GetTimeString(char *timeBuff)
{
    struct timeval tmval;
    Gettimeofday(&tmval, NULL);
    //time_t now = time(NULL);
    time_t now = tmval.tv_sec;
    struct tm newTimeObj;
    struct tm *newTime = localtime_r(&now, &newTimeObj);
    // 2007-12-25 01:45:32:123456
    WRAPPER_SNPRINTF(timeBuff, MAXLEN_TIME, "%4d-%02d-%02d %02d:%02d:%02d:%06ld"
        , newTime->tm_year + 1900, newTime->tm_mon + 1
        , newTime->tm_mday, newTime->tm_hour
        , newTime->tm_min, newTime->tm_sec, (long int)tmval.tv_usec);
    return;
}

void CLogAppender::GetLogData(char *logBuff, log_data_t& data)
{
    int i = 0;
    char *pLog = logBuff, *pStr = NULL;
    long iLenBuff = MAXLEN_LOGBUFF - 2;
   long  iLenStr = 0;

    // 保存时间字符串以供后面切分文件使用
    memcpy(m_timeArr, data.timeArr, MAXLEN_TIME);

    logBuff[MAXLEN_LOGBUFF - 2] = '\n';
    logBuff[MAXLEN_LOGBUFF - 1] = '\0';

    for (i = 0; i + 1 < m_formatNum; i += 2)
    {
        pStr = m_format[i].fmtName;
        iLenStr = -m_format[i].offset;
        iLenStr = (iLenStr < iLenBuff ? iLenStr : iLenBuff);
        __ASSERT(iLenStr >= 0);
        memcpy(pLog, pStr, (size_t)iLenStr );
        iLenBuff -= iLenStr;
        pLog += iLenStr;
        pStr = *(char **)GETMEM(&data, m_format[i+1].offset);
        iLenStr = (int)strlen(pStr);
        iLenStr = (iLenStr < iLenBuff ? iLenStr : iLenBuff);
        __ASSERT(iLenStr >= 0);
        memcpy(pLog, pStr, (size_t)iLenStr );
        iLenBuff -= iLenStr;
        pLog += iLenStr;
    }
    pStr = m_format[i].fmtName;
    iLenStr = -m_format[i].offset;
    iLenStr = (iLenStr < iLenBuff ? iLenStr : iLenBuff);
    __ASSERT(iLenStr >= 0);
    memcpy(pLog, pStr, (size_t)iLenStr );
    iLenBuff -= iLenStr;
    pLog += iLenStr;

    // 如果输出的长度较短，按照实际的长度加入字符串结束标记
    if (iLenBuff > 0)
    {
        memcpy(pLog, "\n\0", 2);
    }
}

#if (0)
LogAppender& operator << (LogAppender& appender, const log_data_t& data)
{
    appender.write(data.log);
    return appender;
}
#endif

static CMutex g_consoleMutex;

CConsoleAppender::CConsoleAppender(const  std::string& format)
: CLogAppender(format)
{
}

CConsoleAppender::~CConsoleAppender(void)
{
}

void CConsoleAppender::Write(const char* msg)
{
    CMutexLocker locker(g_consoleMutex);
    cout << msg << std::flush;
}

/*-----------------------------------------------------------------------------------------------
大小:   0:使用默认大小；    非0:指定大小，不过会检查范围，如果太离谱，会取接近的合法值
时间:   0:不按照时间切分；  非0:按照指定的小时数切分，要求在范围(0, 24]内且能够整除24
-----------------------------------------------------------------------------------------------*/
CFileAppender::CFileAppender(const std::string& file, const std::string& format, 
    uint64_t size, unsigned int divTime/* = 0 */)
: CLogAppender(format)
, m_pathLen(0)
, m_pathFileNameLen(0)
, m_divTime(divTime)
, m_fileExpUid(0)
, m_maxSize(size)
, m_actSize(0)
, m_fd(-1)
{
    Pthread_mutex_init(&m_mutex, NULL);
    // 记录带目录的文件名
    m_pathFileNameLen = file.length();
    m_pathFileNameLen = (m_pathFileNameLen < sizeof(m_pathFileName)) ? 
        (m_pathFileNameLen) : (sizeof(m_pathFileName) - 1);
    memset(m_pathFileName, 0, sizeof(m_pathFileName));
    memcpy(m_pathFileName, file.c_str(), m_pathFileNameLen);
    
    //获取文件的原始大小
    struct stat st;
    if (stat(m_pathFileName, &st) == -1)
    {
        //printf("errno: %d\n", errno); // errno == ENOENT
        //perror("statFail: ");
        m_actSize = 0;
    }
    else
    {
        m_actSize = (uint64_t)st.st_size;
    }
    
    // 检查文件大小切分机制,目前会强制使用大小切分方法
    if (m_maxSize == 0)
    {
        // 如果不指定切分文件大小，使用默认的文件大小
        m_maxSize = LOG_DEF_FILESIZE;
    }
    else if (m_maxSize < LOG_MIN_FILESIZE)
    {
        m_maxSize = LOG_MIN_FILESIZE;
    }
    else if (m_maxSize > LOG_MAX_FILESIZE)
    {
        m_maxSize = LOG_MAX_FILESIZE;
    }

    // 检查时间切分机制
    if (m_divTime > 0 && (24 % m_divTime != 0) )
    {
        m_divTime = 24 / (24 / m_divTime + 1);
    }
    if (m_divTime > 0)
    {
        for (unsigned int i = 0; i < sizeof(m_hourArr) / sizeof(m_hourArr[0]); ++i)
        {
            m_hourArr[i] = i - i % m_divTime;
        }
    }
    else
    {
        for (unsigned int i = 0; i < sizeof(m_hourArr) / sizeof(m_hourArr[0]); ++i)
        {
            m_hourArr[i] = i;
        }
    }

    // 初始化文件相关信息
    memset(m_divFileName, 0, sizeof(m_divFileName));
    //getTimeFileName(m_divFileName);
    {
        unsigned int hour = 0;
        // 12345678901234567890123456789012
        // 2007-12-25 01:45:32:123456
        // 2007-12-25_01
        memcpy(m_divFileName, m_timeArr, 13);
        m_divFileName[10] = '_';
        m_divFileName[13] = '\0';
        if (m_divTime > 0)
        {
            sscanf(m_divFileName + 11, "%u", &hour);
            WRAPPER_SNPRINTF(m_divFileName + 11, MAXLEN_TIME_FILE_SUFFIX - 11, "%02d", 
                m_hourArr[hour]);
        }
    }

    // 取出可能存在的路径名称
    memset(m_path, 0, sizeof(m_path));
    size_t pos = file.rfind("/");
    if(pos != string::npos)
    {
        std::string path = file.substr(0, pos);
        m_pathLen = path.length();
        m_pathLen = (m_pathLen < sizeof(m_path)) ? (m_pathLen) : (sizeof(m_path) - 1);
        memcpy(m_path, path.c_str(), m_pathLen);
    }

    // 打开文件
    m_fd = open(m_pathFileName, LOG_OPENFLAG, LOG_CREATEMODE);
    if (m_fd == -1)
    {
        // 如果打开文件失败，尝试创建单级缺少的目录后重新打开
        if (m_pathLen > 0)
        {
            Mkdir(m_path, 0777);
        }
        m_fd = open(m_pathFileName, LOG_OPENFLAG, LOG_CREATEMODE);
    }
}

CFileAppender::~CFileAppender(void)
{
    close(m_fd);
    Pthread_mutex_destroy(&m_mutex);
}



// checkFile() is a private function, so DO NOT NEED mutex.locker
bool CFileAppender::CheckFile(uint32_t size)
{
    //int ret = 0;
    bool isNeedDiv = false;
    bool isFileExist = true;
    char newFileName[LOG_MAX_FILENAMELEN];
    size_t newFileNameLen = 0;
    char timeFileName[MAXLEN_TIME_FILE_SUFFIX];
    unsigned int newExpUId = 0;
    //char fName[LOG_MAX_FILENAMELEN];

    //memset(timeFileName, 0, MAXLEN_TIME_FILE_SUFFIX);
    //getTimeFileName(timeFileName);
    {
        unsigned int hour = 0;
        // 12345678901234567890123456789012
        // 2007-12-25 01:45:32:123456
        // 2007-12-25_01
        memcpy(timeFileName, m_timeArr, 13);
        timeFileName[10] = '_';
        timeFileName[13] = '\0';
        if (m_divTime > 0)
        {
            sscanf(timeFileName + 11, "%u", &hour);
            WRAPPER_SNPRINTF(timeFileName + 11, MAXLEN_TIME_FILE_SUFFIX - 11, "%02d", 
                m_hourArr[hour]);
        }
    }
    if (m_divTime > 0 && memcmp(m_divFileName, timeFileName, MAXLEN_TIME_FILE_SUFFIX) != 0)
    {
        //printf("t");fflush(stdout); //DEBUG
        isNeedDiv = true;
        memcpy(newFileName, m_pathFileName, m_pathFileNameLen);
        newFileName[m_pathFileNameLen] = '_';
        newFileNameLen = m_pathFileNameLen + 1;
        
        memcpy(newFileName + newFileNameLen, m_divFileName, MAXLEN_TIME_FILE_SUFFIX - 1);
        newFileNameLen += (MAXLEN_TIME_FILE_SUFFIX - 1);
        memcpy(m_divFileName, timeFileName, MAXLEN_TIME_FILE_SUFFIX);
        newExpUId = m_fileExpUid;
        m_fileExpUid = 0;
    }
    else if (m_maxSize >= LOG_MIN_FILESIZE && m_actSize + size > m_maxSize)
    {
        //printf("s");fflush(stdout); //DEBUG
        isNeedDiv = true;
        memcpy(newFileName, m_pathFileName, m_pathFileNameLen);
        newFileName[m_pathFileNameLen] = '_';
        newFileNameLen = m_pathFileNameLen + 1;

        memcpy(newFileName + newFileNameLen, m_divFileName, MAXLEN_TIME_FILE_SUFFIX - 1);
        newFileNameLen += (MAXLEN_TIME_FILE_SUFFIX - 1);

        newExpUId = m_fileExpUid;
        if (memcmp(m_divFileName, timeFileName, MAXLEN_TIME_FILE_SUFFIX) == 0)
        {
            m_fileExpUid++;
        }
        else
        {
            memcpy(m_divFileName, timeFileName, MAXLEN_TIME_FILE_SUFFIX);
            m_fileExpUid = 0;
        }
    }
    else
    {
        struct stat st;
        if (stat(m_pathFileName, &st) == -1)
        {
            //printf("errno: %d\n", errno); // errno == ENOENT
            //perror("statFail: ");
            isFileExist = false;
        }
        else
        {
            isFileExist = true;
        }
    }

    if (isNeedDiv)
    {
        while (1)
        {
            WRAPPER_SNPRINTF(newFileName + newFileNameLen, sizeof(newFileName) - newFileNameLen, 
                "_%03u", newExpUId);
            struct stat st;
            if (stat(newFileName, &st) == -1)
            {
                // file not exist.
                break;
            }
            else
            {
                // file exist.
                newExpUId++;
            }
        }
        if (m_fileExpUid != 0)
        {
            m_fileExpUid = newExpUId + 1;
        }
        
        Close(m_fd);
        Rename(m_pathFileName, newFileName);
        m_fd = open(m_pathFileName, LOG_OPENFLAG, LOG_CREATEMODE);
        m_actSize = 0;
    }
    else if (!isFileExist)
    {
        // file is not exist now
        Close(m_fd);
        m_fd = open(m_pathFileName, LOG_OPENFLAG, LOG_CREATEMODE);
        m_actSize = 0;
    }
    m_actSize += size;

    return isNeedDiv;
}

void CFileAppender::GetTimeFileName(char buff[MAXLEN_TIME_FILE_SUFFIX])
{
	unsigned int hour = 0;
	// 12345678901234567890123456789012
	// 2007-12-25 01:45:32:123456
	// 2007-12-25_01
	memcpy(buff, m_timeArr, 13);
	buff[10] = '_';
	buff[13] = '\0';
	if (m_divTime > 0)
	{
		sscanf(buff + 11, "%u", &hour);
	    WRAPPER_SNPRINTF(buff + 11, MAXLEN_TIME_FILE_SUFFIX - 11, "%02d", 
            hour - hour % m_divTime);
	}
}

//write() is a public interface function, so need mutex
void CFileAppender::Write(const char* msg)
{
    ssize_t ret = 0;
    bool isCut = false;
    
    Pthread_mutex_lock(&m_mutex);
    isCut = CheckFile(static_cast<uint32_t>(strlen(msg)));
    if (isCut)
    {
        // For Debug Use
    }
    
    ret = ::write(m_fd, msg, strlen(msg));
    if (ret == -1)
    {
        // TODO
    }
    Pthread_mutex_unlock(&m_mutex);
}

