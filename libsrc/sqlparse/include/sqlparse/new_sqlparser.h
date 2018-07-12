#ifndef _NEW_SQLPARSER_H
#define _NEW_SQLPARSER_H
#include<iostream>
#include"bison.tab.h"
#include <stdint.h>
#include<vector>
#include<string>
#include "pool.h"

typedef void* yyscan_t;
#define YY_DECL int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner, pool::simple_pool<char>&mypool, struct Help_Flag &help_flag)
YY_DECL;

enum SqlType {
  SQL_UNKNOWN = 0,
  SQL_SELECT = 1,
  SQL_UPDATE = 2,
  SQL_INSERT = 3,
  SQL_REPLACE = 4,
  SQL_DELETE = 5,
  SQL_SET = 6,
  SQL_SHOW = 7,
  SQL_START = 8,
  SQL_BEGIN = 9,
  SQL_COMMIT = 10,
  SQL_ROLLBACK = 11
};

enum FilterType {
  NOT_FILTER = -1,
  FILTER_EQ = 0,
  FILTER_NE = 1,
  FILTER_LT = 2,
  FILTER_GT = 3,
  FILTER_LE = 4,
  FILTER_GE = 5,
  FILTER_IN = 6,
  FILTER_NOT_IN = 7,
  FILTER_BETWEEN = 8,
  FILTER_NOT_BETWEEN = 9,
  FILTER_IS = 10,
  FILTER_IS_NOT = 11
};

enum BinaryOp {
  NOT_AND_OR = 0,
  BINARY_AND = 1,
  BINARY_OR = 2
};

class SqlParserColumnInfo {
  public:
    std::vector<std::string> column_names_;
    std::string as_name_;
    int aggre_;
  public:
    SqlParserColumnInfo(std::vector<std::string> column_names, std::string as_name, int aggre):
       column_names_(column_names),as_name_(as_name),aggre_(aggre) {}
};

class ParserTableInfo {
  public:
    std::string table_name_;
    std::string as_name_;
  public:
    ParserTableInfo() {table_name_="";as_name_="";}
};

class GroupOrderExpr {
  public:
    std::vector<std::string> column_names_;
    int aggre_;
    int type_;
    int direction_;
  public:
    GroupOrderExpr(std::vector<std::string> column_names, int aggre, int type, int direction):
      column_names_(column_names), aggre_(aggre), type_(type), direction_(direction) {}
};

class FilterExpr {
  public:
    std::string column_name_;
    FilterType filterop_;
    std::vector<std::string> filter_vals_;
    BinaryOp binaryop_;
    bool is_reverse_;
  public:
    FilterExpr() {column_name_ = "";  filterop_ = FILTER_EQ; binaryop_ = NOT_AND_OR; is_reverse_ = false;}
};

class WhenThenExpr {
  public:
    std::string when_column_name_;
    std::string when_value_;
    std::string then_value_;

  public:
    WhenThenExpr() {when_value_ = ""; then_value_ = ""; when_column_name_="";}
};

class CaseEndExpr {
  public:
    std::string else_value_;
    std::vector<WhenThenExpr> whenthen_vals_;
  public:
    CaseEndExpr() {else_value_ = "";}
};

class NewSqlParser {
  public:
    std::string original_sql_;
    std::vector<SqlParserColumnInfo> select_columns_;
    ParserTableInfo table_;
    std::vector<GroupOrderExpr> group_order_exprs_;
    std::vector<FilterExpr> filters_;
    std::vector<FilterExpr> sets_;
    std::string force_index_;
    std::string update_set_oper_clause_;
    int64_t limit_start_;
    int limit_limit_;
    std::vector<std::string> insert_columns_;  // insert into table(c1, ...)中的列名
    std::vector<std::string> insert_vals_; // insert into table(..) values(v1, ...)中的列值
    std::vector<std::vector<std::string> > insert_vals_vec_; // insert into table(..) values(v1, ...)中的列值
    CaseEndExpr case_end_expr_;
    std::string case_expr_;
  public:
    bool find_table_;
    bool find_where_;
    bool find_set_;
    bool find_values_;
    bool use_ignore_;
    bool use_into_;
    bool update_set_oper_;
    int filter_num_;
    int holder_num_;
    bool is_direct_;
    SqlType sql_type_;
    bool is_valid_sql_;
    bool read_master_;
    bool is_support_sql_;
  public:
    NewSqlParser() {
        find_table_ = false;
        find_where_ = false;
        find_set_ = false;
        find_values_ = false;
        use_ignore_ = false;
        use_into_ = false;
        update_set_oper_ = false;
        filter_num_ = 0;
        holder_num_ = 0;
        limit_start_ = 0;
        limit_limit_ = -1;
        is_direct_ = false;
        is_valid_sql_ = true;
        read_master_ = false;
        is_support_sql_ = true;
        sql_type_ = SQL_UNKNOWN;
    }
};
class ParseServer {
  public:
    static int ParseNewSql(const std::string& sql, NewSqlParser& myparser);
};

#endif
