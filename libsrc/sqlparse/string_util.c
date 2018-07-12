#include "sqlparse/string_util.h"
#include <algorithm>
#include <string.h>

using namespace std;



void StringUtil::Split(const string& src, 
                      const string& det, 
                      vector<string>& result) {
    string::size_type pos = 0;
    string::size_type use;
    string::size_type add = det.length();
    use = src.find(det);
    while(use != string::npos) {
        result.push_back(src.substr(pos, use - pos));
        pos = use + add;
        use = src.find(det, pos);
    }
    result.push_back(src.substr(pos));
}

void StringUtil::Trim(string& src, const string& drop) {
    src.erase(0, src.find_first_not_of(drop));
    src.erase(src.find_last_not_of(drop) + 1);
}

void StringUtil::ToUpper(string& str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void StringUtil::ToLower(std::string& str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}

int StringUtil::StringToEnumValue(const char *func) {
  std::string data(func);
  ToUpper(data);
  if(strcmp(data.c_str(), "AVG") == 0) {
    return 1;
  } else if(strcmp(data.c_str(), "MIN") == 0) {
    return 2;
  } else if(strcmp(data.c_str(), "MAX") == 0) {
    return 3;
  } else if(strcmp(data.c_str(), "SUM") == 0) {
    return 4;
  } else if(strcmp(data.c_str(), "COUNT") == 0) {
    return 5;
  } else if(strcmp(data.c_str(), "UNIX_TIMESTAMP") == 0) {
    return 9;
  } else if(strcmp(data.c_str(), "GREATEST") == 0) {
    return 6;
  } else if(strcmp(data.c_str(), "LEAST") == 0) {
    return 7;
  } else {
    return 0;
  }
}

bool StringUtil::FilterMysqlFunc(const char *str) {
  std::string data(str);
  ToUpper(data);
  if (data == "SLEEP" || data == "GET_LOCK" || data == "LAST_INSERT_ID" ||
      data == "RELEASE_LOCK" || data == "IS_FREE_LOCK" || data == "IS_USED_LOCK" ||
      data == "CONNECTION_ID") {
    return true;
  }

  return false;
}
