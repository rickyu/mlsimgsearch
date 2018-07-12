/** 
 * \file imysql_conn.h
 * \brief mysql connection object interface
 * \author sjcui
 * \date 2009-08-26
 * Copyright (C) Baidu Company
 */

#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_IMYSQL_CONN_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_IMYSQL_CONN_H_
#include <mysql.h>
#include <string>
#include "utils/iref.h"

namespace db{
enum RESULT_STATUS
{
    MYSQL_ERROR_DUP_KEY = -1062,
    FAILED_OPERATION = -2,
    MYSQL_ERROR = -1,

    NON_NULL_RESULT = 0,
    
    NULL_RESULT = 1,
    NULL_RESULT_WITHOUT_AFFECTED_ROWS = 1,
    NULL_RESULT_WITH_AFFECTED_ROWS = 2,
};

/** 
 * \brief a structure to describe mysql config
 */
struct SMysqlConf
{
    /// host name( or ip addres )
    std::string m_host;
    /// login name
    std::string m_user;
    /// login passwd 
    std::string m_passwd;
    /// target database name 
    std::string m_db;
    /// table name
    std::string m_table;
    /// cluster name
    std::string m_cluster;
    /// connection timeout
    uint32_t m_connTimeout;
    /// port 
    uint16_t m_port;
    /// read timeout
    uint32_t m_readTimeout;
    /// write timeout
    uint32_t m_writeTimeout;
};

class IMysqlConn :
    public IRef
{
public:
    virtual ~IMysqlConn() = 0;

    /** 
     * \brief Init
     *    load the config of mysql
     * 
     * \param conf      [in] the config of mysql
     * 
     * \return      0 for success, -1 otherwise
     */
    virtual int32_t Init( SMysqlConf& conf ) = 0;
    /** 
     * \brief IsDefault
     *   check if the conn is default connection,
     *   used by Mysql Conn Pool
     * 
     * \return true for a default connection, false otherwise 
     */
    virtual bool IsDefault() const = 0;


    /** 
     * \brief SetDefault
     *   set the conn as default connection
     *   used by Mysql Conn Pool
     */
    virtual void SetDefault() = 0;
    
    /** 
     * \brief RealQuery
     *      use mysql_real_query, GOOD
     *      when sucess, \param ret should be freed with mysql_free_result
     *
     * \param sql           [in] sql command(binary data allowed)
     * \param size          [in] the length of the sql 
     * \param ret           [out]Query result
     * 
     * \return              <0 for failure
     *                      0 for success with data
     *                      1 for success without data
     */
    virtual int32_t RealQuery( const char* sql, uint32_t size, MYSQL_RES** ret ) = 0;
	
    /** 
     * \brief GetMysqlConn
     *      get raw mysql conn structure
     *    DONT free the return value without enough
     *    consideration of the conn obj 
     * 
     * \return a pointer to raw mysql conn structure
     */
	virtual MYSQL* GetMysqlConn() const = 0;

    /** 
     * \brief GetHost
     *      get ip & port & db & user information
     * 
     * \return a information string
     */
	virtual std::string GetHost() const = 0;

    /** 
     * \brief Connect
     *      perform a connect operation on the conn obj
     *    a connect-success operation implies bool() will return true
     *    until a fail operation on the conn obj.
     * 
     * \return 0 for connect success, -1 otherwise
     */
    virtual int32_t Connect() = 0;

    /** 
     * \brief IsWorking() 
     *      get the status of the connection
     * 
     * \return true for ok, false for error
     */
    virtual bool IsWorking() const = 0;

    /** 
     * \brief GetNextResult
     *      iterator of the result from mysql-multi-Query 
     *    remember free the result(s) at the end of the process
     * \param ret               [out]the next result
     * \param needAffected      [in] if it's true, this function will return
     *                          NULL_RESULT_WITHOUT_AFFECTED_ROWS for those
     *                          write operations which affect no rows. 
     * 
     * \return                  -1 for failure, no more result 
     *                          0 for non-null result
     *                          >0 for null-result 
     */
    virtual int32_t GetNextResult( MYSQL_RES** ret, bool needAffected ) = 0;

    /** 
     * \brief Execute, deprecated for miss \param sqlSize
     * \param sql           [in] sql command(binary data allowed)
     * \param id		    [out] return the id allocated by db when inserting a record into a table
     *                          with auto-increasing id, NULL means ignore the id
     * \param ignore        [in] whether ignore the situation that 0 rows affected
     * 
     * \return              <0 invalid input param
     *                      0  success
     *                      1  success but 0 rows affected or matched; or failure
     */
    virtual int32_t Execute(const char* sql, uint64_t* id = NULL, bool ignore = false) = 0;

    /** 
     * \brief GetAffectRows
     *                  returns the number of rows changed, deleted, or
     *                  inserted by the last statement 
     * \return              number of rows
     */
    virtual int32_t GetAffectRows() const = 0;

    /** 
     * \brief BatchExecute 批量执行sql语句，只关注成功或失败
     * 
     * \param sql           [in] sql command(binary data allowed)
     * \param size          [in] length 
     * 
     * \return              -1 failure
     *                      0  success
     */
    virtual int32_t BatchExecute( const char* sql, uint32_t size ) = 0;
    
    /** 
     * \brief Query, deprecated for miss \param sqlSize
     *      when sucess, the return value should be freed with mysql_free_result
     *
     * \param sql           [in] sql command(binary data allowed)
     * \param reason        [out] ignored, not used
     * \return              NULL	  fail	
     *                      not NULL  success
     */
    virtual MYSQL_RES* Query(const char* sql,int32_t* reason=NULL) = 0;
    
    /** 
     * \brief StmtExecute, DEPRECATED
     * \param sql           [in] sql command
     * \param count         [in] the count of arg in args
     * \param id		    [out] return the id allocated by db when inserting a record into a table
     *                           with auto-increasing id, NULL means ignore the id
     * 
     * \return              -1	failure
     *                      0	success
     *                      1	success but 0 rows affected or matched
     */
    virtual int32_t StmtExecute(const char* sql, uint32_t count, MYSQL_BIND* args, int* affect_rows, uint64_t* id = NULL) = 0;
    
    /** 
     * \brief BeginTransaction, DEPRECATED
     *       begin a transaction on the connection
     *
     * \return true for success, false for others 
     */
	virtual bool BeginTransaction() = 0;
    
    /** 
     * \brief RollBack, DEPRECATED
     *      rollback the current transaction on the connection
     * 
     * \return true for success, false for others 
     */
	virtual bool RollBack() = 0;
    
    /** 
     * \brief EndTransaction, DEPRECATED
     *      commit the current transaction on the connection
     * 
     * \return true for success, false for others 
     */
	virtual bool EndTransaction() = 0;

    /** 
     * \brief Ping
     *      check the connection to see if it's alive
     * 
     * \return 0 for connection alive, non-zero otherwise
     */
    virtual int32_t Ping() = 0;
};
}//end of namespace db
#endif

