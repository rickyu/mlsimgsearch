#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "blitz/tcp_server.h"
#include "blitz/tcp_connect.h"
#include "blitz/framework.h"
#include "blitz/network_util.h"
#include "utils/logger.h"
namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("framework");

inline void FreeTcpConn(IoHandler* handler,void* /* ctx */){
  delete handler;
}

int TcpServer::Bind(const char* ip,uint16_t port){
  if (!ip || port == 0) { return kInvalidParam; }
  if (fd_ >= 0) {
    //已经绑定,只能绑定一个.
    return 1;
  }
  fd_ = socket(AF_INET,SOCK_STREAM,0);
  if (fd_ == -1) {
    char str[80];
    str[0] = '\0';
    char* returned_str = strerror_r(errno, str, sizeof(str));
    LOGGER_ERROR(g_log_framework, "CreateSocket errno=%d err=%s",
                 errno, returned_str);
    return -1;
  }
  evutil_make_socket_nonblocking(fd_);
  evutil_make_listen_socket_reuseable(fd_);
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  if (strcmp(ip,"*") == 0) {
    sin.sin_addr.s_addr = INADDR_ANY;
  } else {
    sin.sin_addr.s_addr = inet_addr(ip);
  }
  sin.sin_port = htons(port);
  if (bind(fd_, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
    char str[80];
    str[0] = '\0';
    char* returned_str = strerror_r(errno, str, sizeof(str));
    LOGGER_ERROR(g_log_framework, "bind fd=%d errno=%d err=%s",
                 fd_, errno, returned_str);
    close(fd_);
    fd_ = -1;
    return -2;
  }
  if (listen(fd_, 255) != 0) {
    char str[80];
    str[0] = '\0';
    char* returned_str = strerror_r(errno, str, sizeof(str));
    LOGGER_ERROR(g_log_framework, "listen fd=%d errno=%d err=%s",
                 fd_, errno, returned_str);

    close(fd_);
    fd_ = -1;
    return -3;
  }
  return 0;
}
int TcpServer::OnRead() {
  struct sockaddr_storage ss;
  while (1) {
    socklen_t slen = sizeof(ss);
    int accepted_fd = accept(fd_, (struct sockaddr*)&ss, &slen);
    if (accepted_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else  {
        char err_str[80];
        err_str[0] = '\0';
        int save_errno = errno;
        char* returned_str = strerror_r(save_errno, err_str, sizeof(err_str));
        LOGGER_ERROR(g_log_framework, "accept error fd=%d errno=%d err=%s",
                     fd_, save_errno, returned_str);
        if (save_errno == EINTR || save_errno == EMFILE ||
            save_errno == ENFILE || save_errno == ENOBUFS ||
            save_errno == ENOMEM) {
          // 改成用task的方式是因为直接重新加，可能马上就会产生回调，导致其他task没机会执行.
          if (framework_->ReactivateIO(my_id_) != 0) {
            abort();
          }
          break;
        } else {
          // 其他错误,不知道怎么处理，直接崩溃.
          abort();
        }     
      }
    } else {
      TcpConn* conn = new TcpConn(framework_,processor_,msg_parser_factory_,observer_);
      assert(conn != NULL);
      /*int ret = conn->PassiveConnected(accepted_fd);
      LOGGER_DEBUG(g_log_framework, "accept fd=%d new_fd=%d name=[%s-%s]", fd_,
          accepted_fd, conn->get_local_name(), conn->get_remote_name());
 
      if (ret != 0) {
        LOGGER_ERROR(g_log_framework, "Accept err=PassiveConnected(%d) act=Close fd=%d"
                    " new_fd=%d name=[%s-%s]", ret, fd_,
                    accepted_fd, conn->get_local_name(), conn->get_remote_name());
        delete conn;
        continue;
      }*/
      uint64_t ioid = 0;
      framework_->AssignFdIoId(accepted_fd, &(ioid));
      conn->ioinfo_.set_id(ioid);
      int ret = framework_->AddAcceptTcp(conn->ioinfo_.get_id(), conn, FreeTcpConn, NULL);
      if (ret != 0) {
        LOGGER_ERROR(g_log_framework, "Accept err=AddFd(%d) act=Close fd=%d"
                    " new_fd=%d name=[%s-%s]", ret, fd_,
                    accepted_fd, conn->get_local_name(), conn->get_remote_name());
        delete conn;
      }
    }
  }
  return 0;
}

int TcpServer::OnWrite(){
  abort();
  return 0;
}
int TcpServer::OnError(){
  Close();
  return 0;
}


int TcpServer::Close() {
  if(fd_ >= 0){
    close(fd_);
    fd_ = -1;
  }
  return 0;
}
int TcpServer::SendMsg(OutMsg* /* out_msg */) {
  abort();
  return 0;
}

} // namespace blitz.
