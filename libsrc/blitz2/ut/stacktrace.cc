// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include <blitz2/stacktrace.h>

using namespace blitz2;
using namespace std;
class ClassA {
 public:
  void Func() {
    PrintStacktrace(cout, 100);
  }
};
void MethodB(int a) {
    PrintStacktrace(cout, a);
}
TEST(PrintStacktrace, cout) {
  cout << "--------output in TEST-------" << endl;
  int ret = PrintStacktrace(cout, 10);
  EXPECT_EQ(ret, 0);
  cout << "--------output in MethodB-------" << endl;
  MethodB(100);
  cout << "--------output in ClassA::Func-------" << endl;
  ClassA a;
  a.Func();
}

