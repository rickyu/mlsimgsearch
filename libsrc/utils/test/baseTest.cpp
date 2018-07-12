#include <vector>
#include <cassert>
#include <iostream>
#include "utils/string_util.h"
#include "utils/json_parser.h"
#include "utils/md5_new.h"
#include "utils/common.h"

using namespace std;

int Test_RandomGenerator() {
  ServiceRandomGenerator gen1;
  gen1.Init(100);
  int tmp = -1;
  do {
    tmp = gen1.GetRandom();
    cout << tmp << std::endl;
  } while (tmp >= 0);
  return 0;
}

int Test_StringUtil_Split(){
  string s("abc,def,ghi");
  vector<string> result;
  StringUtil::Split(s,",",result);
  assert(result.size() == 3);
  cout<<result[0]<<endl;
  return 0;

}

void test_json_parser() {
  JsonParser jp;
  std::string text = "{\"function\":\"filterRange\",\"attribute\":\"catalog_id\",\"start\":2000000000000,\"end\":2999999999999,\"include\":True}";
  text = "{\"function\":\"filter\",\"attribute\":\"catalog_id\",\"value\":[\"2001017000000\",\"4829048920842903\"],\"include\":false}";
  bool a;
  jp.LoadMem(text.c_str(), text.size());
  int64_t b;
  int len;
  jp.GetBool("/include", a);
  jp.GetArrayNodeLength("/value",len);
  for(int i = 0; i < len; i++){
    std::string index = "/value[";
    index += StringUtil::IntToString(i);
    index += "]";
    jp.GetInt64(index.c_str(),b);
    std::cout << b << std::endl;
  }
}

void test_md5() {
  MD5 m;
  m.update("123", 3);
  m.finalize();
  string res = m.hexdigest();
  cout << res << endl;
}

int main(){
  //Test_StringUtil_Split();
  //test_json_parser();
  //test_md5();
  Test_RandomGenerator();
}
