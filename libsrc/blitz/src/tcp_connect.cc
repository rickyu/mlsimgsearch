#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils/logger.h"
#include <event2/event.h>
#include "blitz/tcp_connect.h"
#include "blitz/network_util.h"
#include "blitz/framework.h"
namespace blitz {
//implementation of TcpConn.

static CLogger& g_log_framework = CLogger::GetInstance("framework");
TcpConn::TcpConn(Framework* framework,
        MsgProcessor* processor,
        MsgDecoderFactory* parser_factory,
        IoStatusObserver* status_observer
        ) {
  fd_ = -1;
  // my_id_ = 0;
  connect_status_ = kClosed;
  framework_ = framework;
  msg_parser_ = NULL;
  processor_ = processor;
  msg_parser_factory_ = parser_factory;
  sending_msg_ = NULL;
  data_sent_ = 0;
  sending_buffer_index_ = 0;
  status_observer_ = status_observer;
  local_name_[0] = '\0';
  remote_name_[0] = '\0';
}
TcpConn::~TcpConn(){
  LOGGER_INFO(g_log_framework,
               "Destructor ioid=0x%lx fd=%d name=%s-%s",
               ioinfo_.get_id(), fd_, local_name_, remote_name_);
  Close();
}
int TcpConn::Connect(const char* ip,uint16_t port) {
  if (fd_ < 0) { return -1; }
  if (connect_status_ != kClosed) { return 1; }
  msg_parser_ = msg_parser_factory_->Create();
  if (!msg_parser_) {
    LOGGER_ERROR(g_log_framework, "FailToCreateDecoder fd=%d addr=%s",
                 fd_, addr_.c_str());
    Close();
    return -1;
  }
  evutil_make_socket_nonblocking(fd_);
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(ip);
  sin.sin_port = htons(port);
  ioinfo_.get_remote_addr().SetTcpIpv4(sin);
  int ret = connect(fd_,(sockaddr*)&sin,sizeof(sin));
  int saved_errno = errno;
  NetworkUtil::GetLocalName(fd_, local_name_, sizeof(local_name_));
  // NetworkUtil::GetRemoteName(fd_, remote_name_, sizeof(remote_name_));
  sprintf(remote_name_, "%s:%d", ip, port);
  if (ret == 0) {
    connect_status_ = kConnected;
    LOGGER_DEBUG(g_log_framework, "Connect success ioid=0x%lx fd=%d name=%s-%s",
               ioinfo_.get_id(), fd_, local_name_, remote_name_);
    // if (status_observer_) {
    //    status_observer_->OnOpened(ioinfo_);
    // }
    return 0;
  }
  else if(ret == -1){
    if(saved_errno == EINPROGRESS){
      LOGGER_DEBUG(g_log_framework,
                   "Connect inprogress ioid=0x%lx fd=%d name=%s-%s",
                   ioinfo_.get_id(), fd_, local_name_, remote_name_);
      connect_status_ = kConnecting;
      return 1;
    }
    else {
      char str[80];
      str[0] = '\0';
      char* returned_str = strerror_r(errno, str, sizeof(str));
      LOGGER_WARN(g_log_framework,
                   "ConnectFail ioid=0x%lx fd=%d name=%s-%s errno=%d err=%s",
                   ioinfo_.get_id(), fd_, local_name_, remote_name_,
                   saved_errno, returned_str);
      Close();
      return -1;
    }
  }
  return 0;
}

int TcpConn::PassiveConnected(int fd) {
  if(connect_status_ != kClosed){
    return -1;
  }
  fd_ = fd;
  evutil_make_socket_nonblocking(fd_);
  connect_status_ = kConnected;
  NetworkUtil::GetLocalName(fd_, local_name_, sizeof(local_name_));
  NetworkUtil::GetRemoteName(fd_, remote_name_, sizeof(remote_name_));
  struct sockaddr_in addr;
  socklen_t length = sizeof(addr);
  const char a = 1;
  setsockopt(fd,   IPPROTO_TCP,   TCP_NODELAY,   &a,   sizeof(char));
  getpeername(fd, reinterpret_cast<sockaddr*>(&addr), &length);
  ioinfo_.get_remote_addr().SetTcpIpv4(addr);
  msg_parser_ = msg_parser_factory_->Create();
  if (status_observer_) {
    status_observer_->OnOpened(ioinfo_);
  }
  return 0;
}
int TcpConn::OnRead() {
  using namespace std;
  void* buf = NULL;
  uint64_t time_start = TimeUtil::GetTickMs();
  uint32_t length = 0;
  int ret = 0;
  uint32_t got = 0;
  while (1) {
    //从parser中获取存放数据的内存，避免内存拷贝.
    buf = NULL;
    length = 0;
    if (msg_parser_->GetBuffer(&buf,&length) != 0 || length==0) {
      LOGGER_ERROR(g_log_framework,
                   "Decoder::GetBuffer ioid=0x%lx fd=%d name=%s-%s length=%d",
                   ioinfo_.get_id(), fd_, local_name_, remote_name_, length);
      // ---- yu zuo 2012-03-28
      // 这里close会有点小问题:fd被close后，还没有从fd_manager中移除，如果
      // 这个时候accept，AddFd会失败.
      // CloseWithoutLock();
      ret = -1;
      break;
    }
    got = 0;
    int32_t recv_result = NetworkUtil::Recv(fd_,buf,length,&got);
    LOGGER_INFO(g_log_framework, "recv=%d got=%u ioid=0x%lx fd=%d name=%s-%s",
                recv_result, got, ioinfo_.get_id(), fd_, local_name_,
                remote_name_);
    switch (recv_result) {
      case 0: 
        //可以继续读取,循环.
        break;
      case 1:
        //EAGAIN,跳出循环.
        ret = 0;
        break;
      case 2:
        ioinfo_.SetEof();
        //连接关闭.
        LOGGER_DEBUG(g_log_framework, "recv=FIN ioid=0x%lx fd=%d name=%s-%s",
            ioinfo_.get_id(), fd_, get_local_name(), get_remote_name());
        // CloseWithoutLock();
        ret = 1;
        break;
      default:
        //出错.
        {
          ioinfo_.SetError();
          char err_str[80];
          char* returned_str = strerror_r(errno, err_str, sizeof(err_str));
          LOGGER_INFO(g_log_framework,
                       "close reason=recvError ioid=0x%lx fd=%d"
                       " name=%s-%s errno=%d err=%s",
                       ioinfo_.get_id(), fd_,
                       get_local_name(), get_remote_name(),
                       errno, returned_str);
        }
        // CloseWithoutLock();
        ret = -1;
        break;
    }

    if (got > 0) {
      std::vector<InMsg> msgs;
      //报告读到了多少数据.
      uint64_t feed_start = TimeUtil::GetTickUs();
      ret = msg_parser_->Feed(buf, got, &msgs);
      uint64_t feed_time = TimeUtil::GetTickUs() - feed_start;
      if (ret < 0) {
        LOGGER_ERROR(g_log_framework, "Decoder::Feed fd=%d got=%u ret=%d"
                     " name=%s=%s",
                     fd_, got, ret, local_name_, remote_name_);
        // CloseWithoutLock();
        ret = -1;
        break;
      } 
      ret = 0;
      if (!msgs.empty()) {
        InMsg* msg = &msgs[0];
        size_t count = msgs.size();
        LOGGER_DEBUG(g_log_framework, "Recv all, fd=%d, ioid=0x%lx, time=%lu", fd_, ioinfo_.get_id(), feed_time);
        for (size_t i = 0; i < count; ++i) {
          //读到完整的消息.
          processor_->ProcessMsg(ioinfo_, *(msg+i));
        }
      }
    }
    if (recv_result != 0) {
      break;
    }
  }
  LOGGER_DEBUG(g_log_framework, "OnRead end, fd=%d, ioid=0x%lx, time=%d,", 
            fd_, ioinfo_.get_id(), TimeUtil::GetTickMs() - time_start);
  return ret;
}
int TcpConn::OnWrite() {
  using namespace std;
  if (connect_status_ == kConnecting) {
    int error = -1;
    socklen_t len = sizeof(error);
    if (getsockopt(fd_,SOL_SOCKET,SO_ERROR,&error,&len) != 0) {
      char str[80];
      str[0] = '\0';
      char* returned_str = strerror_r(errno, str, sizeof(str));
      LOGGER_ERROR(g_log_framework, "getsockopt ioid=0x%lx fd=%d name=%s-%s"
                   " errno=%d err=%s",
                   ioinfo_.get_id(), fd_, local_name_, remote_name_,
                   errno, returned_str);
      // CloseWithoutLock();
      return -1;
    }
    if (error != 0) {
      char str[80];
      str[0] = '\0';
      char* returned_str = strerror_r(error, str, sizeof(str));
      LOGGER_ERROR(g_log_framework, "TestConnect ioid=0x%lx fd=%d name=%s-%s"
                  " error=%d err=%s",
                   ioinfo_.get_id(), fd_, local_name_, remote_name_,
                   error,  returned_str);
      // CloseWithoutLock();
      return -1;
    } else {
      connect_status_ = kConnected;
      LOGGER_INFO(g_log_framework,
                  "TestConnect Success ioid=0x%lx fd=%d name=%s-%s",
                  ioinfo_.get_id(), fd_, local_name_, remote_name_);
      if (status_observer_) {
        status_observer_->OnOpened(ioinfo_);
      }
    }
  }
  return TrySend();
}
int TcpConn::OnError() {
 LOGGER_ERROR(g_log_framework,
             "MeetError ioid=0x%lx fd=%d name=%s-%s status=%d",
             ioinfo_.get_id(), fd_, local_name_, remote_name_, connect_status_);
  Close();
  return 0;
}
int TcpConn::SendMsg(OutMsg* msg) {
  LOGGER_DEBUG(g_log_framework, "SendMsg, ioid=0x%lx fd=%d, status=%d",
                ioinfo_.get_id(), fd_, connect_status_);
  if(!msg) {
    return -1;
  }
  out_msg_queue_.push(msg);
  if (sending_msg_) {
    return 0;
  }
  if (connect_status_ != kConnected) {
    return 0;
  }
  LOGGER_DEBUG(g_log_framework, "ready to trysend, ioid=0x%lx, fd=%d, status=%d",
               ioinfo_.get_id(), fd_, connect_status_);
  int ret =  TrySend();
  if (ret != 0) {
    // CloseWithoutLock();
  }
  return ret;
}
int TcpConn::TrySend(){
  LOGGER_DEBUG(g_log_framework, "Into trysend, ioid=0x%lx, fd=%d, status=%d",
               ioinfo_.get_id(), fd_, connect_status_);
  while (1) {
    //try to pop msg from queue to sending_msg_
    if (!sending_msg_ ) {
      if (out_msg_queue_.empty()) {
        // 没有数据需要发送
        return 0;
      }
      sending_msg_ = out_msg_queue_.front();
      out_msg_queue_.pop();
      assert(sending_msg_->get_buffer_count() > 0);
      sending_buffer_index_ = 0;
      data_sent_ = 0; 
    } 
    // try sending sending_msg_
    if (sending_msg_) {
      size_t sent = 0;
      const DataBuffer* buffer = sending_msg_->get_buffers() + sending_buffer_index_;
      int32_t send_result = NetworkUtil::Send(fd_, buffer->ptr + data_sent_,
          buffer->data_length - data_sent_, sent);
      LOGGER_INFO(g_log_framework, "send fd=%d name=%s-%s sent=%zu ret=%d",
                  fd_, local_name_, remote_name_, sent, send_result);
      data_sent_ += sent;
      if (data_sent_ == buffer->data_length) {
        ++sending_buffer_index_;
        if (sending_buffer_index_ == sending_msg_->get_buffer_count()) {
          LOGGER_DEBUG(g_log_framework, "MsgSent ioid=0x%lx name=%s-%s msgid=%d ",
                  ioinfo_.get_id(), local_name_, remote_name_, sending_msg_->get_msgid());
          sending_msg_->Release();
          sending_msg_ = NULL;
        } else {
          data_sent_ = 0;
        }
      }
      if (send_result == 0) {
        //继续循环发送.
      } else if(send_result == 1) {
        // EAGAIN，没有数据要读.
        return 0;
      } else {
        char str[80];
        str[0] = '\0';
        char* returned_str = strerror_r(errno, str, sizeof(str));
        LOGGER_ERROR(g_log_framework,
                     "SendFail ioid=0x%lx fd=%d name=%s-%s msgid=%d msg=%p"
                     " errno=%d err=%s",
                     ioinfo_.get_id(), fd_, local_name_, remote_name_,
                     sending_msg_->get_msgid(), sending_msg_, 
                     errno, returned_str);
        // Close();
        return -1;
      }
    }
  }
  return 0;
}

int TcpConn::Close() {
  return CloseWithoutLock();
}

int TcpConn::CloseWithoutLock() {
  LOGGER_INFO(g_log_framework,
               "Close ioid=0x%lx fd=%d name=%s-%s",
               ioinfo_.get_id(), fd_, local_name_, remote_name_);

  if(fd_ < 0) {
    return -1;
  }
  close(fd_);
  fd_ = -1;
  if(status_observer_ && connect_status_ == kConnected) {
      status_observer_->OnClosed(ioinfo_);
      status_observer_ = NULL;
  }
  connect_status_ = kClosed;
  if(msg_parser_) {
    msg_parser_factory_->Revoke(msg_parser_);
    msg_parser_ = NULL;
  }
  //释放没有发送出去的消息.
  RemoveSentFailMsg();
  return 0;

}
int TcpConn::RemoveSentFailMsg() {
  if(sending_msg_){
    LOGGER_ERROR(g_log_framework,
        "MsgUnsent ioid=0x%lx name=%s-%s msgid=%d msg=%p",
        ioinfo_.get_id(), local_name_, remote_name_, sending_msg_->get_msgid(),
        sending_msg_);
    sending_msg_->Release();
    sending_msg_ = NULL;
    data_sent_ = 0;
    sending_buffer_index_ = 0;
  }
  while(!out_msg_queue_.empty()){
    OutMsg* msg = out_msg_queue_.front();
    out_msg_queue_.pop();
    LOGGER_ERROR(g_log_framework,
        "MsgUnsent ioid=0x%lx name=%s-%s msgid=%d msg=%p",
        ioinfo_.get_id(), local_name_, remote_name_, msg->get_msgid(), msg);
    msg->Release();
  }
  return 0;
}
} // namespace blitz.
