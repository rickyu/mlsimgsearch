#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <blitz2/control_channel.h>
namespace blitz2 {
void ControlChannel::OnRead() {
  LOG_DEBUG(logger_, "ControlChannel::OnRead fd=%d", fd_);
  int newfd = -1;
  int32_t type = 0;
  while(1) {
    int ret = ReadFd(&newfd, &type);
    if(ret == 0) {
      // call event processor
      LOG_DEBUG(logger_, "fd=%d recvfd=%d type=%d", fd_, newfd, type);
      recv_fd_callback_(this, newfd, type);
    } else if(ret==2) {
      // read buffer is empty.
      break;
    } else {
      LOG_ERROR(logger_, "fd=%d errno=%d", fd_, errno);
      RealClose(ret==1?1:-1);
      break;
    }
  }
}
void ControlChannel::OnWrite() {
  while(!fds_.empty()) {
    TransferedFd tfd = fds_.front();
    int ret = WriteFd(tfd.fd, tfd.type);
    LOG_DEBUG(logger_, "fd=%d fdtransfered=%d", fd_, tfd.fd);
    if(ret==0) {
      fds_.pop();
      break;
    } else {
      LOG_ERROR(logger_, "fd=%d errno=%d", fd_, errno);
      RealClose(-2);
    }
  }
}
void ControlChannel::SendFd(int fd_to_send, int type) {
  if(!fds_.empty()) {
    TransferedFd tfd = {fd_to_send, type};
    fds_.push(tfd);
    return;
  }
  WriteFd(fd_to_send, type);
}
int ControlChannel::WriteFd(int sendfd, int32_t type) {
  struct msghdr msg;
  struct iovec  iov[1];

  union {
    struct cmsghdr      cm;
    char   control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr        *cmptr;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int *) CMSG_DATA(cmptr)) = sendfd;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = &type;
  iov[0].iov_len = sizeof(int32_t);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  do {
    ssize_t n = sendmsg(fd_, &msg, 0);
    ::close(sendfd);
    LOG_DEBUG(logger_, "sendmsg=%ld fd=%d", n, fd_);
    if(n==sizeof(int32_t)) {
      return 0;
    } else {
      if(n==-1 && errno==EINTR) {
        continue;
      }
      return -1;
    }
  } while(1);
  return -2;
}
int ControlChannel::ReadFd(int* recvfd, int32_t* type) {
  struct msghdr msg;
  union {
    struct cmsghdr      cm;
    char   control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr        *cmptr;
  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  struct iovec  iov[1];
  iov[0].iov_base = type;
  iov[0].iov_len = sizeof(int32_t);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  do {
    ssize_t n = ::recvmsg(fd_, &msg, 0);
    if(n==sizeof(int32_t)) {
      if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
        cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmptr->cmsg_level != SOL_SOCKET)
          return -2;
        if (cmptr->cmsg_type != SCM_RIGHTS)
          return -2;
        *recvfd = *((int *) CMSG_DATA(cmptr));
        return 0;
      } else {
        *recvfd = -1;               /* descriptor was not passed */
        return -2;
      }
    } else if(n==0) {
      return 1;
    } else {
      if(errno==EAGAIN) {
        return 2;
      }
      return -1;
    }
  } while(1);
  return -3;
}
}  // namespace blitz2
