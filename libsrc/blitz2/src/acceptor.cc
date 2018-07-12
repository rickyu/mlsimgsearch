#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <blitz2/acceptor.h>
namespace blitz2 {
Acceptor::Observer Acceptor::s_default_observer_;
int Acceptor::BindAndListen(const IoAddr& addr) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd  == -1) {
    return -2;
  }
  evutil_make_socket_nonblocking(fd);
  evutil_make_listen_socket_reuseable(fd);
  int ret = bind(fd, addr.sockaddr(), addr.sockaddrlen());
  if (ret==-1) {
    ::close(fd);
    return -3;
  }
  listen(fd, 128);
  return fd;
}
int Acceptor::Bind(const std::string& addr) {
  if(addr_.Parse(addr) != 0) {
    return -1;
  }
  return Bind();
}
int Acceptor::Bind() {
  if(fd_!=-1) {
    return 1;
  }
  fd_ = BindAndListen(addr_);
  if (fd_ < 0) {
    return -1;
  }
  reactor_.RegisterIoObject(this);
  return 0;
}
void Acceptor::ProcessEvent(const IoEvent& event) {
  if (event.haveWrite()) {
    abort();
    return;
  }
  if (!event.haveRead()) {
    return;
  }
  struct sockaddr_storage ss;
  while (1) {
    socklen_t slen = sizeof(ss);
    int accepted_fd = accept(fd_, (struct sockaddr*)&ss, &slen);
    if(accepted_fd>0) {
      LOG_DEBUG(logger_, "GotOneConn fd=%d from=%s", accepted_fd,
          addr_.str().c_str());
      observer_->OnConnectAccepted(this, accepted_fd);
    }
    else  {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else  {
        char err_str[80];
        err_str[0] = '\0';
        int save_errno = errno;
        char* returned_str = strerror_r(save_errno, err_str, sizeof(err_str));
        LOG_ERROR(logger_, "accept error fd=%d errno=%d err=%s",
                     fd_, save_errno, returned_str);
        if (save_errno == EINTR || save_errno == EMFILE ||
            save_errno == ENFILE || save_errno == ENOBUFS ||
            save_errno == ENOMEM) {
          // 改成用task的方式是因为直接重新加，可能马上就会产生回调，导致其他task没机会执行.
          reactor_.RemoveIoObject(this);
          reactor_.RegisterIoObject(this);
          break;
        } else {
          // 其他错误,不知道怎么处理，直接崩溃.
          abort();
        }     
      }
    } 
  }
}
}  // namespace blitz2
