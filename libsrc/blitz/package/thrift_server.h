// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
// thrift应用相关类.
#ifndef __BLITZ_THRIFT_SERVER_H_
#define __BLITZ_THRIFT_SERVER_H_ 1

#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <boost/asio.hpp>
#include <TProcessor.h>
#include <protocol/TBinaryProtocol.h>
#include <TApplicationException.h>
#include "thrift_msg.h"
#include "framework.h"
namespace blitz {
 
class Framework;
class ThriftHandler;
// 使用Thrift协议的异步服务.
// 用法:
// Framework framework;
// ThriftServier server(framework);
// server.Bind("tcp://127.0.0.1:7070");
// server.Bind("tcp://127.0.0.1:7071");
// 
// server.RegisterHandler("GetOne", handler1);
// framework.Start();
// Thrift异步处理服务器.
class ThriftServer : public MsgProcessor {
 public:
  // 一些内部类型定义，方便写代码.
  typedef std::unordered_map<std::string, ThriftHandler*> THandlerMap;
  typedef THandlerMap::iterator THandlerMapIter;
  typedef THandlerMap::const_iterator THandlerMapConstIter;

 public:
  ThriftServer(blitz::Framework& framework):
    framework_(framework) {
  }
  ~ThriftServer() {
  }
  int Init();
  int Bind(const char* addr);
  int RegisterHandler(const char* fname, ThriftHandler* handler);
  ThriftHandler* UnregisterHandler(const char* fname);
  ThriftHandler* GetHandler(const char* fname) {
    THandlerMapIter iter = handlers_.find(fname);
    if (iter == handlers_.end()) {
      return NULL;
    }
    return iter->second;
  }
  // 实现MsgProcessor.
  int ProcessMsg(const IoInfo& ioinfo, const blitz::InMsg& msg);
  // 辅助函数，发送应答.
  template <typename T>
  static int SendReply(Framework* framework, 
                uint64_t io_id,
                T& struc,
                const std::string& fname,
                ::apache::thrift::protocol::TMessageType mtype,
                int32_t seq_id);
 protected:
  blitz::Framework& framework_;
  THandlerMap handlers_;
  ThriftFramedMsgFactory decoder_factory_;
};

// 使用Thrift协议的同步服务,直接用thrift生成的processor.
// 用法:
// Framework framework;
// ThriftSyncServer server(framework, processor);
// server.Bind("tcp://127.0.0.1:7070");
// server.Bind("tcp://127.0.0.1:7071");
// framework.Start();
//
class ThriftSyncServer:public MsgProcessor {
 public:
  // @param framework 
  // @param processor Thrift生成的代码.
  ThriftSyncServer(Framework& framework, std::vector<apache::thrift::TProcessor*> &processor)
    :processor_(processor), framework_(framework) {
      timeout_ = 0;
      //work_ = NULL;
      idx_ = 0;
      thread_count_ = 16;
  }
  int Bind(const char* addr);
  int ProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  int Start(int thread_count);
  int Stop();
  int RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg, int idx);
  void set_timeout(int v) { timeout_ = v; }
  int get_timeout() const { return timeout_; }
 protected:
  static void ProcessMsgTask(const RunContext& run_context,
                             const TaskArgs& task);
 protected:
  std::vector<apache::thrift::TProcessor*> processor_;
  Framework& framework_;
  ThriftFramedMsgFactory decoder_factory_;
  int timeout_; // 收到的请求多久没有执行就超时丢弃.
  std::vector<boost::asio::io_service *> io_service_;
  std::vector<boost::asio::io_service::work *> work_;
  boost::thread_group threads_;
  int idx_;
  int thread_count_;
};

// 整个进程中有且仅有一个实例
class ThriftTaskMgrServer : public MsgProcessor {
 public:
  // @param framework 
  // @param processor Thrift生成的代码.
  ThriftTaskMgrServer(Framework& framework, apache::thrift::TProcessor* processor)
    :processor_(processor), framework_(framework) {
      timeout_ = 0;
  }
  int Bind(const char* addr);
  int ProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  int RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  // @return the number of pop task number
  int PopValidTask(Task &task);
  void set_timeout(int v) { timeout_ = v; }
  int get_timeout() const { return timeout_; }
 protected:
  static void ProcessMsgTask(const RunContext& run_context,
                             const TaskArgs& task);
 protected:
  apache::thrift::TProcessor* processor_;
  Framework& framework_;
  ThriftFramedMsgFactory decoder_factory_;
  int timeout_; // 收到的请求多久没有执行就超时丢弃.
  boost::mutex task_queue_mutex_;
  std::queue<Task> task_queue_;
};

class ThriftHandler {
 public:
  // 处理Thrift消息.
  // 实例代码如下：
  // try {
  //    Test_GetOne_args args;
  //    args.read(iprot);
  //    iprot->readMessageEnd();
  //    uint32_t bytes = iprot->getTransport()->readEnd();
  //    GetOneHandler::Input input;
  //    input.key = args.key;
  //    input.redis_pool = redis_pool_;
  //    state_machine_.StartOneContext(
  //        input, OnGetOneEnd, params);
  //  }
  //  catch (...) {
  //   iprot->skip(::apache::thrift::protocol::T_STRUCT);
  //   iprot->readMessageEnd();
  //   iprot->getTransport()->readEnd();
  //   ::apache::thrift::TApplicationException x(
  //       ::apache::thrift::TApplicationException::UNKNOWN_METHOD,
  //       "Invalid method name: '"+fname+"'");
  //   SendReply(io_id, x, fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);

  //    
  //  }
  virtual int Process(::apache::thrift::protocol::TProtocol* iprot,
                      int seqid, const IoInfo& ioinfo) = 0; 
};

template <typename T>
int ThriftServer::SendReply(Framework* framework,
              uint64_t io_id,
              T& struc,
              const std::string& fname,
              ::apache::thrift::protocol::TMessageType mtype,
              int32_t seq_id) {
  using boost::shared_ptr;
  using ::apache::thrift::protocol::TBinaryProtocol;
  shared_ptr<ThriftMemoryOutTransport> out_transport(new ThriftMemoryOutTransport());
  shared_ptr<TBinaryProtocol> oprot(new TBinaryProtocol(out_transport));
  oprot->writeMessageBegin(fname, mtype, seq_id);
  struc.write(oprot.get());
  oprot->writeMessageEnd();
  oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();
  blitz::OutMsg* out_msg = NULL;
  blitz::OutMsg::CreateInstance(&out_msg);
  if (!out_msg ) {
    abort();
  }
  uint8_t* reply_data = NULL;
  uint32_t reply_data_len  = 0;
  out_transport->Detach(&reply_data, &reply_data_len);
  int ret = 0;
  if (reply_data != NULL && reply_data_len != 0) {
    out_msg->AppendData(reply_data,reply_data_len,ThriftMemoryOutTransport::FreeBuffer,NULL);
    ret = framework->SendMsg(io_id,out_msg) ;
  }
  out_msg->Release();
  if (ret != 0) {
    framework->CloseIO(io_id);
    return -1;
  }
  return 0;
}


} // namespace blitz.
#endif // __BLITZ_THRIFT_SERVER_H_
