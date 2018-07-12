#ifndef __IM_MISC_H__
#define __IM_MISC_H__

#include <vector>
#include <string>
#include <sstream>


class StringUtil {
  public:
    static void Split(const std::string& src, 
                const std::string& det,
                std::vector<std::string>& result); 

    /// ���ַ�����ָ�����ַ�ȥ��
    static void Trim(std::string& src, const std::string& drop = " \r\n\t");
    /// �ַ���ת��Ϊ��С������ֵΪ�ַ���
    static void ToUpper(std::string& str);

    static int StringToEnumValue(const char *str);

    static bool FilterMysqlFunc(const char *str);
    
    /// ���ַ���ת��ΪСд
    static void ToLower(std::string& str);
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

