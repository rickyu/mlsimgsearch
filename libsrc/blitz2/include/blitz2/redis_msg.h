#pragma once

#include <stdlib.h>
#include <memory>
#include <hiredis/hiredis.h>
#include <blitz2/outmsg.h>
namespace blitz2 {
class RedisReply {
 public:
  RedisReply() {reply_=NULL; }
  RedisReply(redisReply* reply) {
    reply_ = reply;
  }
  int Init(redisReply* reply) {
    assert(reply_ == NULL);
    reply_ = reply;
    return 0;
  }
  redisReply* get_reply() { return reply_;}
  virtual ~RedisReply() {
    if(reply_) { freeReplyObject(reply_);}
  }
 protected:
  redisReply* reply_;
};
class RedisReplyDecoder {
  public:
    typedef SharedObject<RedisReply> TMsg;
    class InitParam {
     public:
      InitParam() {
        start_buf_length_ = 4*1024;
        max_buf_length_ = 4*1024*1024;
      }
      InitParam(size_t start_len, size_t max_len) {
        start_buf_length_ = start_len;
        max_buf_length_ =  max_len;
      }
      void set_start_buf_length(size_t start_len) {
        start_buf_length_ = start_len;
      }
      void set_max_buf_length(size_t max_len) {
        max_buf_length_ = max_len;
      }
      size_t start_buf_length() const { return start_buf_length_;}
      size_t max_buf_length() const { return max_buf_length_;}
     private:
      size_t start_buf_length_;
      size_t max_buf_length_;
    };
    typedef InitParam TInitParam;
    RedisReplyDecoder() {
      redis_reader_ = NULL;
      buf_ = NULL;
      buf_length_ = 0;
      max_buf_length_ = 4*1024*1024;
    }
    ~RedisReplyDecoder();
  private:
    RedisReplyDecoder(const RedisReplyDecoder&);
    RedisReplyDecoder& operator=(const RedisReplyDecoder&);
  public:
    int Init(const TInitParam& );
    int GetBuffer(void** buf,uint32_t* length) ;
    template<typename Report>
    int Feed(void* buf,uint32_t length, Report report);
    void Uninit();
  private:
    char *buf_;
    size_t buf_length_;
    redisReader* redis_reader_;
    size_t max_buf_length_;
};
inline RedisReplyDecoder::~RedisReplyDecoder(){
  Uninit();
}
inline int RedisReplyDecoder::Init(const TInitParam& init_param) {
  max_buf_length_ = init_param.max_buf_length();
  if (!redis_reader_) {
    redis_reader_ = redisReplyReaderCreate();
  }
  if (!redis_reader_) { return -1; }
  if (!buf_) { 
    buf_length_ = init_param.start_buf_length();
    buf_ = new char[buf_length_];
    if (!buf_) {
      return -2;
    }
  }
  return 0;
}
inline void RedisReplyDecoder::Uninit() {
  if (redis_reader_) {
    redisReplyReaderFree(redis_reader_);
    redis_reader_ =NULL;
  }
  if (buf_) {
    delete [] buf_;
    buf_length_ = 0;
  }
}
inline int RedisReplyDecoder::GetBuffer(void** buf,uint32_t* length) {
  *buf = buf_;
  *length = buf_length_;
  return 0;
}
template<typename Report>
inline int RedisReplyDecoder::Feed(void* buf,uint32_t length, Report report) {
  redisReplyReaderFeed(redis_reader_,(const char*)buf,length);
  void* aux = NULL;
  int ret = redisReplyReaderGetReply(redis_reader_,&aux);
  if (ret != REDIS_OK) {
    return -1;
  }
  if (aux != NULL) {
    SharedPtr<TMsg> msg = TMsg::CreateInstance(reinterpret_cast<redisReply*>(aux));
    report(msg);
    return 0;
  } else {
    size_t new_length = buf_length_ << 1;
    if (new_length > max_buf_length_) {
      new_length = max_buf_length_;
    }
    char* new_buf = new char[new_length];
    if (new_buf) {
      delete [] buf_;
      buf_ = new_buf;
      buf_length_ = new_length;
    }
    return 1;
  }
}
} // namespace blitz2

