// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// Base server .
#ifndef __BLITZ2_BASE_SERVER_H_
#define __BLITZ2_BASE_SERVER_H_ 1
#include <blitz2/stream_socket_channel.h>
#include <blitz2/acceptor.h>
namespace blitz2 {
template<typename Decoder, typename ProcessorFunc>
class BaseServer:public StreamSocketChannel<Decoder>::Observer,
                   public Acceptor::Observer  {
 public:
  typedef Decoder TDecoder;
  typedef StreamSocketChannel<TDecoder> TChannel;
  /** because handler need hold channel to send reply, so we use ref count*/
  typedef SharedStreamSocketChannel<TDecoder> TSharedChannel;
 public:
  BaseServer(Reactor& reactor)
    :reactor_(reactor),
    logger_(Logger::GetLogger("blitz2::BaseServer")) {
    }
  virtual ~BaseServer() { }
  void set_decoder_init_param(const typename TDecoder::TInitParam& param) {
    decoder_init_param_ = param;
  }
  virtual void OnConnectAccepted(Acceptor* acceptor, int fd)  {
    (void)(acceptor);
    TSharedChannel* channel = TSharedChannel::CreateInstance(reactor_, 
        decoder_init_param_);
    channel->BeConnected(fd);
    channel->SetObserver(this);
  }
  virtual void OnClosed(TChannel* channel) {
    TSharedChannel* shared_channel = dynamic_cast<TSharedChannel*>(channel);
    shared_channel->Release();
  }
  virtual void OnMsgArrived(TChannel* channel, typename TDecoder::TMsg* msg) {
    TSharedChannel* shared_channel = dynamic_cast<TSharedChannel*>(channel);
    SharedPtr<TSharedChannel> channel_wrap(shared_channel);
    // ProcessMsg(msg, channel_wrap);
    processor_(msg, channel_wrap);
  }
  ProcessorFunc& processor() { return processor_;}
 protected:
  Reactor& reactor_;
  Logger& logger_;
  typename TDecoder::TInitParam decoder_init_param_;
  ProcessorFunc processor_;
};

} // namespace blitz2.
#endif // __BLITZ2_BASE_SERVER_H_


