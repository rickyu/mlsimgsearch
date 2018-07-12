/** 
 * \file mysql_conn.h
 * \brief mysql connection object
 * \author sjcui
 * \date 2009-08-25
 * Copyright (C) Baidu Company
 */

#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_H_

#include "imysql_conn.h"

namespace db{
extern const char* const LOG_LIBSRC_MYSQL; /// < defination is in mysql_conn.cpp >

class CMysqlConn :
    public IMysqlConn
{
public:
    CMysqlConn();
    virtual ~CMysqlConn();
    
    virtual int32_t Init( SMysqlConf& conf );
    virtual bool IsDefault() const;
    virtual void SetDefault();

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
    virtual int32_t Execute(const char* sql, uint64_t* id = NULL, bool ignore = false);
    
    virtual int32_t GetAffectRows() const{
      return static_cast<int32_t>(mysql_affected_rows( m_mysql ));
    }

    virtual int32_t BatchExecute( const char* sql, uint32_t size );
    
    /** 
     * \brief RealQuery
     *      use mysql_real_query
     * \param sql           [in] sql command(binary data allowed)
     * \param size          [in] the length of the sql 
     * \param ret           [out]Query result
     * 
     * \return              <0 for failure
     *                      0 for success with data
     *                      1 for success without data
     */
    virtual int32_t RealQuery( const char* sql, uint32_t size, MYSQL_RES** ret );
    
    /** 
     * \brief Query, deprecated for miss \param sqlSize
     * \param sql           [in] sql command(binary data allowed)
     * \param reason        [in] ignored, not used
     * \return              NULL	  fail	
     *                      not NULL  success
     */
    virtual MYSQL_RES* Query(const char* sql,int32_t* reason=NULL);
	
    /** 
     * \brief GetMysqlConn
     *      get raw mysql conn structure
     *    DONT free this structure without enough
     *    consideration of the conn obj 
     * 
     * \return a pointer to raw mysql conn structure
     */
	virtual MYSQL* GetMysqlConn() const
	{
		return m_mysql;
	}

    /** 
     * \brief GetHost
     *      get ip & port & db & user information
     * 
     * \return a information string
     */
    virtual std::string GetHost() const
	{
		return m_conf.c_str();
	}

    /** 
     * \brief Connect
     *      perform a connect operation on the conn obj
     *    a connect-success operation implies bool() will return true
     *    until a fail operation on the conn obj.
     * 
     * \return 0 for connect success, -1 otherwise
     */
    virtual int32_t Connect();

    /** 
     * \brief bool
     *      get the status of the connection
     * 
     * \return true for ok, false for error
     */
    virtual bool IsWorking() const;

    /** 
     * \brief GetNextResult
     *      iterator of the result from mysql-multi-Query 
     *    remember free the result(s) at the end of the process
     * \param ret               [out]the next result
     * 
     * \return                  -1 for failure, no more result 
     *                          0 for non-null result
     *                          >0 for null-result 
     */ 
    virtual int32_t GetNextResult( MYSQL_RES** ret, bool needAffected );
    
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
    virtual int32_t StmtExecute(const char* sql, uint32_t count, MYSQL_BIND* args, int* affect_rows, uint64_t* id = NULL);
    
    /** 
     * \brief BeginTransaction, DEPRECATED
     *       begin a transaction on the connection
     *
     * \return true for success, false for others 
     */
	virtual bool	BeginTransaction();
    
    /** 
     * \brief RollBack, DEPRECATED
     *      rollback the current transaction on the connection
     * 
     * \return true for success, false for others 
     */
	virtual bool	RollBack();
    
    /** 
     * \brief EndTransaction, DEPRECATED
     *      commit the current transaction on the connection
     * 
     * \return true for success, false for others 
     */
	virtual bool	EndTransaction();

    virtual int32_t Ping();

private:
    void LogError(const char* file, const char* func, int line, const char* prefix,
            int32_t err = 0);

private:
    MYSQL* m_mysql;
    
    std::string m_host;		///the host address of mysql server
    std::string m_user;		///the user account of mysql server
    std::string m_passwd;	///the user password of mysql server
    std::string m_db;		///the database name of mysql server
	uint16_t m_port;		///the listening port of mysql server
    
    bool m_valid;
    bool m_default;
    bool m_conned;	///indicating whether connected

    std::string m_conf;
    bool m_bInited;
};
}//end of namespace db
#endif

