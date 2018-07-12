#include <utils/time_util.h>
#include <utils/logger.h>
#include "blitz/mysql_server.h"
namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("framework");
int MysqlServer::Init() {
  return 0;
}

int MysqlServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL, hello_sender_);
}

int MysqlServer::ProcessMsg(const IoInfo& ioinfo, const blitz::InMsg& msg) {
  uint64_t io_id = ioinfo.get_id();
  std::map<uint64_t, MysqlPackProcessor*>::iterator iter = processor_map_.end();
  {
    CMutexLocker lock(map_mutex_);
    iter = processor_map_.find(io_id);
  }
  if (iter == processor_map_.end()) {
    LOGGER_ERROR(g_log_framework,"err=can't find processor for id %lu", io_id);
    return -1;
  } else {
    char* data = new char[msg.bytes_length];
    memcpy(data, msg.data, msg.bytes_length);
    uint32_t length = static_cast<uint32_t>(msg.bytes_length);
    //char* data = reinterpret_cast<char*>(msg.data);
    if (msg.fn_free) {
      msg.fn_free(msg.data,msg.free_ctx);
    }
    return iter->second->ProcessPack(ioinfo, data, length);
  }
}

int MysqlServer::SendReply(Framework* framework,
                uint64_t io_id,
                char* data,
                int length) {
  blitz::OutMsg* out_msg = NULL;
  blitz::OutMsg::CreateInstance(&out_msg);
  if (!out_msg ) {
    abort();
  }
  int ret = 0;
  if (data != NULL && length != 0) {
    out_msg->AppendData(data, length, FreeBuffer, NULL);
    ret = framework->SendMsg(io_id, out_msg);
  }
  out_msg->Release();
  if (ret != 0) {
    framework->CloseIO(io_id);
    return -1;
  }
  return 0;
}


MysqlPackProcessor::MysqlPackProcessor(MysqlPacketRealProcessor* processor) {
  isauth_ = false;
  processor_ = processor;
}


MysqlHelloSender::MysqlHelloSender(MysqlServer* server) {
  mysql_server_ = server;
}

int MysqlHelloSender::OnOpened(const IoInfo& ioinfo) {
  char sHandshake1[] =
  	  "\x00\x00\x00" // packet length
   		"\x00" // packet id
   		"\x0A"; // protocol version; v.10
    
  char sHandshake2[] =
   		"\x01\x00\x00\x00" // thread id
   		"\x01\x02\x03\x04\x05\x06\x07\x08" // scramble buffer (for auth)
   		"\x00" // filler
   		"\x08\x82" // server capabilities; CLIENT_PROTOCOL_41 | CLIENT_CONNECT_WITH_DB | SECURE_CONNECTION
   		"\x21" // server language
   		"\x02\x00" // server status
   		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // filler
   		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d"; // scramble buffer2 (for auth, 4.1+)
  //const char * sVersion = "5.1.58-log";
  const char * sVersion = "5.5.28-rel29.2-log";
  char *handshake = new char[128];
  int iLen = strlen(sVersion);
  int shake_length = sizeof(sHandshake1) + strlen(sVersion) + sizeof(sHandshake2) - 1;
  char * p = handshake;
  memcpy ( p, sHandshake1, sizeof(sHandshake1)-1 );
  memcpy ( p+sizeof(sHandshake1)-1, sVersion, iLen+1 );
  memcpy ( p+sizeof(sHandshake1)+iLen, sHandshake2, sizeof(sHandshake2)-1 );
  handshake[0] = (char)(shake_length-4); // safe, as long as buffer size is 128
  blitz::OutMsg* out_msg = NULL;
  blitz::OutMsg::CreateInstance(&out_msg);
  out_msg->AppendData(handshake, shake_length, FreeBuffer, NULL);
  int ret = mysql_server_->framework_.SendMsg(ioinfo.get_id(), out_msg);
  out_msg->Release();
  MysqlPackProcessor* processor = new MysqlPackProcessor(mysql_server_->processor_);
  LOGGER_DEBUG(g_log_framework, "debug=new MysqlPackProcessor,id=%lu", ioinfo.get_id());
  {
    CMutexLocker lock(mysql_server_->map_mutex_);
    mysql_server_->processor_map_.insert(std::pair<uint64_t, MysqlPackProcessor*>(ioinfo.get_id(), processor));
  }
  return ret;
}

int MysqlHelloSender::OnClosed(const IoInfo& ioinfo) {
  uint64_t io_id = ioinfo.get_id();
  std::map<uint64_t, MysqlPackProcessor*>::iterator iter = mysql_server_->processor_map_.end();
  {
    CMutexLocker lock(mysql_server_->map_mutex_);
    iter = mysql_server_->processor_map_.find(io_id);
  }
  if (iter == mysql_server_->processor_map_.end()) {
    LOGGER_ERROR(g_log_framework, "err=not found processor for id = %lu", io_id);
    return -1;
  } else {
    delete iter->second;
    LOGGER_DEBUG(g_log_framework, "debug=delete processor for id = %lu", io_id);
    {
      CMutexLocker lock(mysql_server_->map_mutex_);
      mysql_server_->processor_map_.erase(iter);
    }
  }
  return 0;
}

} // namespace blitz.
