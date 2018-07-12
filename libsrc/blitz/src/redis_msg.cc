#include "blitz/redis_msg.h"
namespace blitz {
RedisReplyDecoder::~RedisReplyDecoder(){
  if(redis_reader_){
    redisReplyReaderFree(redis_reader_);
    redis_reader_ =NULL;
  }
}
int32_t RedisReplyDecoder::GetBuffer(void** buf,uint32_t* length) {
  *buf = buf_;
  *length = 2048;
  return 0;
}
int32_t RedisReplyDecoder::Feed(void* buf,uint32_t length, std::vector<InMsg>* msgs) {
  if(!redis_reader_){
    redis_reader_ = redisReplyReaderCreate();
  }
  redisReplyReaderFeed(redis_reader_,(const char*)buf,length);
  void* aux = NULL;
  int ret = redisReplyReaderGetReply(redis_reader_,&aux);
  if(ret != REDIS_OK){
    return -1;
  }
  if(aux != NULL){
    InMsg msg;
    msg.type = "redisReply";
    msg.data = aux;
    msg.fn_free = FreeRedisReply;
    msg.free_ctx = NULL;
    msg.bytes_length = 0;
    msgs->push_back(msg);
    return 0;
  }
  return 1;
}
} // namespace blitz.
