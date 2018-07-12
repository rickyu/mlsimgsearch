#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <utils/compile_aux.h>
#include "msg.h"

namespace blitz {
#define MEMCACHED_DEFAULT_COMMAND_SIZE 350
#define MEMCACHED_MAX_BUFFER 8196

// for responce
class MemcachedReplyDecoder : public MsgDecoder {
 public:
  MemcachedReplyDecoder();
  virtual ~MemcachedReplyDecoder();

  virtual int32_t GetBuffer(void** buf, uint32_t* length);
  virtual int32_t Feed(void* buf, uint32_t length, std::vector<InMsg>* msgs);

  static void FreeMemcachedReply(void* data, void* ctx);

 private:
  MemcachedReplyDecoder(const MemcachedReplyDecoder&);
  MemcachedReplyDecoder& operator=(const MemcachedReplyDecoder&);

private:
  char *result_buf_;
  uint32_t result_buf_len_;

  char *recved_buf_;
  uint32_t recved_buf_size_;
  uint32_t recved_buf_len_;
};

class MemcachedReplyDecoderFactory : public MsgDecoderFactory {
public:
  virtual MsgDecoder* Create()  {
    return new MemcachedReplyDecoder();
  }
    
  virtual void Revoke(MsgDecoder* parser) {
    if (parser) delete parser;
  }
};


// for request
typedef enum {
  SET_OP,
  REPLACE_OP,
  ADD_OP,
  PREPEND_OP,
  APPEND_OP,
  CAS_OP,
} memcached_storage_op_t;

struct MemcachedReqData{
  char* command;
  int len;
};

class MemcachedReqFunction {
public:
  static int GetOutMsg(void* data,OutMsg** out_msg){
    MemcachedReqData* redis_req = (MemcachedReqData*)data;
    OutMsg::CreateInstance(out_msg);
    (*out_msg)->AppendData(reinterpret_cast<uint8_t*>(redis_req->command), 
        redis_req->len, FreeBuffer, NULL);
    return 0;
  }
  static void FreeBuffer(void* data,void* ctx){
   AVOID_UNUSED_VAR_WARN (ctx);
    if (data) {
      free(data);
      data = NULL;
    }
  }

  /**
   * @brief 构造get命令，以字符串的形式返回
   * @param req_buf [out]
   * @param buf_size [in]
   * @param key [in]
   * @param key_len [in]
   */
  static int Get(char *req_buf, size_t buf_size,
		 const char *key, size_t key_len);
  static int Mget(char *req_buf, size_t buf_size,
		  const char* const* key, const size_t *key_length,
		  size_t number_of_keys);
	
  static int Delete(char *req_buf, size_t buf_size,
		    const char *key, size_t key_length,
		    time_t expiration);

  static int Increase(char *req_buf, size_t buf_size,
		      const char *key, size_t key_length,
		      uint64_t delta);
  static int Decrease(char *req_buf, size_t buf_size,
		      const char *key, size_t key_length,
		      uint64_t delta);

  static int Storage(char *req_buf, size_t buf_size,
		     const char *key, size_t key_length,
		     const char *value, size_t value_length,
		     time_t expiration,
		     uint32_t flags,
		     uint64_t cas,
		     memcached_storage_op_t op);
};
} // namespace blitz.
