#ifndef __SQLPARSER_UPDATESTMT_H_
#define __SQLPARSER_UPDATESTMT_H_

#include <vector>
#include <string>
#include "expr_node.h"
#include "common_sql.h"

namespace sqlparse {

class UpdateStmt : public CommonSql {
 public:
  UpdateStmt() : CommonSql(SqlType::SQL_UPDATE) {}
  ~UpdateStmt() {}

 private:
  std::vector<std::string> set_columns_;
  std::vector<ExprNode*> set_values_;
  ExprNode* where_expr_;
  LimitElem limit_;
};

}

#endif // __SQLPARSER_UPDATESTMT_H_

