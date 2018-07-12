#include <string.h>
#include "thread.h"
#include "logger.h"

CThread::CThread( bool joinable )
: m_bThreadRun( false ),
  m_bIsJoinable( joinable ),
  m_loggerName( OMNI_THREAD_LOG )
{
}

CThread::~CThread()
{
    /// Just to make sure I'm stopped
    /// CThread::Stop should be called 
    /// by any object inherented from CThread
    /// first before it desctruct its own data
    /// members, when it comes here to do the
    /// real thread stopping stuff, it always 
    /// has the potentitiality to cause access violation
    Stop();
}


CThread* CThread::CreateInstance( bool joinable )
{
    return new(std::nothrow) CThread(joinable);
}

void CThread::Destroy()
{
    delete this;
}

bool CThread::Start()
{    
    CLogger& log = CLogger::GetInstance( m_loggerName );
    bool ret = true;
    
    if( !m_bThreadRun )
    {
        m_bThreadRun = true;
        pthread_attr_t attr;
        pthread_attr_t* attrArg = NULL;
        
        if( !m_bIsJoinable ) 
        {
            attrArg = &attr;
            pthread_attr_init( attrArg  );
            pthread_attr_setdetachstate ( attrArg, PTHREAD_CREATE_DETACHED );
        }
        
        int32_t result = pthread_create( &m_ThreadID, attrArg, ThreadRoutine, this );
        
        if( result ) 
        {
            ret = false;
        }
        
        if( ret ) 
        {
            // Thread is running
            LOGGER_DEBUG( log, "Thread starts success");
        }
        else
        {
            // Thread failed to start
            LOGGER_DEBUG( log, "Thread starts fail, system error:%s. ", strerror( result ) ); 
        }
    }
    else 
    {
        // Thread is already running
        LOGGER_DEBUG( log, "Thread already running" );
    }
    
    return ret;
}

bool CThread::Stop( int timeout )
{
    if( m_bThreadRun )
    {
//        printf( "before CThread::Stop()/m_bThreadRun = false\n" );
        m_bThreadRun = false;
//        printf( "after CThread::Stop()/m_bThreadRun = %d\n", m_bThreadRun );
        // timeout no use right now
//        printf( "before CThread::Wait()/m_bThreadRun = false\n" );
        return Wait(timeout);
//        printf( "before CThread::Wait()/m_bThreadRun = false\n" );
    }

    return true;
}

bool CThread::IsRunning() 
{
    return m_bThreadRun;
}

bool CThread::Pause()
{
    return !m_bThreadRun;
}

bool CThread::Resume()
{
    return m_bThreadRun;
}

bool CThread::Wait( int timeout )
{
  

    if( m_bIsJoinable )
    {
        return ( 0 == pthread_join(m_ThreadID, NULL) );
    }
    else
    {
        return true;
    }
}

uint32_t CThread::GetThreadID()
{
    if( m_bThreadRun )
        return m_ThreadID;
    else
        return 0;
}

int32_t CThread::OnException() 
{
    CLogger& log = CLogger::GetInstance( m_loggerName );
    LOGGER_ERROR( log, "The thread meets exception");
    return 0;
}

int32_t CThread::OnThreadStart()
{
    return 0;
}

int32_t CThread::OnThreadStop()
{
    return 0;
}

bool CThread::Run()
{
    usleep(100);
    return true;
}

void* CThread::ThreadRoutine( void* arg )
{
    CThread* obj = static_cast<CThread*>( arg );
    obj->OnThreadStart();
    
    try
    {
        while( obj->m_bThreadRun && obj->Run() );
/*        {
            printf( "obj->m_bThreadRun=%d thread=%d\n", obj->m_bThreadRun, pthread_self() );
        }*/
    }
    catch( const std::bad_alloc& )
    {
        obj->OnException();
    }
    catch( ... )
    {
        obj->OnException();
    }
    
    obj->m_bThreadRun = false;
    obj->OnThreadStop();
    
    return 0;
}

void CThread::SetLogger( const char* logger )
{
    if( logger )
        m_loggerName = logger;
}
