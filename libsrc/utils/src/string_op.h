#ifndef __CONFIGURE_STRING_OP_H_20100127_1415__
#define __CONFIGURE_STRING_OP_H_20100127_1415__

#include <vector>
#include <string>
#include <sys/time.h>

/// ���ַ�����ָ�����ַ�ȥ��
std::string _configure_trim(const std::string& src, const std::string& drop = " \r\n\t");
/// �ַ���ת��Ϊ��С������ֵΪ�ַ���
std::string _configure_upper(const std::string& src);

/// ���ַ���ת��ΪСд
void _configure_str_lower(std::string& str);
/// ���ַ���ת��Ϊ��д
void _configure_str_upper(std::string& str);


#endif 

