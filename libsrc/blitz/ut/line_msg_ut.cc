#include "gtest/gtest.h"
#include "blitz/line_msg.h"



TEST(LineMsgDecoder, Construct) {
  blitz::LineMsgDecoder decoder;
  EXPECT_EQ(decoder.get_data_length(), 0);
  EXPECT_EQ(decoder.get_max_length(), 
            blitz::LineMsgDecoder::kDefaultMaxLineLength);
  EXPECT_TRUE(decoder.get_buffer() == NULL);
  blitz::LineMsgDecoder decoder2(128);
  EXPECT_TRUE(decoder2.get_buffer() == NULL);
  EXPECT_EQ(decoder2.get_max_length(), 128);
}
TEST(LineMsgDecoder, GetBuffer) {
  blitz::LineMsgDecoder decoder;
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
  EXPECT_TRUE(buf != NULL);
  EXPECT_EQ(static_cast<int>(length), decoder.get_max_length());
  EXPECT_EQ(0, decoder.get_data_length());
  EXPECT_TRUE(buf == decoder.get_buffer());
}
TEST(LineMsgDecoder, Feed) {
  blitz::LineMsgDecoder decoder;
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "12345";
  const char* data2 = "67\n";
  memcpy(buf, data, strlen(data));
  std::vector<blitz::InMsg> msgs;
  ret = decoder.Feed(buf, strlen(data), &msgs);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(msgs.size(), 0u);
  ret  = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
  EXPECT_TRUE( reinterpret_cast<uint8_t*>(buf)
             == decoder.get_buffer() + strlen(data));
  memcpy(buf, data2, strlen(data2));
  ret = decoder.Feed(buf, strlen(data2), &msgs);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(msgs.size(), 1u);
  blitz::InMsg* msg = &msgs[0];
  EXPECT_TRUE(msg->data != NULL);
  EXPECT_TRUE(strncmp(reinterpret_cast<const char*>(msg->data), "1234567\n",
        msg->bytes_length) == 0);
  EXPECT_TRUE(msg->bytes_length == 8);
  EXPECT_TRUE(msg->fn_free != NULL);
}
TEST(LineMsgDecoder, Feed2) {
  blitz::LineMsgDecoder decoder;
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "12345\n67\n\nabc";
  memcpy(buf, data, strlen(data));
  std::vector<blitz::InMsg> msgs;
  ret = decoder.Feed(buf, strlen(data), &msgs);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(msgs.size(), 3u);
  blitz::InMsg* msg = &msgs[0];
  EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "12345\n") == 0);
  ++msg;
  EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "67\n") == 0);
  ++msg;
  EXPECT_TRUE(strcmp(reinterpret_cast<char*>(msg->data), "\n") == 0);
}

