#ifndef __BLITZ_FIX_HEAD_MSG_H_
#define __BLITZ_FIX_HEAD_MSG_H_ 1
#include <cstddef>
#include <utils/compile_aux.h>
#include "msg.h"


namespace blitz {
typedef int (*FnFixHeadMsgGetBodyLen)(void*,uint32_t *body_len);
class FixHeadMsgDecoder : public MsgDecoder {
  public:
    FixHeadMsgDecoder(uint32_t head_len,FnFixHeadMsgGetBodyLen fn_get_body_len)
    : head_len_(head_len),fn_get_body_len_(fn_get_body_len){
      buffer_ = NULL;
      buffer_len_ = 0;
      data_len_ = 0;
      msg_len_ = 0;
    }
    ~FixHeadMsgDecoder();
    static void FreeData(void* data,void* ctx){
      AVOID_UNUSED_VAR_WARN(ctx);
      free(data);
    }
  private:
    FixHeadMsgDecoder(const FixHeadMsgDecoder&);
    FixHeadMsgDecoder& operator=(const FixHeadMsgDecoder&);
  public:
    virtual int32_t GetBuffer(void** buf,uint32_t* length) ;
    virtual int32_t Feed(void* buf,uint32_t length,std::vector<InMsg>* msgs) ;
  private:
    const uint32_t head_len_;
    FnFixHeadMsgGetBodyLen fn_get_body_len_;
    void* buffer_;
    uint32_t buffer_len_;
    uint32_t data_len_;
    uint32_t msg_len_;
};

} // namespace blitz.
#endif // __BLITZ_FIX_HEAD_MSG_H_
