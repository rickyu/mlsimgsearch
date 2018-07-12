#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <blitz2/network_util.h>

namespace blitz2 {

int32_t NetworkUtil::Recv(int fd,void* buffer,uint32_t want,uint32_t* got) {
  *got = 0;
  while (1) {
    ssize_t result = recv(fd,(uint8_t*)buffer + *got,want - *got,0);
    if (result > 0) {
      *got += result;
      if (*got == want) {
        return 0;
      }
    }
    else if (result == 0) {
      return 2;
    } else {
      if (errno == EAGAIN) {
        return 1;
      }
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
  }

}
int32_t NetworkUtil::Send(int fd,const void* data,size_t length,size_t& sent){
  sent = 0;
  if (length == 0) { return 0; }
  while (1) {
    ssize_t result = send(fd, (const uint8_t*)data + sent, length - sent, 0);
    if (result > 0) {
      sent += result;
      if (sent == length) { return 0; }
    }
    else {
      if (errno == EAGAIN) { return 1; }
      if (errno == EINTR) { continue; }
      return -1;
    }
  }
}
int NetworkUtil::GetLocalName(int fd, char* buf, size_t buf_length) {
  struct sockaddr_in addr;
  socklen_t length = sizeof(addr);
  int ret = getsockname(fd, reinterpret_cast<sockaddr*>(&addr),
      &length);
  if (ret == 0) {
    return GetAddrString(addr, buf, buf_length);
  }
  return -1;
}
int NetworkUtil::GetRemoteName(int fd, char* buf, size_t buf_length) {
  struct sockaddr_in addr;
  socklen_t length = sizeof(addr);
  int ret = getpeername(fd, reinterpret_cast<sockaddr*>(&addr),
      &length);
  if (ret == 0) {
    return GetAddrString(addr, buf, buf_length);
  }
  return -1;
}
int NetworkUtil::GetAddrString(const sockaddr_in& addr, char* buf,
                                      size_t buf_length) {
  char ip_buf[16];
  const char* str = inet_ntop(AF_INET, &addr.sin_addr, ip_buf, sizeof(ip_buf));
  if (!str) {
    return NULL;
  }
  return snprintf(buf, buf_length, "%s:%d", str, ntohs(addr.sin_port));
}
} // namespace blitz.
