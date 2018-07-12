#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <tr1/functional>
#include <iostream>

const int MAX_LOOP = 10000000;
typedef void (*FnFunc)(int, void* );
static int64_t TimeDeltaUs(struct timeval& t1, struct timeval& t2) {
  int64_t us1 = t1.tv_sec * 1000000 + t1.tv_usec;
  int64_t us2 = t2.tv_sec * 1000000 + t2.tv_usec;
  return us2 - us1;
}
struct Result {
  int param;
  int result;
};
void c_func(int a, void* ptr) {
  Result* result = reinterpret_cast<Result*>(ptr);
  result->result = a + result->param;
}
void c_func2(int a) {
  std::cout<<"haha"<<std::endl;
}

class ObjFun {
 public:
  ObjFun(int param) {
    param_ = param;
    result_ = 0;
  }
  void operator()(int a) {
    result_ = a + param_;
  }
 private:
  int result_;
  int param_;
 
};
class VirtualBase {
 public:
  virtual void Func(int a) = 0;
  virtual ~VirtualBase() {}

};
class VirtualChild : public VirtualBase {
 public:
  VirtualChild(int param) {
    result_ = 0;
    param_ = param;
  }
  virtual void Func(int a)  {
    result_ = a + param_;
  }
 public:
  int result_;
  int param_;
 
};

Result g_result;
class ObjMember {
 public:
  explicit ObjMember(int param) {
    param_ = param;
    result_ = 0;
  }
  void Func(int a) {
    result_ = a + param_;
  }
  static void StaticFunc(int a, void* ptr) {
    ObjMember* p = reinterpret_cast<ObjMember*>(ptr);
    p->Func(a);
  }
 public:
  int param_;
  int result_;
};
ObjMember g_obj(10);
void TestFunctionPtr() {
  FnFunc func = c_func; 
  struct timeval start,end;
  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    func(i, &g_obj);
  }
  gettimeofday(&end, NULL);
  printf("result=%d function ptr:%ld\n", g_obj.result_, TimeDeltaUs(start, end));
}
void TestGlobalFunction() {
  struct timeval start,end;
  ObjMember obj(10);
  std::tr1::function<void(int, void*)> func(c_func);
  g_obj.result_ = 0;
  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    func(i, &g_obj);
  }
  gettimeofday(&end, NULL);
  printf("result=%d GlobalFunction:%ld\n", g_obj.result_, TimeDeltaUs(start, end));
}

void TestMemberFunction() {
  struct timeval start,end;
  ObjMember obj(10);
  std::tr1::function<void(int)> func =  std::tr1::bind(&ObjMember::Func, &obj, std::tr1::placeholders::_1);
  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    func(i);
  }
  gettimeofday(&end, NULL);
  printf("result=%d MemberFunction:%ld\n", obj.result_, TimeDeltaUs(start, end));
}
void TestVirtualFunction() {
  struct timeval start,end;
  VirtualChild* child = new VirtualChild(10);
  VirtualBase* p = child;
  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    p->Func(i);
  }
  gettimeofday(&end, NULL);
  printf("result=%d VirtualFunction:%ld\n", child->result_, TimeDeltaUs(start, end));
}
class A {
  int a[4];
};
struct B {
  int64_t b[6];
};
int G_VAR1 = 0;
void UnusedFunc(const B& b) {
  G_VAR1 += b.b[0];
}
void TestNewAndCopy() {
  struct timeval start,end;
  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    A* a = new A;
    delete a;
  }
  gettimeofday(&end, NULL);
  printf("new:%ld\n", TimeDeltaUs(start, end));

  B b;
  b.b[0] = 0;
  b.b[1] = 1;
  b.b[2] = 3;
  b.b[3] = 4;
  b.b[4] = 5;
  b.b[5] = 6;

  gettimeofday(&start, NULL);
  for (int i = 0; i < MAX_LOOP; ++i){
    B bb = b;
    UnusedFunc(bb);
  }
  gettimeofday(&end, NULL);
  printf("copy:%ld\n", TimeDeltaUs(start, end));

}

class MyState {
 public:
  MyState(int m1, int m2) {
    member1_ = m1;
    member2_ = m2;
  }
  int Callback1(int result, int arg2) {
    std::cout<<"callback1:" << "m1:" << member1_ << ",result" << result << std::endl;
  }
  int Callback2(int result, int arg2) {
    std::cout<<"callback2:" << "m1:" << member1_ << ",result" << result << std::endl;
  }
 private:
  int member1_;
  int member2_;
};

int main() {
//  TestNewAndCopy();
  TestFunctionPtr();
  TestMemberFunction();
  TestGlobalFunction();
  TestVirtualFunction();
  typedef std::tr1::function<void(int)> Callback;
  std::vector<Callback> callbacks;
  std::tr1::function<void(int)> p = c_func2;
  callbacks.push_back(c_func2);
  callbacks.push_back(ObjFun(1));
  callbacks.push_back(ObjFun(2));
  ObjMember obj(3);
  callbacks.push_back(std::tr1::bind(&ObjMember::Func, &obj, std::tr1::placeholders::_1));
  size_t n = callbacks.size();
  Callback* callback = &callbacks[0];
  for (size_t i = 0; i < n; ++i) {
    (*callback)(10);
    ++callback;
  }
  return 0;
}
