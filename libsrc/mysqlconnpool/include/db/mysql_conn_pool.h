/** 
 * \file mysql_conn_pool.h
 * \brief mysql connection pool
 * \author sjcui
 * \date 2009-08-25
 * Copyright (C) Baidu Company
 */

#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_POOL_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_POOL_H_

#include <semaphore.h>
#include <vector>
#include <deque>
#include <map>
#include <utility>
#include <set>

#include "utils/mutex.h"
#include "utils/thread.h"

#include "db/imysql_conn.h"
#include "db/imysql_conn_pool.h"

// CThread::Start should be called to start the mysql thread,
// instead of CMySQLConnPool::Run directly
namespace db {

extern const char* const LOG_LIBSRC_MYSQL_POOL;
    
class CMysqlConnPool :
    public IMysqlConnPool
{
public:
    CMysqlConnPool();
    virtual ~CMysqlConnPool();

    virtual void Start();
    virtual void Stop();
    
    virtual std::vector< IMysqlConn* > AddDefault( SMysqlConf conf, uint32_t conns );
    virtual std::vector< IMysqlConn* > AddDBServer( SMysqlConf conf, uint32_t conns );

    virtual void DelServer( std::string conf );

    /** 
     * \brief SetTimeout
     *      set the timeout conf-value of GetConn operation
     * 
     * \param ms    [in] the conf-value to set
     * 
     * \return old-version timeout conf-value of GetConn operation.
     *         0 for 1st call of this function
     */
    virtual uint32_t SetTimeout(uint32_t ms);

    /** 
     * \brief IsAvailable 
     *      see if there are available connections in the pool
     * 
     * \return true for yes, false otherwise
     */
    virtual bool IsAvailable() const;

    /** 
     * \brief GetConn
     *     the implementaion of IMysqlConnPool::GetConn
     * 
     * \param ms        [in] timeout
     * 
     * \return          NULL for fail
     */
    virtual IMysqlConn* GetConn(uint32_t ms = 0);
    virtual void FreeConn(IMysqlConn* conn);
    virtual uint32_t GetAvailNum() const;

private:
    typedef std::deque< IMysqlConn* > conn_type_t;

private:
    /** 
     * \brief DelConnection     use it in SINGLE thread
     *    delete specified connection from the queue
     * 
     * \param connQueue         [in/out] connection queue
     * \param conn              [in] the connection to delete
     * 
     * \return 0 for success
     *         if the connection not exist, return -1
     */
    int32_t DelConnection( conn_type_t& connQueue, const IMysqlConn* conn );
    uint32_t CheckOneConn(IMysqlConn* conn);
    uint32_t GetDefaultCount();
    uint32_t GetNormalCount();
    
    std::vector< IMysqlConn* > AddServer( SMysqlConf conf, uint32_t conns, bool isDefault );

private:
    /*
     * NOTE:
     *     no needs for sem here, what we want is simply
     *   cond + counter
     * */
    sem_t*			m_sem;
    conn_type_t		m_invalid_default;
    conn_type_t		m_invalid_others;
    conn_type_t		m_default;
    conn_type_t		m_others;
    uint32_t		m_inuse_default_size;
    uint32_t		m_inuse_others_size;
    uint32_t		m_ms;
    CMutex    m_mutex;

    class CMysqlConnRetry :
        public CThread
    {
        public:
            CMysqlConnRetry( CMysqlConnPool* pool );
            ~CMysqlConnRetry();
            bool Run();
            bool Stop();
        private:
            CMysqlConnPool* m_pool;
    };
    CMysqlConnRetry m_retryWorker;

    std::map< std::string, std::pair< uint32_t, std::vector< IMysqlConn* > > > m_availConn;
    bool m_isAvailConnSafe;
};

class CMysqlRefPoolUnit :
    public IMysqlRefPoolUnit
{
public:
    CMysqlRefPoolUnit();
    /** 
     * \brief ~CMysqlRefPool
     *     release connection referenced by this pool
     *      do a Release() to m_pool
     */
    virtual ~CMysqlRefPoolUnit();
    
    virtual void Init( IMysqlConnPool* pool, bool addRef = true );
    virtual void AddServer( SMysqlConf conf, uint32_t conns, bool isDefault );
    
    virtual IMysqlConn* GetConn( uint32_t ms = 0 );
    virtual void FreeConn(IMysqlConn* conn);
    virtual uint32_t SetTimeout(uint32_t ms);
    virtual bool IsAvailable() const;
private:
    IMysqlConnPool* m_pool;
    std::set< IMysqlConn* > m_avail;
    std::set< std::string > m_conf;
    CRWMutex    m_mutex;
    bool m_bInited;
};

class CMysqlRefPool :
    public IMysqlRefPool
{
public:
    CMysqlRefPool();
    /** 
     * \brief ~CMysqlRefPool
     *   do a Release() to m_poolUnit
     */
    virtual ~CMysqlRefPool();
    
    virtual void Init( IMysqlRefPoolUnit* refPoolUnit, bool addRef = true );
    virtual IMysqlRefPoolUnit* Get();
    virtual void Set( IMysqlRefPoolUnit* refPoolUnit, bool addRef = true );

private:
    IMysqlRefPoolUnit* m_poolUnit;
    bool m_bInited;
    CMutex    m_mutex;
};

class CMysqlPoolMgr :
    public IMysqlPoolMgr
{
public:
    CMysqlPoolMgr();
    virtual ~CMysqlPoolMgr();
    
    virtual void Init( const PoolDesc* pools, uint32_t count, bool addRef = true );
    virtual IMysqlRefPoolUnit* Get( uint32_t rw, uint64_t key );
    virtual IMysqlRefPool* GetPool( uint32_t rw, uint64_t key );
    virtual int32_t GetSampleKey( uint32_t rw, uint64_t* key, uint32_t count );

private:
    PoolDesc* m_pools;
    uint32_t m_count;
};

}
#endif

