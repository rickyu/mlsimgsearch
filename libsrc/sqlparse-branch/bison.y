%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sqlparse/sql_parser.h"
using namespace sqlparse;
void yyerror(void *param, SqlParser &myparser,const char *s, ...);
//void emit(const char *s, ...);
%}

%parse-param	{ void *parm }
%lex-param	{ void *parm }
%parse-param	{ SqlParser &pParser }
%pure-parser
%error-verbose


%union {
  int64_t ival;
  double fval;
  int subtok;
  char *sval;
}

%token	TOK_IDENT
%token	TOK_ATIDENT
%token	TOK_CONST_INT
%token	TOK_CONST_FLOAT
%token	TOK_CONST_MVA
%token	TOK_QUOTED_STRING
%token	TOK_USERVAR
%token	TOK_SYSVAR

%token	TOK_JOIN
%token	TOK_LEFT
%token	TOK_AS
%token	TOK_ASC
%token	TOK_AVG
%token	TOK_BEGIN
%token	TOK_BETWEEN
%token	TOK_BY
%token	TOK_CALL
%token	TOK_COLLATION
%token	TOK_COMMIT
%token	TOK_COUNT
%token	TOK_CREATE
%token	TOK_DELETE
%token	TOK_DESC
%token	TOK_DESCRIBE
%token	TOK_DISTINCT
%token	TOK_DIV
%token	TOK_DROP
%token	TOK_FALSE
%token	TOK_FLOAT
%token	TOK_FROM
%token	TOK_FUNCTION
%token	TOK_GLOBAL
%token	TOK_GROUP
%token	TOK_ID
%token	TOK_IN
%token	TOK_INSERT
%token	TOK_INT
%token	TOK_INTO
%token	TOK_LIMIT
%token	TOK_MATCH
%token	TOK_MAX
%token	TOK_META
%token	TOK_MIN
%token	TOK_MOD
%token	TOK_NAMES
%token	TOK_NULL
%token	TOK_OPTION
%token	TOK_ORDER
%token	TOK_RAND
%token	TOK_REPLACE
%token	TOK_RETURNS
%token	TOK_ROLLBACK
%token	TOK_SELECT
%token	TOK_SET
%token	TOK_SESSION
%token	TOK_SHOW
%token	TOK_SONAME
%token	TOK_START
%token	TOK_STATUS
%token	TOK_SUM
%token	TOK_TABLES
%token	TOK_TRANSACTION
%token	TOK_TRUE
%token	TOK_UPDATE
%token	TOK_VALUES
%token	TOK_VARIABLES
%token	TOK_WARNINGS
%token	TOK_WEIGHT
%token	TOK_WHERE
%token	TOK_WITHIN
%token	TOK_UNKNOWN
%token	TOK_ON


%left TOK_OR
%left TOK_AND
%left '|'
%left '.'
%left '&'
%left '=' TOK_NE
%left '<' '>' TOK_LTE TOK_GTE
%left '+' '-'
%left '*' '/' '%' TOK_DIV TOK_MOD
%nonassoc TOK_NOT
%nonassoc TOK_NEG

%{
// some helpers
#include <float.h> // for FLT_MAX

%}

%start request
%%

request:
   statement
   ;

statement:
	delete_from  { YYACCEPT; }
	| update { YYACCEPT; }
	| insert_into { YYACCEPT; }
	| select_from  { YYACCEPT; }
	;


//////////////////////////////////////////////////////////////////////////

select_from:
	TOK_SELECT select_items_list
	TOK_FROM ident_list
        opt_left_join_clause
	opt_where_clause
		{
			pParser.SetSqlType( SQL_SELECT );
			pParser.table_.table_name_ = $<sval>4;
		}
	;

select_items_list:
	select_item
	| select_items_list ',' select_item
	;

select_item:
	'*'						{ }
	| select_expr opt_alias
	;

opt_alias:

	| TOK_IDENT					{ } 
	| TOK_AS TOK_IDENT				{ }
	;

select_expr:
	expr						{  }
	| TOK_AVG '(' expr ')'				{  } 
	| TOK_MAX '(' expr ')'				{  }
	| TOK_MIN '(' expr ')'				{  }
	| TOK_SUM '(' expr ')'				{  } 
	| TOK_COUNT '(' '*' ')'				{  }
	| TOK_WEIGHT '(' ')'				{  }
	| TOK_COUNT '(' TOK_DISTINCT TOK_IDENT ')' 	{  }
	;
expr:
    TOK_IDENT {}
    ;

ident_list:
	TOK_IDENT
	| ident_list ',' TOK_IDENT			{  } 
	;

opt_left_join_clause:
        
	| left_join_item left_join_item 
	;
left_join_item:
	TOK_LEFT TOK_JOIN TOK_IDENT TOK_ON TOK_IDENT '.' TOK_IDENT '=' TOK_IDENT '.' TOK_IDENT {}
	;


//////////////////////////////////////////////////////////////////////////

insert_into:
	insert_or_replace TOK_INTO TOK_IDENT opt_column_list TOK_VALUES insert_rows_list
		{
			pParser.table_.table_name_ = $<sval>3;
		}
	;

insert_or_replace:
	TOK_INSERT		{ pParser.SetSqlType( SQL_INSERT ); } 
	| TOK_REPLACE	{ pParser.SetSqlType( SQL_REPLACE ); }
        ;

opt_column_list:
	// empty
	| '(' column_list ')'
	;

column_list:
	expr_ident		{ } 
	| column_list ',' expr_ident  { } 
	;

insert_rows_list:
	insert_row
	| insert_rows_list ',' insert_row
	;

insert_row:
	'(' insert_vals_list ')'{ } 
	;

insert_vals_list:
	insert_val							{}
	| insert_vals_list ',' insert_val	{ } 
	;

insert_val:
	const_num				{ } 
	| TOK_QUOTED_STRING		{}
	| '(' const_list ')'	{ }
	;

//////////////////////////////////////////////////////////////////////////

update:
	TOK_UPDATE TOK_IDENT TOK_SET update_items_list opt_where_clause
		{
			pParser.SetSqlType( SQL_UPDATE );
			pParser.table_.table_name_ = $<sval>2;
		}
	;

update_items_list:
        update_item
	| update_items_list ',' update_item
	;

update_item:
	TOK_IDENT '=' const_num { }
	;


//////////////////////////////////////////////////////////////////////////


delete_from:
	TOK_DELETE TOK_FROM TOK_IDENT opt_where_clause
		{
			pParser.SetSqlType( SQL_DELETE );
			pParser.table_.table_name_ = $<sval>3;
		}
	;

opt_where_clause:
        
       | TOK_WHERE where_expr
       | TOK_WHERE expr_ident TOK_BETWEEN where_between
       ;
         
where_between:
       const_num TOK_AND const_num   {}
       ;

where_expr:
	where_item 
	| where_expr TOK_AND where_item  { $<ival>$ = pParser.AddNodeOp ( TOK_AND, $<ival>1, $<ival>3 ); } 
	| where_expr TOK_OR where_item   { $<ival>$ = pParser.AddNodeOp ( TOK_OR, $<ival>1, $<ival>3 ); } 
	;

where_item:
        expr_ident '=' const_num     { $<ival>$ = pParser.AddNodeOp ( '=', $<ival>1, $<ival>3 ); }                   
	| expr_ident TOK_IN '(' const_list ')' { $<ival>$ = pParser.AddNodeOp ( TOK_IN, $<ival>1, $<ival>4 ); }
	| expr_ident TOK_NOT TOK_IN '(' const_list ')'  //{ $<ival>$ = pParser.AddNodeOp ( TOK_NOTIN, $<ival>1, $<ival>5 ); }
	| expr_ident '>' const_num  { $<ival>$ = pParser.AddNodeOp ( '>', $<ival>1, $<ival>3 ); } 
	| expr_ident '<' const_num  { $<ival>$ = pParser.AddNodeOp ( '<', $<ival>1, $<ival>3 ); }
	| expr_ident TOK_GTE const_num  { $<ival>$ = pParser.AddNodeOp ( TOK_GTE, $<ival>1, $<ival>3 ); }
	| expr_ident TOK_LTE const_num  { $<ival>$ = pParser.AddNodeOp ( TOK_LTE, $<ival>1, $<ival>3 ); }
	| expr_ident TOK_NE const_num   { $<ival>$ = pParser.AddNodeOp ( TOK_NE, $<ival>1, $<ival>3 ); }
        | const_num '=' const_num     { $<ival>$ = pParser.AddNodeOp ( '=', $<ival>1, $<ival>3 ); }                   
        | const_num '>' const_num     { $<ival>$ = pParser.AddNodeOp ( '>', $<ival>1, $<ival>3 ); }                   
        | const_num '<' const_num     { $<ival>$ = pParser.AddNodeOp ( '<', $<ival>1, $<ival>3 ); }                   
	| const_num  TOK_GTE expr_ident { $<ival>$ = pParser.AddNodeOp ( TOK_LTE, $<ival>3, $<ival>1 ); }
	| const_num  TOK_LTE expr_ident { $<ival>$ = pParser.AddNodeOp ( TOK_GTE, $<ival>3, $<ival>1 ); }
	| const_num  TOK_NE expr_ident { $<ival>$ = pParser.AddNodeOp ( TOK_NE, $<ival>3, $<ival>1 ); }
	;

expr_ident:
	TOK_IDENT  { $<ival>$ = pParser.AddNodeAttr ( $<sval>1 ); }
        ;
//////////////////////////////////////////////////////////////////////////

const_num:
	TOK_CONST_INT	    { $<ival>$ = pParser.AddNodeInt ( $<ival>1 ); } 	       
	| TOK_CONST_FLOAT	    { $<ival>$ = pParser.AddNodeFloat ( $<fval>1 ); } 	       
	| TOK_UNKNOWN       { $<ival>$ = pParser.AddNodeUnknown ( $<ival>1 ); }
	;

list_item:
	TOK_CONST_INT  { $<ival>$ = pParser.AddNodeIntList ( $<ival>1 ); }
	| TOK_UNKNOWN  { $<ival>$ = pParser.AddNodeUnknownList ( $<ival>1 ); }
	;
      
const_list:
	  list_item      //{ $<ival>$ = pParser.AddNodeIntList ( $<ival>1 ); }
	| const_list ',' list_item //{ $<ival>$ = pParser.AddNodeIntList ( $<ival>1 , $<ival>3); }
	;


//////////////////////////////////////////////////////////////////////////

%%

void yyerror(void *param, SqlParser &myparser,const char *s, ...) {
return ;
}


