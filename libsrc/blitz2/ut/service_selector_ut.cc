// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <iostream>  // NOLINT(readability/streams)
#include "gtest/gtest.h"
#include <blitz2/service_selector.h>


using namespace blitz2;
TEST(RandomSelector, Construct) {
  RandomSelector<int, int> selector;
}
TEST(RandomSelector, AddItem) {
  RandomSelector<int, int> selector;
  EXPECT_EQ(selector.AddItem(1, 1, Configure()), 0);
  EXPECT_EQ(selector.AddItem(1, 1, Configure()), 1);
}

TEST(RandomSelector, Select) {
  RandomSelector<int,int> selector;
  selector.AddItem(1, 1, Configure());
  selector.AddItem(2, 2, Configure());
  OneSelectResult<int> select_result;
  EXPECT_EQ(selector.Select(0, &select_result), 0);
  EXPECT_EQ(select_result.id, 0);
  EXPECT_EQ(select_result.item, 1);
  EXPECT_EQ(selector.Select(0, &select_result), 0);
  EXPECT_EQ(select_result.id, 1);
  EXPECT_EQ(select_result.item, 2);
}
TEST(RandomSelector, SelectById) {
  RandomSelector<int,int> selector;
  selector.AddItem(1, 1, Configure());
  selector.AddItem(2, 2, Configure());
  OneSelectResult<int> select_result;
  EXPECT_EQ(selector.SelectById(0, &select_result), 0);
  EXPECT_EQ(select_result.id, 0);
  EXPECT_EQ(select_result.item, 1);
  EXPECT_EQ(selector.SelectById(1, &select_result), 0);
  EXPECT_EQ(select_result.id, 1);
  EXPECT_EQ(select_result.item, 2);
  EXPECT_EQ(selector.SelectById(2, &select_result), -1);
}

