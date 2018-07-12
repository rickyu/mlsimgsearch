#include "db/mysql_conn_util.h"

namespace db
{
CPoolHelper::CPoolHelper(db::IMysqlRefPool* pool, uint32_t ms, bool addRef)
    : m_pool(pool)
    , m_conn(NULL)
    , m_ms(ms)
{
    if( NULL != m_pool )
    {
        if( addRef  )
        {
            (void)m_pool->AddRef();
        }
        IMysqlRefPoolUnit* p = m_pool->Get();
        m_conn = p->GetConn(m_ms);
        p->Release();
    }
}

CPoolHelper::CPoolHelper(db::IMysqlPoolMgr* poolMgr, uint32_t rw, uint64_t key, uint32_t ms, bool addRef)
    : m_pool(NULL)
    , m_conn(NULL)
    , m_ms(ms)
{
    if( NULL != poolMgr )
    {
        m_pool = poolMgr->GetPool( rw, key );
    }
    //CPoolHelper( m_pool, ms, addRef );
    if( NULL != m_pool )
    {
        if( addRef  )
        {
            (void)m_pool->AddRef();
        }
        IMysqlRefPoolUnit* p = m_pool->Get();
        m_conn = p->GetConn(m_ms);
        p->Release();
    }
}

CPoolHelper::~CPoolHelper()
{
    if ( NULL != m_conn && NULL != m_pool )
    {
        IMysqlRefPoolUnit* p = m_pool->Get();
        p->FreeConn(m_conn);
        p->Release();
    }

    if( NULL != m_pool )
    {
        m_pool->Release();
    }
}

bool CPoolHelper::IsOk() const
{
    return (m_conn != NULL);
}

IMysqlConn* CPoolHelper::Get() const
{
    return m_conn;
}

CMysqlQueryResProxy::CMysqlQueryResProxy( MYSQL_RES* result )
    : m_result( result )
{
}

CMysqlQueryResProxy::~CMysqlQueryResProxy()
{
    if( NULL != m_result )
    {
        mysql_free_result( m_result );
        m_result = NULL;
    }
}

MYSQL_RES* CMysqlQueryResProxy::Get()
{
    return m_result;
}

int32_t CMysqlQueryResProxy::IsEmptySet() const
{
    if( NULL == m_result )
    {
        return -1;
    }

    uint32_t nRows = static_cast<uint32_t>(mysql_num_rows( m_result ));
    return nRows;
}

}

