#include <TApplicationException.h>
#include <utils/time_util.h>
#include <utils/logger.h>
#include "blitz/thrift_server.h"
namespace blitz {
static CLogger& g_log_pro = CLogger::GetInstance("protocol",CLogger::LEVEL_DEBUG);
static CLogger& g_log_framework = CLogger::GetInstance("framework");
static CLogger& request_logger = CLogger::GetInstance("request");
int ThriftServer::Init() {
  return 0;
}
int ThriftServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}
int ThriftServer::RegisterHandler(const char* fname, ThriftHandler* handler) {
  std::pair<THandlerMapIter, bool> result = handlers_.insert(
      std::make_pair(fname, handler));
  if (!result.second) {
    // 已经存在，需要先unregister.
    return 1;
  }
  return 0;
}
ThriftHandler* ThriftServer::UnregisterHandler(const char* fname) {
  THandlerMapIter iter = handlers_.find(fname);
  if (iter == handlers_.end()) {
    return NULL;
  }
  ThriftHandler* handler = iter->second;
  handlers_.erase(iter);
  return handler;
}
int ThriftServer::ProcessMsg(const IoInfo&  ioinfo,const blitz::InMsg& msg) {
  using boost::shared_ptr;
  using blitz::ThriftMemoryInTransport;
  using apache::thrift::protocol::TBinaryProtocol;
  shared_ptr<ThriftMemoryInTransport> in_transport(
      new ThriftMemoryInTransport((uint8_t*)msg.data + 4, msg.bytes_length - 4));
  shared_ptr<TBinaryProtocol> iprot(new TBinaryProtocol(in_transport));
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;
  int32_t seqid;
  iprot->readMessageBegin(fname, mtype, seqid);
  if (mtype != ::apache::thrift::protocol::T_CALL
      && mtype != ::apache::thrift::protocol::T_ONEWAY) {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(
        ::apache::thrift::TApplicationException::INVALID_MESSAGE_TYPE);
    SendReply(&framework_, ioinfo.get_id(), x, fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    if (msg.fn_free) {
      msg.fn_free(msg.data, msg.free_ctx);
    }
    return 0;
  }
  ThriftHandler* handler = GetHandler(fname.c_str());
  if (handler) {
    handler->Process(iprot.get(), seqid, ioinfo);
  } else {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(
        ::apache::thrift::TApplicationException::UNKNOWN_METHOD,
        "Invalid method name: '"+fname+"'");
    SendReply(&framework_, ioinfo.get_id(), x, fname,
              ::apache::thrift::protocol::T_EXCEPTION, seqid);
  }
  if (msg.fn_free) {
    msg.fn_free(msg.data, msg.free_ctx);
  }
  return 0;
}
// ThriftSyncServer implementation code.
//

int ThriftSyncServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}

struct MsgHandler {
  MsgHandler(ThriftSyncServer* server, const IoInfo& ioinfo, const InMsg& msg,
      uint64_t expire_time, int idx) {
    server_ = server;
    ioinfo_ = ioinfo;
    msg_ = msg;
    expire_time_ = expire_time;
    idx_ = idx;
  }
  void operator()() {
    if (expire_time_ > 0) {
      uint64_t cur_time = TimeUtil::GetTickMs();
      if (cur_time > expire_time_) {
        if (msg_.fn_free) {
          msg_.fn_free(msg_.data, msg_.free_ctx);
        }
        LOGGER_ERROR(g_log_framework, "attention ThriftSyncServer tasktimeout");
        return;
      }
    }
    server_->RealProcessMsg(ioinfo_, msg_, idx_);
  }
  ThriftSyncServer* server_;
  InMsg msg_;
  IoInfo ioinfo_;
  uint64_t expire_time_;
  int idx_;
};
int ThriftSyncServer::ProcessMsg(const IoInfo& ioinfo,const InMsg& msg) {
  uint64_t expire_time = 0;
  if(timeout_ > 0) { 
    expire_time = TimeUtil::GetTickMs() + timeout_;
  }
  idx_ = (++idx_)%thread_count_;
  MsgHandler handler(this, ioinfo, msg, expire_time, idx_);
  io_service_[idx_]->post(handler);
  return 0;
} 
int ThriftSyncServer::Start(int thread_count) {
  thread_count_ = thread_count;
  for(int i = 0; i < thread_count; ++i ) {
    boost::asio::io_service *tmp_io = new boost::asio::io_service();
    io_service_.push_back(tmp_io);
  }
  for(int i = 0; i < thread_count; ++i ) {
    boost::asio::io_service::work *tmp_work = new boost::asio::io_service::work(*io_service_[i]);
    work_.push_back(tmp_work);
    threads_.create_thread(boost::bind(&boost::asio::io_service::run, 
          io_service_[i]));
  }
  return 0;
}
int ThriftSyncServer::Stop() {
  for(int i = 0; i < thread_count_; ++i ) {
    io_service_[i]->stop();
    delete work_[i];
    delete io_service_[i];
  }
  threads_.join_all();
  return 0;
}
int ThriftSyncServer::RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg, int idx ) {
  uint64_t start = TimeUtil::GetTickUs();
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  uint64_t io_id = ioinfo.get_id();
  using boost::shared_ptr;
  using apache::thrift::protocol::TBinaryProtocol;
  boost::shared_ptr<ThriftMemoryInTransport> in_transport(
      new ThriftMemoryInTransport((uint8_t*)msg.data + 4,msg.bytes_length - 4));
  boost::shared_ptr<ThriftMemoryOutTransport> out_transport(
                                       new ThriftMemoryOutTransport());
  shared_ptr<TBinaryProtocol> iprot(new TBinaryProtocol(in_transport));
  shared_ptr<TBinaryProtocol> oprot(new TBinaryProtocol(out_transport));


  //get fname
  boost::shared_ptr<ThriftMemoryInTransport> in_transport_for_log( new ThriftMemoryInTransport((uint8_t*)msg.data + 4,msg.bytes_length - 4));
  shared_ptr<TBinaryProtocol> iprot_for_log(new TBinaryProtocol(in_transport_for_log));
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;
  int32_t seqid;
  iprot_for_log->readMessageBegin(fname, mtype, seqid);
  iprot_for_log->readMessageEnd();
  //get remote ip:port
  std::string remoteip;
  ioinfo.get_remote_addr().ToString(&remoteip);



  int ret = 0;
  try {
    LOGGER_DEBUG(g_log_pro,"[THRIFT-SERVER] -> processing thrift request.");
    processor_[idx]->process(iprot,oprot,NULL);
  } catch(const std::string& str) { 
    LOGGER_ERROR(g_log_framework, "CatchException io_id=0x%lx str=%s",
                 ioinfo.get_id(), str.c_str());
    ret = -1;
  } catch(std::exception& e) {
    LOGGER_ERROR(g_log_framework, "CatchException::exception io_id=0x%lx ex=%s",
                 io_id, e.what());
    ret = -2;
  } catch(...) {
    LOGGER_ERROR(g_log_framework, "CatchException::Unknown io_id=0x%lx",
                 io_id);
    ret = -3;
  }
  if (msg.fn_free) {
    msg.fn_free(msg.data, msg.free_ctx);
  }
  LOGGER_LOG(request_logger, "%s %lu %s %d", fname.c_str(), TimeUtil::GetTickUs()-start, remoteip.c_str(), ret);
  if (ret != 0) {
    framework_.CloseIO(io_id);
    return ret;
  }
  OutMsg* out_msg = NULL;
  OutMsg::CreateInstance(&out_msg);
  if (out_msg == NULL) {
    abort();
  }
  uint8_t* reply_data = NULL;
  uint32_t reply_data_len  = 0;
  out_transport->Detach(&reply_data,&reply_data_len);
  if (reply_data != NULL && reply_data_len != 0) {
    out_msg->AppendData(reply_data,reply_data_len,
                        ThriftMemoryOutTransport::FreeBuffer,NULL);
    ret = framework_.SendMsg(io_id,out_msg) ;
  }
  out_msg->Release();
  if (ret != 0) {
    framework_.CloseIO(io_id);
    return -1;
  }
  return 0;
}
// for ThriftTaskMgrServer implement
class ThriftTaskMrgMsgProcessTask {
 public:
  static int CreateTask(ThriftTaskMgrServer* server, const InMsg& msg,
      const IoInfo& ioinfo, Task& task) {
    task.fn_callback = TaskCallback;
    task.fn_delete = TaskFreeArg;
    task.args.arg1.ptr = new ThriftTaskMrgMsgProcessTask(server, msg, ioinfo);
    task.args.arg2.u64 = 0;
    task.expire_time = 0;
    return 0;
  }
  static void TaskCallback(const RunContext& run_context,
                          const TaskArgs& args) {
    AVOID_UNUSED_VAR_WARN(run_context);
    ThriftTaskMrgMsgProcessTask* task = reinterpret_cast<ThriftTaskMrgMsgProcessTask*>(args.arg1.ptr);
    // InMsg代表的内存由ProcessMsg释放.
    task->server_->RealProcessMsg(task->ioinfo_, task->msg_);
    delete task;
  }
  static void TaskFreeArg(const TaskArgs& args) {
    ThriftTaskMrgMsgProcessTask* task = reinterpret_cast<ThriftTaskMrgMsgProcessTask*>(args.arg1.ptr);
    if(task->msg_.fn_free) {
      task->msg_.fn_free(task->msg_.data,task->msg_.free_ctx);
    }
    delete task;
  }
  ThriftTaskMrgMsgProcessTask(ThriftTaskMgrServer* server,const InMsg& msg,const IoInfo& ioinfo)
    : server_(server),
      msg_(msg),ioinfo_(ioinfo) {
  }
  ThriftTaskMgrServer* server_;
  InMsg msg_;
  IoInfo ioinfo_;
};

int ThriftTaskMgrServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}

int ThriftTaskMgrServer::ProcessMsg(const IoInfo& ioinfo,const InMsg& msg) {
  Task task;
  ThriftTaskMrgMsgProcessTask::CreateTask(this, msg, ioinfo, task);
  if (timeout_ > 0) {
    task.expire_time = TimeUtil::GetTickMs() + timeout_;
  } else {
    task.expire_time = 0;
  }
  boost::mutex::scoped_lock guard(task_queue_mutex_);
  task_queue_.push(task);
  return 0;
}
int ThriftTaskMgrServer::PopValidTask(Task &task) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  boost::mutex::scoped_lock guard(task_queue_mutex_);
  static int total_proc_task_num = 0;
  static int timeout_task_num = 0;
  static int processed_num = 0;
  while (!task_queue_.empty()) {
    task = task_queue_.front();
    total_proc_task_num++;
    if (task.expire_time > 0) {
      uint64_t cur_time = TimeUtil::GetTickMs();
      if (cur_time > task.expire_time) {
	timeout_task_num++;
	task.fn_delete(task.args);
	task_queue_.pop();
	LOGGER_ERROR(g_log_framework, "TaskTimeout");
	continue;
      }
    }
    task_queue_.pop();
    processed_num++;
    LOGGER_INFO(g_log_framework, "cur_queue_size=%ld, total_proc_task_num=%d,"
	       "processed_task_num=%d, timeout_task_num=%d",
	       task_queue_.size(), total_proc_task_num,
	       processed_num, timeout_task_num);
    return 1;
  }
  return 0;
}
int ThriftTaskMgrServer::RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg ) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  uint64_t io_id = ioinfo.get_id();
  using boost::shared_ptr;
  using apache::thrift::protocol::TBinaryProtocol;
  boost::shared_ptr<ThriftMemoryInTransport> in_transport(
      new ThriftMemoryInTransport((uint8_t*)msg.data + 4,msg.bytes_length - 4));
  boost::shared_ptr<ThriftMemoryOutTransport> out_transport(
                                       new ThriftMemoryOutTransport());
  shared_ptr<TBinaryProtocol> iprot(new TBinaryProtocol(in_transport));
  shared_ptr<TBinaryProtocol> oprot(new TBinaryProtocol(out_transport));
  int ret = 0;
  try {
    processor_->process(iprot,oprot,NULL);
  } catch(const std::string& str) { 
    LOGGER_ERROR(g_log_framework, "CatchException io_id=0x%lx str=%s",
                 ioinfo.get_id(), str.c_str());
    ret = -1;
  } catch(std::exception& e) {
    LOGGER_ERROR(g_log_framework, "CatchException::exception io_id=0x%lx ex=%s",
                 io_id, e.what());
    ret = -2;
  } catch(...) {
    LOGGER_ERROR(g_log_framework, "CatchException::Unknown io_id=0x%lx",
                 io_id);
    ret = -3;
  }
  if (msg.fn_free) {
    msg.fn_free(msg.data, msg.free_ctx);
  }
  if (ret != 0) {
    framework_.CloseIO(io_id);
    return ret;
  }
  OutMsg* out_msg = NULL;
  OutMsg::CreateInstance(&out_msg);
  if (out_msg == NULL) {
    abort();
  }
  uint8_t* reply_data = NULL;
  uint32_t reply_data_len  = 0;
  out_transport->Detach(&reply_data,&reply_data_len);
  if (reply_data != NULL && reply_data_len != 0) {
    out_msg->AppendData(reply_data,reply_data_len,
                        ThriftMemoryOutTransport::FreeBuffer,NULL);
    ret = framework_.SendMsg(io_id,out_msg) ;
  }
  out_msg->Release();
  if (ret != 0) {
    framework_.CloseIO(io_id);
    return -1;
  }
  return 0;
}

} // namespace blitz.
