

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "wrapper.h"

namespace log
{
/// 包装函数
void Pthread_mutex_lock(pthread_mutex_t *pMutex)
{
    int ret = 0;
    ret = pthread_mutex_lock(pMutex);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Pthread_mutex_unlock(pthread_mutex_t *pMutex)
{
    int ret = 0;
    ret = pthread_mutex_unlock(pMutex);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Vsnprintf(char *pStr, size_t size, const char *pFormat, va_list ap)
{
    int ret = 0;
    ret = vsnprintf(pStr, size, pFormat, ap);
    if (ret < 0)
    {
        // TODO
    }
}

/// 包装函数
void Fflush(FILE *pStream)
{
    int ret = 0;
    ret = fflush(pStream);
    if (0 != ret)
    {
        // TODO
    }
}


/// 包装函数
void Pthread_mutex_destroy(pthread_mutex_t *pMutex)
{
    int ret = 0;
    ret = pthread_mutex_destroy(pMutex);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Pthread_mutex_init(pthread_mutex_t *pMutex, const pthread_mutexattr_t *pAttr)
{
    int ret = 0;
    ret = pthread_mutex_init(pMutex, pAttr);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Gettimeofday(struct timeval *pTv, struct timezone *pTz)
{
    int ret = 0;
    ret = gettimeofday(pTv, pTz);
    if (0 != ret)
    {
        // TODO
        memset(pTv, 0, sizeof(timeval));
    }
}

/// 包装函数
void Settimeofday(const struct timeval *pTv , const struct timezone *pTz)
{
    int ret = 0;
    ret = settimeofday(pTv, pTz);
    if (0 != ret)
    {
        // TODO
    }
}


/// 包装函数
void Rename(const char *pOldpath, const char *pNewpath)
{
    int ret = 0;
    ret = rename(pOldpath, pNewpath);
    if (0 != ret)
    {
        // TODO
    }
}


/// 包装函数
void Mkdir(const char *pPathname, mode_t mode)
{
    int ret = 0;
    ret = mkdir(pPathname, mode);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Close(int fd)
{
    int ret = 0;
    ret = close(fd);
    if (0 != ret)
    {
        // TODO
    }
}

/// 包装函数
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
        void *(*start_routine)(void*), void *arg)
{
    int ret = 0;
    ret = pthread_create( thread, attr, start_routine, arg );
    if ( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_join(pthread_t thread, void **value_ptr)
{
    int ret = 0;
    ret = pthread_join( thread, value_ptr );
    if( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    int ret = 0;
    ret = pthread_cond_init( cond, attr );
    if( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_cond_destroy(pthread_cond_t *cond)
{
    int ret = 0;
    ret = pthread_cond_destroy( cond );
    if( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int ret = 0;
    ret = pthread_cond_wait( cond, mutex );
    if( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_cond_signal(pthread_cond_t *cond)
{
    int ret = 0;
    ret = pthread_cond_signal( cond );
    if( 0 != ret )
    {
        // TODO
    }
}

/// 包装函数
void Pthread_cancel(pthread_t thread)
{
    int ret = 0;
    ret = pthread_cancel( thread );
    if( 0 != ret )
    {
        // TODO
    }
}


}


