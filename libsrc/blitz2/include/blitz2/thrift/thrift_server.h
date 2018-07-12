// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// thrift应用相关类.
#ifndef __BLITZ2_THRIFT_SERVER_H_
#define __BLITZ2_THRIFT_SERVER_H_ 1

#include <arpa/inet.h>
#include <unordered_map>
#include <boost/asio.hpp>
#include <TProcessor.h>
#include <protocol/TBinaryProtocol.h>
#include <TApplicationException.h>
#include <blitz2/fix_head_msg.h>
#include <blitz2/ref_object.h>
#include <blitz2/outmsg.h>
#include <blitz2/thrift/thrift_msg.h>
#include <blitz2/stream_socket_channel.h>
#include <blitz2/acceptor.h>
#include <blitz2/base_server.h>

namespace blitz2 
{
// 使用Thrift协议的异步服务.
// 用法:
// Reactor reactor;
// ThriftServier server(framework);
// server.RegisterHandler("GetOne", handler1);
// Acceptor acceptor(reactor);
// acceptor.Bind("tcp://127.0.0.1:7070");
// acceptor.Bind("tcp://127.0.0.1:7071");
// acceptor.SetObserver(&server);
// reactor.Start();
// Thrift异步处理服务器.
                        
class ThriftProcessor {
 public:
  typedef SharedStreamSocketChannel<ThriftFramedMsgDecoder> TSharedChannel;
  /**
   *int ProcessXxx(::apache::thrift::protocol::TProtocol* iprot,
                      int seqid, SharedPtr<TSharedChannel>& channel); 
   */
  typedef std::function<int(::apache::thrift::protocol::TProtocol*, int, SharedPtr<TSharedChannel>&)> THandler;
  typedef std::unordered_map<std::string, THandler> THandlerMap;
  void operator()(ThriftFramedMsgDecoder::TMsg* msg,
      SharedPtr<TSharedChannel> channel);
  int RegisterHandler(const char* fname, const THandler& handler) {
    handlers_[fname] = handler;
    return 0;
  }
  void UnregisterHandler(const char* fname) {
    handlers_.erase(fname);
  }
  THandler* GetHandler(const char* fname) {
    auto iter = handlers_.find(fname);
    if (iter == handlers_.end()) {
      return NULL;
    }
    return &(iter->second);
  }
  // 辅助函数，发送应答.
  template <typename T>
  static void SendReply( 
      TSharedChannel* channel,
                T& struc,
                const std::string& fname,
                ::apache::thrift::protocol::TMessageType mtype,
                int32_t seq_id) {
    SharedPtr<OutMsg> msg = CreateOutmsgFromThriftStruct(struc,
        fname, mtype, seq_id);
    channel->SendMsg(msg.get_pointer());
    
  }

 protected:
  THandlerMap handlers_;
};
typedef BaseServer<ThriftFramedMsgDecoder, ThriftProcessor> ThriftServer;
} // namespace blitz2
#endif // __BLITZ2_THRIFT_SERVER_H_
