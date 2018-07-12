#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_UTIL_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_MYSQL_CONN_UTIL_H_

#include <boost/noncopyable.hpp>
#include "imysql_conn.h"
#include "imysql_conn_pool.h"

namespace db
{
  using boost::noncopyable;
class CPoolHelper :
    public noncopyable
{
public:
    CPoolHelper(db::IMysqlRefPool* pool, uint32_t ms = 0, bool addRef = true);
    CPoolHelper(db::IMysqlPoolMgr* poolMgr, uint32_t rw, uint64_t key, uint32_t ms = 0, bool addRef = true);

    virtual ~CPoolHelper();
    
    virtual bool IsOk() const;
    virtual IMysqlConn* Get() const;

private:
    db::IMysqlRefPool* m_pool;
    db::IMysqlConn* m_conn;
    uint32_t	m_ms;
};

class CMysqlQueryResProxy
{
public:
    CMysqlQueryResProxy() :m_result(NULL) {}
    CMysqlQueryResProxy( MYSQL_RES* result );
    ~CMysqlQueryResProxy();

    /** 
     * @brief IsEmptySet
     *      only for read operation
     * @return 
     *      -1      read operation failed, or a write operation is performed
     *      0       is an empty select set
     *      >0      row number
     */
    int32_t IsEmptySet() const;
    /** 
     * \brief Get
     *     DONT free the returning pointer,
     *     CMysqlQueryResProxy object will free it.
     * 
     * \return      the proxy-pointer
     */
    MYSQL_RES* Get();

    void Set( MYSQL_RES* result ) { m_result = result; }

private:
    MYSQL_RES*     m_result;
};

}

#endif

