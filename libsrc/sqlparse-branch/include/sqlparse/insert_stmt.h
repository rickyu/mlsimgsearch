#ifndef __SQLPARSER_INSERTSTMT_H_
#define __SQLPARSER_INSERTSTMT_H_

#include <vector>
#include <string>

#include "common_sql.h"
#include "expr_node.h"

namespace sqlparse {

class InsertStmt : public CommonSql {
 public:
  InsertStmt() : CommonSql(SqlType:SQL_INSERT) {}
  ~InsertStmt() {}

 private:
  std::vector<std::string> columns_;
  std::vector<ExprNode*> values_;
};

}

#endif // __SQLPARSER_INSERTSTMT_H_

