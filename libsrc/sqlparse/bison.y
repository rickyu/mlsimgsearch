%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sqlparse/new_sqlparser.h"
#include "sqlparse/pool.h"
#include "sqlparse/string_util.h"
#include "sqlparse/help_flag.h"
void yyerror(void *param, NewSqlParser &myparser, pool::simple_pool<char> &mypool, struct Help_Flag &help_flag, const char *s, ...);
void emit(const char *s, ...);
%}
%{
%}
%union {
  int ival;
  double doubleval;
  int subtok;
  char *sval;
}
%token <sval> SELECT
%token <sval> SHOW
%token <sval> UPDATE
%token <sval> SET
%token <sval> NAMES
%token <sval> CHARSET
%token <sval> AUTOCOMMIT
%token <sval> DEFAULT 
%token <sval> DELETE
%token <sval> INSERT
%token <sval> REPLACE
%token <sval> INTO
%token <sval> FROM
%token <sval> WHERE
%token <sval> VALUES
%token <sval> VALUE
%token <sval> IN
%token <sval> ON
%token <sval> ASC
%token <sval> DESC
%token <sval> AS
%token <sval> DISTINCT
%token <sval> BETWEEN
%token <sval> LEFT
%token <sval> RIGHT
%token <sval> INNER
%token <sval> OUTER
%token <sval> JOIN
%token <sval> ORDER
%token <sval> GROUP
%token <sval> BY
%token <sval> LIMIT
%token <sval> STRVAR
%token <sval> VAR
%token <sval> IGNORE
%token <sval> FORCE
%token <sval> INDEX
%token <sval> AMMSC
%token <sval> TIME_FUNC
%token <sval> COMPEST
%token <sval> NUMVAL
%token <sval> IS
%token <sval> CASE
%token <sval> WHEN
%token <sval> THEN
%token <sval> ELSE
%token <sval> END
%token <sval> PLACEHOLDER
%token <sval> NULL_SYM
%token <doubleval> DOUBLENUM
%token <sval> START
%token <sval> TRANSACTION
%token <sval> BEGIN_SYM
%token <sval> COMMIT
%token <sval> ROLLBACK
%token <sval> EXPLAIN
%token <sval> GLOBAL
%token <sval> SESSION
%token <sval> TIME_ZONE
%left <sval> OR
%left <sval> AND
%left <sval> NOT
%left <subtok> COMPARISON
%left '|'
%left '&'
%left '+' '-'
%left '*' '/' '%' 
%left '^'
%nonassoc UMINUS
%type <ival> tables
%parse-param {void *parm}
%lex-param {void *parm}
%lex-param {pool::simple_pool<char> &mypool}
%lex-param {struct Help_Flag &help_flag}
%parse-param {NewSqlParser &myparser}
%parse-param {pool::simple_pool<char> &mypool}
%parse-param {struct Help_Flag &help_flag}
%pure-parser
%start stmts
%%
stmts: stmt ';'
| stmts stmt ';'
;
stmt: 
  select_stmt { YYACCEPT; }
  | update_stmt { YYACCEPT; }
  | insert_stmt { YYACCEPT; }
  | delete_stmt { YYACCEPT; }
  | set_stmt { YYACCEPT; }
  | show_stmt { YYACCEPT; }
  | begin_stmt { YYACCEPT; }
  | commit_stmt { YYACCEPT; }
  | rollback_stmt { YYACCEPT; }
  ;

select_stmt: select_opt select_start select_scalar_exp_commalist FROM tables opt_as_alias opt_force_index opt_join_clause opt_where_clause opt_group_or_order opt_limit_clause {}

select_opt:
  | EXPLAIN {}
  ;

select_start:
  SELECT {if(myparser.sql_type_ == SQL_UNKNOWN) myparser.sql_type_ = SQL_SELECT;}
  ;

update_stmt: update_start table set_clause opt_where_clause  {}
  ;

update_start:
  UPDATE {myparser.sql_type_ = SQL_UPDATE; }
  ;

//update_set_clause:
//   set_clause
//  | update_set_oper_clause {myparser.update_set_oper_ = true;
//    myparser.find_set_ = true;}
//  ;
//update_set_oper_clause:
//  SET scalar_exp COMPARISON scalar_exp '+' atom {
//  //myparser.update_set_oper_clause_ = $<sval>2 + std::string("=");
//  //myparser.update_set_oper_clause_ += $<sval>4 ;
//  //myparser.update_set_oper_clause_ += std::string("+") ;
//  //myparser.update_set_oper_clause_ += $<sval>6 ;
//  //free($<sval>2);
//  //free($<sval>4);
//  //free($<sval>6);
//  myparser.set_num_ ++;
//  }
//  |
//  SET scalar_exp COMPARISON scalar_exp '-' atom {
//  myparser.set_num_ ++;
//  }
//  |
//  SET scalar_exp COMPARISON scalar_exp '*' atom {
//  myparser.set_num_ ++;
//  }
//  |
//  SET scalar_exp COMPARISON scalar_exp '/' atom {
//  myparser.set_num_ ++;
//  }
//  ;
set_stmt: 
  set_start NAMES VAR {}
  | set_start NAMES STRVAR {}
  | set_start CHARSET VAR {}
  | set_start CHARSET STRVAR {}
  | set_start AUTOCOMMIT COMPARISON NUMVAL {
      if(static_cast<FilterType>($3) != FILTER_EQ) {
        myparser.is_valid_sql_ = false;
        YYABORT;
      }
    }
  | set_start global_session_time_zone COMPARISON scalar_exp {
      myparser.sql_type_ = SQL_SET;
      myparser.is_support_sql_ = false;
    }
  ;

set_start:
  SET {myparser.sql_type_ = SQL_SET;}
  ;
/*
  | SET scalar_exp COMPARISON scalar_exp {myparser.sql_type_ = SQL_SET;
      if(static_cast<FilterType>($3) != FILTER_EQ) {
        myparser.is_valid_sql_ = false;
        YYABORT;
      }
    }
*/

global_session_time_zone:
  TIME_ZONE
  | GLOBAL TIME_ZONE
  | SESSION TIME_ZONE
  | '@' '@' GLOBAL '.' TIME_ZONE
  | '@' '@' SESSION '.' TIME_ZONE
  | '@' '@' TIME_ZONE
  ;

show_stmt: SHOW VAR {myparser.sql_type_ = SQL_SHOW; /*free($<sval>2);*/ }
  ;

begin_stmt: BEGIN_SYM {myparser.sql_type_ = SQL_BEGIN;}
  | START TRANSACTION {myparser.sql_type_ = SQL_START;}
  ;

commit_stmt: COMMIT {myparser.sql_type_ = SQL_COMMIT;}
  ;

rollback_stmt: ROLLBACK {myparser.sql_type_ = SQL_ROLLBACK;}
  ;

insert_stmt: insert_or_replace opt_ignore_into table opt_insert_column_list keyword_value insert_value_list {}
  | insert_or_replace opt_ignore_into table set_clause {}
  ;

insert_or_replace:
  INSERT {myparser.sql_type_ = SQL_INSERT;}
  | REPLACE {myparser.sql_type_ = SQL_REPLACE;}
  ;

opt_ignore_into:
  
  | IGNORE {myparser.use_ignore_ = true;}
  | INTO {myparser.use_into_ = true;}
  | IGNORE INTO {myparser.use_ignore_ = true; myparser.use_into_ = true;}
  ;

keyword_value:
  VALUE {myparser.find_values_ = true;}
  | VALUES {myparser.find_values_ = true;}
  ;

delete_stmt: delete_start FROM table opt_where_clause
  ;

delete_start:
  DELETE { myparser.sql_type_ = SQL_DELETE; }
  ;

set_clause:
  SET update_items_list { myparser.find_set_ = true;}
  ;

update_items_list:
       update_item   
  |  update_items_list ',' update_item {}
  ;

update_item:
  scalar_exp COMPARISON scalar_exp {
   // free($<sval>1); 
    //if (static_cast<FilterType>($2) != FILTER_EQ && myparser.sql_type_ == SQL_INSERT) YYABORT;
    if (static_cast<FilterType>($2) != FILTER_EQ) {
      myparser.is_valid_sql_ = false;
      YYABORT;
    }
    //if (myparser.sql_type_ == SQL_INSERT) {
    //  FilterExpr filter;
    //  filter.column_name_ = $<sval>1;
    //  myparser.filters_.push_back(filter);
    //}
  }
  | scalar_exp COMPARISON case_scalar_exp {
    if (static_cast<FilterType>($2) != FILTER_EQ) {
      myparser.is_valid_sql_ = false;
      YYABORT;
    }
  }
  ;

case_scalar_exp:
  case_start case_exp when_then_item_list else_item END {help_flag.find_end = true; if(help_flag.when_num != help_flag.then_num) YYABORT;}
  ;

case_start:
  CASE {help_flag.find_case = true;}
  ;

case_exp: {}
  | scalar_exp
  ;

when_then_item_list: when_then_exp  {}
  | when_then_item_list when_then_exp {}
  ;

when_then_exp: when_item_list then_item_list
  ;

when_item_list:
  when_start when_item
  ;

when_start:
  WHEN {help_flag.find_when = true; help_flag.when_num++;}
  ;

when_item:
  when_value
  |  search_condition {}
  ;

when_value:
  scalar_exp
  ;

then_item_list: 
  then_start statement_list {}
  ;

then_start: THEN {help_flag.find_then = true;help_flag.then_num++;}
  ;

statement_list:
  scalar_exp {}
  ;

else_item: 
  | else_start statement_list
  ;

else_start: ELSE {help_flag.find_else = true;}
  ;

opt_force_index:

  | FORCE INDEX '(' VAR ')' {
      myparser.force_index_ = $4;
      //free($4);
    }

opt_group_or_order:
  
  |  opt_order_clause 
  |  opt_group_clause
  |  opt_order_clause opt_group_clause
  |  opt_group_clause opt_order_clause
  ;

opt_order_clause:

   ORDER BY order_clause {
    int size = static_cast<int>(myparser.group_order_exprs_.size());
    myparser.group_order_exprs_[size - 1].type_ = 0;
  }
  ;
order_clause:
  ordering_spec opt_asc_desc
  ;
ordering_spec:
    ordering_spec ',' column_ref {
      int size = static_cast<int>(myparser.group_order_exprs_.size());
      if (size == 0) {
        YYABORT;
      }
      myparser.group_order_exprs_[size-1].column_names_.push_back($<sval>3);
    }
  | column_ref  {
      std::vector<std::string> column;
      column.push_back($<sval>1);
      GroupOrderExpr group_order_expr(column, 0, 0, 0);
      myparser.group_order_exprs_.push_back(group_order_expr);
      //free($<sval>1);
    }
  | function_ref 
  ;

opt_join_clause:
  
  | join_start column_ref ON column_ref COMPARISON column_ref {/*free($<sval>2);free($<sval>4);free($<sval>6);*/ myparser.is_direct_ = true;}
  ;

join_start:
  LEFT inner_or_outer JOIN
  | RIGHT inner_or_outer JOIN
//  | inner_or_outer JOIN {printf("none join\n");}
  ;

inner_or_outer:
  
  | INNER
  | OUTER
  ;

opt_asc_desc:
  
  | ASC  {
     myparser.group_order_exprs_[static_cast<int>(myparser.group_order_exprs_.size()) - 1].direction_ = 0; 
  }
  | DESC  {
     myparser.group_order_exprs_[static_cast<int>(myparser.group_order_exprs_.size()) - 1].direction_ = 1; 
  }
  ;

opt_group_clause:

   GROUP BY order_clause {
    myparser.group_order_exprs_[static_cast<int>(myparser.group_order_exprs_.size()) - 1].type_ = 1;
  }
  ;

opt_limit_clause:

   | LIMIT NUMVAL ',' NUMVAL {
     myparser.limit_start_ = atoll($2);
     myparser.limit_limit_ = atoi($4);
     //free($2); 
     //free($4);
     }
   | LIMIT NUMVAL   {
     myparser.limit_limit_ = atoi($2);
     myparser.limit_start_ = 0;
     //free($2);
     }
   ;

opt_where_clause:
                 {myparser.find_where_ = true;}
  | where_clause {myparser.find_where_ = true;}
  ;
where_clause:
  WHERE {help_flag.where_key = true;} search_condition
  ;

search_condition:
	|	search_condition OR search_condition  {
      myparser.filter_num_ ++;
      FilterExpr filter;
      filter.binaryop_ = BINARY_OR;
      filter.filterop_ = NOT_FILTER;
      myparser.filters_.push_back(filter);
    }
  | search_condition AND search_condition  {
      myparser.filter_num_ ++;
      FilterExpr filter; 
      filter.binaryop_ = BINARY_AND;
      filter.filterop_ = NOT_FILTER;
      myparser.filters_.push_back(filter);
    }
	|	'(' search_condition ')' 
	| predicate  {}
    | normal_atom AND search_condition {
      myparser.filter_num_ ++;
      FilterExpr filter;
      filter.binaryop_ = BINARY_AND;
      filter.filterop_ = NOT_FILTER;
      myparser.filters_.push_back(filter);
     // free($<sval>1);
    }
	/*|	NOT search_condition*/
	;
 
predicate:
    comparison_predicate
  | in_predicate
  | between_predicate
  ;
comparison_predicate:
    scalar_exp COMPARISON scalar_exp {
      int size = static_cast<int>(myparser.filters_.size());
      if (size == 0) //when's search_condition size=0, YYABORT
        YYABORT;
      if (myparser.filters_[size - 1].is_reverse_ == false) {
        myparser.filters_[size - 1].filterop_ = static_cast<FilterType>($2);
      } else {
        switch (static_cast<FilterType>($2)) {
          case FILTER_LT:
            myparser.filters_[size-1].filterop_ = FILTER_GT;
            break;
          case FILTER_LE:
            myparser.filters_[size-1].filterop_ = FILTER_GE;
            break;
          case FILTER_GT:
            myparser.filters_[size-1].filterop_ = FILTER_LT;
            break;
          case FILTER_GE:
            myparser.filters_[size-1].filterop_ = FILTER_LE;
            break;
          default:
            ;
        }
      }
      myparser.filter_num_ ++;
    }
  | scalar_exp IS is_item {
      std::string str_ret = "";
      if(help_flag.is_null == true)
      {
        str_ret="NULL";
      } else {
        str_ret="NOT NULL";
      }
      if(help_flag.find_case == true && help_flag.where_key == false) {
        int size = static_cast<int>(myparser.case_end_expr_.whenthen_vals_.size());
        if(help_flag.find_when == true) {
          if(size == help_flag.when_num) {
            myparser.case_end_expr_.whenthen_vals_[size-1].when_value_ = str_ret;
          } else {
            YYABORT;
          }
          help_flag.find_when = false;
        }
      } else {
        int size = static_cast<int>(myparser.filters_.size());
        myparser.filters_[size-1].filterop_ = FILTER_IS;
        if(myparser.find_table_ == true && myparser.find_where_ == false) {
          if(myparser.sql_type_ == SQL_UPDATE) {
            if(myparser.find_set_ == true) {
              if (size < myparser.filter_num_ + 1) {
                FilterExpr filter;
                filter.filter_vals_.push_back(str_ret);
                myparser.filters_.push_back(filter);
              } else {
              myparser.filters_[size - 1].filter_vals_.push_back(str_ret);
            }
          }
        } else if(myparser.sql_type_ == SQL_SELECT) { 
          if(size < myparser.filter_num_ + 1) {
            FilterExpr filter;
            filter.filter_vals_.push_back(str_ret);
            myparser.filters_.push_back(filter);
        } else {
            myparser.filters_[size - 1].filter_vals_.push_back(str_ret);
          }
        } else if(myparser.sql_type_ == SQL_DELETE) {
          if(size < myparser.filter_num_ + 1) {
            FilterExpr filter;
            filter.filter_vals_.push_back(str_ret);
            myparser.filters_.push_back(filter);
          } else {
            myparser.filters_[size - 1].filter_vals_.push_back(str_ret);
          }
        }
      } else {
          YYABORT;
        }
          myparser.filter_num_++;
      }
    }
  ;

is_item: 
  NULL_SYM  { help_flag.is_null = true;}
  | NOT NULL_SYM { help_flag.is_null = false;}
  ;

in_predicate:
    scalar_exp IN '(' atom_commalist ')' { 
      myparser.filters_[myparser.filters_.size() - 1].filterop_ = FILTER_IN;
      myparser.filter_num_ ++;
    }
  | scalar_exp NOT IN '(' atom_commalist ')'{ 
      myparser.filters_[myparser.filters_.size() - 1].filterop_ = FILTER_NOT_IN;
      myparser.filter_num_ ++;
    }
  | scalar_exp IN '(' select_stmt ')' { myparser.is_direct_  = true; }
  ;
between_predicate:
    scalar_exp BETWEEN scalar_exp AND scalar_exp {
      myparser.filters_[myparser.filters_.size() - 1].filterop_ = FILTER_BETWEEN;
      myparser.filter_num_ ++;
    }
  | scalar_exp NOT BETWEEN scalar_exp AND scalar_exp {
      myparser.filters_[myparser.filters_.size() - 1].filterop_ = FILTER_NOT_BETWEEN;
      myparser.filter_num_ ++;
    }
  ;

insert_value_list:
   '(' atom_commalist ')' {myparser.insert_vals_vec_.push_back(myparser.insert_vals_); myparser.insert_vals_.clear();}
   | insert_value_list ',' '(' atom_commalist ')' {myparser.insert_vals_vec_.push_back(myparser.insert_vals_); myparser.insert_vals_.clear();} 
   ;

atom_commalist:
     atom
   | atom_commalist ',' atom
   ;

scalar_exp:
		scalar_exp '+' scalar_exp 
	|	scalar_exp '-' scalar_exp
	|	scalar_exp '*' scalar_exp
	|	scalar_exp '/' scalar_exp
//	|	'+' scalar_exp %prec UMINUS
//	|	'-' scalar_exp %prec UMINUS
  | atom
	|	column_ref  {
    if (myparser.find_table_ == false && myparser.find_where_ == false) {
      std::vector<std::string> column;
      column.push_back($<sval>1);
      SqlParserColumnInfo column_info(column,"",0);
      myparser.select_columns_.push_back(column_info);
    } else if ((myparser.find_table_ == true && myparser.find_where_ == false)
              || myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_UPDATE ||
              myparser.sql_type_ == SQL_REPLACE) {
      if (myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_REPLACE) {
        FilterExpr filter; //default op = EQ
        filter.column_name_ = $<sval>1;
        myparser.sets_.push_back(filter);
      } else if (myparser.sql_type_ == SQL_UPDATE && myparser.find_set_ == false) {
        if(help_flag.find_case == true && help_flag.where_key == false) {
          int size = static_cast<int>(myparser.case_end_expr_.whenthen_vals_.size());
          if(help_flag.find_when == true) {
            if(help_flag.when_num == help_flag.then_num + 1) {
                WhenThenExpr when_then_expr_;
                when_then_expr_.when_column_name_ = $<sval>1;
                myparser.case_end_expr_.whenthen_vals_.push_back(when_then_expr_);
              } else {
                //free($<sval>1);
                YYABORT;
              }
            } else {
              myparser.case_expr_ = $<sval>1;
            }
          } else {
            FilterExpr filter;
            filter.column_name_ = $<sval>1;
            myparser.sets_.push_back(filter);
          }
      } else {
        int size = static_cast<int>(myparser.filters_.size());
        if (size < myparser.filter_num_ + 1) {
          FilterExpr filter;
          filter.column_name_ = $<sval>1;
          myparser.filters_.push_back(filter);
        } else {
          if (myparser.sql_type_ == SQL_SELECT || myparser.sql_type_ == SQL_UPDATE || 
               myparser.sql_type_ == SQL_DELETE) {
            if (myparser.filters_[size - 1].column_name_ == "") {
              if (myparser.filters_[size-1].filter_vals_.size() == 1) {
                myparser.filters_[size - 1].column_name_ = $<sval>1;
                myparser.filters_[size-1].is_reverse_ = true;
              } else {
                //free($<sval>1);
                myparser.is_valid_sql_ = false;
                YYABORT;
              }
            } else {
              /* twitter_id = twitter_show_type TODO*/
              //free($<sval>1);
             // YYABORT;
            }
          }
        }
      }
    } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
      std::vector<std::string> column;
      column.push_back($<sval>1);
      GroupOrderExpr group_order_expr(column, 0, 0, 0);
      myparser.group_order_exprs_.push_back(group_order_expr);
    }
    //free($<sval>1);
  }
  | function_ref
  | '(' scalar_exp ')'
	;
/*scalar_exp_commalist:
    scalar_exp
  | scalar_exp_commalist ',' scalar_exp
  ;*/

select_scalar_exp:
  scalar_exp opt_as_alias 
  ;


select_scalar_exp_commalist:
    select_scalar_exp
  | select_scalar_exp_commalist ',' select_scalar_exp
  | '*' {
      std::vector<std::string> column;
      column.push_back("*");
      SqlParserColumnInfo column_info(column,"",0);
      myparser.select_columns_.push_back(column_info);
  }
  | VAR'.''*' {
      std::vector<std::string> column;
      column.push_back("*");
      SqlParserColumnInfo column_info(column,"",0);
      myparser.select_columns_.push_back(column_info);
      //free($1);
  }
  ;

opt_as_alias:
  
  | AS VAR {
      if (myparser.find_table_ == false) {
        int size = static_cast<int>(myparser.select_columns_.size());
        myparser.select_columns_[size-1].as_name_ = $2;
      } else {
        myparser.table_.as_name_ = $2;
      }
    }
  | VAR {
     if (myparser.find_table_ == false) {
        int size = static_cast<int>(myparser.select_columns_.size());
        myparser.select_columns_[size-1].as_name_ = $1;
      } else {
        myparser.table_.as_name_ = $1;
      } 
    }
  | AS AMMSC { 
      if (myparser.find_table_ == false) {
        int size = static_cast<int>(myparser.select_columns_.size());
        myparser.select_columns_[size-1].as_name_ = $2;
      } else {
        myparser.table_.as_name_ = $2;
      }
    }
  | AMMSC {
      if (myparser.find_table_ == false) {
        int size = static_cast<int>(myparser.select_columns_.size());
        myparser.select_columns_[size-1].as_name_ = $1;
      } else {
        myparser.table_.as_name_ = $1;
      }
  }
  ;

tables: table {
  $$ = 1;
}
| tables ',' table {
  $$ = $1 + 1;
}
table: 
   VAR {
     myparser.table_.table_name_ = $1;
     myparser.find_table_ = true;
    // free($1);
   }
   | VAR '.' VAR {
     myparser.table_.table_name_ = $3;
     myparser.find_table_ = true;
    // free($1);
    // free($3);
   }
   ;
/*table:
   column_ref {
     myparser.table_.table_name_ = $<sval>1;
     myparser.find_table_ = true;
     free($<sval>1);
   }
   ;*/
atom:
    normal_atom {
      if (myparser.find_table_ == false && myparser.find_where_ == false) {
        std::vector<std::string> column;
        column.push_back($<sval>1);
        SqlParserColumnInfo column_info(column,"",0);
        myparser.select_columns_.push_back(column_info);
      } else if ((myparser.find_table_ == true && myparser.find_where_ == false) 
                || myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_UPDATE
                || myparser.sql_type_ == SQL_REPLACE) {
        if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_REPLACE) && myparser.find_values_) {
          myparser.insert_vals_.push_back($<sval>1);
        }
        //if (myparser.sql_type_ == SQL_INSERT || (myparser.sql_type_ == SQL_UPDATE && myparser.find_set_ == false)) {
        else if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_UPDATE 
                  || myparser.sql_type_ == SQL_REPLACE) && myparser.find_set_ == false) {
          if(help_flag.find_case == true && help_flag.where_key == false) {
            int size = static_cast<int>(myparser.case_end_expr_.whenthen_vals_.size());
            if(help_flag.find_when == true) {
              if(help_flag.when_num == help_flag.then_num + 1) {
               WhenThenExpr when_then_expr_;
               when_then_expr_.when_value_ = $<sval>1;
               myparser.case_end_expr_.whenthen_vals_.push_back(when_then_expr_);
              } else{
      //          free($<sval>1);
                YYABORT;
              }
              help_flag.find_when = false;
            } else if(help_flag.find_then == true) {
              if(help_flag.then_num == size) {
                myparser.case_end_expr_.whenthen_vals_[size-1].then_value_ = $<sval>1;
              } else {
        //        free($<sval>1);
                YYABORT;   
              }
              help_flag.find_then = false;
            } else if (help_flag.find_else == true) {
              myparser.case_end_expr_.else_value_ = $<sval>1;
            }
          } else {
            int size = static_cast<int>(myparser.sets_.size());
            myparser.sets_[size - 1].filter_vals_.push_back($<sval>1);
          }
        } else {      
          int size = static_cast<int>(myparser.filters_.size());
          if (size < myparser.filter_num_ + 1) {
            FilterExpr filter;
            filter.filter_vals_.push_back($<sval>1);
            myparser.filters_.push_back(filter);
          } else {
            myparser.filters_[size - 1].filter_vals_.push_back($<sval>1);
          }
        }
      } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
        std::vector<std::string> column;
        column.push_back($<sval>1);
        GroupOrderExpr group_order_expr(column, 0, 0, 0);
        myparser.group_order_exprs_.push_back(group_order_expr);
      } 
    //  free($<sval>1);
    }
   | '-' NUMVAL {
      std::string str_ret = $2;
      if (myparser.find_table_ == false && myparser.find_where_ == false) {
        std::vector<std::string> column;
        column.push_back("-"+str_ret);
        SqlParserColumnInfo column_info(column,"",0);
        myparser.select_columns_.push_back(column_info);
      } else if (myparser.find_table_ == true && myparser.find_where_ == false) {
        int size = static_cast<int>(myparser.sets_.size());
        if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_REPLACE) && myparser.find_values_) {
          myparser.insert_vals_.push_back("-"+str_ret);
        }
        else if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_UPDATE ||
                  myparser.sql_type_ == SQL_REPLACE) && myparser.find_set_ == false) {
          if(help_flag.find_case == true && help_flag.where_key == false) {
           if(help_flag.find_when == true) { 
             if(help_flag.when_num == help_flag.then_num + 1) {
               WhenThenExpr when_then_expr_;
               when_then_expr_.when_value_ = "-"+str_ret;
               myparser.case_end_expr_.whenthen_vals_.push_back(when_then_expr_);
             } else {
      //          free($2);
                YYABORT;
             }
            help_flag.find_when = false;
           } else if(help_flag.find_then == true) {
             size = static_cast<int>(myparser.case_end_expr_.whenthen_vals_.size());
             if(help_flag.then_num == size) {
               myparser.case_end_expr_.whenthen_vals_[size-1].then_value_ = "-"+ str_ret;
             } else {
       //         free($2);
                YYABORT;
             }
             help_flag.find_then = false;
           } else if (help_flag.find_else == true) {
             myparser.case_end_expr_.else_value_ = "-"+ str_ret;
             }
           } else {
             myparser.sets_[size - 1].filter_vals_.push_back("-"+str_ret);
           }
        } else {         
          int size_filter = static_cast<int>(myparser.filters_.size());
          if (size_filter < myparser.filter_num_ + 1) {
            FilterExpr filter;
            filter.filter_vals_.push_back("-"+str_ret);
            myparser.filters_.push_back(filter);
          } else {
            myparser.filters_[size_filter - 1].filter_vals_.push_back("-"+str_ret);
          }
        }
      } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
        std::vector<std::string> column;
        column.push_back("-"+str_ret);
        GroupOrderExpr group_order_expr(column, 0, 0, 0);
        myparser.group_order_exprs_.push_back(group_order_expr);
      }
 //     free($2);
     }
   | default_or_null {
      std::string str_ret = ""; 
      if(help_flag.find_default == true) {
        str_ret = "DEFAULT";
      } else {
        str_ret = "NULL";
      }
      if (myparser.find_table_ == true && myparser.find_where_ == false) {
        int size = static_cast<int>(myparser.sets_.size());
        if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_REPLACE) && myparser.find_values_) {
          myparser.insert_vals_.push_back(str_ret);
        } else if ((myparser.sql_type_ == SQL_INSERT || myparser.sql_type_ == SQL_UPDATE ||
                    myparser.sql_type_ == SQL_REPLACE) && myparser.find_set_ == false) {
          myparser.sets_[size - 1].filter_vals_.push_back(str_ret);
        }
      }
   }
  | function_ref { }
   ;

default_or_null:
  DEFAULT {help_flag.find_default = true;}
  | NULL_SYM {help_flag.find_default = false;}

normal_atom:
     NUMVAL
   | STRVAR
   | PLACEHOLDER { myparser.holder_num_ ++;}
   ;
function_ref:
    AMMSC '(' '*' ')' {
    int value = StringUtil::StringToEnumValue($1);
    if (myparser.find_table_ == false && myparser.find_where_ == false) {
      std::vector<std::string> column;
      column.push_back("*");
      SqlParserColumnInfo column_info(column,"",value);
      myparser.select_columns_.push_back(column_info);
    } else if (myparser.find_table_ == true && myparser.find_where_ == false) {
      myparser.is_valid_sql_ = false;
      YYABORT;
    } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
      std::vector<std::string> column;
      column.push_back("*");
      GroupOrderExpr group_order_expr(column, value, 0, 0);
      myparser.group_order_exprs_.push_back(group_order_expr);
    }
  }
  | AMMSC '(' DISTINCT column_ref ')' { int value = StringUtil::StringToEnumValue($1);/*free($<sval>4);*/YYABORT;}
  | AMMSC '(' scalar_exp ')' {
    if (StringUtil::FilterMysqlFunc($1)) {
      myparser.is_support_sql_ = false;
      YYABORT;
    }
    int value = StringUtil::StringToEnumValue($1);
    if (myparser.find_table_ == false && myparser.find_where_ == false) {
      int size = static_cast<int>(myparser.select_columns_.size());
      myparser.select_columns_[size-1].aggre_ = value;
    } else if (myparser.find_table_ == true && myparser.find_where_ == false) {
      /*
      int size = static_cast<int>(myparser.filters_.size());
      if (help_flag.func_in_value) {
        if (size < myparser.filter_num_ + 1) {
          FilterExpr filter;
          filter.filter_vals_.push_back($<sval>3);
          myparser.filters_.push_back(filter);
        } else {
          myparser.filters_[size - 1].filter_vals_.push_back($<sval>3);
        }
        help_flag.func_in_value = false;
      }
      */
      //myparser.is_valid_sql_ = false;
      //YYABORT;
    } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
      myparser.group_order_exprs_[myparser.group_order_exprs_.size() - 1].aggre_ = value;
    }
  }
  | COMPEST '(' column_ref ',' column_ref ')'  {
    if (StringUtil::FilterMysqlFunc($1)) {
      myparser.is_support_sql_ = false;
      YYABORT;
    }
    int value = StringUtil::StringToEnumValue($1);
    if (myparser.find_table_ == false && myparser.find_where_ == false) {
      std::vector<std::string> column;
      column.push_back($<sval>3);
      column.push_back($<sval>5);
      SqlParserColumnInfo column_info(column,"",value);
      myparser.select_columns_.push_back(column_info);
     // free($<sval>3);
     // free($<sval>5);
    } else if (myparser.find_table_ == true && myparser.find_where_ == false) {
     // free($<sval>3);
     // free($<sval>5);
      myparser.is_valid_sql_ = false;
      YYABORT;
    } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
      std::vector<std::string> column;
      column.push_back($<sval>3);
      column.push_back($<sval>5);
      GroupOrderExpr group_order_expr(column, value, 0, 0);
      myparser.group_order_exprs_.push_back(group_order_expr);
    //  free($<sval>3);
    //  free($<sval>5);
    }
  }
  | AMMSC '(' ')' { if (StringUtil::FilterMysqlFunc($1)) {myparser.is_support_sql_ = false; YYABORT;}}
  | TIME_FUNC '(' date_sys ',' format_sys ')' {}
  ;

date_sys:
  column_ref {
    int value = 0;
    if (myparser.find_table_ == false && myparser.find_where_ == false) {
      std::vector<std::string> column;
      column.push_back($<sval>1);
      SqlParserColumnInfo column_info(column,"",value);
      myparser.select_columns_.push_back(column_info);
    } else if (myparser.find_table_ == true && myparser.find_where_ == false) {
      int size = static_cast<int>(myparser.filters_.size());
      if (size < myparser.filter_num_ + 1) {
        FilterExpr filter;
        filter.column_name_ = $<sval>1;
        myparser.filters_.push_back(filter);
      } else {
        //myparser.filters_[size - 1].column_name_ = $<sval>1;
      }
    } else if (myparser.find_table_ == true && myparser.find_where_ == true) {
      std::vector<std::string> column;
      column.push_back($<sval>1);
      GroupOrderExpr group_order_expr(column, value, 0, 0);
      myparser.group_order_exprs_.push_back(group_order_expr);
    }
  }
  | normal_atom {}
  | function_ref {}
  ;

format_sys:
  | normal_atom {}
  ;

opt_insert_column_list:
  
  | '(' column_list ')'
  ;

column_list:
  column_ref          {  myparser.insert_columns_.push_back($<sval>1);/* free($<sval>1);*/ }
  |  column_list ',' column_ref {  myparser.insert_columns_.push_back($<sval>3);/* free($<sval>3);*/ }
  ;

column_ref:
		VAR 
	|	VAR '.' VAR	/* needs semantics */ {$<sval>$=$<sval>3;/*free($<sval>1);*/}
	|	VAR '.' VAR '.' VAR   {$<sval>$=$<sval>5;/*free($<sval>1);free($<sval>3);*/}
	;
%%

void yyerror(void *parm, NewSqlParser &myparser, pool::simple_pool<char> &mypool, struct Help_Flag &help_flag, const char *s, ...) {
  return;
}
void emit(const char *s, ...) {
  va_list ap;
  va_start(ap, s);
  fprintf(stdout, "line %d, sql_parse: ", 1);
  vfprintf(stdout, s, ap);
  fprintf(stdout, "\n");
}
