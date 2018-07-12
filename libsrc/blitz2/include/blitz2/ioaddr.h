#pragma once
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ostream>


namespace blitz2 {

/**
 * represent io address, for example: tcp://192.168.0.1:1234, udp://ip:port, 
 * unix://path.
 * only support tcp://ip:port now.
 */
class IoAddr {
 public:
  enum IoAddrType {
    kUnknown = 0,
    kTcpIpv4 = 1
  };
  IoAddr() {type_ = kUnknown; }
  IoAddrType type() const {return type_;}
  int Parse(const std::string& addr);
  void SetTcpIpv4(const struct sockaddr_in& ipv4); 
  uint32_t get_ipv4() const {
    if (type_ == kTcpIpv4) {
      return ipv4_.sin_addr.s_addr;
    }
    return 0;
  }
  const struct sockaddr* sockaddr() const {
    switch(type_) {
      case kTcpIpv4:
        return reinterpret_cast<const struct sockaddr*>(&ipv4_);
      default:
        return NULL;
    }
  }
  socklen_t sockaddrlen() const {
    switch(type_) {
      case kTcpIpv4:
        return sizeof(ipv4_);
      default:
        return 0;
    }
  }
  const std::string& str() const {
    return str_;
  }
  
  int get_ipv4_str(std::string* str) const {
    if (type_ != kTcpIpv4) {
      return -1;
    }
    char buf[32];
    const char* s = inet_ntop(AF_INET, &ipv4_.sin_addr, buf, 32);
    *str = s;
    return 0;
  }
  void ToString(std::string* str) const {
    switch(type_) {
      case kTcpIpv4:
        {
          char buf[32];
          const char* s = inet_ntop(AF_INET, &ipv4_.sin_addr, buf, 32);
         *str  = s;
         sprintf(buf, ":%d", ntohs(ipv4_.sin_port));
         *str += buf;
        }
        break;
      default:
        break;
    }
  }
  bool operator==(const IoAddr& rhs) {
    if(type_ != rhs.type_) {
      return false;
    }
    switch(type_) {
      case kTcpIpv4:
        if(ipv4_.sin_family == rhs.ipv4_.sin_family
            && ipv4_.sin_port == rhs.ipv4_.sin_port
            && ipv4_.sin_addr.s_addr == rhs.ipv4_.sin_addr.s_addr) {
          return true;
        }
      default:
        return false;
    }
    return true;
  }
  bool operator!=(const IoAddr& rhs) {
    return !(*this==rhs);
  }
 public:
  std::string str_;
  IoAddrType type_;
  union {
    struct sockaddr_in ipv4_;
  };
};
inline std::ostream& operator<<(std::ostream& os, const IoAddr& addr) {
  os<<addr.str_;
  return os;
}
} // namespace blitz2.
