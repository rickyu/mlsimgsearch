#include <stdio.h>
#include <stdlib.h>
#include "sqlparse/bison.tab.h"
#include "sqlparse/flex.h"
#include "sqlparse/new_sqlparser.h"
#include "sqlparse/pool.h"
#include "sqlparse/help_flag.h"
extern int yyparse(void *YYPARSE_PARAM, NewSqlParser &myparser,pool::simple_pool<char>&mypool, struct Help_Flag &help_flag);
int ParseServer::ParseNewSql(const std::string &sql, NewSqlParser &myparser) {
  myparser.original_sql_ = sql;
  yyscan_t yy_scanner;
  yylex_init(&yy_scanner);
  yy_switch_to_buffer(yy_scan_string(sql.c_str(), yy_scanner), yy_scanner);
  pool::simple_pool<char>mypool;
  struct Help_Flag help_flag;
  int ret = yyparse(yy_scanner,myparser,mypool,help_flag);
  yylex_destroy (yy_scanner) ;
  return ret;
}
