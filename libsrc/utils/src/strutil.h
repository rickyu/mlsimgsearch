#ifndef __LOG_STRUTIL_H__
#define __LOG_STRUTIL_H__

#include <vector>
#include <string>


namespace log
{
std::vector<std::string> split(const std::string& src, const std::string& det, unsigned int count = 0);
std::vector<std::string> split(const std::string& src, char det, unsigned int count = 0);
}


#endif // __LOG_STRUTIL_H__

