#include "gtest/gtest.h"
#include <blitz2/line_msg.h>

using namespace blitz2;


TEST(LineMsgDecoder, Construct) {
  LineMsgDecoder decoder;
  EXPECT_EQ(decoder.data_length(), 0U);
  EXPECT_EQ(decoder.max_length(), 
            LineMsgDecoder::kDefaultMaxLineLength);
  EXPECT_EQ(decoder.Init(128u), 0);
  EXPECT_EQ(decoder.max_length(), 128u);
}

TEST(LineMsgDecoder, GetBuffer) {
  LineMsgDecoder decoder;
  decoder.Init(128u);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
  EXPECT_TRUE(buf != NULL);
  EXPECT_EQ(length, decoder.buffer_capacity());
}
struct CountFunc {
  CountFunc(size_t *count_ptr) {
    line_count = count_ptr;
  }
  void operator()(SharedLine* msg) {
    (void)(msg);
    ++(*line_count);
  }
  size_t* line_count;
}; 
struct CompareReporter {
  CompareReporter(const char* const* patterns, size_t count,
      size_t* equal_count_ptr, size_t* line_count_ptr) {
    this->patterns = patterns;
    this->pattern_count = count;
    this->equal_count = equal_count_ptr;
    this->line_count = line_count_ptr;
    index = 0;
  }
  void operator()(SharedLine* msg) {
    ++(*line_count);
    if (index < pattern_count) {
      if (msg->length() == strlen(patterns[index])  
          && strncmp(msg->line(), patterns[index], msg->length()) == 0) {
        ++(*equal_count);
      }
    }
    ++index;

  }
  size_t index;
  const char* const* patterns;
  size_t pattern_count;
  size_t *equal_count;
  size_t *line_count;
};
TEST(LineMsgDecoder, Feed) {
  LineMsgDecoder decoder;
  decoder.Init(16u);
  void* buf = NULL;
  uint32_t length = 0;
  size_t line_count = 0;
  size_t equal_count = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "12345";
  const char* data2 = "67\n";
  const char* const patterns[] =  {"1234567\n"};
  memcpy(buf, data, strlen(data));
  CompareReporter report(patterns, 1, &equal_count, &line_count);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(line_count, 0u);
  EXPECT_EQ(equal_count, 0u);
  line_count = 0u;
  equal_count = 0u;
  ret  = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
  memcpy(buf, data2, strlen(data2));
  ret = decoder.Feed(buf, strlen(data2), report);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(line_count, 1u);
  EXPECT_EQ(equal_count, 1u);
}
TEST(LineMsgDecoder, Feed2) {
  LineMsgDecoder decoder;
  decoder.Init(128u);
  void* buf = NULL;
  uint32_t length = 0;
  size_t line_count = 0;
  size_t equal_count = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "1234567\nabcd\nefg\n";
  const char* const patterns[] =  {"1234567\n", "abcd\n", "efg\n"};
  memcpy(buf, data, strlen(data));
  CompareReporter report(patterns, 3, &equal_count, &line_count);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(line_count, 3u);
  EXPECT_EQ(equal_count, 3u);
}
/** 测试行太长的情况 */
TEST(LineMsgDecoder, Feed3) {
  LineMsgDecoder decoder;
  decoder.Init(8u);
  const char* data = "123456789\n";
  const char* const patterns[] =  {"123456789\n"};
  void* buf = NULL;
  uint32_t length = 0;
  size_t line_count = 0;
  size_t equal_count = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  memcpy(buf, data, length);
  CompareReporter report(patterns, 3, &equal_count, &line_count);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, -1);
}


/*
TEST(LineMsgDecoder, Feed2) {
  // LineMsgDecoder decoder;
  // void* buf = NULL;
  // uint32_t length = 0;
  // int ret = decoder.GetBuffer(&buf, &length);
  // const char* data = "12345\n67\n\nabc";
  // memcpy(buf, data, strlen(data));
  // std::vector<InMsg> msgs;
  // ret = decoder.Feed(buf, strlen(data), &msgs);
  // EXPECT_EQ(ret, 0);
  // EXPECT_EQ(msgs.size(), 3u);
  // InMsg* msg = &msgs[0];
  // EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "12345\n") == 0);
  // ++msg;
  // EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "67\n") == 0);
  // ++msg;
  // EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "\n") == 0);
}
*/
