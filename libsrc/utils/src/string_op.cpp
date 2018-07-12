#include "string_op.h"
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <errno.h>

std::string _configure_trim(const std::string& src, const std::string& drop)
{
    std::string ret = src;
    ret.erase(0, ret.find_first_not_of(drop));
    ret.erase(ret.find_last_not_of(drop) + 1);
    return ret;
}

std::string _configure_upper(const std::string& src)
{
    std::string ret = src;
    transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
    return ret;
}

void _configure_str_lower(std::string& str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}
void _configure_str_upper(std::string& str)
{
    transform(str.begin(), str.end(), str.begin(), ::toupper);
}

