#pragma once
#include <stdint.h>

namespace blitz {
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
    static const char* GetAddrString(const sockaddr_in& addr, char* buf,
                                      size_t buf_length);
};
} // namespace blitz.
