#pragma once
#include <blitz2/stdafx.h>

namespace blitz2 {
static const size_t kInetSocketNameMaxLen = 50;
class NetworkUtil {
  public:
    /**
 * 读size个字节.
 * @return 2 : fd被关闭.
 *         -1 : 出错.
 *         1 : EAGAIN.
 *         0 : 成果读到want个字节.
 */

    static int32_t Recv(int fd,void* buffer,uint32_t want,uint32_t* got);
    /**
     *
     * @return 0 : 成功
     *         1: EAGAIN.
     *         -1 : 出错.
     */
    static int32_t Send(int fd,const void* data,size_t length,size_t& sent);
    static int GetLocalName(int fd, char* buf, size_t buf_length);
    static int GetRemoteName(int fd, char* buf, size_t buf_length);
    static int GetRemoteAddr(int fd, uint32_t* ip, uint16_t* port);
    static int GetAddrString(const sockaddr_in& addr, char* buf,
                                      size_t buf_length);
    static int GetInetSocketName(int fd, char* str, size_t length) {
      int len = GetLocalName(fd, str, length);
      if(len>0 && len < (int)length) {
        str[len] = '-';
        ++len;
        int len2 = GetRemoteName(fd, str+len, length-len);
        if(len2>0) {
          return len+len2;
        }
      }
      return -1;
    }
    
};
} // namespace blitz2.
