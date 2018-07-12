%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlparse/bison.tab.h"
#include "sqlparse/new_sqlparser.h"
#include "sqlparse/string_util.h"
#include "sqlparse/pool.h"

void yyerror(void *param, NewSqlParser &parser,pool::simple_pool<char> &mypool, const char *s, ...);
%}
%s COMMENT
%option reentrant noyywrap 
%option bison-bridge  case-insensitive
%%
     int oldstate;
SELECT		{ return SELECT; }
SHOW		{ return SHOW; }
DELETE		{ return DELETE; }
UPDATE { return UPDATE; }
SET { return SET; }
NAMES { return NAMES; }
CHARSET { return CHARSET; }
AUTOCOMMIT { return AUTOCOMMIT; }
DEFAULT { return DEFAULT; }
INSERT { return INSERT; }
REPLACE { return REPLACE; }
INTO { return INTO; }
WHERE		{ return WHERE; }
FROM		{return FROM; }
VALUES		{return VALUES; }
VALUE		{return VALUE; }
AND    {return AND;}
OR   {return OR;}
IN   {return IN;}
ORDER   {return ORDER;}
GROUP   {return GROUP;}
BY   {return BY;}
ASC   {return ASC;}
AS   {return AS;}
DESC   {return DESC;}
IGNORE   {return IGNORE;}
FORCE   {return FORCE;}
INDEX   {return INDEX;}
LEFT   {return LEFT;}
RIGHT   {return RIGHT;}
INNER { return INNER; }
OUTER { return OUTER; }
ON    {return ON;}
JOIN   {return JOIN;}
DISTINCT   {return DISTINCT;}
BETWEEN   {return BETWEEN;}
LIMIT   {return LIMIT;}
NOT   {return NOT;}
IS    {return IS;}
NULL_SYM {return NULL_SYM;}
CASE {return CASE;}
WHEN {return WHEN;}
THEN {return THEN;}
ELSE {return ELSE;}
END {return END;}
START {return START;}
TRANSACTION {return TRANSACTION;}
COMMIT {return COMMIT;}
ROLLBACK {return ROLLBACK;}
EXPLAIN {return EXPLAIN;}
TIME_ZONE {return TIME_ZONE;}
GLOBAL {return GLOBAL;}
SESSION {return SESSION;}
AVG {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
MIN {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
MAX {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
SUM {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
COUNT {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
NOW {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
CURDATE {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
DATE {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
UNIX_TIMESTAMP {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
DATE_FORMAT {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return TIME_FUNC;}
SLEEP {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
GET_LOCK {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
LAST_INSERT_ID {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
RELEASE_LOCK {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
IS_FREE_LOCK {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
IS_USED_LOCK {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
CONNECTION_ID {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return AMMSC;}
GREATEST   {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return COMPEST;}
LEAST   {size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return COMPEST;}



"?"	{/*(*yylval).sval = strdup(yytext);*/ size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return PLACEHOLDER;}
"="	{(*yylval).subtok = 0; return COMPARISON;}
"<>" 	{(*yylval).subtok = 1; return COMPARISON;}
"!=" 	{(*yylval).subtok = 1; return COMPARISON;}
"<"	 {(*yylval).subtok = 2; return COMPARISON;}
">"	 {(*yylval).subtok = 3; return COMPARISON;}
"<="	{(*yylval).subtok = 4; return COMPARISON;}
">="		{ (*yylval).subtok = 5; return COMPARISON; }
"&&"		{ return AND; }
"||"		{ return OR; }
"/"+"*"  { oldstate = YYSTATE;BEGIN(COMMENT);}
<COMMENT>[^*\n]* ;
<COMMENT>"*"+"/" {BEGIN(oldstate);}
[A-Za-z_][A-Za-z0-9_]* {
    std::string data(yytext);
    StringUtil::ToUpper(data);
  if(strcmp(data.c_str(), "NULL") == 0)
  {
    return NULL_SYM;
  }
  if(strcmp(data.c_str(), "BEGIN") == 0)
  {
    return BEGIN_SYM;
  }
  size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext);
  //(*yylval).sval = strdup(yytext);
  return VAR;
}
`[^`/\\.\n]+` {
  //(*yylval).sval = strdup(yytext+1);
  size_t len = strlen(yytext+1)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext+1);
  (*yylval).sval[yyleng-2] = 0;
  return VAR;
}
[-+*/:(),.;@]  { return yytext[0]; }
([0-9]+"."[0-9]*)([Ee]?[+-]?[0-9]*) |
("."[0-9]*)([Ee]?[+-]?[0-9]*)	{/* (*yylval).sval = strdup(yytext);*/ size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext);return NUMVAL; }
[0-9]+	 {/*(*yylval).sval = strdup(yytext);*/size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return NUMVAL;}
'(\\.|''|[^'\n])*' |
\"(\\.|\"\"|[^"\n])*\" {/*(*yylval).sval = strdup(yytext);*/ size_t len = strlen(yytext)+1;(*yylval).sval = (char*)mypool.simple_pool_malloc(len); strcpy((*yylval).sval, yytext); return STRVAR;}
[ \t\n] {}
"--".*$ ;
. {}
%%
