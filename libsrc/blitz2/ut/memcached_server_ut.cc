#include "gtest/gtest.h"
#include <blitz2/memcached.h>


using namespace blitz2;

typedef SharedStreamSocketChannel<MemcachedReqDecoder> MemcachedChannel;
class Processor {
 public:
  void operator()(MemcachedReq* msg, SharedPtr<MemcachedChannel> channel) {
     switch(msg->type()) {
       case MemcachedReq::TYPE_GET:
         ProcessGet(dynamic_cast<MemcachedGetReq*>(msg), channel);
         break;
       default:
         channel->Close(true);
         break;
       }
     }
  void ProcessGet(MemcachedGetReq* get_req, SharedPtr<MemcachedChannel> channel) {
    (void)(get_req);
    (void)(channel);
    channel->Close(true);
  }
 };
typedef BaseServer<MemcachedReqDecoder, Processor> MemcachedServer;

TEST(MemcachedServer, Construct) {
  Reactor reactor;
  MemcachedServer server(reactor);
}

