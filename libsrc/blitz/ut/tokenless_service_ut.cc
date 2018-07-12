// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include "blitz/tokenless_service_pool.h"
#include "blitz/msg.h"
#include "blitz/line_msg.h"
#include "blitz/service_selector.h"

TEST(TokenlessServicePool, Construct) {
  blitz::Framework framework;
  blitz::SimpleMsgDecoderFactory<blitz::LineMsgDecoder> decoder_factory;
  std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
  blitz::TokenlessServicePool service_pool(framework, &decoder_factory,
                                           selector);
}

TEST(TokenlessServicePool, AddService) {
  blitz::Framework framework;
  framework.Init();
  blitz::SimpleMsgDecoderFactory<blitz::LineMsgDecoder> decoder_factory;
  std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
  blitz::TokenlessServicePool service_pool(framework, &decoder_factory,
                                           selector);
  blitz::ServiceConfig service_config;
  int ret = service_pool.Init();
  EXPECT_EQ(ret, 0);
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, -2);
  service_config.set_addr("tcp://192.168.0.1:8080");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  blitz::RefCountPtr<blitz::TokenlessService> service =
    service_pool.GetService("tcp://192.168.0.1:8080");
  EXPECT_TRUE(service.get_pointer() != NULL);
  EXPECT_EQ(service->GetRefCount(), 2);
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 1);
  ret = service_pool.RemoveService("tcp://192.168.0.1:8081");
  EXPECT_EQ(ret, 1);
  ret = service_pool.RemoveService("tcp://192.168.0.1:8080");
  EXPECT_EQ(ret, 0);
  ret = service_pool.RemoveService("tcp://192.168.0.1:8080");
  EXPECT_EQ(ret, 1);
}
TEST(TokenlessServicePool, Uninit) {
  blitz::Framework framework;
  framework.Init();
  blitz::SimpleMsgDecoderFactory<blitz::LineMsgDecoder> decoder_factory;
  std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
  blitz::TokenlessServicePool service_pool(framework, &decoder_factory,
                                           selector);
  int ret = service_pool.Init();
  EXPECT_EQ(ret, 0);
  blitz::ServiceConfig service_config;
  service_config.set_addr("tcp://192.168.0.1:8080");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  service_config.set_addr("tcp://192.168.0.1:8081");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  ret = service_pool.Uninit();
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(blitz::TokenlessService::get_obj_count(), 0);
  ret = service_pool.Init();
  EXPECT_EQ(ret, 0);
  service_config.set_addr("tcp://192.168.0.1:8080");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  service_config.set_addr("tcp://192.168.0.1:8081");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  ret = service_pool.Uninit();
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(blitz::TokenlessService::get_obj_count(), 0);
}
TEST(TokenlessServicePool, Query) {
  blitz::Framework framework;
  framework.Init();
  blitz::SimpleMsgDecoderFactory<blitz::LineMsgDecoder> decoder_factory;
  std::shared_ptr<blitz::RandomSelector> selector(new blitz::RandomSelector());
  blitz::TokenlessServicePool service_pool(framework, &decoder_factory,
                                            selector);
  int ret = service_pool.Init();
  EXPECT_EQ(ret, 0);
  blitz::ServiceConfig service_config;
  service_config.set_addr("tcp://192.168.0.1:8080");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  service_config.set_addr("tcp://192.168.0.1:8081");
  ret = service_pool.AddService(service_config);
  EXPECT_EQ(ret, 0);
  // 测试选择算法.
  blitz::SelectKey key;
  blitz::RpcCallback callback;
  ret = service_pool.Query(NULL, key, callback);
  EXPECT_EQ(ret, -1);
  blitz::OutMsg* out_msg = NULL;
  blitz::OutMsg::CreateInstance(&out_msg);
  ret = service_pool.Query(out_msg, key, callback);
  EXPECT_EQ(ret, 0);
}
