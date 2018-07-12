#include "string_util.h"
#include <algorithm>

using namespace std;

string StringUtil::IntToString(int num){
  char buffer[20];
  sprintf(buffer,"%d",num);
  return buffer;
}

string StringUtil::IntToString(unsigned int num) {
  char buffer[20];
  sprintf(buffer,"%u",num);
  return buffer;
}

//实现string到uint32_t的转换
uint32_t StringUtil::StringToUInt(const std::string& str) {
  uint32_t result(0);//最大可表示值为4294967296（=2‘32-1）
  //从字符串首位读取到末位（下标由0到str.size() - 1）
  for (int i=str.size()-1;i>=0;i--) {
    uint32_t temp(0),k = str.size()-i-1;
    //判断是否为数字
    if (isdigit(str[i])) {
      //求出数字与零相对位置
      temp = str[i] - '0';
      while (k--)
        temp *= 10;
      result += temp;
    }
    else
      break;
  }
  return result;
}

//实现string到uint64_t的转换
uint64_t StringUtil::StringToUInt64(const std::string& str) {
  uint64_t result(0);//最大可表示值为4294967296（=2‘32-1）
  int k=0;
  //从字符串首位读取到末位（下标由0到str.size() - 1）
  for (int i=str.size()-1;i>=0;i--) {
    uint64_t temp(0);
    k = str.size()-i-1;
    //判断是否为数字
    if (isdigit(str[i])) {
      //求出数字与零相对位置
      temp = str[i] - '0';
      while (k--)
        temp *= 10;
      result += temp;
    }
    else
      break;
  }
  return result;
}

void StringUtil::Split(const string& src, 
    const string& det, 
    vector<string>& result) {
  result.clear();
  string::size_type pos = 0;
  string::size_type use;
  string::size_type add = det.length();
  use = src.find(det);
  while(use != string::npos) {
    result.push_back(src.substr(pos, use - pos));
    pos = use + add;
    use = src.find(det, pos);
  }
  // fixed a bug for det is the end of src or empty src!
  if (pos < src.length())
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

std::string StringUtil::ReplaceAll(std::string old_string, std::string new_string, std::string& target) {
  int pos= 0;
  int size = old_string.size();
  int length = new_string.size();
  while ((pos = target.find(old_string, pos)) != std::string::npos) {
    target.replace(pos, size, new_string);
    pos += length;
  }
  return target;
}
