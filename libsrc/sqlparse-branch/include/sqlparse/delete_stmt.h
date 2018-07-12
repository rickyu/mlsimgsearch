#ifndef __SQLPARSER_DELETESTMT_H_
#define __SQLPARSER_DELETESTMT_H_ 

#include "common_sql.h"

namespace sqlparse {

class DeleteStmt : public CommonSql {
 public:
  DeleteStmt() : CommonSql(SqlType::SQL_DELETE) {}
  ~DeleteStmt() {}

 private:
  ExprNode* where_expr_;
  LimitElem limit_;
};

}

#endif // __SQLPARSER_DELETESTMT_H_

