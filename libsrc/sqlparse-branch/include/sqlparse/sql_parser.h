#ifndef __SQLPARSER_SQLPARSER_H_
#define __SQLPARSER_SQLPARSER_H_

#include <stdio.h>
#include <stdlib.h>
//#include <string>
#include "bison.tab.h"
//#include "flex.h"
#include "table_ref.h"
#include "common_sql.h"
#include "expr_node.h"

typedef void* yyscan_t;
#define YY_DECL int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner)
YY_DECL;

namespace sqlparse {

class SqlNode {
  public:
    int       start_;
    int       end_;
    char*     svalue_;
    int64_t   ivalue_;
    float     fvalue_;
    int       instype_;

    SqlNode():ivalue_(0){}
};



class SqlParser {
  public:
    //int   yylex ( YYSTYPE * lvalp, SqlParser * pParser );
    //int   yyparse ( SqlParser * pParser );
    //void  yyerror ( SqlParser * pParser, const char * sMessage );

  public:
    SqlParser () { expr_size_ =-1; ttype_=0; unknown_num_ = 0;nodes_.reserve(20);};
    void  Parser(char * sql);

  public:
    
    int   SetSqlType ( SqlType type );
    int   SetTable ( TableRef table);
    int   AddNodeConditions (int iType );
    int   AddNodeInt ( int64_t iValue );
    int   AddNodeFloat ( float iValue );
    int   AddNodeAttr ( char * sValue );
    int   AddNodeOp ( int iOp, int iLeft, int iRight);
    int   AddNodeFunc ( int iFunc, int iLeft, int iRight);
    int   AddNodeUnknown ( int iUnk);
    int   AddNodeUnknownList ( int iUnk);
    int   AddNodeIntList ( int64_t iValue );
    int   AddNodeBool ( bool flag );

  public:
    int   GetToken ( YYSTYPE * lvalp );
    int   CreateTree ( int node );
    int   WalkTree ( int node);
    ExprNode & AddNode();

  public:
    TableRef table_;
    //CommonSql *sql_;
    SqlType type_;
    int ttype_;
    int expr_size_;
    std::vector < ExprNode >  nodes_;
    int unknown_num_;
    const char * pbuf_;
    const char * lastTokenStart_;
    char * sql_;
    char * sqlcur_;
    //ExprNode* where_expr_;
};

}

#endif // __SQLPARSER_COMMONSQL_H_

