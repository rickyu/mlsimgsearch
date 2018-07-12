
/*

作者: fengxq

作用: 封装函数调用

*/

#ifndef _WRAPPER_H_080724_
#define _WRAPPER_H_080724_

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

namespace log 
{

/// 包装函数
#define WRAPPER_VSNPRINTF(fmt, ...) do \
    { \
        int tmpRet = vsnprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)

/// 包装函数
#define  WRAPPER_SNPRINTF(fmt, ...) do \
    { \
        int tmpRet =  snprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)
    
/// 包装函数
#define   WRAPPER_FPRINTF(fmt, ...) do \
    { \
        int tmpRet =   fprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)



/// 包装函数
void Pthread_mutex_lock(pthread_mutex_t *pMutex);
/// 包装函数
void Pthread_mutex_unlock(pthread_mutex_t *pMutex);
/// 包装函数
void Pthread_mutex_destroy(pthread_mutex_t *pMutex);
/// 包装函数
void Pthread_mutex_init(pthread_mutex_t *pMutex, const pthread_mutexattr_t *pAttr);

/// 包装函数
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
    void *(*start_routine)(void*), void * arg);
/// 包装函数
void Pthread_join(pthread_t thread, void **value_ptr);
/// 包装函数
void Pthread_cancel(pthread_t thread);

/// 包装函数
void Pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
/// 包装函数
void Pthread_cond_destroy(pthread_cond_t *cond);
/// 包装函数
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
/// 包装函数
void Pthread_cond_signal(pthread_cond_t *cond);

/// 包装函数
void Vsnprintf(char *pStr, size_t size, const char *pFormat, va_list ap);

/// 包装函数
void Fflush(FILE *pStream);


/// 包装函数
void Gettimeofday(struct timeval *pTv, struct timezone *pTz);
/// 包装函数
void Settimeofday(const struct timeval *pTv , const struct timezone *pTz);


/// 包装函数
void Rename(const char *pOldpath, const char *pNewpath);
/// 包装函数
void Mkdir(const char *pPathname, mode_t mode);
/// 包装函数
void Close(int fd);

}
#endif


