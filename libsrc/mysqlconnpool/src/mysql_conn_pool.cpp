#include <errno.h>
#include <cstring>
#include <sys/time.h>

#include "db/imysql_conn_pool.h"
#include "db/mysql_conn_pool.h"
#include "db/mysql_conn.h"
#include "utils/logger.h"
#include "utils/atomic.h"

namespace db {
const char* const LOG_LIBSRC_MYSQL_POOL = "mysql_pool";

const uint32_t US_PER_NS = 1000;
const uint32_t MS_PER_US = 1000;
const uint32_t MS_PER_NS = MS_PER_US * US_PER_NS;
const uint32_t SEC_PER_MS = 1000;
const uint32_t SEC_PER_US = SEC_PER_MS * MS_PER_US;
const uint32_t SEC_PER_NS = SEC_PER_MS * MS_PER_US * US_PER_NS;
const uint32_t MS100_PER_US = 100 * MS_PER_US;
const uint32_t MS_DELETE_WAIT = 100;

const int64_t TimeDec( const timeval &left, const timeval &right )
{
	timeval tvResult = { 0 };
	int64_t	n64USeconds = 0;
    int64_t n64Seconds = 0;

	n64Seconds = ( int64_t )left.tv_sec - ( int64_t )right.tv_sec;
	n64USeconds = ( int64_t )left.tv_usec - ( int64_t )right.tv_usec;

    while( n64Seconds > 0 )
    {
        --n64Seconds;
        n64USeconds += ( int64_t )SEC_PER_US;
    }

    if( n64USeconds < 0 )
    {
        return 0;
    }
	return n64USeconds;
}

void GenerateSemTime( timespec& to, uint32_t ms )
{
    memset( &to, 0, sizeof( to ) );
	clock_gettime(CLOCK_REALTIME, &to);
	to.tv_sec += MS_DELETE_WAIT / SEC_PER_MS;
	to.tv_nsec += ( MS_DELETE_WAIT % SEC_PER_MS ) * MS_PER_NS;
	if( to.tv_nsec >= SEC_PER_NS )
	{
		to.tv_nsec -= SEC_PER_NS;
		to.tv_sec++;
	}
}

int32_t SemWait( sem_t* sem, uint32_t nSem )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    for( uint32_t i = 0; i < nSem; ++i )
    {
        int32_t result = -1;
		timespec to = {0};
        GenerateSemTime( to, MS_DELETE_WAIT );
        do
        {
            result = sem_timedwait(sem, &to);
        } while( result && EINTR == errno );

        if( 0 != result )
        {
            LOGGER_WARN( log, "desc=sem_wait_fail nSem=%u errno=%d done=%u",
                    nSem, errno, i );
            return i;
        }
    }
    return nSem;
}

void SemPost( sem_t* sem, uint32_t nSem )
{
    for( uint32_t i = 0; i< nSem; ++i )
    {
        sem_post( sem );
    }
}

IMysqlConnPool::~IMysqlConnPool()
{
}

CMysqlConnPool::CMysqlConnPool()
: m_sem(NULL)
, m_inuse_default_size(0)
, m_inuse_others_size(0)
, m_ms(0)
, m_retryWorker( this )
, m_isAvailConnSafe( true )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    m_sem = new (std::nothrow) sem_t;
    if( NULL != m_sem )
    {
        if( 0 != sem_init(m_sem, 0, 0) )
        {
            delete m_sem;
            m_sem = NULL;
        }
    }

    Start();
    LOGGER_LOG( log, "desc=construct mysql_conn_pool=%p", this );
}

CMysqlConnPool::~CMysqlConnPool()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    Stop();
    if( NULL != m_sem )
    {
        sem_destroy(m_sem);
    }
    
    /// NOTE MUST be atomic operation
    m_isAvailConnSafe = false;
    
    std::set< std::string > toDel;
    {/// copy conf for delete
        CMutexLocker lock( m_mutex );
        typeof (m_availConn.begin()) iterBegin = m_availConn.begin();
        typeof (m_availConn.end()) iterEnd = m_availConn.end();

        for( typeof(m_availConn.begin()) iter = iterBegin;
                iter != iterEnd;
                ++iter )
        {
            toDel.insert( iter->first );
        }
    }

    typeof (toDel.begin()) iterBegin = toDel.begin();
    typeof (toDel.end()) iterEnd = toDel.end();

    for( typeof(iterBegin) iter = iterBegin;
            iter != iterEnd;
            ++iter )
    {
        DelServer( *iter );
    }

    LOGGER_LOG( log, "desc=deconstruct mysql_conn_pool=%p", this );
}

void CMysqlConnPool::Start()
{
    m_retryWorker.Start();
}

void CMysqlConnPool::Stop()
{
    m_retryWorker.Stop();
}

std::vector< IMysqlConn* > CMysqlConnPool::AddDefault( SMysqlConf mysqlConf, uint32_t conns )
{
    return AddServer( mysqlConf, conns, true );
}

std::vector< IMysqlConn* > CMysqlConnPool::AddDBServer( SMysqlConf mysqlConf, uint32_t conns )
{
    return AddServer( mysqlConf, conns, false );
}

std::vector< IMysqlConn* > CMysqlConnPool::AddServer( SMysqlConf mysqlConf, uint32_t conns, bool isDefault )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);

    std::vector< IMysqlConn* > ret( 0 );
    std::string conf = GetConf( mysqlConf, conns, isDefault );
 
    {
        CMutexLocker lock( m_mutex );
        if( !m_isAvailConnSafe )
        {
            LOGGER_ERROR( log, "desc=avail_conn_unsafe" );
            return ret;
        }

        if( m_availConn.end() != m_availConn.find( conf ) )
        {
            std::vector< IMysqlConn* >& vi = m_availConn[ conf ].second;
            uint32_t nVi = static_cast<uint32_t>(vi.size());
            for( uint32_t i = 0; i < nVi; ++i )
            {//thread-safe
                (void)vi[i]->AddRef();
            }
            m_availConn[ conf ].first += nVi;
            return m_availConn[ conf ].second;
        }
    }

    std::vector< IMysqlConn* > vi;
    for(uint32_t i = 0; i < conns; i++)
    {
        IMysqlConn* conn = new (std::nothrow) CMysqlConn;
        if( NULL == conn )
        {
            LOGGER_FATAL( log, "desc=fail_malloc" );
            return ret;
        }
        conn->Init( mysqlConf );
        (void)conn->AddRef();
        vi.push_back( conn );

        if( isDefault )
        {
            conn->SetDefault();
        }

        if ( conn->Connect() )
        {
            LOGGER_ERROR( log, "desc=fail_connect target=%s conn=%p",
                    conn->GetHost().c_str(), conn );
            if( isDefault )
            {
                CMutexLocker lock( m_mutex );
                m_invalid_default.push_back(conn);
            }
            else
            {
                CMutexLocker lock( m_mutex );
                m_invalid_others.push_back(conn);
            }
        }
        else
        {
            LOGGER_LOG( log, "desc=connect_success target=%s conn=%p",
                    conn->GetHost().c_str(), conn );
            
            if( isDefault )
            {
                CMutexLocker lock( m_mutex );
                m_default.push_back(conn);
            }
            else
            {
                CMutexLocker lock( m_mutex );
                LOGGER_INFO( log, "desc=push_into_others conn=%p", conn );
                m_others.push_back(conn);
            }

            sem_post(m_sem);
            if( log.CanLog( CLogger::LEVEL_INFO ) )
            {
                int32_t value = 0;
                sem_getvalue( m_sem, &value );
                LOGGER_INFO( log, "desc=sem_post value=%d", value );
            }
        }
    }

    CMutexLocker lock( m_mutex );
    m_availConn[ conf ] = make_pair( vi.size(), vi );
    return m_availConn[ conf ].second;
}

int32_t CMysqlConnPool::DelConnection( conn_type_t& connQueue, const IMysqlConn* conn )
{
    conn_type_t::iterator iterBegin = connQueue.begin();
    conn_type_t::iterator iterEnd = connQueue.end();
    conn_type_t::iterator toDel = iterEnd;
    
    for( conn_type_t::iterator it = iterBegin; it != iterEnd; ++it )
    {
        if( conn == *it )
        {
            toDel = it;
            break;
        }
    }
    if( iterEnd != toDel )
    {
        connQueue.erase( toDel );
        return 0;
    }
    return -1;
}

/** 
 * \brief CMysqlConnPool::DelServer
 *      delete a server-conf
 *      [NOTE]
 *      when this operation is performed, the same conf might
 *    be used by diffrenent IMysqlRefPoolUnits. only when the ref
 *    count be zero, can conns do sem_wait operations
 *      also be aware that some conns might be used by apps,
 *    which have done sem_wait already, kick them out when 
 *    do sem_wait
 * 
 * \param conf 
 */
void CMysqlConnPool::DelServer( std::string conf )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    uint32_t nSemToRelease = 0;
    uint32_t nSemWait = 0;
    {
        CMutexLocker lock( m_mutex );

        /// NOTE: DONT check m_isAvailConnSafe here, for we can stand by dup-DelServer
        
        if( m_availConn.end() == m_availConn.find( conf ) )
        {
            LOGGER_ERROR( log, "desc=server_not_exist conf=%s", conf.c_str() );
            return ;
        }
        
        std::vector< IMysqlConn* >& vi = m_availConn[ conf ].second;
        uint32_t nVi = static_cast<int>(vi.size());
        for( uint32_t i = 0; i < nVi; ++i )
        {
            int32_t ret = vi[i]->Release();
            if( 0 == ret )
            {
                ++ nSemToRelease;
            }
            LOGGER_INFO( log, "desc=release_conn ret=%d conn=%p", ret, vi[i] );
        }
        m_availConn[ conf ].first -= nVi;

        if( 0 < m_availConn[ conf ].first )
        {
            // assert( 0 == nSemToRelease );
            return ;
        }
   
        /*
    }
    {
        CMutexLocker lock( m_mutex );
        if( m_availConn.end() == m_availConn.find( conf ) )
        {/// another DelServer happened
            LOGGER_ERROR( log, "desc=server_not_exist conf=%s", conf.c_str() );
            SemPost( m_sem, nSemWait );
            return ;
        }
        
        std::vector< IMysqlConn* >& vi = m_availConn[ conf ].second;
        uint32_t nVi = vi.size();
        if( 0 < m_availConn[ conf ].first )
        {/// another AddServer happened
            LOGGER_LOG( log, "desc=server_add_again conf=%s", conf.c_str() );
            SemPost( m_sem, nSemWait );
            return ;
        }
        */
        
        for( uint32_t i = 0; i < nVi; ++i )
        {
            if( 0 == DelConnection( m_invalid_default, vi[i] ) )
            {
                --nSemToRelease;
                continue;
            }

            if( 0 == DelConnection( m_invalid_others, vi[i] ) )
            {
                --nSemToRelease;
                continue;
            }
            
            if( 0 == DelConnection( m_default, vi[i] ) )
            {
                continue;
            }

            if( 0 == DelConnection( m_others, vi[i] ) )
            {
                continue;
            }
        }
        m_availConn.erase( conf );
        LOGGER_LOG( log, "desc=del_server conf=%s conn.size=%u conn_pool=%p",
                conf.c_str(), nVi, this );
    }

    /// NOTE AddServer(conf) will NOT sem_post a connection if it's in m_availConn already
    nSemWait = SemWait( m_sem, nSemToRelease );
    if( nSemWait != nSemToRelease )
    {
        SemPost( m_sem, nSemWait );
        nSemWait = 0;
    }

    if( log.CanLog( CLogger::LEVEL_INFO ) )
    {
        int32_t value = 0;
        sem_getvalue( m_sem, &value );
        LOGGER_INFO( log, "desc=sem_wait value=%d", value );
    }
}

bool CMysqlConnPool::IsAvailable() const
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    bool ret = false;
    int32_t value = 0;
    if( ( NULL != m_sem ) && ( 0 == sem_getvalue(m_sem, &value) ) )
    {
        ret = (value > 0);
    }
    LOGGER_DEBUG( log, "ret=%d value=%d", ret, value );
    return ret;
}

uint32_t CMysqlConnPool::SetTimeout(uint32_t ms)
{
    uint32_t ret = m_ms;
    m_ms = ms;
    return ret;
}

IMysqlConn* CMysqlConnPool::GetConn(uint32_t ms)
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);

    IMysqlConn* ret = NULL;
    if(0 == ms)
    {
        ms = m_ms;
    }
    LOGGER_DEBUG( log, "desc=try_get_connection timeout=%ums", ms);
    int result = 0;

    /*
     * NOTE
     *   whatever the way of get connection, the value of sem
     * will be decreasesd if the operation succeed.
     * */
    if ( 0 != ms )
    {
		timespec to = {0};
        GenerateSemTime( to, ms );
        
        do
        {
            result = sem_timedwait(m_sem, &to);
        } while( result && EINTR == errno );
    }
    else
    {
        do
        {
            result = sem_wait(m_sem);
        } while( result && EINTR == errno );
    }

    if( 0 != result ) 
    {
        LOGGER_ERROR( log, "desc=operation_fail errno=%d", errno );
        return ret;
    }

    {
        CMutexLocker lock( m_mutex );
        if( !m_default.empty() )
        {
            ret = m_default.front();
            if( NULL != ret )
            {
                (void)ret->AddRef();
                m_default.pop_front();
                m_inuse_default_size++;
            }
        }
        
        if(NULL == ret && !m_others.empty() )
        {
            ret = m_others.front();
            if( NULL != ret )
            {
                (void)ret->AddRef();
                m_others.pop_front();
                m_inuse_others_size++;
            }
        }
    }

    /*
     * NOTE
     *   it's odd that get connection failed.
     *     however, the value of sem is decreased before,
     *   get it up anyway...
     * */
    if(NULL == ret)
    {
        LOGGER_WARN( log, "desc=fail_get_mysql_connection" );
        sem_post( m_sem );
    }
    else
    {
        LOGGER_DEBUG( log, "desc=get_conn_success conn=%p", ret );
    }
    return ret;
}

void CMysqlConnPool::FreeConn(IMysqlConn* conn)
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if ( NULL == conn )
    {
        LOGGER_ERROR(log, "free an invalid mysql connection");
        return ;
    }

    CMutexLocker lock( m_mutex );
    if( conn->IsDefault() )
    {
        m_inuse_default_size--;
    }
    else
    {
        m_inuse_others_size--;
    }


    bool needSemPost = false;
    
    if ( conn->IsDefault() )
    {
        if ( conn->IsWorking() )
        {
            m_default.push_back(conn);
            needSemPost = true;
        }
        else
        {
            m_invalid_default.push_back(conn);
        }
    }
    else
    {
        if ( conn->IsWorking() )
        {
            LOGGER_INFO( log, "desc=push_into_others conn=%p", conn );
            m_others.push_back(conn);
            needSemPost = true;
        }
        else
        {
            m_invalid_others.push_back(conn);
        }
    }

    if( needSemPost )
    {
        int32_t ret = conn->Release();

        /*
         * NOTE: 
         *   the conresponding config in m_availConn must be cleard already.
         * since this connection is about to free, we shall NOT put it back
         * to deque, NOR shall we perform sem_post
         * */
        if( 0 == ret )
        {
            LOGGER_LOG(log, "conn freed");
            DelConnection( m_invalid_default, conn );
            DelConnection( m_invalid_others, conn );
            DelConnection( m_default, conn );
            DelConnection( m_others, conn );
            return ;
        }
        sem_post(m_sem);
    }
}

uint32_t CMysqlConnPool::GetDefaultCount()
{
    CMutexLocker lock( m_mutex );
    uint32_t ret = static_cast<uint32_t>(m_default.size() + m_invalid_default.size() + m_inuse_default_size);
    return ret;
}

uint32_t CMysqlConnPool::GetNormalCount()
{
    CMutexLocker lock( m_mutex );
    uint32_t ret = static_cast<uint32_t>(m_others.size() + m_invalid_others.size() + m_inuse_others_size);
    return ret;
}

uint32_t CMysqlConnPool::CheckOneConn(IMysqlConn* conn)
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);

    if( log.CanLog( CLogger::LEVEL_INFO ) )
    {
        int32_t value = 0;
        sem_getvalue( m_sem, &value );
        LOGGER_INFO( log, "desc=check_conn value=%d", value );
    }

    if( NULL == conn )
    {
        LOGGER_INFO( log, "desc=null_conn" );
        return SEC_PER_US;
    }

    int32_t result = conn->Connect();

    if ( 0 != result )
    {
        if ( conn->IsDefault() )
        {
            LOGGER_DEBUG(log, "conn=%p code=%d error=%s desc=default_conn_still_unavailable",
                    conn, result, mysql_error(conn->GetMysqlConn()));
            CMutexLocker lock( m_mutex );
            m_invalid_default.push_back(conn);
            int32_t ref = conn->Release();
            if( 0 == ref )
            {
                DelConnection( m_invalid_default, conn );
            }
        }
        else
        {
            LOGGER_DEBUG(log, "conn=%p code=%d error=%s desc=ordinary_conn_still_unavailable",
                    conn, result, mysql_error(conn->GetMysqlConn()));
            CMutexLocker lock( m_mutex );
            m_invalid_others.push_back(conn);
            int32_t ref = conn->Release();
            if( 0 == ref )
            {
                DelConnection( m_invalid_others, conn );
            }
        }
    }
    else
    {
        bool needSemPost = true;
        if ( conn->IsDefault() )
        {
            LOGGER_DEBUG(log, "conn=%p code=%d error=%s desc=default_conn_is_available",
                    conn, result, mysql_error(conn->GetMysqlConn()));
            
            CMutexLocker lock( m_mutex );
            m_default.push_back(conn);
            int32_t ref = conn->Release();
            if( 0 == ref )
            {
                DelConnection( m_others, conn );
                needSemPost = false;
            }
        }
        else
        {
            LOGGER_DEBUG(log, "conn=%p code=%d error=%s desc=ordinary_conn_is_available",
                    conn, result, mysql_error(conn->GetMysqlConn()));
            
            CMutexLocker lock( m_mutex );
            LOGGER_INFO( log, "desc=push_into_others conn=%p", conn );
            m_others.push_back(conn);
            int32_t ref = conn->Release();
            if( 0 == ref )
            {
                DelConnection( m_others, conn );
                needSemPost = false;
            }
        }

        if( needSemPost )
        {
            sem_post(m_sem);
        }
    }
    return SEC_PER_US;
}

std::string GetConf( SMysqlConf mysqlConf, uint32_t conns, bool isDefault )
{
    std::ostringstream oss;
    oss << mysqlConf.m_host << "|" << mysqlConf.m_user << "|" << mysqlConf.m_passwd
        << "|" << mysqlConf.m_db << "|" << mysqlConf.m_port << "|" << mysqlConf.m_connTimeout
        << "|" << conns << "|" << isDefault;
    return oss.str();
}

CMysqlConnPool::CMysqlConnRetry::CMysqlConnRetry( CMysqlConnPool* pool )
    : CThread(),
    m_pool( pool )
{
}

CMysqlConnPool::CMysqlConnRetry::~CMysqlConnRetry()
{
}

bool CMysqlConnPool::CMysqlConnRetry::Stop()
{
    bool ret = true;
    if(m_bThreadRun)
    {
        m_bThreadRun = false;
        {
            pthread_join(m_ThreadID, NULL);
        }
    }
    return ret;
}

bool CMysqlConnPool::CMysqlConnRetry::Run()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    IMysqlConn* conn = NULL;
    conn_type_t::iterator iter;

    {
        CMutexLocker lock( m_pool->m_mutex );
        LOGGER_DEBUG(log, "desc=default_conn available=%u inuse=%u bad=%u",
                static_cast<uint32_t>(m_pool->m_default.size()),
                static_cast<uint32_t>(m_pool->m_inuse_default_size),
                static_cast<uint32_t>(m_pool->m_invalid_default.size()));
        LOGGER_DEBUG(log, "desc=ordinary_conn_info available=%u inuse=%u bad=%u",
                static_cast<uint32_t>(m_pool->m_others.size()),
                static_cast<uint32_t>(m_pool->m_inuse_others_size),
                static_cast<uint32_t>(m_pool->m_invalid_others.size()));
        if( log.CanLog( CLogger::LEVEL_INFO ) )
        {
            for( typeof( m_pool->m_others.begin() ) it = m_pool->m_others.begin();
                    it != m_pool->m_others.end();
                    ++ it )
            {
                LOGGER_INFO( log, "desc=avail conn=%p", *it );
            }
        }
        if(!m_pool->m_invalid_default.empty())
        {
            conn = m_pool->m_invalid_default.front();
            m_pool->m_invalid_default.pop_front();
            LOGGER_LOG( log, "desc=reconnect_default_connection conn=%p target=%s",
                    conn, conn->GetHost().c_str() );
            conn->AddRef();
        }
    }

    uint32_t toSleep1 = m_pool->CheckOneConn(conn);
    conn = NULL;

    {
        CMutexLocker lock( m_pool->m_mutex );
        if(!m_pool->m_invalid_others.empty())
        {
            conn = m_pool->m_invalid_others.front();
            m_pool->m_invalid_others.pop_front();
            LOGGER_LOG( log, "desc=reconnect_common_connection conn=%p target=%s",
                    conn, conn->GetHost().c_str() );
            conn->AddRef();
        }
    }

    uint32_t toSleep2 = m_pool->CheckOneConn(conn);

    if(m_bThreadRun)
    {
        usleep(toSleep1 > toSleep2 ? toSleep2 : toSleep1);
    }
    return true;
}

uint32_t CMysqlConnPool::GetAvailNum() const
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    int32_t value = 0;
    sem_getvalue( m_sem, &value );
    LOGGER_INFO( log, "desc=avail_num value=%d", value );
    if( value < 0 )
    {
        value = 0;
    }
    return value;
}

IMysqlRefPoolUnit::~IMysqlRefPoolUnit()
{
}

CMysqlRefPoolUnit::CMysqlRefPoolUnit() :
    m_pool( NULL),
    m_bInited( false )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    LOGGER_LOG( log, "desc=construct ref_pool_unit=%p", this );
}

void CMysqlRefPoolUnit::Init( IMysqlConnPool* pool, bool addRef )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    m_pool = pool;
    if( addRef )
    {
        m_pool->AddRef();
    }
    LOGGER_LOG( log, "desc=init this=%p pool=%p addRef=%d",
            this, m_pool, addRef );

    m_bInited = true;
}

CMysqlRefPoolUnit::~CMysqlRefPoolUnit()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( NULL != m_pool )
    {
        LOGGER_LOG( log, "desc=del_server conf.size=%u ref_pool=%p",
            static_cast<uint32_t>(m_conf.size()), this );
        for( typeof( m_conf.begin() ) iter = m_conf.begin();
                iter != m_conf.end();
                ++iter )
        {
            m_pool->DelServer( *iter );
            LOGGER_LOG( log, "desc=del_server conf=%s", (*iter).c_str() );
        }
        m_pool->Release();
    }
    LOGGER_LOG( log, "desc=deconstruct ref_pool_unit=%p mysql_conn_pool=%p", this, m_pool );
    m_pool = NULL;
}

void CMysqlRefPoolUnit::AddServer( SMysqlConf mysqlConf, uint32_t conns, bool isDefault )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( NULL == m_pool || !m_bInited )
    {
        LOGGER_ERROR( log, "desc=null_init" );
        return ;
    }

    std::vector< IMysqlConn* > vi;
    std::string conf = GetConf( mysqlConf, conns, isDefault );

    CWriteLocker lock( m_mutex );
    if( m_conf.end() != m_conf.find( conf ) )
    {
        LOGGER_ERROR( log, "desc=add_server_duplicated conf=%s", conf.c_str() );
        return ;
    }

    if( isDefault )
    {
        vi = m_pool->AddDefault( mysqlConf, conns );
    }
    else
    {
        vi = m_pool->AddDBServer( mysqlConf, conns );
    }

    uint32_t nVi = static_cast<uint32_t>(vi.size());
    for( uint32_t i = 0; i < nVi; ++i )
    {
        LOGGER_DEBUG( log, "desc=add_conn conn=%p", vi[i] );
        m_avail.insert( vi[i] );
    }
    m_conf.insert( conf );
    LOGGER_LOG( log, "desc=add_server conf=%s", conf.c_str() );
}

IMysqlConn* CMysqlRefPoolUnit::GetConn( uint32_t ms )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( NULL == m_pool || !m_bInited )
    {
        LOGGER_ERROR( log, "desc=null_init" );
        return NULL;
    }

    IMysqlConn* conn = NULL;

    timeval pre = {0};
    timeval now = {0};
    gettimeofday( &pre, NULL );
    
    /*
     * NOTE:
     *   m_pool->GetConn() is supposed to return quickly at
     * most situations. When it's hanged to timeout, it means 
     * there are no avail conns in the pool, so we won't bother
     * to retry: timeout ONCE is enough
     * */
    for( ; ; )
    {
        conn = m_pool->GetConn( ms );
        
        if( NULL == conn )
        {/// ³¬Ê±1´Î
            LOGGER_ERROR( log, "desc=get_conn_timeout" );
            break;
        }

        bool isVis = true;
        {
            CReadLocker lock( m_mutex );
            if( m_avail.end() == m_avail.find( conn ) )
            {
                LOGGER_INFO( log, "desc=get_invisible conn=%p", conn );
                isVis = false;
            }
        }
        if( isVis )
        {
            LOGGER_INFO( log, "desc=get_a_connection conn=%p", conn );
            break;
        }

        m_pool->FreeConn( conn );
        conn = NULL;

        gettimeofday( &now, NULL );
        if( TimeDec( now, pre ) > ms * MS_PER_US )
        {
            LOGGER_ERROR( log, "desc=get_conn_timeout" );
            break;
        }

        usleep( MS_PER_US );
    }
    return conn;
}

void CMysqlRefPoolUnit::FreeConn(IMysqlConn* conn)
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( !m_bInited )
    {
        LOGGER_ERROR( log, "desc=null_init" );
        return ;
    }

    if( NULL != m_pool )
    {
        m_pool->FreeConn( conn );
    }
}

uint32_t CMysqlRefPoolUnit::SetTimeout(uint32_t ms)
{
    return m_pool->SetTimeout( ms );
}

bool CMysqlRefPoolUnit::IsAvailable() const
{
    return m_pool->IsAvailable();
}

IMysqlRefPool::~IMysqlRefPool()
{
}

CMysqlRefPool::CMysqlRefPool() :
    m_poolUnit( NULL ),
    m_bInited( false )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    LOGGER_LOG( log, "desc=construct ref_pool=%p", this );
}

void CMysqlRefPool::Init( IMysqlRefPoolUnit* refPoolUnit, bool addRef )
{
    /*
     * NOTE:
     *   m_bInited MUST be set at last to make sure the pool has do an AddRef
     * before Init Complete
     *
     *   Get()/Set() will check m_bInited FIRST, so apps can't operate pool before
     * Init Complete
     * */
    if( addRef )
    {/// make sure refPoolUnit is available
        refPoolUnit->AddRef();
    }
    m_poolUnit = refPoolUnit;

    m_bInited = true;
}

CMysqlRefPool::~CMysqlRefPool()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    LOGGER_LOG( log, "desc=deconstruct ref_pool=%p cur_ref_pool_unit=%p", this, m_poolUnit );
    if( NULL != m_poolUnit )
    {
        m_poolUnit->Release();
    }
    m_poolUnit = NULL;
}

IMysqlRefPoolUnit* CMysqlRefPool::Get()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( !m_bInited )
    {
        LOGGER_ERROR( log, "desc=null_init" );
        return NULL;
    }

    /// use a m_mutex to protect m_poolUnit from changing
    CMutexLocker lock( m_mutex );
    m_poolUnit->AddRef();
    return m_poolUnit;
}

void CMysqlRefPool::Set( IMysqlRefPoolUnit* refPoolUnit, bool addRef )
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    if( !m_bInited )
    {
        LOGGER_ERROR( log, "desc=null_init" );
        return ;
    }
    IMysqlRefPoolUnit* oldRefPoolUnit = m_poolUnit;
    if( addRef )
    {
        refPoolUnit->AddRef();
    }
    {
        CMutexLocker lock( m_mutex );
        m_poolUnit = refPoolUnit;
    }

    /// drop the old now
    oldRefPoolUnit->Release();
}

IMysqlPoolMgr::~IMysqlPoolMgr() {}

CMysqlPoolMgr::CMysqlPoolMgr()
    :m_pools(NULL),
    m_count(0)
{
}

CMysqlPoolMgr::~CMysqlPoolMgr()
{
    static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL_POOL);
    LOGGER_LOG( log, "desc=deconstruct_pool_mgr this=%p", this );
    if( NULL != m_pools )
    {
        for( uint32_t i = 0; i < m_count; ++i )
        {
            m_pools[i].pRefPool->Release();
        }
        delete [] m_pools;
    }
}

void CMysqlPoolMgr::Init( const PoolDesc* pools, uint32_t count, bool addRef )
{
    if( NULL == m_pools && 0 < m_count )
    {
        return ;
    }
    PoolDesc* newPools = new PoolDesc[ m_count + count ];
    for( uint32_t i = 0; i < m_count; ++i )
    {
        newPools[i] = m_pools[i];
    }
    for( uint32_t i = 0; i < count; ++i )
    {
        newPools[ m_count + i ] = pools[i];
        if( addRef )
        {
            pools[i].pRefPool->AddRef();
        }
    }
    if( m_pools )
    {
        delete []m_pools;
    }
    m_pools = newPools;
    m_count += count;
}

IMysqlRefPoolUnit* CMysqlPoolMgr::Get( uint32_t rw, uint64_t key )
{
    IMysqlRefPool* refPool = GetPool( rw, key );
    if( NULL == refPool )
    {
        return NULL;
    }
    return refPool->Get();
}
 
IMysqlRefPool* CMysqlPoolMgr::GetPool( uint32_t rw, uint64_t key )
{
    uint32_t total = 0;

    for( uint32_t i = 0; i < m_count; ++i )
    {
        if( m_pools[i].segmentLo > key )
        {
            continue;
        }
        if( m_pools[i].segmentHi < key )
        {
            continue;
        }
        // segmentLo <= key <= segmentHi
        if( 0 == ( m_pools[i].rw & rw ) )
        {
            continue;
        }

        if (!m_pools[i].pRefPool->Get()->IsAvailable())
        {
            continue;
        }
        // return m_pools[i].pRefPool;
        ++ total;
    }

    if (total == 0)
    {
        return (NULL);
    }
    
    uint32_t cnt = 0;
    uint32_t tag = static_cast<uint32_t>(key % total);

    for( uint32_t i = 0; i < m_count; ++i )
    {
        if( m_pools[i].segmentLo > key )
        {
            continue;
        }
        if( m_pools[i].segmentHi < key )
        {
            continue;
        }
        // segmentLo <= key <= segmentHi
        if( 0 == ( m_pools[i].rw & rw ) )
        {
            continue;
        }

        if (!m_pools[i].pRefPool->Get()->IsAvailable())
        {
            continue;
        }
        
        // return m_pools[i].pRefPool;
        if( cnt == tag )
        {
            return m_pools[i].pRefPool;
        }
        ++ cnt;
    }
    return NULL;
}

int32_t CMysqlPoolMgr::GetSampleKey( uint32_t rw, uint64_t* key, uint32_t count )
{
    if( NULL == key )
    {
        return m_count;
    }
    if( count < m_count )
    {
        return -1;
    }
    uint32_t cnt = 0;
    for( uint32_t i = 0; i < m_count; ++i )
    {
        if( 0 == ( m_pools[i].rw & rw ) )
        {
            continue;
        }
        key[cnt++] = ( m_pools[i].segmentLo + m_pools[i].segmentHi ) / 2;
    }
    return cnt;
}

}//end of namespace db

