// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <iostream>  // NOLINT(readability/streams)
#include "gtest/gtest.h"
#include "blitz/service_selector.h"

typedef int (*CompareFunc)(int, int);
int CompareInt(int a, int b) {
  return a-b;
}
TEST(BinarySearch, test1) {
  std::vector<int> array;
  std::pair<size_t, bool> search_result;
  int v = 1;
  search_result = blitz::BinarySearch(v, reinterpret_cast<int*>(NULL), 0,
                                      CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 0U);
  // push第一个元素.
  array.push_back(5);  // [5]
  search_result = blitz::BinarySearch(5, &array[0], 1, CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 0u);
  search_result = blitz::BinarySearch(4, &array[0], 1, CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 0u);
  search_result = blitz::BinarySearch(6, &array[0], 1, CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 1u);
  // push第2个元素.
  array.push_back(7);  // [5, 7]
  search_result = blitz::BinarySearch(7, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 1u);
  search_result = blitz::BinarySearch(8, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 2u);
  search_result = blitz::BinarySearch(6, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 1u);
  search_result = blitz::BinarySearch(5, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 0u);
  search_result = blitz::BinarySearch(4, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 0u);
  // push第3个元素.
  array.push_back(9); // [5, 7, 9]
  search_result = blitz::BinarySearch(11, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 3u);
  search_result = blitz::BinarySearch(9, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 2u);
  search_result = blitz::BinarySearch(8, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 2u);
  search_result = blitz::BinarySearch(7, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 1u);
  search_result = blitz::BinarySearch(6, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 1u);
  search_result = blitz::BinarySearch(5, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, true);
  EXPECT_EQ(search_result.first, 0u);
  search_result = blitz::BinarySearch(4, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 0u);
}
TEST(BinarySearch, test2) {
  std::vector<int> array;
  std::pair<size_t, bool> search_result;
  array.push_back(5);
  array.push_back(5); // [5, 5]
  search_result = blitz::BinarySearch(6, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 2u);
  search_result = blitz::BinarySearch(4, &array[0], array.size(), CompareInt);
  EXPECT_EQ(search_result.second, false);
  EXPECT_EQ(search_result.first, 0u);
}

TEST(RandomSelector, Construct) {
  blitz::RandomSelector selector;
}
TEST(RandomSelector, Select) {
  blitz::RandomSelector selector;
  int32_t service_id = 0;
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        blitz::PropertyMap(), &service_id), 0);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        blitz::PropertyMap(), &service_id), 0);
  blitz::SelectNode select_result;
  int count1 = 0;
  int count2 = 0;
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_result), 0);
  if (select_result.get_data() == reinterpret_cast<void*>(1)) {
    ++ count1;
  } else {
    ++ count2;
  }
  select_result.set_data(NULL);
   EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_result), 0);
  if (select_result.get_data() == reinterpret_cast<void*>(1)) {
    ++ count1;
  } else {
    ++ count2;
  }
  select_result.set_data(NULL);
   EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_result), 0);
  if (select_result.get_data() == reinterpret_cast<void*>(1)) {
    ++ count1;
  } else {
    ++ count2;
  }
  EXPECT_EQ(count1, 1);
  EXPECT_EQ(count2, 2);

}
TEST(RandomSelector, RemoveItem) {
  blitz::RandomSelector selector;
  int32_t service_id1 = 0;
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        blitz::PropertyMap(), &service_id1), 0);
  int32_t service_id2 = 0;
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        blitz::PropertyMap(), &service_id2), 0);
  EXPECT_EQ(selector.RemoveItem(service_id1+service_id2), 1);
  EXPECT_EQ(selector.RemoveItem(service_id1), 0);
  for (int i=0; i<10; ++i) {
    blitz::SelectNode select_node;
    EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_node), 0);
    EXPECT_EQ(select_node.get_data(), reinterpret_cast<void*>(2));
  }
}
TEST(RandomSelector, UpdateMark) {
  blitz::RandomSelector selector;
  int32_t service_id1 = 0;
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        blitz::PropertyMap(), &service_id1), 0);
  int32_t service_id2 = 0;
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        blitz::PropertyMap(), &service_id2), 0);
  EXPECT_EQ(selector.UpdateMark(service_id1, -100), 0);
  for (int i=0; i<10; ++i) {
    blitz::SelectNode select_node;
    EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_node), 0);
    EXPECT_EQ(select_node.get_data(), reinterpret_cast<void*>(2));
  }
  EXPECT_EQ(selector.UpdateMark(service_id2, -105), 0);
  EXPECT_EQ(selector.UpdateMark(service_id1, 50), 0);
  for (int i=0; i<10; ++i) {
    blitz::SelectNode select_node;
    EXPECT_EQ(selector.Select(blitz::SelectKey(), &select_node), 0);
    EXPECT_EQ(select_node.get_data(), reinterpret_cast<void*>(1));
  }

}

TEST(ConsistHashSelector, Construct) {
  blitz::ConsistHashSelector selector;
}

TEST(ConsistHashSelector, Select) {
  blitz::ConsistHashSelector selector;
  int32_t service_id = 0;
  blitz::PropertyMap properties;
  properties.SetInt("consist-hash", 5);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        properties, &service_id), 0);
  properties.SetInt("consist-hash", 10);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        properties, &service_id), 0);
  blitz::SelectNode select_result;
  select_result.set_data(NULL);
  blitz::SelectKey select_key;
  select_key.sub_key = 4;
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));

  select_key.sub_key = 5;
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));
  
  select_key.sub_key = 6;
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(2));
  
  select_key.sub_key = 11;
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));
}
TEST(ConsistHashSelector, RemoveItem) {
  blitz::ConsistHashSelector selector;
  int32_t service_id1 = 0;
  blitz::PropertyMap properties;
  properties.SetInt("consist-hash", 5);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        properties, &service_id1), 0);
  int32_t service_id2 = 0;
  properties.SetInt("consist-hash", 10);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        properties, &service_id2), 0);
  int32_t service_id3 = 0;
  properties.SetInt("consist-hash", 15);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(3), 2,
        properties, &service_id3), 0);

  blitz::SelectKey select_key;
  select_key.sub_key = 11;
  blitz::SelectNode select_result;
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(3));

  EXPECT_EQ(selector.RemoveItem(service_id1+service_id2+service_id3), 1);
  EXPECT_EQ(selector.RemoveItem(service_id3), 0);

  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));
}
TEST(ConsistHashSelector, UpdateMark) {
  blitz::ConsistHashSelector selector;
  int32_t service_id1 = 0;
  blitz::PropertyMap properties;
  properties.SetInt("consist-hash", 5);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(1), 1,
        properties, &service_id1), 0);
  int32_t service_id2 = 0;
  properties.SetInt("consist-hash", 10);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(2), 2,
        properties, &service_id2), 0);
  int32_t service_id3 = 0;
  properties.SetInt("consist-hash", 15);
  EXPECT_EQ(selector.AddItem(reinterpret_cast<void*>(3), 2,
        properties, &service_id3), 0);


  blitz::SelectKey select_key;
  select_key.sub_key = 11;
  blitz::SelectNode select_result;
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(3));

  EXPECT_EQ(selector.UpdateMark(service_id3, -100), 0);
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(1));

  EXPECT_EQ(selector.UpdateMark(service_id1, -105), 0);
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(2));
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(2));


  EXPECT_EQ(selector.UpdateAllMark(5), 0);
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(3));
  select_result.set_data(NULL);
  EXPECT_EQ(selector.Select(select_key, &select_result), 0);
  EXPECT_EQ(select_result.get_data(), reinterpret_cast<void*>(3));
}


