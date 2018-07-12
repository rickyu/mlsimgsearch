#ifndef __CONFIGURE_STRING_OP_H_20100127_1415__
#define __CONFIGURE_STRING_OP_H_20100127_1415__

#include <vector>
#include <string>
#include <sys/time.h>

/// 将字符串中指定的字符去掉
std::string _configure_trim(const std::string& src, const std::string& drop = " \r\n\t");
/// 字符串转换为大小，返回值为字符串
std::string _configure_upper(const std::string& src);

/// 将字符串转换为小写
void _configure_str_lower(std::string& str);
/// 将字符串转换为大写
void _configure_str_upper(std::string& str);


#endif 

