// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// StreamSocketChannel. 
#ifndef __BLITZ2_STREAM_SOCKET_CHANNEL_H_
#define __BLITZ2_STREAM_SOCKET_CHANNEL_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/outmsg.h>
#include <blitz2/ioobject.h>
#include <blitz2/ioaddr.h>
#include <blitz2/logger.h>
#include <blitz2/reactor.h>
#include <blitz2/network_util.h>
namespace blitz2 {
// class XxxDecoder {
//   public:
//   typedef xxx TMsg;
//   typedef xxx TInitParam;
//   int Init(const TInitParam& param);
//   void Uninit();
//   /**
//    * 请求存放数据的缓冲区.
//    */
//   int GetBuffer(void** buf,uint32_t* length) ;
//   /**
//    * 读到数据后，马上汇报.
//    * @param buf : in 
//    * @param length : [IN] 读取到的数据长度 .
//    * @param msg : [OUT]
//    * @param report: functor, void report(TMsg msg)
//    * @return 0: 成功.
//    *         other: 失败.
//    */
//   template<typename Reporter>
//   int Feed(void* buf,uint32_t length,Reporter report) ;
// };
// 
template<typename Decoder>
class StreamSocketChannel: public IoObject {
 public:
  typedef Decoder TDecoder;
  typedef typename TDecoder::TInitParam TDecoderInitParam;
  typedef typename TDecoder::TMsg TMsg;
  typedef StreamSocketChannel<Decoder> TMyself;
  class Observer {
   public:
    virtual void OnOpened(TMyself* channel) {(void)(channel);}
    virtual void OnClosed(TMyself* channel) {(void)(channel);}
    virtual void OnMsgArrived(TMyself* channel,
        TMsg* msg)  {
      (void)(channel);
      (void)(msg);
    }
  };
  enum ConnectStatus {
     kClosed = 0,
     kConnecting = 1,
     kConnected = 2
  };
  enum ECloseReason {
    kActiveClose=0,
    kPassiveClosed,
    kReadFail,
    kWriteFail,
    kConnectFail,
    kDecoderFail, 
    kChangeAddr
  };

  StreamSocketChannel(Reactor& reactor, 
      const TDecoderInitParam& decoder_init_param)
    :reactor_(reactor),
    logger_(Logger::GetLogger("blitz2::StreamSocketChannel")),
    msg_report_(this) {
      fd_ = -1;
      connect_status_ = kClosed;
      observer_ = NULL;
      sending_msg_=NULL;
      sending_buffer_index_ = 0;
      data_sent_ = 0;
      passived_ = false;
      decoder_init_param_ = decoder_init_param;
      observer_ = &s_default_observer_;
      name_[0] = '\0';
      ResetIovec();
  }
  virtual ~StreamSocketChannel() {
    Close(false);
    RemoveSentFailMsg();
  }

  void Connect();
  void Connect(const IoAddr& addr) {
    if(connect_status_ == kClosed) {
      addr_ = addr;
      Connect();
    } else {
      if(addr_ != addr) {
        RealClose(kChangeAddr, true);
        addr_ = addr;
        Connect();
      }
    }
  }
  void Close(bool trigger_event) { 
    RealClose(kActiveClose, false);
    if(trigger_event) {
      reactor_.PostTask(std::bind(ClosedTask, this));
    }
  }
  void SendMsg(OutMsg* outmsg);
  virtual void ProcessEvent(const IoEvent& event);
  int BeConnected(int fd) {
    if(connect_status_ != kClosed) { return 1; }
    fd_ = fd;
    NetworkUtil::GetInetSocketName(fd_, name_, sizeof(name_));
    SetSocketOptions();
    connect_status_ = kConnected ;
    decoder_.Init(decoder_init_param_);
    reactor_.RegisterIoObject(this);
    return 0;
  }
  TDecoder& decoder() { return decoder_; }
  const char* get_name() const { return name_; }
  const TDecoder& decoder()  const { return decoder_; }
  void SetObserver(Observer* observer) {
    observer_ = observer;
    if(!observer_) { observer_ = &s_default_observer_; }
  }
  static void OpenedTask(TMyself* channel) {
    channel->observer_->OnOpened(channel);
  }
  static void ClosedTask(TMyself* channel) {
    channel->observer_->OnClosed(channel);
  }


 protected:
  void OnRead();
  void OnWrite();
  void TrySend(bool eventloop);
  int RemoveSentFailMsg();
  void ProcessChangeAddr() {
    Connect();
  }
  void RealClose(ECloseReason reason, bool notify);
  int SetSocketOptions();
  void PrepareIovec();
  void ForwardIovec(size_t length);
  void ResetIovec();
  // deprecated method.
  void __TrySendUseSend(bool eventloop);
 protected:
  struct MsgReport {
    MsgReport(TMyself* channel) {
      this->channel_ = channel;
    }
    void operator() (TMsg* msg) {
      channel_->observer_->OnMsgArrived(channel_, msg);
    }
    TMyself* channel_;
  };

  Reactor& reactor_;
  Logger& logger_;
  TDecoder decoder_;
  TDecoderInitParam decoder_init_param_;
  Observer* observer_;
  int connect_status_;
  IoAddr addr_;
  bool passived_;
  char name_[kInetSocketNameMaxLen];
  std::queue<OutMsg*> out_msg_queue_;
  OutMsg* sending_msg_;
  size_t sending_buffer_index_;
  size_t data_sent_;
  iovec* iovec_; //< 当前正在发送的iovec.
  int iovec_count_ ;
  size_t iovec_data_len_; //< 当前正在发送的iovec包含的数据长度.
  vector<iovec> iovec_container_;
  MsgReport msg_report_;
  static Observer s_default_observer_;
};

template<typename Decoder>
typename StreamSocketChannel<Decoder>::Observer StreamSocketChannel<Decoder>::s_default_observer_;
template<typename Decoder>
void StreamSocketChannel<Decoder>::Connect() {
  if(connect_status_ != kClosed) { 
    LOG_DEBUG(logger_, "alreadyConnect fd=%d name=%s status=%d this=%p",
        fd_, name_, connect_status_, this);
    return; 
  }
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_<0) { 
    LOG_FATAL(logger_, "failToSocket this=%p errno=%d", this, errno);
    reactor_.PostTask(std::bind(ClosedTask, this));
    return; 
  }
  SetSocketOptions();
  int ret = connect(fd_,addr_.sockaddr(),addr_.sockaddrlen());
  int saved_errno = errno;
  NetworkUtil::GetInetSocketName(fd_, name_, sizeof(name_));
  if (ret == 0) {
    connect_status_ = kConnected;
    LOG_DEBUG(logger_, "connected fd=%d name=%s this=%p", fd_, name_, this);
    // we should use task here,
    // because we don't want to invoke in public interface which may lead to  dead loop.
    reactor_.PostTask(std::bind(OpenedTask, this));
  } else if(ret == -1) {
    if(saved_errno == EINPROGRESS) {
      connect_status_ = kConnecting;
      LOG_DEBUG(logger_, "connecting fd=%d name=%s this=%p", fd_, name_, this);
    }
    else {
      LOG_ERROR(logger_, "connectfail errno=%d fd=%d name=%s this=%p",
          errno, fd_, name_, this);
      RealClose( kConnectFail, false );
      reactor_.PostTask(std::bind(ClosedTask, this));
      return ;
    }
  }
  decoder_.Init(decoder_init_param_);
  reactor_.RegisterIoObject(this);
};

template<typename Decoder>
inline void StreamSocketChannel<Decoder>::ProcessEvent(const IoEvent& event) {
  if (event.haveRead()) {
    OnRead();
  }
  if (fd_!=-1 && event.haveWrite()) {
    OnWrite();
  }
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::OnRead() {
  using namespace std;
  void* buf = NULL;
  uint32_t length = 0;
  int ret = 0;
  uint32_t got = 0;
  while (1) {
    //从parser中获取存放数据的内存，避免内存拷贝.
    buf = NULL;
    length = 0;
    if (decoder_.GetBuffer(&buf,&length) != 0 || length==0) {
      LOG_ERROR(logger_, "Decoder::GetBuffer fd=%d name=%s this=%p",
                fd_, name_, this);
      RealClose(kDecoderFail, true);
      break;
    }
    got = 0;
    int32_t recv_result = NetworkUtil::Recv(fd_,buf,length,&got);
    LOG_DEBUG(logger_, "recv=%d got=%u fd=%d name=%s this=%p",
                recv_result, got, fd_, name_, this);
    if(recv_result == 0 || recv_result == 1) {
      if (got > 0) {
        ret = decoder_.Feed(buf, got, msg_report_);
        if (ret < 0) {
          LOG_ERROR(logger_,
                       "Decoder::Feed ret=%d fd=%d name=%s this=%p",
                       ret, fd_, name_, this);
          RealClose(kDecoderFail, true);
          break;
        }
      }
      if(recv_result==1) {
        //EAGAIN,跳出循环.
        break;
      }
      //可以继续读取,循环.
      // check status because OnMsgArrived may call Close to change status.
      if(connect_status_ != kConnected) {
        LOG_DEBUG(logger_, "check_status_after_msg_feed"
            " fd=%d status=%d name_=%s this=%p",
            fd_, connect_status_,  name_, this);
        break;
      }
      continue;
    } else if(recv_result == 2) {
      //连接关闭, 数据还处理吗？半关闭连接?
      LOG_DEBUG(logger_, "FIN fd=%d name_=%s this=%p",
            fd_, name_, this);
      RealClose(kPassiveClosed, true);
      break;
    } else {
      // ERROR, we won't process data recieved.
      LOG_ERROR(logger_, "recvError fd=%d name=%s errno=%d this=%p",
                       fd_, name_, errno, this);
      RealClose(kReadFail, true);
      break;
    }
  }
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::OnWrite() {
  using namespace std;
  if (connect_status_ == kConnecting) {
    int error = -1;
    socklen_t len = sizeof(error);
    if (getsockopt(fd_,SOL_SOCKET,SO_ERROR,&error,&len) != 0) {
      LOG_ERROR(logger_, "getsockopt fd=%d name=%s errno=%d this=%p",
                   fd_, name_, errno , this);
      RealClose(kConnectFail, true);
      return;
    }
    if (error != 0) {
      LOG_ERROR(logger_, "TestConnect fd=%d name=%s error=%d this=%p",
                   fd_, name_, error, this);
      RealClose(kConnectFail, true);
      return;
    } else {
      LOG_DEBUG(logger_, "connected fd=%d name=%s error=%d this=%p",
                   fd_, name_, error, this);
      connect_status_ = kConnected;
      observer_->OnOpened(this);
    }
  }
  if(connect_status_ == kConnected) {
    TrySend(true);
  }
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::SendMsg(OutMsg* msg) {
  if(!msg) { 
    LOG_ERROR(logger_, "sendmsg fd=%d msg=NULL status=%d",
      fd_, connect_status_);
    return;  
  }
  if (connect_status_ == kClosed) {
    LOG_ERROR(logger_, "connect_closed fd=%d name=%s this=%p msg=%p status=%d",
      fd_, name_, this, msg, connect_status_);
    return;
  }
  LOG_DEBUG(logger_, "sendmsg fd=%d name=%s this=%p msg=%p status=%d",
      fd_, name_, this, msg, connect_status_);
  msg->AddRef();
  if (sending_msg_) {
    // we have msg which is not sent yet.
    out_msg_queue_.push(msg);
    LOG_DEBUG(logger_, "sendmsg put msg into out_msg_queue");
    return ;
  } else {
    sending_msg_ = msg;
  }
  if (connect_status_ != kConnected) {
    return ;
  }
  TrySend(false);
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::TrySend(bool eventloop) {
  if (!iovec_) {
    PrepareIovec();
    if (!iovec_) {
      return;
    }
  }
  while (1)  {
    ssize_t sent = ::writev(fd_, iovec_, iovec_count_);
    if (sent > 0) {
      if (static_cast<size_t>(sent)  == iovec_data_len_) {
        // finish the iovec.
        LOG_DEBUG(logger_,
            "MsgSent fd=%d name=%s this=%p msg=%p msg_logid=%u:%u",
            fd_, name_, this,  sending_msg_,
            sending_msg_->logid().id1, sending_msg_->logid().id2);
        sending_msg_->Release();
        ResetIovec();
        PrepareIovec();
        if (!iovec_) {
          // there's no data to send.
          break;
        }
      } else {
        LOG_TRACE(logger_, "writev fd=%d name=%s this=%p  ret=%ld ",
              fd_, name_, this, sent);
        // forword iovec.
        ForwardIovec(static_cast<size_t>(sent));
      }
    }  else {
      if (errno == EAGAIN||errno == EWOULDBLOCK) {
        break;
      } else if (errno == EINTR) {
      } else {
        LOG_ERROR(logger_,
          "SendFail fd=%d name=%s this=%p msg_logid=%u:%u msg=%p errno=%d ",
          fd_, name_, this, sending_msg_->logid().id1,
          sending_msg_->logid().id2, sending_msg_, errno);
        if (eventloop) {
          RealClose(kWriteFail, true);
        } else {
          RealClose(kWriteFail, false);
          reactor_.PostTask(std::bind(ClosedTask, this));
        }
        break;
      }
    }
  }
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::__TrySendUseSend(bool eventloop) {
  while (1) {
    //try to pop msg from queue to sending_msg_
    if (!sending_msg_ ) {
      if (out_msg_queue_.empty()) {
        // 没有数据需要发送
        return ;
      }
      sending_msg_ = out_msg_queue_.front();
      out_msg_queue_.pop();
      assert(sending_msg_->data_vec().size() > 0);
      sending_buffer_index_ = 0;
      data_sent_ = 0; 
    } 
    // try sending sending_msg_
    if (sending_msg_) {
      size_t sent = 0;
      const std::vector<MsgData>& data_vec = sending_msg_->data_vec();
      const MsgData& data = data_vec[sending_buffer_index_];
      int32_t send_result = NetworkUtil::Send(fd_,
          data.mem() + data.offset() + data_sent_,
          data.length() - data_sent_, sent);
      LOG_DEBUG(logger_, "send fd=%d name=%s this=%p sent=%zu ret=%d ",
                  fd_, name_, this, sent, send_result );
      data_sent_ += sent;
      if (data_sent_ == data.length()) {
        ++sending_buffer_index_;
        if (sending_buffer_index_ == data_vec.size()) {
          LOG_DEBUG(logger_,
              "MsgSent fd=%d name=%s this=%p msg=%p msg_logid=%u:%u",
              fd_, name_, this,  sending_msg_,
              sending_msg_->logid().id1, sending_msg_->logid().id2);
          sending_msg_->Release();
          sending_msg_ = NULL;
          sending_buffer_index_ = 0;
          data_sent_ = 0;
        } else {
          data_sent_ = 0;
        }
      }
      if (send_result == 0) {
        //继续循环发送.
      } else if(send_result == 1) {
        // EAGAIN，write buffer may be full.
        return ;
      } else {
        LOG_ERROR(logger_,
            "SendFail fd=%d name=%s this=%p msg_logid=%u:%u msg=%p errno=%d ",
            fd_, name_, this, sending_msg_->logid().id1,
            sending_msg_->logid().id2, sending_msg_, errno);
        if(eventloop) {
          RealClose(kWriteFail, true);
        } else {
          RealClose(kWriteFail, false);
          reactor_.PostTask(std::bind(ClosedTask, this));
        }
      }
    }
  }
}

template<typename Decoder>
void StreamSocketChannel<Decoder>::RealClose(ECloseReason reason, bool notify) {
  if(fd_ < 0) {
    return ;
  }
  LOG_DEBUG(logger_, "close fd=%d name=%s this=%p reason=%d",
      fd_, name_, this, reason);
  decoder_.Uninit();
  reactor_.RemoveIoObject(this);
  connect_status_ = kClosed;
  close(fd_);
  fd_ = -1;
  //释放没有发送出去的消息.
  RemoveSentFailMsg();
  if(notify) {
    observer_->OnClosed(this);
  }
}

template<typename Decoder>
int StreamSocketChannel<Decoder>::SetSocketOptions() {
  evutil_make_socket_nonblocking(fd_);
  int flag = 1;
  int ret = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, 
      reinterpret_cast<char*>(&flag), sizeof(flag));
  if (ret == -1) {
    assert(0);
    return -1;
  }
  return 0;
}
template<typename Decoder>
int StreamSocketChannel<Decoder>::RemoveSentFailMsg() {
  if(sending_msg_){
    LOG_ERROR(logger_,
        "RemoveMsg name=%s this=%p msg_logid=%u:%u msg=%p",
        name_, this, sending_msg_->logid().id1, 
        sending_msg_->logid().id2 ,sending_msg_);
    sending_msg_->Release();
    sending_msg_ = NULL;
    data_sent_ = 0;
    sending_buffer_index_ = 0;
  }
  while(!out_msg_queue_.empty()) {
    OutMsg* msg = out_msg_queue_.front();
    out_msg_queue_.pop();
    LOG_ERROR(logger_,
        "RemoveMsg name=%s this=%p msgid=%u:%u msg=%p",
        name_, this, msg->logid().id1, msg->logid().id2, msg);
    msg->Release();
  }
  return 0;
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::PrepareIovec() {
  //try to pop msg from queue to sending_msg_
  if (!sending_msg_ ) {
    if (out_msg_queue_.empty()) {
      // 没有数据需要发送
      return ;
    }
    sending_msg_ = out_msg_queue_.front();
    out_msg_queue_.pop();
    assert(sending_msg_->data_vec().size() > 0);
    sending_buffer_index_ = 0;
    data_sent_ = 0; 
  } 
  if (iovec_container_.size() < sending_msg_->data_vec_count()) {
    iovec_container_.resize(sending_msg_->data_vec_count());
  }
  iovec_ = &iovec_container_[0];
  iovec_count_ = sending_msg_->Convert2iovec(iovec_);
  iovec_data_len_ = sending_msg_->length();
}
template<typename Decoder>
void StreamSocketChannel<Decoder>::ForwardIovec(size_t len) {
  int i = 0;
  size_t left = len;
  for (; i<iovec_count_; ++i) {
    if (iovec_[i].iov_len  > left) {
      break;
    }
    left -= iovec_[i].iov_len;
  }
  iovec_data_len_ -= len;
  iovec_ = iovec_+i;
  iovec_->iov_len -= left;
  iovec_->iov_base = reinterpret_cast<uint8_t*>(iovec_->iov_base) + left;
  iovec_count_ -= i;
}
template<typename Decoder>
inline void StreamSocketChannel<Decoder>::ResetIovec() {
  sending_msg_ = NULL;
  sending_buffer_index_ = 0;
  data_sent_ = 0;
  iovec_ = NULL;
  iovec_count_ = 0;
  iovec_data_len_ = 0;
}

template<typename Decoder>
class SharedStreamSocketChannel:public StreamSocketChannel<Decoder>,
                          public RefCountImpl 
{
 public:
  typedef StreamSocketChannel<Decoder> TParent;
  typedef SharedPtr< SharedStreamSocketChannel<Decoder> > TSharedPtr;
  SharedStreamSocketChannel(Reactor& reactor,  
                      const typename Decoder::TInitParam& param)
    :TParent(reactor, param) {
  }
  static  TSharedPtr  CreateInstance(Reactor& reactor,
      const typename Decoder::TInitParam& param) {
    TSharedPtr object(
        new SharedStreamSocketChannel<Decoder>(reactor, param));
    return object;
  }

};  
} // namespace blitz2.
#endif // __BLITZ2_STREAM_SOCKET_CHANNEL_H_

