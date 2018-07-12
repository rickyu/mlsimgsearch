// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include "blitz/array_list.h"

TEST(ArrayList, Construct) {
  blitz::ArrayList<int> list;
  EXPECT_EQ(list.get_max_count(), 0);
  EXPECT_EQ(list.get_count(), 0);
}
TEST(ArrayList, PushBack) {
  blitz::ArrayList<int> list;
  EXPECT_EQ(list.Init(100), 0);
  EXPECT_EQ(list.Init(100), 1);
  list.PushBack(10);
  list.PushBack(20);
  EXPECT_EQ(list.get_count(), 2);
  EXPECT_EQ(list.First(), 1);
  EXPECT_EQ(list.Next(1), 2);
  EXPECT_EQ(*list.Get(1), 10);
  EXPECT_EQ(*list.Get(2), 20);
}
TEST(ArrayList, PushFront) {
  blitz::ArrayList<int> list;
  EXPECT_EQ(list.Init(100), 0);
  list.PushFront(10);
  list.PushFront(20);
  list.PushFront(30);
  EXPECT_EQ(list.get_count(), 3);
  EXPECT_EQ(list.First(), 3);
  EXPECT_EQ(list.Next(3), 2);
  EXPECT_EQ(list.Next(2), 1);
  EXPECT_EQ(*list.Get(1), 10);
  EXPECT_EQ(*list.Get(2), 20);
  EXPECT_EQ(*list.Get(3), 30);
}
TEST(ArrayList, InsertAfter) {
  blitz::ArrayList<int> list;
  EXPECT_EQ(list.Init(100), 0);
  EXPECT_EQ(list.InsertAfter(0, 10), 1);
  EXPECT_EQ(list.InsertAfter(1, 20), 2);
  EXPECT_EQ(list.InsertAfter(1, 30), 3);
  EXPECT_EQ(list.First(), 1);
  EXPECT_EQ(*list.Get(1), 10);
  EXPECT_EQ(list.Next(1), 3);
  EXPECT_EQ(*list.Get(3), 30);
  EXPECT_EQ(list.Next(3), 2);
  EXPECT_EQ(*list.Get(2), 20);
}
TEST(ArrayList, Remove) {
  blitz::ArrayList<int> list;
  EXPECT_EQ(list.Init(100), 0);
  EXPECT_EQ(list.InsertAfter(0, 10), 1);
  EXPECT_EQ(list.InsertAfter(1, 20), 2);
  EXPECT_EQ(list.InsertAfter(1, 30), 3);
  EXPECT_EQ(list.get_count(), 3);
  list.Remove(list.First());
  EXPECT_EQ(list.First(), 3);
  EXPECT_EQ(list.get_count(), 2);
  EXPECT_EQ(list.InsertAfter(list.Last(), 40), 1);
  EXPECT_EQ(list.get_count(), 3);
  list.Remove(2);
  EXPECT_EQ(list.get_count(), 2);
  EXPECT_EQ(list.First(), 3);
  EXPECT_EQ(list.Next(3), 1);
  EXPECT_EQ(list.Next(1), 0);
  EXPECT_EQ(list.Last(), 1);
  EXPECT_EQ(list.Prev(1), 3);
  EXPECT_EQ(list.Prev(3), 0);
}

