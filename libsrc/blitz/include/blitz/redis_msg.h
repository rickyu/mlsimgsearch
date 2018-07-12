#pragma once

#include <stdlib.h>
#include <hiredis/hiredis.h>
#include <utils/compile_aux.h>
#include "msg.h"

namespace blitz {
class RedisReplyDecoder : public MsgDecoder {
  public:
    RedisReplyDecoder() {
      redis_reader_ = NULL;
    }
    ~RedisReplyDecoder();
    static void FreeRedisReply(void* data,void* ctx){
      AVOID_UNUSED_VAR_WARN(ctx);
      freeReplyObject(data);
    }
  private:
    RedisReplyDecoder(const RedisReplyDecoder&);
    RedisReplyDecoder& operator=(const RedisReplyDecoder&);
  public:
    virtual int32_t GetBuffer(void** buf,uint32_t* length) ;
    virtual int32_t Feed(void* buf,uint32_t length, std::vector<InMsg>* msgs) ;
  private:
    char buf_[2048];
    redisReader* redis_reader_;
};


class RedisReplyDecoderFactory : public MsgDecoderFactory {
  public:
    virtual MsgDecoder* Create()  {
      return new RedisReplyDecoder();
    }
    
    virtual void Revoke(MsgDecoder* parser) {
      delete parser;
    }
};


struct RedisReqData{
  char* command;
  int len;
};
class RedisReqFunction {
  public:
  static int GetOutMsg(void* data,OutMsg** out_msg){
    RedisReqData* redis_req = (RedisReqData*)data;
    OutMsg::CreateInstance(out_msg);
    (*out_msg)->AppendData(reinterpret_cast<uint8_t*>(redis_req->command),
        redis_req->len, FreeBuffer, NULL);
    return 0;
  }
    static void FreeBuffer(void* data,void* ctx){
      AVOID_UNUSED_VAR_WARN(ctx);
      free(data);
    }
};
} // namespace blitz.
