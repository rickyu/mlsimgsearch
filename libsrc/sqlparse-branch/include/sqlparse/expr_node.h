#ifndef __SQLPARSER_EXPRNODE_H_
#define __SQLPARSER_EXPRNODE_H_

#include <vector>

namespace sqlparse {

typedef enum {
  AGG_EXPR = 0,  // 聚合函数表达式
  ARITHMETIC_EXPR, // 算术表达式
  BINARY_PRED,
  BOOL_LITERAL,
  DATE_LITERAL,
  FLOAT_LITERAL,
  INT_LITERAL,
  IN_PRED,
  UNKNOWN_LITERAL,
  IS_NULL_PRED,
  LIKE_PRED ,
  LITERAL_PRED,
  NULL_LITERAL,
  STRING_LITERAL,
  INT_LIST_LITERAL,
  UNK_LIST_LITERAL,
} ExprNodeType;


class ExprNode {
  public:
    ExprNode(ExprNodeType type) : type_(type) {};
    ExprNode() { type_ =0 ; left_=0; right_=0;};
    virtual ~ExprNode() {};

    int type_;
    //ExprNodeType type_;
    ExprNodeType argtype_;
    ExprNodeType rettype_;
    union
    {
      int64_t      iConst_;
      float        fConst_;
      int          iFunc_;
      char         sVal_[50];
      int          iUnknown_;
      bool         res_; 
    };

    std::vector<int64_t> listint_;
    std::vector<float> listfloat_;
  
    int left_;
    int right_;

  //std::vector<ExprNode*> children_;

};
}
#endif // __SQLPARSER_EXPRNODE_H_

