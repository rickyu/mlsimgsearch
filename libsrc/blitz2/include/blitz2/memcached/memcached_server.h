// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// memcached server 应用相关类.
#ifndef __BLITZ2_MEMCACHED_SERVER_H_
#define __BLITZ2_MEMCACHED_SERVER_H_ 1
//#include <blitz2/logger.h>
//#include <blitz2/reactor.h>
//#include <blitz2/base_server.h>
//#include <blitz2/memcached/memcached_req.h>
namespace blitz2 {
//
// typedef SharedStreamSocketChannel<MemcachedReqDecoder> MemcachedChannel;
//  class Processor {
//   public:
//    void operator()(MemcachedReq* msg, SharedPtr<MemcachedChannel> channel) {
//       switch(msg->type()) {
//         case MemcachedReq::TYPE_GET:
//           ProcessGet(dynamic_cast<MemcachedGetReq*>(msg), channel);
//           break;
//         case MemcachedReq::TYPE_SET:
//           ProcessSet(dynamic_cast<MemcachedSetReq*>(msg), channel);
//           break;
//         default:
//           channel->Close();
//           break;
//       }
//    }
//    void ProcessGet(MemcachedGetReq* get_req, SharedPtr<MemcachedChannel> channel);
//    void ProcessSet(MemcachedSetReq* get_req, SharedPtr<MemcachedChannel> channel);
//  };
//  typedef BaseServer<MemcachedReqDecoder, Processor> MemcachedServer;
// 
} // namespace blitz2.
#endif // __BLITZ2_MEMCACHED_SERVER_H_


