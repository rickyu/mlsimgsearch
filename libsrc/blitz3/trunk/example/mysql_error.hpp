#ifndef __MYSQL_ERROR_HPP_
#define __MYSQL_ERROR_HPP_
#include <boost/system/error_code.hpp>
class MysqlErrorCategory: public boost::system::error_category {
 public:
  MysqlErrorCategory() {}
  const char* name() const {
    return "mysql_error";
  }
  std::string message(int ev) const { 
    return "";
  }
};
const boost::system::error_category& GetMysqlCategory() {
  static const MysqlErrorCategory mysql_error_category_const;
  return mysql_error_category_const;
}
boost::system::error_code MysqlError(int val) {
  return boost::system::error_code(val, GetMysqlCategory());
}
#endif
