#ifndef __SQLPARSER_TABLEREF_H_
#define __SQLPARSER_TABLEREF_H_
#include <string>
namespace sqlparse {

class TableRef {
 public:
  TableRef() : db_name_(""),table_name_(""),as_name_("") {}

 public:
  std::string db_name_;
  std::string table_name_;
  std::string as_name_;
};

}

#endif // __SQLPARSER_TABLEREF_H_

