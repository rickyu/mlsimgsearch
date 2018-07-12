#include <stdlib.h>
#include <utils/compile_aux.h>
#include "blitz/mysql_encoder.h"
#include <mysql.h>
#include <my_global.h>
#include <m_string.h>

namespace blitz { 

int MysqlAuthReq::Encode(OutMsg* msg){
  AVOID_UNUSED_VAR_WARN(msg);
  uint8_t* buffer1 = (uint8_t*)malloc(1024);
  uint8_t* buffer = buffer1 + 4;
  int4store(buffer,client_flag_);
  int4store(buffer+4,max_packet_size_);
  buffer[8] = charset_number_;
  bzero(buffer+9,32-9);
  uint8_t* end = buffer + 32;
  strmake((char*)end,user_,USERNAME_LENGTH);
  if(passwd_){
    *end = SCRAMBLE_LENGTH;
    ++ end;
    scramble((char*)end,scramble_,passwd_);
    end += SCRAMBLE_LENGTH;
  }
  else {
    *end = 0;
    ++ end;
  }
  uint32_t length = end - buffer;
  int3store(buffer1,length);
  buffer1[3] = packet_no_;
  return 0;
}


int MysqlProtocolGreeting::Parse(const uint8_t* data,uint32_t length){
  AVOID_UNUSED_VAR_WARN(length);
  const uint8_t* data_end = data;
  protocol_version_ = data[0];
  data_end += 1;
  while(*data_end != '\0'){
    ++ data_end ;
  }
  strcpy(server_version_,(const char*)(data + 1));
  data_end += 1;
  thread_id_ = uint4korr(data_end);
  memcpy(scramble_,data_end,kScrambleLength323);
  data_end += kScrambleLength323 + 1;
  server_capabilities_ = uint2korr(data_end);
  server_language_ = data_end[2];
  server_status_ = uint2korr(data_end + 3);
  data_end += 18;
  memcpy(scramble_ + kScrambleLength323,data_end,kScrambleLength - kScrambleLength323);
  scramble_[kScrambleLength] = 0;
  return 0;
}
} // namespace blitz.
