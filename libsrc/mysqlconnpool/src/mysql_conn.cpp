#include <errmsg.h>
#include <iostream>
#include <cstring>
#include "utils/logger.h"
#include "utils/time_util.h"
#include "db/mysql_conn.h"
#include "db/mysql_conn_util.h"
#include <my_global.h>
#include <my_sys.h>

using namespace db;
using std::cout;
using std::endl;
using std::min;

const char* const db::LOG_LIBSRC_MYSQL = "mysql";

db::IMysqlConn::~IMysqlConn() {
}

db::CMysqlConn::CMysqlConn() :
  m_mysql(NULL),
  m_host(""),
  m_user(""),
  m_passwd(""),
  m_db(""),
  m_port(0),
  m_valid(false),
  m_default(false),
  m_conned(false),
  m_conf(""),
  m_bInited( false )
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  LOGGER_INFO( log, "desc=construct conn=%p", this );
}

int32_t db::CMysqlConn::Init( SMysqlConf& conf )
{
  if( NULL != m_mysql )
  {
    mysql_close(m_mysql);
    m_mysql = NULL;
  }
  m_mysql = mysql_init(NULL);

  if( NULL != m_mysql )
  {
    my_bool flag = true;
    mysql_options(m_mysql, MYSQL_OPT_RECONNECT, (const char*)&flag);
    mysql_options(m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&conf.m_connTimeout);
    // mysql¿¿readtime¿¿¿¿¿¿¿¿¿¿¿¿so¿¿¿¿¿
    unsigned int read_timeout = 200;
    if (conf.m_readTimeout != 0)
      read_timeout = conf.m_readTimeout;
    //std::cout << "read time out: " << read_timeout << endl;
    mysql_options(m_mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
    mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    //mysql_options(m_mysql, MYSQL_INIT_COMMAND, "set names 'utf8'");
  }
  m_host = conf.m_host;
  m_user = conf.m_user;
  m_db = conf.m_db;
  m_passwd = conf.m_passwd;
  m_port = conf.m_port;

  std::ostringstream oss;
  oss << m_host << ":" << m_port << "|" << m_user << "|" << m_db;
  m_conf = oss.str();
  m_bInited = true;
  return 0;
}

db::CMysqlConn::~CMysqlConn()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  LOGGER_INFO( log, "desc=deconstruct conn=%p", this );
  if( NULL != m_mysql )
  {
    mysql_close(m_mysql);
    m_mysql = NULL;
  }
  my_thread_end();
}

bool db::CMysqlConn::IsDefault() const
{
  return m_default;
}

void db::CMysqlConn::SetDefault()
{
  m_default = true;
}

bool db::CMysqlConn::BeginTransaction()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return false;
  }
  return ( 0 == mysql_autocommit(m_mysql, 0) );
}

bool db::CMysqlConn::RollBack()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return false;
  }
  my_bool ret1 = mysql_rollback(m_mysql);
  my_bool ret2 = mysql_autocommit(m_mysql,1);
  return (0 == ret1) && (0 == ret2);
}

bool db::CMysqlConn::EndTransaction()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return false;
  }
  my_bool ret1 = mysql_commit(m_mysql);
  my_bool ret2 = mysql_autocommit(m_mysql, 1);
  return (0 == ret1) && (0 == ret2);
}

void db::CMysqlConn::LogError(const char* file, const char* func, int line, const char* prefix,
    int32_t err)
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return ;
  }

  if( NULL == m_mysql )
  {
    log.Error( file, func, line, "prefix=%s desc=set_connection_invalid", prefix );
    m_valid = false;
    return ;
  }

  if( 0 == err )
  {
    return ;
  }

  err = mysql_errno(m_mysql);
  if(err > 0)
  {
    log.Error(file, func, line, "prefix=%s code=%d error=%s", prefix, err,
        mysql_error(m_mysql));
    if((err == CR_SERVER_GONE_ERROR) || (err == CR_SERVER_LOST) || (err == CR_CONN_HOST_ERROR ) )
    {
      log.Error(file, func, line, "desc=set_connection_invalid");
      m_valid = false;
    }
  }
}

int32_t db::CMysqlConn::Execute(const char* sql, uint64_t* id, bool ignore)
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  if( NULL == sql )
  {
    return -1;
  }
  MYSQL_RES* res = NULL;
  int32_t ret = RealQuery( sql, static_cast<uint32_t>(strlen(sql)), &res );
  mysql_free_result( res );

  if( MYSQL_ERROR_DUP_KEY == ret )
  {
    return ret;
  }

  if( !ignore )
  {
    if( NULL_RESULT_WITHOUT_AFFECTED_ROWS == ret )
    {
      return 1;
    }
  }

  if( NULL != id )
  {
    *id = mysql_insert_id(m_mysql);
  }

  if( ret < 0 )
  {
    return 1;
  }

  return 0;
}

int32_t db::CMysqlConn::BatchExecute(const char* sql, uint32_t size )
{/// ²Î¿¼dbserver.cpp:ExecuteTransaction
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  if( NULL == sql )
  {
    return -1;
  }
  MYSQL_RES* res = NULL;
  int32_t ret = RealQuery( sql, size, &res );
  if( 0 > ret )
  {
    LOGGER_ERROR( log, "err=failed_on_real_query" );
    return -1;
  }

  int32_t status = 0;

  do
  {
    CMysqlQueryResProxy resProxy( res );
  } while( 0 <= ( status = GetNextResult( &res, false ) ) );

  if( FAILED_OPERATION == status )
  {
    return -1;
  }
  return 0;
}

int32_t db::CMysqlConn::RealQuery( const char* sql, uint32_t size, MYSQL_RES** ret )
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  uint64_t start = TimeUtil::GetTickUs();
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  if( NULL == sql )
  {
    return -1;
  }

  if( NULL == ret || NULL != (*ret) )
  {
    return -1;
  }

  uint32_t descLen = 1024;
  char desc[ descLen ];

  int32_t result = mysql_real_query( m_mysql, sql, size );
  if( 0 != result )
  {
    int mysqlErrno = mysql_errno(m_mysql);
    std::string tmp = std::string(sql, 0, 1024);
    if( MYSQL_ERROR_DUP_KEY == -mysqlErrno )
    {   
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=%s] [sql=%s]", m_host.c_str(), m_port, m_db.c_str(), mysqlErrno, mysql_error(m_mysql), tmp.c_str());
      return MYSQL_ERROR_DUP_KEY;
    }
    LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [error=%s] [sql=%s]", m_host.c_str(), m_port, m_db.c_str(), mysql_error(m_mysql), tmp.c_str());
    return MYSQL_ERROR;
  }

  *ret = mysql_store_result(m_mysql);
  if( NULL == *ret )
  {
    if( 0 == mysql_field_count( m_mysql ) )
    {// log affect rows with mysql_affected_rows
      int32_t affectedRows = static_cast<int32_t>(mysql_affected_rows( m_mysql ));
      LOGGER_LOG( log,  "SQL[%s] desc=execute_success_affected=%d target=%s",
          sql, affectedRows, GetHost().c_str() );
      if( 0 == affectedRows )
      {
        return NULL_RESULT_WITHOUT_AFFECTED_ROWS;
      }
      return NULL_RESULT_WITH_AFFECTED_ROWS;
    }
    else
    {// error, "dont free @mysql" ( from Manual )
      result = mysql_errno(m_mysql);
      if( 0 != result )
      {
        std::string tmp = std::string(sql, 0, 1024);
        LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=%s] [sql=%s]", m_host.c_str(), m_port, m_db.c_str(), result, mysql_error(m_mysql), tmp.c_str());
        return MYSQL_ERROR;
      }
      else
      {
        LOGGER_LOG( log, "SQL[%s] desc=success_null_result target=%s", sql,
            GetHost().c_str() );
        return NULL_RESULT;
      }
    }
  }

  int size_sql = static_cast<int>(strlen(sql));
  int log_info_max_len = min(300, size_sql);
  char* sql_tmp = new char[log_info_max_len + 1];
  for (int i = 0; i < log_info_max_len && sql[i]; i ++) {
    if (sql[i] != '\n') {
      sql_tmp[i] = sql[i];
    } else {
      sql_tmp[i] = ' ';
    }
  }
  if(size_sql > log_info_max_len) {
    sql_tmp[log_info_max_len - 1] = '.';
    sql_tmp[log_info_max_len - 2] = '.';
    sql_tmp[log_info_max_len - 3] = '.';
  }
  sql_tmp[log_info_max_len] = 0;
  int64_t time_consume = TimeUtil::GetTickUs() - start;
  if (time_consume <= 500000) {
    LOGGER_LOG( log, "SQL[%s] desc=success_with_result target=%s time_consume=%ld", sql_tmp,
        GetHost().c_str(), time_consume );
  } else {
    LOGGER_LOG( log, "LONG QUERY SQL[%s] desc=success_with_result target=%s time_consume=%ld",
        sql_tmp, GetHost().c_str(), time_consume );
  }
  delete []sql_tmp;
  return NON_NULL_RESULT;
}

MYSQL_RES* db::CMysqlConn::Query(const char* sql, int32_t* reason )
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return NULL;
  }
  MYSQL_RES* res = NULL;
  int32_t ret = RealQuery( sql, static_cast<uint32_t>(strlen( sql )), &res );
  if( NULL != reason )
  {
    *reason = ret;
  }
  return res;
}

int32_t db::CMysqlConn::GetNextResult( MYSQL_RES** ret, bool needAffected )
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  if( NULL == m_mysql || NULL == ret || NULL != (*ret) )
  {
    return -1;
  }

  int32_t status = mysql_next_result( m_mysql );
  if( 0 < status )
  {
    LogError( __FILE__, __FUNCTION__, __LINE__, "err=failed_multi_query_need_rollback", status );
    return FAILED_OPERATION;
  }

  ///< no more data to process >
  if( -1 == status )
  {
    LOGGER_DEBUG( log, "desc=no_next_result");
    return -1;
  }

  *ret = mysql_store_result( m_mysql );

  if( NULL == *ret )
  {
    if( 0 == mysql_field_count( m_mysql ) )
    {// log affect rows with mysql_affected_rows
      int32_t affectedRows = static_cast<int32_t>(mysql_affected_rows( m_mysql ));
      LOGGER_DEBUG( log, "desc=query_success affected=%d", affectedRows);
      if( needAffected && 0 == affectedRows )
      {
        return NULL_RESULT_WITHOUT_AFFECTED_ROWS;
      }
      return NULL_RESULT_WITH_AFFECTED_ROWS;
    }
    else
    {// error, "dont free @mysql" ( from Manual )
      LogError( __FILE__, __FUNCTION__, __LINE__,
          "err=failed_multi_query_need_rollback" );
      return -1;
    }
  }

  return NON_NULL_RESULT;
}

int32_t db::CMysqlConn::StmtExecute(const char* sql, uint32_t count, MYSQL_BIND* args, int* affect_rows, uint64_t* id)
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  uint64_t start = TimeUtil::GetTickUs();
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  if( NULL == sql || NULL == args || NULL == id)
  {
    return -1;
  }
  log.Log(__FILE__, __FUNCTION__, __LINE__, "desc=stmt_exec sql=%s", sql);
  int32_t ret = -1;
  MYSQL_STMT* stmt = NULL;
  stmt = mysql_stmt_init(m_mysql);
  if( NULL == stmt )
  {
    LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [error=fail_init]", m_host.c_str(), m_port, m_db.c_str());
    return ret;
  }

  int32_t result = -1;

  do
  {
    result = mysql_stmt_prepare(stmt, sql, strlen(sql));
    if( 0 != result )
    {
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=fail_prepare result]", m_host.c_str(), m_port, m_db.c_str(), result);
      break;
    }

    if(mysql_stmt_param_count(stmt) != count)
    {
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [error=invalid_nparam] [nexpect=%lu] [ngot=%u]", m_host.c_str(), m_port, m_db.c_str(), mysql_stmt_param_count(stmt), count);
      break;
    }

    result = mysql_stmt_bind_param(stmt, args);
    if( 0 != result )
    {
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=fail_bind result]", m_host.c_str(), m_port, m_db.c_str(), result);
      break;
    }

    result = mysql_stmt_execute(stmt);
    if( 0 != result )
    {
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=fail_exec result]", m_host.c_str(), m_port, m_db.c_str(), result);
      break;
    }

    my_ulonglong affected = mysql_affected_rows(m_mysql);
    *affect_rows = static_cast<int>(affected);
    if ( 0 == affected )
    {//no matched row
      log.Debug(__FILE__,__FUNCTION__,__LINE__,"desc=match_0_rows");
      ret = 1;
      break;
    }

    if( NULL != id )
    {
      *id = mysql_insert_id(m_mysql);
    }

    LOGGER_DEBUG(log, "desc=success");
    ret = 0;
  }while(0);
  result = mysql_stmt_close(stmt);
  int64_t time_consume = TimeUtil::GetTickUs() - start;
  if (time_consume <= 500000) {
    LOGGER_LOG( log, "SQL[%s] desc=success_with_result target=%s time_consume=%ld", sql,
        GetHost().c_str(), time_consume );
  } else {
    LOGGER_LOG( log, "LONG QUERY SQL[%s] desc=success_with_result target=%s time_consume=%ld",
        sql, GetHost().c_str(), time_consume );
  }
  if( 0 != result )
  {
    LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [errorCode=%d] [error=fail_exec result]", m_host.c_str(), m_port, m_db.c_str(), result);
  }

  return ret;
}

int32_t db::CMysqlConn::Connect()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  LOGGER_DEBUG(log,"desc=connect conf=%s", m_conf.c_str());
  int32_t ret = -1;
  do
  {
    if( NULL == m_mysql )
    {
      LOGGER_ERROR(log, "desc=m_mysql_invalid");
      break;
    }

    if( m_valid )
    {
      break;
    }

    int32_t status = -1;
    if ( m_conned )
    {
      status = mysql_ping(m_mysql);
    }

    if ( 0 == status )
    {
      m_valid = true;
      break;
    }
    //mysql_thread_init();
    MYSQL* result = mysql_real_connect(m_mysql, m_host.c_str(), m_user.c_str(),
        m_passwd.c_str(), m_db.c_str(), m_port, NULL, CLIENT_FOUND_ROWS |
        CLIENT_MULTI_STATEMENTS );
    //mysql_thread_end();
    if( NULL == result )
    {
      LOGGER_ERROR(log, "[attention=mysql] [host=%s] [port=%d] [DBname=%s] [error=connect_fail]", m_host.c_str(), m_port, m_db.c_str());
      break;
    }
    /* TODO: maybe it is unnecessary to set names utf8, for the setting has been done by mysql_options */
    /*
    const char* cmd = "set names 'utf8'";
    ret = mysql_real_query(m_mysql,cmd,strlen(cmd));
    if(ret != 0){
      cout << "failed to set utf8"<<endl;
    }
    */



    //if( 0 == mysql_set_character_set(m_mysql, "utf8") )
    //{
    //	LOGGER_LOG(log, "set_client_character_set=%s", mysql_character_set_name(m_mysql));
    //}
    //else
    //{
    //	LOGGER_ERROR(log, "desc=set_names_utf8_failed");
    //}

    m_valid = true;
    m_conned = true;
    LOGGER_LOG( log, "desc=connect_success conf=%s", m_conf.c_str() );
  }while(0);

  if( ( NULL != m_mysql ) && m_valid )
  {
    if (strcmp(mysql_character_set_name(m_mysql),"utf8") == 0)
      ret = 0;
    else {
      ret = -1;
      std::cout << "not utf8 " << std::endl;
    }
  }
  return ret;
}

int32_t db::CMysqlConn::Ping()
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return -1;
  }
  return mysql_ping(m_mysql);
}

bool db::CMysqlConn::IsWorking() const
{
  static CLogger& log = CLogger::GetInstance(LOG_LIBSRC_MYSQL);
  if( !m_bInited )
  {
    LOGGER_ERROR( log, "desc=null_init" );
    return false;
  }
  return (m_mysql && m_valid);
}

