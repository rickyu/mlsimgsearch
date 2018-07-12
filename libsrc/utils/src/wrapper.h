
/*

����: fengxq

����: ��װ��������

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

/// ��װ����
#define WRAPPER_VSNPRINTF(fmt, ...) do \
    { \
        int tmpRet = vsnprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)

/// ��װ����
#define  WRAPPER_SNPRINTF(fmt, ...) do \
    { \
        int tmpRet =  snprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)
    
/// ��װ����
#define   WRAPPER_FPRINTF(fmt, ...) do \
    { \
        int tmpRet =   fprintf(fmt, ##__VA_ARGS__); \
        if (tmpRet < 0) \
        { \
            /* TODO */ \
        } \
    } while(0)



/// ��װ����
void Pthread_mutex_lock(pthread_mutex_t *pMutex);
/// ��װ����
void Pthread_mutex_unlock(pthread_mutex_t *pMutex);
/// ��װ����
void Pthread_mutex_destroy(pthread_mutex_t *pMutex);
/// ��װ����
void Pthread_mutex_init(pthread_mutex_t *pMutex, const pthread_mutexattr_t *pAttr);

/// ��װ����
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
    void *(*start_routine)(void*), void * arg);
/// ��װ����
void Pthread_join(pthread_t thread, void **value_ptr);
/// ��װ����
void Pthread_cancel(pthread_t thread);

/// ��װ����
void Pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
/// ��װ����
void Pthread_cond_destroy(pthread_cond_t *cond);
/// ��װ����
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
/// ��װ����
void Pthread_cond_signal(pthread_cond_t *cond);

/// ��װ����
void Vsnprintf(char *pStr, size_t size, const char *pFormat, va_list ap);

/// ��װ����
void Fflush(FILE *pStream);


/// ��װ����
void Gettimeofday(struct timeval *pTv, struct timezone *pTz);
/// ��װ����
void Settimeofday(const struct timeval *pTv , const struct timezone *pTz);


/// ��װ����
void Rename(const char *pOldpath, const char *pNewpath);
/// ��װ����
void Mkdir(const char *pPathname, mode_t mode);
/// ��װ����
void Close(int fd);

}
#endif


