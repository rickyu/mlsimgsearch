#pragma once

#include "fd_manager.h"
#include "msg.h"
#include "mysql_encoder.h"

namespace blitz {
class MysqlReplyDecoder : public MsgDecoder {
  public:
    MysqlReplyDecoder():head_len_(4) {
      buffer_ = NULL;
      buffer_len_ = 0;
      data_len_ = 0;
      packet_num_ = 0;
      remain_ = head_len_;
      status_ = Status::kReadHead;
      reading_packet_len_ = 0;
    }
    ~MysqlReplyDecoder();
    static void FreeData(void* data,void* ctx){
      AVOID_UNUSED_VAR_WARN(ctx);
      free(data);
    }
  private:
    MysqlReplyDecoder(const MysqlReplyDecoder&);
    MysqlReplyDecoder& operator=(const MysqlReplyDecoder&);
  public:
    virtual int32_t GetBuffer(void** buf,uint32_t* length) ;
    virtual int32_t Feed(void* buf,uint32_t length, std::vector<InMsg>* msgs) ;
    int PrepareBuffer(uint32_t hint);
  private:
    const uint32_t head_len_;
    uint8_t* buffer_;
    uint32_t buffer_len_;
    uint32_t data_len_;
    uint32_t packet_num_;
    uint32_t remain_;
    uint32_t reading_packet_len_;
    enum Status {
      kReadHead = 1,
      kReadBody = 2,
    };
    Status status_;
};
class MysqlReplyDecoderFactory: public MsgDecoderFactory{
  public:
    virtual MsgDecoder* Create()  {
      return new MysqlReplyDecoder();
    }
    virtual void Revoke(MsgDecoder* parser)  {
      delete parser;
    }
};

struct MysqlNode {
  std::string addr;
  std::string user;
  std::string db;
  std::string passwd;
};

class Framework;
class MysqlConnect :public MsgProcessor,
                    public IoStatusObserver {
  public:
    typedef void (*FnQueryCallback)(int code, void* result);
    MysqlConnect(Framework& framework, int id)
      :framework_(framework) {
        id_ = id;
    }
    int Init(const MysqlNode& node);
    int Uninit();
    int Open(); 
    int Close();
    /**
     * 一次只能发一个请求，如果当前有查询未执行完，返回1.
     */
    int Query(const char* sql, size_t sql_len, FnQueryCallback callback);
    /**
     * 
     */
    bool IsReady();
  public:
    virtual int ProcessMsg(uint64_t io_id,const InMsg& msg);
    virtual int OnOpened(uint64_t io_id,const char* id,const char* addr);
    virtual int OnClosed(uint64_t io_id,const char* id,const char* addr);
  protected:
    Framework& framework_;
    int id_;
    MysqlReplyDecoderFactory decoder_factory_;
    uint64_t io_id_;
    MysqlNode node_;
    enum MysqlConnectStatus {
      kStart = 0,
      kConnecting = 1,
      kConnected = 2,
      kGreetingRecved = 3
    };
    MysqlConnectStatus status_;
    MysqlProtocolGreeting greeting_;
};

// class MysqlPool {
//  public:
//   int QueryTable(const char* table,
//             const char* index_name,
//             AnyVal* keys,
//             int keys_count,
//             const char* sql,
//             OnQueryResp fn_resp);
//   int WriteTable(const char* table,
//                  const char* index_name,
//                  AnyVal* keys,
//                  int keys_count,
//                  const char* sql,
//                  OnWriteResp fn_resp);
//   int QueryDB(const char* db, const char* sql, size_t sql_len, OnQueryResp fn_resp);
//   int WriteDB(const char* db, const char* sql, size_t sql_len, OnWriteResp fn_resp);
// };
} // namespace blitz.

