#include <stdio.h>
#include <stdlib.h>
#include "sqlparse/bison.tab.h"
#include "sqlparse/flex.h"
#include "sqlparse/sql_parser.h"
extern int yyparse(void *parm, sqlparse::SqlParser &pParser);
int ParseNewSql(const std::string &sql, sqlparse::SqlParser &myparser) {
  yyscan_t yy_scanner;
  int temp = yylex_init(&yy_scanner);
  YY_BUFFER_STATE yy = yy_scan_string(sql.c_str(), yy_scanner);
  yy_switch_to_buffer(yy_scan_string(sql.c_str(), yy_scanner), yy_scanner);
  //printf("fdfasfdsa:%c\n", ((struct yyguts_t*)yy_scanner)->yy_hold_char);
  //yy_switch_to_buffer(yy_scan_string(sql.c_str(), yy_scanner), yy_scanner);
  int ret = yyparse(yy_scanner, myparser);
  yylex_destroy (yy_scanner) ;
  return ret;
};

//int main() {
//  return 0;
//}

int main () {
  char str[90] = "delete from t_twitter where id=5";
  std::string sql;
  sql = "delete from twitter where goods=5 and tt=7";
  //sql = "DELETE FROM twitter WHERE goods between ? and ?";
  //sql = "DELETE FROM twitter";
  //sql = "DELETE";
  //char str[90] = "delete";
  sqlparse::SqlParser sqlparser;// = new sqlparse::SqlParser();
  //printf("sqltype:%d\n", sqlparser.type_);
  //printf("unkonown:%d\n", sqlparser.unknown_num_);
  int ret = ParseNewSql(sql, sqlparser);
  //printf("sqltype:%d\n", sqlparser.type_);
  //printf("unkonown:%d\n", sqlparser.unknown_num_);
  //printf("table:%s\n", sqlparser.table_.table_name_.c_str());
  //printf("fdfsa:%d\n", sqlparser.nodes_.size());
  //printf("tpe:%d\n", sqlparser.nodes_[0].type_);
  //printf("tpe:%d\n", sqlparser.nodes_[1].right_);
  //printf("fdfsa:%d\n", sqlparser.nodes_.max_size());
  //printf("fdfsa:%d\n", sqlparser.expr_size_);
  //printf("left:%d\n", sqlparser.nodes_[2].left_);
  //printf("right:%d\n", sqlparser.nodes_[2].right_);
  printf("attr:%s\n", sqlparser.nodes_[0].sVal_);
  printf("attr:%d\n", sqlparser.nodes_[1].iConst_);
  //printf("table:%c\n", sqlparser.table_.table_name_);
  return ret;
}
