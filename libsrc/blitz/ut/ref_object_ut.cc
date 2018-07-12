// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include "blitz/ref_object.h"
#include "blitz/msg.h"



TEST(RefCountPtr, Construct) {
  {
    blitz::RefCountPtr< blitz::OutMsg > out_msg;
    blitz::OutMsg::CreateInstance(&out_msg);
    int32_t ref_count = out_msg->GetRefCount();
    EXPECT_EQ(ref_count, 1);
    EXPECT_TRUE(out_msg);
  }
  EXPECT_EQ(blitz::OutMsg::get_instance_count(), 0);
}
TEST(RefCountPtr, CopyConstruct) {
  {
    blitz::RefCountPtr< blitz::OutMsg > out_msg;
    blitz::OutMsg::CreateInstance(&out_msg);
    int32_t ref_count = out_msg->GetRefCount();
    EXPECT_EQ(ref_count, 1);
    blitz::RefCountPtr<blitz::OutMsg> msg2(out_msg);
    EXPECT_EQ(out_msg->GetRefCount(), 2);
    EXPECT_EQ(msg2->GetRefCount(), 2);
    EXPECT_TRUE(msg2);
  }
  EXPECT_EQ(blitz::OutMsg::get_instance_count(), 0);
}
TEST(RefCountPtr, Assign) {
  {
    blitz::RefCountPtr< blitz::OutMsg > out_msg;
    blitz::OutMsg::CreateInstance(&out_msg);
    blitz::RefCountPtr<blitz::OutMsg> msg2;
    EXPECT_FALSE(msg2);
    msg2 = out_msg;
    EXPECT_TRUE(msg2);
    EXPECT_EQ(out_msg->GetRefCount(), 2);
    EXPECT_EQ(msg2->GetRefCount(), 2);
  }
  EXPECT_EQ(blitz::OutMsg::get_instance_count(), 0);
}
TEST(RefCountPtr, AssignSelf) {
  {
    blitz::RefCountPtr< blitz::OutMsg > out_msg;
    blitz::OutMsg::CreateInstance(&out_msg);
    out_msg = out_msg;
    EXPECT_EQ(out_msg->GetRefCount(), 1);
  }
  EXPECT_EQ(blitz::OutMsg::get_instance_count(), 0);
}

TEST(RefCountPtr, Detach) {
  {
    blitz::RefCountPtr< blitz::OutMsg > out_msg;
    blitz::OutMsg::CreateInstance(&out_msg);
    EXPECT_EQ(blitz::OutMsg::get_instance_count(), 1);
    out_msg.Detach();
  }
  EXPECT_EQ(blitz::OutMsg::get_instance_count(), 1);
}
class ClassA {
 public:
  int a;
};

TEST(RefCountObject, CreateInstance) {
  printf("CreateInstance\n");
  typedef blitz::RefCountObject<ClassA> RefCountA;
  typedef blitz::RefCountPtr<RefCountA> RefCountAPtr;
  RefCountAPtr a = RefCountA::CreateInstance2();
  RefCountAPtr pointer_b;
  RefCountA::CreateInstance(&pointer_b);
  EXPECT_EQ(a->GetRefCount(), 1);
  EXPECT_EQ(pointer_b->GetRefCount(), 1);
}
