#include "gtest/gtest.h"
#include <blitz2/memcached/memcached_req.h>

using namespace blitz2;

struct MemcachedReporter {
  MemcachedReporter(const char *input) {
    (void)(input);
  }
  void operator()(MemcachedReq* msg) {
    (void)(msg); 
  }
};

TEST(MemcachedReqDecoder, Construct) {
  InitParam param;
  MemcachedReqDecoder decoder;
  EXPECT_EQ(decoder.Init(param), 0);
}
TEST(MemcachedReqDecoder, GetBuffer) {
  MemcachedReqDecoder decoder;
  InitParam param;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
}
TEST(MemcachedReqDecoder, Feed) {
  InitParam param;
  MemcachedReqDecoder decoder;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "get 1\r\n";
  memcpy(buf, data, strlen(data));
  MemcachedReporter report(data);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}
TEST(MemcachedReqDecoder, Feed2) {
  InitParam param;
  MemcachedReqDecoder decoder;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "set k1 0 0 2\r\nab\r\n";
  MemcachedReporter report(data);
  memcpy(buf, data, strlen(data));
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}

TEST(MemcachedReqDecoder, Feed3) {
  InitParam param;
  MemcachedReqDecoder decoder;
  decoder.Init(param);
  const char* data = "set k2 2 0 5\r\n12345\r\n";
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  memcpy(buf, data, length);
  MemcachedReporter report(data);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}

