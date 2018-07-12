#ifndef __SQLPARSER_QUERYSTMT_H_
#define __SQLPARSER_QUERYSTMT_H_

#include <vector>
#include "common_sql.h"

namespace sqlparse {

class QueryStmt : public CommonSql {
 public:
  QueryStmt(SqpType type) : CommonSql(type) {}
  virtual ~QueryStmt() {}

  void Analyze(Analyzer analyzer);
  std::string ToString();

 private:
  std::vector<Expr> select_exprs_;

};

}

#endif //__SQLPARSER_QUERYSTMT_H_

