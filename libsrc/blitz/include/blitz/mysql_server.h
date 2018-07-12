// Copyright 2012 meilishuo Inc.
// Author: REN XIAOYU
// MYSQL应用相关类.
#ifndef __BLITZ_MYSQL_SERVER_H_
#define __BLITZ_MYSQL_SERVER_H_ 1

#include <arpa/inet.h>
#include <unordered_map>
#include "utils/mutex.h"
#include "framework.h"
#include "mysql_msg.h"
namespace blitz {
 
class Framework;

static void FreeBuffer(void* buffer,void* ctx) {
  AVOID_UNUSED_VAR_WARN(ctx);
  if(buffer){
    free(buffer);
    buffer = NULL;
  }
}
class MysqlPacketRealProcessor {
  public:
    virtual int ProcessMysqlReqPacket(const IoInfo& ioinfo, char* data, uint32_t length, bool& isauth) = 0;
};

class MysqlPackProcessor {
  public:
    MysqlPackProcessor(MysqlPacketRealProcessor* processor);
    int ProcessPack(const IoInfo& ioinfo, char* data, uint32_t length){return processor_->ProcessMysqlReqPacket(ioinfo, data, length, isauth_);} ;
  private:
    bool isauth_;
    MysqlPacketRealProcessor* processor_;
};
class MysqlServer;
class MysqlHelloSender : public IoStatusObserver {
  public:
    MysqlHelloSender(MysqlServer* server);
    int OnOpened(const IoInfo& ioinfo);
    int OnClosed(const IoInfo& ioinfo);
  private:
    MysqlServer* mysql_server_;
};
class MysqlServer : public MsgProcessor {
 public:
  MysqlServer(blitz::Framework& framework, MysqlPacketRealProcessor* processor): framework_(framework), processor_(processor) {
    hello_sender_ = new MysqlHelloSender(this);
  };
  ~MysqlServer() {
    if (NULL != hello_sender_) {
      delete hello_sender_;
      hello_sender_ = NULL;
    }
  }
  int Init();
  int Bind(const char* addr);
  // 实现MsgProcessor.
  int ProcessMsg(const IoInfo& ioinfo, const blitz::InMsg& msg);
  static int SendReply(Framework* framework_, uint64_t io_id, char* data, int length);
 protected:
  blitz::Framework& framework_;
  MysqlMsgFactory decoder_factory_;
  MysqlHelloSender* hello_sender_;
  MysqlPacketRealProcessor* processor_;
  CMutex map_mutex_;
  std::map<uint64_t, MysqlPackProcessor*> processor_map_;
  friend class MysqlHelloSender;
};


} // namespace blitz.
#endif // __BLITZ_MYSQL_SERVER_H_
