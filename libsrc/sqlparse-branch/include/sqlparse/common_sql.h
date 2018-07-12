#ifndef __SQLPARSER_COMMONSQL_H_
#define __SQLPARSER_COMMONSQL_H_

#include "table_ref.h"

namespace sqlparse {

typedef enum {
  SQL_INVALID = 0,
  SQL_SELECT,
  SQL_SELECT_JOIN,
  SQL_INSERT,
  SQL_UPDATE,
  SQL_DELETE,
  SQL_REPLACE,
} SqlType;

typedef struct {
  int start_;
  int64_t limit_;
} LimitElem;

class CommonSql {
 public:
  CommonSql(SqlType type) : type_(type) {}
  //CommonSql() { this(SQL_INVALID); }
  virtual ~CommonSql() {}
  virtual std::string ToString() = 0;

 private:
  TableRef table_;
  SqlType type_;
};

}

#endif // __SQLPARSER_COMMONSQL_H_

