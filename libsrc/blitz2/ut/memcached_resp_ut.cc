#include "gtest/gtest.h"
#include <blitz2/memcached/memcached_resp.h>

using namespace blitz2;

struct MemcachedResporter {
  MemcachedResporter(const char *input) {
    (void)(input);
  }
  void operator()(MemcachedResp* msg) {
    (void)(msg); 
  }
};

TEST(MemcachedRespDecoder, Construct) {
  InitParam param;
  MemcachedRespDecoder decoder;
  EXPECT_EQ(decoder.Init(param), 0);
}

TEST(MemcachedRespDecoder, GetBuffer) {
  MemcachedRespDecoder decoder;
  InitParam param;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  EXPECT_EQ(ret, 0);
}

TEST(MemcachedRespDecoder, Feed) {
  InitParam param;
  MemcachedRespDecoder decoder;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "STORED\r\n";
  memcpy(buf, data, strlen(data));
  MemcachedResporter report(data);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}

TEST(MemcachedRespDecoder, Feed2) {
  InitParam param;
  MemcachedRespDecoder decoder;
  decoder.Init(param);
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  const char* data = "VALUE k1 0 2\r\nab\r\nEND\r\n";
  MemcachedResporter report(data);
  memcpy(buf, data, strlen(data));
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}

TEST(MemcachedRespDecoder, Feed3) {
  InitParam param;
  MemcachedRespDecoder decoder;
  decoder.Init(param);
  const char* data = "VALUE k2 2 5\r\n12345\r\nEND\r\n";
  void* buf = NULL;
  uint32_t length = 0;
  int ret = decoder.GetBuffer(&buf, &length);
  memcpy(buf, data, length);
  MemcachedResporter report(data);
  ret = decoder.Feed(buf, strlen(data), report);
  EXPECT_EQ(ret, 0);
}

