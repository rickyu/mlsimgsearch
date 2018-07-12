#ifndef __IM_MISC_H__
#define __IM_MISC_H__

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <stdint.h>


class StringUtil {
  public:
    static std::string IntToString(int num);
    static std::string IntToString(unsigned int num);
    static uint32_t StringToUInt(const std::string& str);
    static uint64_t StringToUInt64(const std::string& str);
    static void Split(const std::string& src, 
                const std::string& det,
                std::vector<std::string>& result); 

    static void Trim(std::string& src, const std::string& drop = " \r\n\t");
    static void ToUpper(std::string& str);
    
    static void ToLower(std::string& str);
    static std::string ReplaceAll(std::string old_string, std::string new_string, std::string& target);
    template<typename T>
    static void Implode(const std::string& glue,const std::vector<T>& arr,std::string& result) {
      using std::ostringstream;
      using std::string;
      using std::vector;
      vector<T> a;
      typename vector<T>::const_iterator iter = arr.begin();
      if(arr.empty()){
        return;
      }
      ostringstream oss;
      typename vector<T>::const_iterator iter_end = arr.end();
      oss << *iter;
      ++ iter;
      for(; iter != iter_end; ++ iter) {
        oss << glue << *iter  ;
      }
      result = oss.str();

    }
};

#endif // __IM_MISC_H__

