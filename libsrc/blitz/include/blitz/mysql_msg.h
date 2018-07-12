// Copyright 2012 meilishuo Inc.
// Author: REN XIAOYU
#ifndef __BLITZ_MYSQL_MSG_H_
#define __BLITZ_MYSQL_MSG_H_ 1
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <string>
#include "utils/logger.h"
#include "fix_head_msg.h"
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef unsigned char BYTE;
namespace blitz {
class MysqlMsgFactory : public MsgDecoderFactory {
 public:
  virtual MsgDecoder* Create()  {
    return new FixHeadMsgDecoder(3,FnGetBodyLen);
  }
  virtual void Revoke(MsgDecoder* parser) {
    delete parser;
  }
  static int FnGetBodyLen(void* data,uint32_t* body_len){
    *body_len = (*(uint32_t*)data&0xffffffL) + 1;
    return 0;
  }
};
} // namespace blitz.
#endif // __BLITZ_MYSQL_MSG_H_
