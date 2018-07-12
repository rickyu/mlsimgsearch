#include <blitz2/ioaddr.h>
namespace blitz2 {
int IoAddr::Parse(const std::string& addr) {
  str_ = addr;
  size_t pos = addr.find(':');
  if (pos == std::string::npos) { return -1; }
  std::string protocol = addr.substr(0, pos);
  if (protocol == "tcp") {
    // process tcp protocol.
    size_t pos2 = addr.find(':', pos + 1);
    if (pos2 == std::string::npos) { return -2; }
    std::string ip = addr.substr(pos+3, pos2-pos-3);
    std::string port = addr.substr(pos2+1);
    ipv4_.sin_family = AF_INET;
    if (ip == "*") {
      ipv4_.sin_addr.s_addr = INADDR_ANY;
    } else {
      ipv4_.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    ipv4_.sin_port = htons(atoi(port.c_str()));
    type_ = kTcpIpv4;
    return 0;
  }
  return -3;
}
void IoAddr::SetTcpIpv4(const struct sockaddr_in& ipv4) {
  type_ = kTcpIpv4;
  ipv4_ = ipv4;
  str_.clear();
  str_ = "tcp://";
  char buf[32];
  const char* s = inet_ntop(AF_INET, &ipv4_.sin_addr, buf, 32);
  str_ += s;
  str_ += ":";
  sprintf(buf, ":%d", ntohs(ipv4_.sin_port));
  str_ += buf;
}

} // namespace blitz2.
