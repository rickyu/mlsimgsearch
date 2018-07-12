// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include "blitz/framework.h"

TEST(Framework, Constructor) {
  blitz::Framework framework;
  EXPECT_EQ(framework.get_thread_count(), 0);
  EXPECT_EQ(framework.get_max_fd_count(), 0);
  EXPECT_EQ(framework.IsReady(), false);
}
TEST(Framework, Init) {
  blitz::Framework framework;
  EXPECT_EQ(framework.Init(), 0);
  EXPECT_EQ(framework.get_max_fd_count(), 65535);
  EXPECT_EQ(framework.get_thread_count(), 1);
  EXPECT_EQ(framework.IsReady(), true);
  EXPECT_EQ(framework.Init(), 1);
}
TEST(Framework, Uninit) {
  blitz::Framework framework;
  framework.Uninit();
  EXPECT_EQ(framework.Init(), 0);
  framework.Uninit();
  EXPECT_EQ(framework.get_max_fd_count(), 0);
  EXPECT_EQ(framework.get_thread_count(), 0);
  EXPECT_EQ(framework.IsReady(), false);
  EXPECT_EQ(framework.Init(), 0);
}

TEST(Framework, set_thread_count) {
  blitz::Framework framework;
  EXPECT_EQ(framework.set_thread_count(5), 0);
  EXPECT_EQ(framework.get_thread_count(), 5);
  EXPECT_EQ(framework.set_thread_count(10), 0);
  EXPECT_EQ(framework.get_thread_count(), 10);
  EXPECT_EQ(framework.Init(), 0);
  EXPECT_EQ(framework.get_thread_count(), 10);
  framework.Uninit();
  EXPECT_EQ(framework.get_thread_count(), 0);
  EXPECT_EQ(framework.Init(), 0);
  EXPECT_EQ(framework.get_thread_count(), 1);
}

TEST(Framework, set_max_fd_count) {
  blitz::Framework framework;
  EXPECT_EQ(framework.set_max_fd_count(5), 0);
  EXPECT_EQ(framework.get_max_fd_count(), 5);
  framework.Uninit();
  EXPECT_EQ(framework.set_max_fd_count(1000), 0);
  EXPECT_EQ(framework.get_max_fd_count(), 1000);
  EXPECT_EQ(framework.Init(), 0);
  EXPECT_EQ(framework.get_max_fd_count(), 1000);
}


