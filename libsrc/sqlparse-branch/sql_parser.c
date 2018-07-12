#include <stdio.h>
#include <stdlib.h>
#include "sqlparse/bison.tab.h"
#include "sqlparse/flex.h"
#include "sqlparse/sql_parser.h"

namespace sqlparse {

void SqlParser::Parser ( char * sql){
  //sql_     = sql;
  //sqlcur_  = sql;

  //iParsed_ = -1;
  //yyparse ( this );

  //CreateTree ( iParsed_ );

}

ExprNode & SqlParser::AddNode(){
  if (nodes_.size() >= nodes_.max_size())
    nodes_.reserve ( 1+ nodes_.size());
  return nodes_[nodes_.size()+1];


}

int SqlParser::AddNodeInt ( int64_t iValue) {
  ExprNode &node = nodes_[ ++expr_size_ ];
  node.type_ = INT_LITERAL; 
  node.iConst_ = iValue;
  return  expr_size_;
}

int SqlParser::AddNodeBool ( bool flag) {
  ExprNode &node = nodes_[ ++expr_size_ ];
  node.type_ = BOOL_LITERAL; 
  node.res_ = flag;
  return  expr_size_;
}

int SqlParser::AddNodeAttr ( char * sValue) {
  ExprNode &node = nodes_[ ++expr_size_ ];
  node.type_ = STRING_LITERAL; 
  strcpy(node.sVal_, sValue);
  return  expr_size_;
}

int  SqlParser::AddNodeUnknown ( int iUnk ) {
  ExprNode &node = nodes_[ ++expr_size_ ];
  node.type_ = UNKNOWN_LITERAL;
  unknown_num_++;
  return  expr_size_;
}

int SqlParser::AddNodeIntList( int64_t iValue) {
  ExprNode &node = nodes_[ expr_size_ ];
  node.type_ = INT_LIST_LITERAL; 
  node.listint_.push_back(iValue);
  return  expr_size_;

}

int SqlParser::AddNodeUnknownList (int iUnk) {
  unknown_num_++;
  ExprNode &node = nodes_[ expr_size_ ];
  node.type_ = UNK_LIST_LITERAL; 
  node.iUnknown_++;
  return  expr_size_;

}
int SqlParser::AddNodeOp ( int iOp, int iLeft, int iRight ) {
  //ExprNode & node = AddNode();
  
  ExprNode &node = nodes_[ ++expr_size_ ];
  node.type_ = iOp;
  
  //if ( iOp == '=' ) {
  //  node.argtype_ = INT_LITERAL; 
  //  node.rettype_ = INT_LITERAL;
  //}

  node.left_ = iLeft;
  node.right_ = iRight;
  return expr_size_;

}


int SqlParser::SetSqlType(SqlType type ) {
  type_ = type;
}


int SqlParser::GetToken ( YYSTYPE * lvalp ){
}

int SqlParser::CreateTree ( int iNode ) {
}

int SqlParser::WalkTree ( int iRoot ) {
  
  if ( iRoot >= 0 ) {
    const ExprNode & node = nodes_[iRoot];
    //TODO
    WalkTree ( node.left_ );
    WalkTree ( node.right_ );
  }
}

}
