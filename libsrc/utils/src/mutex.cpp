#include "time_util.h"
#include "mutex.h"


CEvent::CEvent(bool autoreset, bool init)
: m_event_signaled(init)
, m_event_autoreset(autoreset)
{
    pthread_mutex_init(&m_event_mutex, NULL);
    pthread_cond_init(&m_event_event, NULL);
}

CEvent::~CEvent(void)
{
//    pthread_mutex_destory(&m_event_mutex);
//    pthread_cond_destory(&m_event_cond);
}

CEvent* CEvent::createEvent(bool autoreset, bool init)
{
    return new CEvent(autoreset, init);
}

int32_t CEvent::setEvent(void)
{
    int32_t ret = 0;
    // �����¼����
    m_event_signaled = true;
    // �����߻���
    pthread_cond_signal(&m_event_event);
    return ret;
}

int32_t CEvent::resetEvent(void)
{
    int32_t ret = 0;
    // ����¼�
    m_event_signaled = false;
    event_leave();
    return ret;
}

void CEvent::event_enter(void)
{
    pthread_mutex_lock(&m_event_mutex);
}

void CEvent::event_leave(void)
{
    pthread_mutex_unlock(&m_event_mutex);
}

bool CEvent::waitCondition(void)
{
    // �����¼�ʹ�ÿ��ã�����������Ա�����
    return m_event_signaled;
}

void* CEvent::onWaited(void)
{
    return NULL;
}

int32_t CEvent::wait(uint32_t ms)
{
    int32_t ret = 0;
    event_enter();
    uint64_t start = TimeUtil::CurrentTickCount();
    if(ms)
    {
        uint64_t passed = 0;
        uint64_t us = ms * 1000;
        // �����ʱ�����¼����ã�����˯����cond_wait�ϣ�����ʹ��cond_wait���ߣ�cond_wait������ʱ������
        while(!waitCondition() && (passed < us))
        {
            uint64_t left = us - passed;
            uint64_t sec = left / 1000000;
            uint64_t nsec = left % 1000000 * 1000;
            timespec to;
            clock_gettime(CLOCK_REALTIME, &to);
            to.tv_sec += sec;
            to.tv_nsec += nsec;
            if(to.tv_nsec >= 1000000000)
            {
                to.tv_nsec -= 1000000000;
                to.tv_sec++;
            }
            // ��������ѣ���ret��Ȼ��0����ʱ����¼����ã��򷵻�ֵ����0�������ʱ���򷵻�ֵ�Ƿ�0
            // �����ʱ�����ǳ�ʱ���¼�������ò��������
            ret = pthread_cond_timedwait(&m_event_event, &m_event_mutex, &to);
            if(ret)
            {
                passed = TimeUtil::CurrentTickCount() - start;
            }
        }
    }
    else
    {
        while(!waitCondition())
        {
            pthread_cond_wait(&m_event_event, &m_event_mutex);
        }
    }
    if(m_event_autoreset)
    {
        resetEvent();
    }
    return ret;
}

CMutex::CMutex( int32_t attribute )
: m_mutex(NULL)
{
    m_mutex = new pthread_mutex_t;
    if(m_mutex)
    {
        pthread_mutexattr_t attr;
        if( ( 0 != pthread_mutexattr_init(&attr) ) ||
            ( 0 != pthread_mutexattr_settype(&attr, attribute) ) ||
            ( 0 != pthread_mutex_init(m_mutex,&attr) ) )
        {
            delete m_mutex;
            m_mutex = NULL;
        }
    }
}

CMutex::~CMutex(void)
{
    if(m_mutex)
    {
        pthread_mutex_destroy(m_mutex);
        delete m_mutex;
    }
    m_mutex = NULL;
}


int32_t CMutex::lock(void)
{
    return pthread_mutex_lock(m_mutex);
}

int32_t CMutex::unlock(void)
{
    return pthread_mutex_unlock(m_mutex);
}

CRWMutex::CRWMutex(bool wf)
: m_mutex(NULL)
{
    m_mutex = new pthread_rwlock_t;
    if(m_mutex)
    {
        int result = 0;
        if(wf)
        {
            result = pthread_rwlock_init(m_mutex, NULL);
        }
        else
        {
            pthread_rwlockattr_t attr = {PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, PTHREAD_PROCESS_PRIVATE};
            result = pthread_rwlock_init(m_mutex, &attr);
        }
        if(result)
        {
            delete m_mutex;
            m_mutex = NULL;
        }
    }
}

CRWMutex::~CRWMutex(void)
{
    if(m_mutex)
    {
        pthread_rwlock_destroy(m_mutex);
        delete m_mutex;
    }
    m_mutex = NULL;
}

int32_t CRWMutex::rlock(void)
{
    return pthread_rwlock_rdlock(m_mutex);
}

int32_t CRWMutex::runlock(void)
{
    return pthread_rwlock_unlock(m_mutex);
}

int32_t CRWMutex::wlock(void)
{
    return pthread_rwlock_wrlock(m_mutex);
}

int32_t CRWMutex::wunlock(void)
{
    return pthread_rwlock_unlock(m_mutex);
}


CRWMutex::operator bool (void)
{
    return (m_mutex != NULL);
}

CRWMutex::operator pthread_rwlock_t* (void)
{
    return m_mutex;
}

CMutex::operator bool (void)
{
    return (m_mutex != NULL);
}

CMutex::operator pthread_mutex_t* (void)
{
    return m_mutex;
}

CMutexHelper::CMutexHelper(pthread_mutex_t* mutex)
: m_mutex(mutex)
{
    if(m_mutex)
    {
        pthread_mutex_lock(m_mutex);
    }
}

CMutexHelper::~CMutexHelper(void)
{
    if(m_mutex)
    {
        pthread_mutex_unlock(m_mutex);
    }
}

CMutexLocker::CMutexLocker(CMutex& mutex)
: m_mutex(mutex)
{
    if(m_mutex)
    {
        m_mutex.lock();
    }
}

CMutexLocker::~CMutexLocker(void)
{
    if(m_mutex)
    {
        m_mutex.unlock();
    }
}

CReadLocker::CReadLocker(CRWMutex& mutex)
: m_mutex(mutex)
{
    if(m_mutex)
    {
        m_mutex.rlock();
    }
}

CReadLocker::~CReadLocker(void)
{
    if(m_mutex)
    {
        m_mutex.runlock();
    }
}

CWriteLocker::CWriteLocker(CRWMutex& mutex)
: m_mutex(mutex)
{
    if(m_mutex)
    {
        m_mutex.wlock();
    }
}

CWriteLocker::~CWriteLocker(void)
{
    if(m_mutex)
    {
        m_mutex.wunlock();
    }
}


