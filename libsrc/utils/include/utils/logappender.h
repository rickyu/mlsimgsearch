#ifndef __LOGAPPENDER_H__
#define __LOGAPPENDER_H__

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

//#include "mutex.h"
#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>



#define DEBUG_LOGAPPENDER 			(0)


#define MAXLEN_TIME_FILE_SUFFIX		(14)



#define MAXLEN_TIME     32
#define MAXLEN_LINE     16
#define MAXLEN_PID      16
#define MAXLEN_TID 		16
#define MAXLEN_PTID     32


/// To avoid the ambiguous error, put the data before the pointers.
typedef struct _log_data
{
    char lineArr[MAXLEN_LINE];
    char timeArr[MAXLEN_TIME];
    char pidArr[MAXLEN_PID];
	char tidArr[MAXLEN_TID];
    char ptidArr[MAXLEN_PTID];
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
} log_data_t;

void GetTimeString(char *);

/// 链表节点
typedef struct _list_t
{
    struct _list_t *pNext;
}list_t;

/// 寻找链表下一个节点
#define NEXT(P) (((list_t *)P)->pNext)
/// 插入一个链表节点
#define INSERT(END, P) do \
    { \
        ((list_t *)END)->pNext = (list_t *)P; \
	((list_t *)P)->pNext = NULL; \
	END = P; \
    } while(0)
/// 设置next节点的值
#define SETNEXT(P, NEXT) do \
    { \
	    ((list_t *)P)->pNext = (list_t *)NEXT; \
    } while(0)

/// logappender信息节点    
typedef struct
{
    const char 		*fmtName;
#if (__WORDSIZE == 64)
    const long int 	offset;
#else
    const int 		offset;
#endif
}logAppender_fmt_t;
    
/// logappender信息节点    
typedef struct
{
    char  		*fmtName;
#if (__WORDSIZE == 64)
    long int 	offset;
#else
    int 		offset;
#endif
}logAppender_format_t;

/// 格式定义起始    
#ifndef LOG_FMT_BEGIN
#define LOG_FMT_BEGIN()  const logAppender_fmt_t CLogAppender::m_fmtFilter[] = { \
	    {"", -1} 
#endif 

/// 定义格式项
#ifndef LOG_FMT_MAP
#define LOG_FMT_MAP(FMT, MEMBER) , {FMT, OFFSET(log_data_t, MEMBER)}
#endif 

/// 格式定义结束
#ifndef LOG_FMT_END
#define LOG_FMT_END() }; \
    const int CLogAppender::m_fmtNum = \
    sizeof(CLogAppender::m_fmtFilter) / sizeof(CLogAppender::m_fmtFilter[0]);
#endif	
	
/// 最大的自定义log部分长度
#define MAXLEN_MSGBUFF	(10 * 1024)
/// 单条log最大长度
#define MAXLEN_LOGBUFF  (32 * 1024)

/// log缓冲区定义
typedef struct _logBuff_t
{
    char				mem[MAXLEN_LOGBUFF];
    struct _logBuff_t 	*pNext;
}logBuff_t;

/// log appender接口
class CLogAppender
{
public:
    CLogAppender(const std::string& format);
    void GetLogData(char *logBuff, log_data_t& data);
    virtual ~CLogAppender(void);
//    virtual void write(const log_data_t& data);
    virtual void Write(const char* msg) = 0;

    int GetData(logAppender_format_t& outFmt);

private:
    CLogAppender(const CLogAppender&);
    CLogAppender& operator=(const CLogAppender&);

protected:
    void ParseFormat(const std::string& format);
	
private:
    //user defined output format
    logAppender_format_t 				*m_format;
    int 								m_formatNum;

    //CLogAppender's format filter table
    static const logAppender_fmt_t 	m_fmtFilter[];
    static const int 					m_fmtNum;
protected:
	// time string used to divide file
	char 								m_timeArr[MAXLEN_TIME];
};

/// 终端
class CConsoleAppender : public CLogAppender
{
private:
    //static Mutex m_mutex;

public:
    CConsoleAppender(const std::string& format);
    ~CConsoleAppender(void);
    void Write(const char* msg);
};


/// 8G
static const unsigned long  LOG_DEF_FILESIZE = (8 * 0x400 * 0x100000LLU);
/// 200G
static const unsigned long  LOG_MAX_FILESIZE = (200 * 0x400 * 0x100000LLU);
/// 1M
static const unsigned long  LOG_MIN_FILESIZE = (1 * 0x100000L);
/// 默认切分时间，单位小时
static const unsigned int   LOG_DEF_DIVTIME  = (1);
/// 最小切分时间，单位小时
static const unsigned int   LOG_MIN_DIVTIME  = (1);
/// 文件的打开模式 
static const          int   LOG_OPENFLAG   	 = (O_CREAT | O_WRONLY | O_APPEND);
/// 新文件的创建模式
static const unsigned int   LOG_CREATEMODE   = (0644);
/// 最大文件名长度
static const unsigned int   LOG_MAX_FILENAMELEN = (256);
/// 最大路径长度
static const unsigned int 	LOG_MAX_PATHNAMELEN = (1024);

/// 文件
class CFileAppender : public CLogAppender
{
    
public:
    CFileAppender(const std::string& file, const std::string& format, uint64_t size = 0x80000000, 
        unsigned int divTime = 0);
    ~CFileAppender(void);
    void Write(const char* msg);

private:

	// 将内部的一个时间字符串转换为基于时间的文件名后缀字符串(不包含结束字节长度13)
	void GetTimeFileName(char buff[MAXLEN_TIME_FILE_SUFFIX]);
	bool CheckFile(uint32_t size);
	
private:
    char 			m_path[LOG_MAX_PATHNAMELEN];
	size_t	m_pathLen;
    char 			m_pathFileName[LOG_MAX_FILENAMELEN];
	size_t	m_pathFileNameLen;
	
    unsigned int 	m_divTime;
	unsigned int 	m_hourArr[25];
    char 			m_divFileName[MAXLEN_TIME_FILE_SUFFIX];
    unsigned int 	m_fileExpUid;
    uint64_t 		m_maxSize;
    uint64_t 		m_actSize;
    pthread_mutex_t m_mutex;

    int 			m_fd;
};

#endif // __LOGAPPENDER_H__
