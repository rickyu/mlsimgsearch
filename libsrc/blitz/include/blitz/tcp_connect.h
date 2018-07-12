#pragma once
#include <queue>
#include <boost/thread.hpp>
#include "msg.h"
#include "fd_manager.h"
#include "utils/time_util.h"
#include "runnable.h"

namespace blitz {
class MsgProcessor;
class MsgDecoder;
class MsgDecoderFactory;
class TcpConn : public IoHandler {
  public:
    TcpConn(Framework* framework,
        MsgProcessor* processor,
        MsgDecoderFactory* parser_factory,
        IoStatusObserver* status_observer
        );
    virtual ~TcpConn();
    //
    // @return 0: 连接成功, OnOpened不会被调用.
    //         1: 连接正在进行，将通过OnOpened进行通知.
    int Connect(const char* ip,uint16_t port);
    int PassiveConnected(int fd);
    int fd() const { return fd_;}
    //IoHandler interface.
    int OnRead();
    int OnWrite();
    int OnError();
    int Close() ;
    int SendMsg(OutMsg* msg);
    const char* get_local_name() const { return local_name_; }
    const char* get_remote_name() const { return remote_name_; }
  private:
    TcpConn(const TcpConn&);
    TcpConn& operator=(const TcpConn&);
    int TrySend();
    int CloseWithoutLock();
    int RemoveSentFailMsg();
    enum ConnectStatus {
      kClosed = 0,
      kConnecting = 1,
      kConnected = 2
    };
  public:
    int fd_;
    IoInfo ioinfo_;
    // uint64_t my_id_;
    std::string addr_;
    ConnectStatus connect_status_;
    Framework* framework_;
    MsgDecoder* msg_parser_;
    MsgProcessor* processor_;
    MsgDecoderFactory* msg_parser_factory_;
    //消息发送相关的成员.
    std::queue<OutMsg*> out_msg_queue_;
    OutMsg* sending_msg_;
    // const void* sending_data_;
    // size_t sending_data_length_;
    int data_sent_;
    int sending_buffer_index_;
    IoStatusObserver* status_observer_;
    char local_name_[32];
    char remote_name_[32];
};
} // namespace blitz.
