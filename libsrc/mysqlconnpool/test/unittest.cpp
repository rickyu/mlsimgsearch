#include <string>

#include <gtest/gtest.h>

#include "db/imysql_conn.h"
#include "db/mysql_conn.h"
#include "db/mysql_conn_pool.h"
#include "db/mysql_conn_util.h"
#include "logger/logger.h"
#include "logger/logappender.h"
#include "omni/thread.h"

using namespace db;

TEST( Logger, Init )
{
    static CLogger& log = CLogger::GetInstance( LOG_LIBSRC_MYSQL );
    CFileAppender* appender1 = new CFileAppender("mysql.log","%L %T %N %P:%p %f(%F:%l) - %M" );
    log.AttachAppender(appender1);
    log.SetLevel(0);
    
    static CLogger& log2 = CLogger::GetInstance( LOG_LIBSRC_MYSQL_POOL );
    CFileAppender* appender2 = new CFileAppender("mysql_pool.log","%L %T %N %P:%p %f(%F:%l) - %M" );
    log2.AttachAppender(appender2);
    log2.SetLevel(0);
    
}

TEST( CMysqlConn, Construct ) 
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    EXPECT_EQ( conn->Connect(), -1 );
    conn->Init( mysqlConf );
    
    EXPECT_TRUE( conn != NULL );
    EXPECT_TRUE( conn->GetMysqlConn() != NULL );
    
    EXPECT_FALSE( conn->IsWorking() );
    EXPECT_EQ( std::string( conn->GetHost() ), "tc-dialog-test03.tc:3306|root|IM" );
    
    delete conn;
}

TEST( CMysqlConn, Connect ) 
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );
    
    EXPECT_EQ( conn->Connect(), 0 );
    EXPECT_TRUE( conn->IsWorking() );
    delete conn;
}

TEST( CMysqlConn, Execute )
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );
    conn->Connect();
    
    EXPECT_EQ( conn->Execute( NULL ), -1 );
    EXPECT_EQ( conn->Execute( "create database auto_test" ), 0 );

    // test affect row 
    EXPECT_EQ( conn->Execute( "use auto_test" ), 1 );
    EXPECT_EQ( conn->Execute( "create table t1( id int(11) unsigned auto_increment primary key ) "
                "engine=innodb charset=utf8", NULL, true ), 0 );
    
    // auto_increment id
    uint64_t id = 0;
    EXPECT_EQ( conn->Execute( "insert into t1 values(NULL)", &id, true ), 0 );
    EXPECT_EQ( id, 1ULL ); 
    
    EXPECT_EQ( conn->Execute( "insert into t1 values(NULL)", &id, true ), 0 );
    EXPECT_EQ( id, 2ULL );
    MYSQL* mysqlConn = conn->GetMysqlConn();
    MYSQL_RES* res = mysql_store_result( mysqlConn );
    EXPECT_TRUE( NULL == res );

    // dup key
    EXPECT_EQ( conn->Execute( "insert into t1 values(2)" ), MYSQL_ERROR_DUP_KEY );
    
    // ignore
    EXPECT_EQ( conn->Execute( "delete from t1 where id=128" ), 1 );
    EXPECT_EQ( conn->Execute( "delete from t1 where id=128", NULL, true ), 0 );
    EXPECT_EQ( conn->Execute( "delete from t1" ), 0 );
    
    EXPECT_EQ( conn->Execute( "drop table t1", NULL, true ), 0 );
    EXPECT_EQ( conn->Execute( "drop database auto_test", NULL, true ), 0 );
    
    // this is a failure test
    EXPECT_EQ( conn->Execute( "drop databse auto_test", NULL, true ), 1 );

    delete conn;
}

TEST( CMysqlConn, RealQuery )
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );
    conn->Connect();
    
    const char* sql2 = NULL;
    uint32_t sqlLen = 0;
    MYSQL_RES* res = NULL;
    EXPECT_EQ( conn->RealQuery( sql2, sqlLen, &res ), -1 );
    res = ( MYSQL_RES* ) 0x1;
    EXPECT_EQ( conn->RealQuery( sql2, sqlLen, &res ), -1 );

    std::string sql;
    // test affect row
    sql = "create database auto_test";
    res = NULL;
    EXPECT_EQ( conn->RealQuery( sql.c_str(), sql.length(), &res ), NULL_RESULT_WITH_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );

    sql = "use auto_test";
    res = NULL;
    EXPECT_EQ( conn->RealQuery( sql.c_str(), sql.length(), &res ), NULL_RESULT_WITHOUT_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );

    conn->Execute( "create table t1( id int(11) unsigned auto_increment primary key ) "
                "engine=innodb charset=utf8", NULL, true );
    
    uint64_t id = 0;
    conn->Execute( "insert into t1 values(NULL)", &id, true );
    conn->Execute( "insert into t1 values(NULL)", &id, true );
    
    // select empty
    sql = "select * from t1 where id > 128";
    res = NULL;
    EXPECT_EQ( conn->RealQuery( sql.c_str(), sql.length(), &res), NON_NULL_RESULT );
    EXPECT_FALSE( NULL == res );
    mysql_free_result( res );

    // select empty
    sql = "select * from t1";
    res = NULL;
    EXPECT_EQ( conn->RealQuery( sql.c_str(), sql.length(), &res), NON_NULL_RESULT );
    EXPECT_FALSE( NULL == res );
    mysql_free_result( res );
    
    conn->Execute( "drop table t1", NULL, true );
    conn->Execute( "drop database auto_test", NULL, true );
    
    sql = "drop databse auto_test";
    res = NULL;
    EXPECT_EQ( conn->RealQuery( sql.c_str(), sql.length(), &res), MYSQL_ERROR );
    EXPECT_TRUE( NULL == res );

    delete conn;
}

TEST( CMysqlConn, GetNextResult )
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );
    conn->Connect();
    
    conn->Execute( "create database auto_test" );
    conn->Execute( "use auto_test" );
    conn->Execute( "create table t1( id int(11) unsigned auto_increment primary key ) "
                "engine=innodb charset=utf8", NULL, true );
   
    std::string sql;
    // test affect row
    sql = "start transaction;insert into t1 values(NULL);insert into t1 values(NULL);"
        "delete from t1 where id=128;delete from t1 where id=128;delete from t1 where id=1;"
        "insert into t1 values(2);select * from t1;";
    MYSQL_RES* res = NULL;
    conn->RealQuery( sql.c_str(), sql.length(), &res );
    //EXPECT_TRUE( NULL == res );

    /// 1st insert
    EXPECT_EQ( conn->GetNextResult( &res, false ), NULL_RESULT_WITH_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );
    
    /// 2st insert
    EXPECT_EQ( conn->GetNextResult( &res, false ), NULL_RESULT_WITH_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );
    
    /// 1st delete
    EXPECT_EQ( conn->GetNextResult( &res, true ), NULL_RESULT_WITHOUT_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );

    /// 2st delete TODO: should return NULL_RESULT
    //EXPECT_EQ( conn->GetNextResult( &res, false ), NULL_RESULT );
    EXPECT_EQ( conn->GetNextResult( &res, false ), NULL_RESULT_WITH_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );

    /// 3st delete
    EXPECT_EQ( conn->GetNextResult( &res, true ), NULL_RESULT_WITH_AFFECTED_ROWS );
    EXPECT_TRUE( NULL == res );

    /// 3st insert, need dup key detect?
    EXPECT_EQ( conn->GetNextResult( &res, false ), FAILED_OPERATION );
    EXPECT_TRUE( NULL == res );
    
    /// select  ====>  is droped
    /*
    EXPECT_EQ( conn->GetNextResult( &res, false ), NON_NULL_RESULT );
    EXPECT_FALSE( NULL == res );
    mysql_free_result( res );
    */

    /// no more result
    EXPECT_EQ( conn->GetNextResult( &res, true ), -1 );
    
    conn->Execute( "rollback", NULL, true );

    conn->Execute( "drop table t1", NULL, true );
    conn->Execute( "drop database auto_test", NULL, true );
    
    delete conn;
}

TEST( CMysqlConn, Ping ) 
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );
    
    ASSERT_DEATH( 0 == conn->Ping(), "" );

    EXPECT_EQ( conn->Connect(), 0 );
    
    EXPECT_TRUE( 0 == conn->Ping() );
    
    delete conn;
}

TEST( CMysqlConn, IRef )
{
    CMysqlConn* conn = new CMysqlConn;
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    conn->Init( mysqlConf );

    conn->AddRef();
    conn->Release();

    ASSERT_DEATH( delete conn, "" );
}

class CMysqlConnConsumer :
    public omni::CThread
{
public:
    CMysqlConnConsumer( IMysqlConnPool* pool )
        : m_pool( pool ), m_refPoolMgr( NULL ) {}

    CMysqlConnConsumer( IMysqlRefPool* refPoolMgr )
        : m_pool( NULL ), m_refPoolMgr( refPoolMgr ) {}
    
    ~CMysqlConnConsumer() {}

    bool Run()
    {
        if( NULL != m_pool )
        {
            IMysqlConn* conn = m_pool->GetConn( 10 );
            usleep( 1000 );
            if( NULL != conn )
            {
                m_pool->FreeConn( conn );
            }
        }
        else
        {
            IMysqlRefPoolUnit* refPool = m_refPoolMgr->Get();
            IMysqlConn* conn = refPool->GetConn( 10 );
            sleep( 1 );
            if( NULL != conn )
            {
                refPool->FreeConn( conn );
            }
            refPool->Release();
        }
        return true;
    }
private:
    IMysqlConnPool* m_pool;
    IMysqlRefPool* m_refPoolMgr;
};

TEST( CMysqlConnPool, Construct ) 
{
    CMysqlConnPool* pool = new CMysqlConnPool;
    EXPECT_FALSE( NULL == pool );

    EXPECT_FALSE( pool->IsAvailable() );
    delete pool;
}

TEST( CMysqlConnPool, Run )
{
    CMysqlConnPool* pool = new CMysqlConnPool;

    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    pool->AddDefault( mysqlConf, 1 );
    pool->AddDBServer( mysqlConf, 1 );
    CMysqlConnConsumer c1( pool );
    CMysqlConnConsumer c2( pool );

    EXPECT_TRUE( pool->IsAvailable() );
    c1.Start();
    c2.Start();

    usleep(100);
    EXPECT_FALSE( pool->IsAvailable() );
    
    c1.Stop();
    c2.Stop();

    usleep(100);
    EXPECT_TRUE( pool->IsAvailable() );
    
    c1.Start();
    
    usleep(100);
    EXPECT_TRUE( pool->IsAvailable() );
    
    c1.Stop();

    usleep(100);
    EXPECT_TRUE( pool->IsAvailable() );
    delete pool;
}

TEST( CMysqlConnPool, Timeout )
{
    CMysqlConnPool* pool = new CMysqlConnPool();

    pool->SetTimeout( 200 );
    CMysqlConnConsumer c1( pool );

    c1.Start();
    usleep(1000);

    c1.Stop();

    delete pool;
}

TEST( CMysqlRefPoolUnit, Construct )
{
    CMysqlConnPool* pool = new CMysqlConnPool();

    EXPECT_FALSE( pool->IsAvailable() );

    CMysqlRefPoolUnit* refPool = new CMysqlRefPoolUnit();
    EXPECT_TRUE( NULL == refPool->GetConn() );

    refPool->Init( pool, false );
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    refPool->AddServer( mysqlConf, 5, true );
    refPool->AddServer( mysqlConf, 5, false );
    
    EXPECT_TRUE( pool->IsAvailable() );
    delete refPool;
 
    EXPECT_FALSE( pool->IsAvailable() );
    delete pool;
}

TEST( CMysqlRefPoolUnit, Update )
{
    CMysqlConnPool* pool = new CMysqlConnPool();

    EXPECT_FALSE( pool->IsAvailable() );

    CMysqlRefPoolUnit* refPool = new CMysqlRefPoolUnit();
    refPool->Init( pool );
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    refPool->AddServer( mysqlConf, 5, true );
    refPool->AddServer( mysqlConf, 5, false );
    
    CMysqlRefPoolUnit* refPool2 = new CMysqlRefPoolUnit();
    refPool2->Init( pool );
    refPool2->AddServer( mysqlConf, 5, true );
    refPool2->AddServer( mysqlConf, 6, false );
    
    EXPECT_TRUE( pool->IsAvailable() );
    delete refPool;
 
    EXPECT_TRUE( pool->IsAvailable() );

    delete refPool2;

    ASSERT_DEATH( delete pool, "" );
}

TEST( CMysqlRefPoolUnit, Run )
{
    CMysqlConnPool* pool = new CMysqlConnPool();
    pool->AddRef();

    CMysqlRefPoolUnit* refPool = new CMysqlRefPoolUnit();
    refPool->AddRef();
    refPool->Init( pool );

    CMysqlRefPoolUnit* refPool2 = new CMysqlRefPoolUnit();
    refPool2->AddRef();
    refPool2->Init( pool );

    CMysqlRefPool* refPoolMgr =  new CMysqlRefPool();
    refPoolMgr->AddRef();
    refPoolMgr->Init( refPool );
    
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    refPool->AddServer( mysqlConf, 1, true );
    refPool->AddServer( mysqlConf, 1, false );
    
    CMysqlConnConsumer c1( refPoolMgr );
    CMysqlConnConsumer c2( refPoolMgr );

    EXPECT_TRUE( pool->IsAvailable() );
    c1.Start();
    usleep(100);
    EXPECT_TRUE( pool->IsAvailable() );
    
    c2.Start();
    usleep(100);
    EXPECT_FALSE( pool->IsAvailable() );

    refPoolMgr->Set( refPool2 );
    
    refPool->Release();

    c1.Stop();
    c2.Stop();

    EXPECT_FALSE( pool->IsAvailable() );
    
    refPoolMgr->Release();
    refPool2->Release();
    pool->Release();
}

TEST( CPoolHelper, Construct )
{
    CMysqlConnPool* pool = new CMysqlConnPool();
    CMysqlRefPoolUnit* refPool = new CMysqlRefPoolUnit();
    refPool->Init( pool );
    CMysqlRefPool* refPoolMgr =  new CMysqlRefPool();
    refPoolMgr->AddRef();
    refPoolMgr->Init( refPool );
    
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    refPool->AddServer( mysqlConf, 1, true );

    {
        CPoolHelper conn( refPoolMgr );
        EXPECT_TRUE( 0 == conn.Get()->Ping() );

        EXPECT_FALSE( pool->IsAvailable() );
    }
    EXPECT_TRUE( pool->IsAvailable() );
    
    refPoolMgr->Release();
    ASSERT_DEATH( delete refPool, "" );
    ASSERT_DEATH( delete pool, "" );
}

TEST( CMysqlQueryResProxy, Construct )
{
    CMysqlQueryResProxy resProxy( NULL );
    EXPECT_TRUE( resProxy.IsEmptySet() );

    CMysqlConnPool* pool = new CMysqlConnPool();
    pool->AddRef();

    CMysqlRefPoolUnit* refPool = new CMysqlRefPoolUnit();
    refPool->AddRef();
    refPool->Init( pool );
    
    SMysqlConf mysqlConf;
    mysqlConf.m_host = "tc-dialog-test03.tc";
    mysqlConf.m_user = "root";
    mysqlConf.m_passwd = "";
    mysqlConf.m_db = "IM";
    mysqlConf.m_connTimeout = 1;
    mysqlConf.m_port = 3306;
    refPool->AddServer( mysqlConf, 1, true );

    CMysqlRefPool* refPoolMgr =  new CMysqlRefPool();
    refPoolMgr->AddRef();
    refPoolMgr->Init( refPool );

    CPoolHelper conn( refPoolMgr );
    EXPECT_EQ( conn.Get()->Execute( "create database auto_test" ), 0 );
    EXPECT_EQ( conn.Get()->Execute( "use auto_test" ), 1 );
    EXPECT_EQ( conn.Get()->Execute( "create table t1( id int(11) unsigned auto_increment primary key ) "
                "engine=innodb charset=utf8", NULL, true ), 0 );
    
    uint64_t id = 0;
    EXPECT_EQ( conn.Get()->Execute( "insert into t1 values(NULL)", &id, true ), 0 );
    EXPECT_EQ( conn.Get()->Execute( "insert into t1 values(NULL)", &id, true ), 0 );
    
    // select empty
    std::string sql = "select * from t1 where id > 128";
    MYSQL_RES* res = NULL;
    EXPECT_EQ( conn.Get()->RealQuery( sql.c_str(), sql.length(), &res), NON_NULL_RESULT );
    {
        CMysqlQueryResProxy resProxy( res );
        EXPECT_FALSE( resProxy.IsEmptySet() );
    }
    ASSERT_DEATH( mysql_free_result( res ), "" );
    
    EXPECT_EQ( conn.Get()->Execute( "drop table t1", NULL, true ), 0 );
    EXPECT_EQ( conn.Get()->Execute( "drop database auto_test", NULL, true ), 0 );
    
    // this is a failure test
    EXPECT_EQ( conn.Get()->Execute( "drop databse auto_test", NULL, true ), 1 );

    /// NOTE: CANT do delete refPoolMgr here
    refPoolMgr->Release();
    refPool->Release();
    pool->Release();

    // leave out CPoolHeler, should do:
    // +-deconstruct CPoolHelper
    // +--get ref pool unit
    // +--free conn
    // +--release ref pool unit
    // +---deconstruct ref pool unit
    // +--release ref pool
    // +---deconstruct ref pool
    // +----release pool
    // +-----deconstruct pool
}

TEST( CMysqlPoolMgr, Run )
{
    CMysqlConnPool* pool = new CMysqlConnPool();

    CMysqlRefPoolUnit* refPoolUnit = new CMysqlRefPoolUnit();
    refPoolUnit->Init( pool );

    CMysqlRefPool* refPool =  new CMysqlRefPool();
    refPool->Init( refPoolUnit );

    
    CMysqlRefPoolUnit* refPoolUnit2 = new CMysqlRefPoolUnit();
    refPoolUnit2->Init( pool );
   
    CMysqlRefPool* refPool2 =  new CMysqlRefPool();
    refPool2->Init( refPoolUnit2 );

    CMysqlPoolMgr* poolMgr = new CMysqlPoolMgr();
    poolMgr->AddRef();

    IMysqlPoolMgr::PoolDesc desc[2] = {0};
    desc[0].pRefPool = refPool;
    desc[0].rw = IMysqlPoolMgr::POOL_WRITE;
    desc[0].segmentLo = 0;
    desc[0].segmentHi = 10000000;
    
    desc[1].pRefPool = refPool2;
    desc[1].rw = IMysqlPoolMgr::POOL_READ;
    desc[1].segmentLo = 10000000;
    desc[1].segmentHi = 20000000;
    poolMgr->Init( desc, 2 );
    
    ASSERT_TRUE( poolMgr->Get( IMysqlPoolMgr::POOL_WRITE, 10000001 ) == NULL );
    IMysqlRefPoolUnit* tmp = poolMgr->Get( IMysqlPoolMgr::POOL_WRITE, 1000000 );
    ASSERT_EQ( tmp, refPoolUnit );
    tmp->Release();
    
    ASSERT_TRUE( poolMgr->Get( IMysqlPoolMgr::POOL_READ, 9999999 ) == NULL );
    ASSERT_TRUE( poolMgr->Get( IMysqlPoolMgr::POOL_READ, 20000001 ) == NULL );
    tmp = poolMgr->Get( IMysqlPoolMgr::POOL_READ, 10000005 );
    ASSERT_EQ( tmp, refPoolUnit2 );
    tmp->Release();
    
    tmp = poolMgr->Get( IMysqlPoolMgr::POOL_WRITE | IMysqlPoolMgr::POOL_READ, 1000000 );
    ASSERT_EQ( tmp, refPoolUnit );
    tmp->Release();
    tmp = poolMgr->Get( IMysqlPoolMgr::POOL_WRITE | IMysqlPoolMgr::POOL_READ, 15000000 );
    ASSERT_EQ( tmp, refPoolUnit2 );
    tmp->Release();
    
    poolMgr->Release();
    ASSERT_DEATH( delete poolMgr, "" );
    ASSERT_DEATH( delete refPool, "" );
    ASSERT_DEATH( delete refPool2, "" );
    ASSERT_DEATH( delete refPoolUnit, "" );
    ASSERT_DEATH( delete refPoolUnit2, "" );
}

