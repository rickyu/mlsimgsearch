#ifndef __BLITZ_MYSQL_ENCODER_H_
#define __BLITZ_MYSQL_ENCODER_H_ 1
#include "msg.h"
namespace blitz {
class MysqlAuthReq {
  public:
    int Encode(OutMsg* msg);
  public:
    uint32_t client_flag_;
    int32_t max_packet_size_;
    char charset_number_;
    const char* user_;
    const char* passwd_;
    const char* scramble_;
    uint8_t packet_no_;
  
};
static const uint32_t kScrambleLength = 20;
static const uint32_t kScrambleLength323 = 8;
static const uint32_t kServerVersionLength = 50;
class MysqlProtocolGreeting {
  public:
    int Parse(const uint8_t* data,uint32_t length);
    uint8_t protocol_version_;
    char server_version_[kServerVersionLength];
    uint32_t thread_id_;
    char  scramble_[kScrambleLength + 1];
    uint16_t server_capabilities_;
    uint8_t server_language_;
    uint16_t server_status_;

};

} // namespace blitz.
#endif // __BLITZ_MYSQL_ENCODER_H_
