#ifndef __LIBSRC_IM_MUTEX_H__
#define __LIBSRC_IM_MUTEX_H__

#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>

class CEvent
{

public:
    virtual ~CEvent(void);

    static CEvent* createEvent(bool autoreset = false, bool init = false);

    virtual int32_t setEvent(void);
    virtual int32_t resetEvent(void);
    int32_t wait(uint32_t ms = 0);

protected:
    CEvent(bool autoreset = false, bool init = false);

    void event_enter(void);
    void event_leave(void);
    virtual bool waitCondition(void);
    virtual void* onWaited(void);
private:
    pthread_mutex_t m_event_mutex;
    pthread_cond_t m_event_event;
    bool m_event_signaled;
    bool m_event_autoreset;
};

class CMutex
{

public:
    CMutex(int attribute = PTHREAD_MUTEX_NORMAL);
    virtual ~CMutex(void);
    virtual int32_t lock(void);
    virtual int32_t unlock(void);

    operator bool (void);
    operator pthread_mutex_t* (void);
private:
    CMutex( const CMutex& );
    CMutex& operator = ( const CMutex& );
protected:
    pthread_mutex_t* m_mutex;
};

class CMutexHelper
{
public:
    CMutexHelper(pthread_mutex_t* mutex);
    ~CMutexHelper(void);
private:
    CMutexHelper(const CMutexHelper&);
    CMutexHelper& operator=(const CMutexHelper&);

    pthread_mutex_t* m_mutex;

};

// CRWMutex : 默认为写优先

class CRWMutex
{

public:
    CRWMutex(bool wf = true);
    ~CRWMutex(void);
    int32_t rlock(void);
    int32_t runlock(void);
    int32_t wlock(void);
    int32_t wunlock(void);

    operator bool (void);
    operator pthread_rwlock_t* (void);

private:
    CRWMutex(const CRWMutex&);
    CRWMutex& operator=(const CRWMutex&);

    pthread_rwlock_t* m_mutex;
};

class CMutexLocker
{

public:
    CMutexLocker(CMutex& mutex);
    ~CMutexLocker(void);
private:
    CMutex& m_mutex;
};

class CReadLocker
{
public:
    CReadLocker(CRWMutex& mutex);
    ~CReadLocker(void);
private:
    CRWMutex& m_mutex;

};

class CWriteLocker
{

public:
    CWriteLocker(CRWMutex& mutex);
    ~CWriteLocker(void);
private:
    CRWMutex& m_mutex;
};


#endif // __IM_MUTEX_H__
