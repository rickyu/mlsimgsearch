#include <iostream>
#include <string>
using namespace std;

struct Help_Flag {
  bool find_case;
  bool find_end;
  bool where_key;
  bool is_null;
  bool find_default;
  bool find_when;
  bool find_then;
  bool find_else;
  bool func_in_value;
  int when_num;
  int then_num;
  Help_Flag():find_case(false),find_end(false),where_key(false),is_null(false),find_default(false),find_when(false),find_then(false),find_else(false), func_in_value(false), 
  when_num(0),then_num(0){}
};
