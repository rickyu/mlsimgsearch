// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include <blitz2/ref_object.h>
#include <blitz2/outmsg.h>

using namespace blitz2;

TEST(RefCountPtr, Construct) {
  {
    SharedPtr< blitz2::OutMsg > out_msg = OutMsg::CreateInstance();
    int32_t ref_count = out_msg->GetRefCount();
    EXPECT_EQ(ref_count, 1);
    EXPECT_TRUE(out_msg);
  }
}
TEST(RefCountPtr, CopyConstruct) {
  {
    SharedPtr< blitz2::OutMsg > out_msg = OutMsg::CreateInstance();
    int32_t ref_count = out_msg->GetRefCount();
    EXPECT_EQ(ref_count, 1);
    SharedPtr<blitz2::OutMsg> msg2(out_msg);
    EXPECT_EQ(out_msg->GetRefCount(), 2);
    EXPECT_EQ(msg2->GetRefCount(), 2);
    EXPECT_TRUE(msg2);
  }
}
TEST(RefCountPtr, Assign) {
  {
    SharedPtr< blitz2::OutMsg > out_msg =  OutMsg::CreateInstance();
    SharedPtr<blitz2::OutMsg> msg2;
    EXPECT_FALSE(msg2);
    msg2 = out_msg;
    EXPECT_TRUE(msg2);
    EXPECT_EQ(out_msg->GetRefCount(), 2);
    EXPECT_EQ(msg2->GetRefCount(), 2);
  }
}
TEST(RefCountPtr, AssignSelf) {
  {
    SharedPtr< blitz2::OutMsg > out_msg = OutMsg::CreateInstance();
    out_msg = out_msg;
    EXPECT_EQ(out_msg->GetRefCount(), 1);
  }
}

TEST(RefCountPtr, Detach) {
  {
    SharedPtr< blitz2::OutMsg > out_msg = OutMsg::CreateInstance();
    out_msg.Detach();
  }
}
class ClassA {
 public:
  int a;
  void Init() {
  }
};

TEST(RefCountObject, CreateInstance) {
  printf("CreateInstance\n");
  typedef blitz2::SharedObject<ClassA> RefCountA;
  typedef SharedPtr<RefCountA> RefCountAPtr;
  RefCountAPtr a = RefCountA::CreateInstance();
  EXPECT_EQ(a->GetRefCount(), 1);
}
