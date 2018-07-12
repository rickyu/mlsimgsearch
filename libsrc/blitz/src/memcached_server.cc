#include <utils/time_util.h>
#include <utils/logger.h>
#include <string>
#include <sys/time.h>
#include "blitz/memcached_server.h"
using namespace std;

namespace blitz {

static CLogger& g_log_framework = CLogger::GetInstance("framework");

static void FreeBuffer(void* buffer,void* ctx) {
  AVOID_UNUSED_VAR_WARN(ctx);
  if(buffer){
    free(buffer);
    buffer = NULL;
  }
}

class MemcachedTaskProcessTask {
 public:
  static int CreateTask(MemcachedTaskServer* server,
                        const InMsg& msg,
                        const IoInfo& ioinfo,
                        Task& task) {
    task.fn_callback = TaskCallback;
    task.fn_delete = TaskFreeArg;
    task.args.arg1.ptr = new MemcachedTaskProcessTask(server, msg, ioinfo);
    task.args.arg2.u64 = 0;
    task.expire_time = 0;
    return 0;
  }
  static void TaskCallback(const RunContext& run_context,
                          const TaskArgs& args) {

    LOGGER_INFO(g_log_framework, "act=TaskCallback");
    AVOID_UNUSED_VAR_WARN(run_context);
    MemcachedTaskProcessTask* task = reinterpret_cast<MemcachedTaskProcessTask*>(args.arg1.ptr);
    // InMsg代表的内存由ProcessMsg释放.
    task->server_->RealProcessMsg(task->ioinfo_, task->msg_);
    delete task;
  }
  static void TaskFreeArg(const TaskArgs& args) {
    MemcachedTaskProcessTask* task = reinterpret_cast<MemcachedTaskProcessTask*>(args.arg1.ptr);
    if(task->msg_.fn_free) {
      task->msg_.fn_free(task->msg_.data,task->msg_.free_ctx);
    }
    delete task;
  }
  MemcachedTaskProcessTask(MemcachedTaskServer* server,
                           const InMsg& msg,
                           const IoInfo& ioinfo)
    : server_(server),
      msg_(msg),ioinfo_(ioinfo) {
  }
  MemcachedTaskServer* server_;
  InMsg msg_;
  IoInfo ioinfo_;
};

int MemcachedTaskServer::EchoString(const IoInfo& ioinfo,
                                    const char *str) {
  int ret = 0;
  blitz::OutMsg* out_msg = NULL;
  size_t len = strlen(str);
  size_t wbytes = 0;
  char *wbuf = new char[HEAD_MAX_LENGTH];
  if ((len + 2) > HEAD_MAX_LENGTH) { 
    str = "SERVER_ERROR output line too long";
    len = strlen(str);
  }
  memcpy(wbuf, str, len);
  memcpy(wbuf + len, "\r\n", 2);
  wbytes = len + 2;

  blitz::OutMsg::CreateInstance(&out_msg);
  out_msg->AppendData(wbuf,
                      wbytes,
                      FreeBuffer,
                      NULL );
  ret = framework_.SendMsg(ioinfo.get_id(), out_msg);
  out_msg->Release();
  return ret;
}

int MemcachedTaskServer::EchoData(const IoInfo& ioinfo,
                                  char* data,
                                  size_t data_len, 
                                  std::string &key) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  int ret = 0;
  blitz::OutMsg* out_msg = NULL;

  char *seps = new char[7];
  memcpy(seps, "\r\nEND\r\n", 7);

  size_t head_len =  key.length() + 2;

  if (head_len > KEY_MAX_LENGTH) {
    ret = EchoString(ioinfo, "SERVER_ERROR out of memory preparing response");
    LOGGER_ERROR(g_log_framework, "act=EchoData,err=head_len > KEY_MAX_LENGTH");
    return ret;
  }
  if (0 == data_len) {
    ret = EchoString(ioinfo, "SERVER_ERROR out of memory preparing response");
    LOGGER_ERROR(g_log_framework, "act=EchoData,err=data_len is 0");
    return ret;
  }

  char *head = new char[KEY_MAX_LENGTH];
  sprintf(head, "VALUE %s 0 %ld\r\n", key.c_str(), data_len);
  head_len = strlen(head);

  blitz::OutMsg::CreateInstance(&out_msg);
  if (out_msg == NULL) {
    LOGGER_ERROR(g_log_framework, "act=EchoData,err=CreateInstance fail");
    abort();
  }

  out_msg->AppendData(head,
                      head_len,
                      FreeBuffer,
                      NULL);
  out_msg->AppendData(data,
                      data_len,
                      FreeBuffer,
                      NULL);
  out_msg->AppendData(seps,
                      7,
                      FreeBuffer,
                      NULL);
  LOGGER_INFO(g_log_framework, 
              "act=EchoData,info=AppendData,head=%s,data_len=%ld",
              head, data_len);
  ret = framework_.SendMsg(ioinfo.get_id(), out_msg);
  out_msg->Release();
  return ret; 
}

size_t MemcachedTaskServer::TokenizeCommand(char *command,
                                            token_t *tokens,
                                            const size_t max_tokens) {
  char *s, *e;
  size_t ntokens = 0;

  assert(command != NULL && tokens != NULL && max_tokens > 1);

  for (s = e = command; ntokens < max_tokens - 1; ++e) {
    if (*e == ' ') {
      if (s != e) {
        tokens[ntokens].value = s;
        tokens[ntokens].length = e - s;
        ntokens++;
        *e = '\0';
      }
      s = e + 1;
    }
    else if (*e == '\0') {
      if (s != e) {
        tokens[ntokens].value = s;
        tokens[ntokens].length = e - s;
        ntokens++;
      }
      break; 
    }
  }
  tokens[ntokens].value =  *e == '\0' ? NULL : e;
  tokens[ntokens].length = 0;
  ntokens++;

  return ntokens;
}

//处理memcached的get请求,只有get key一种情况，不支持get多个key
int MemcachedTaskServer::ProcessGetCommand(const IoInfo& ioinfo,
                                           token_t *tokens,
                                           size_t ntokens) {
  timeval t1,t2;
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  std::string request,key;
  size_t nkey;
  char *data = NULL; 
  size_t data_len = 0;
  int ret;
  if (ntokens < 2) {
    return 0;
  }
  if (tokens[KEY_TOKEN].value == NULL || tokens[KEY_TOKEN].length == 0){
    ret = EchoString(ioinfo, "END");
    return ret;
  }
  request = tokens[KEY_TOKEN].value; 
  nkey = tokens[KEY_TOKEN].length;
  if(nkey > KEY_MAX_LENGTH) {
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
    return ret;
  }
  ParseRequest(request, key);
  gettimeofday(&t1, NULL);
  ret = processor_->GetItemData(data, data_len, key);
  gettimeofday(&t2, NULL);
  int time = t2.tv_sec*1000000 + t2.tv_usec - t1.tv_sec*1000000 - t1.tv_usec;
  LOGGER_INFO(g_log_framework, "GetItemData time = %d", time);
  if (0 != ret) {
    LOGGER_ERROR(g_log_framework, 
                 "act=ProcessGetCommand,err=GetItemData fail,key=%s",
                 key.c_str());
    ret = EchoString(ioinfo, "END");
    return ret;
  }
  LOGGER_INFO(g_log_framework, 
              "act=ProcessGetCommand,info=GetItemData,data_len=%ld,key=%s",
              data_len, key.c_str());
  ret = EchoData(ioinfo, data, data_len, key);
  return ret;
}

int MemcachedTaskServer::ProcessCommand(const IoInfo& ioinfo,
                                        const char* command) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  token_t tokens[MAX_TOKENS];
  size_t ntokens;
  int ret = -1;
  ntokens = TokenizeCommand(const_cast<char *>(command), tokens, MAX_TOKENS);
  LOGGER_INFO(g_log_framework, "act=ProcessCommand, ntokens=%ld", ntokens);
  if (ntokens < 3 ) { 
    LOGGER_ERROR(g_log_framework, "act=ProcessCommand,err=ntokens < 3,command=%s",command);
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
    return ret;
  }
  if (strcmp(tokens[COMMAND_TOKEN].value, "get") == 0)  {
    ret = ProcessGetCommand(ioinfo, tokens, ntokens);
  } else {
    LOGGER_ERROR(g_log_framework, "act=ProcessCommand,err=token has not been supported,command=%s", command);
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
  }      
  return ret;
}

int MemcachedTaskServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}

int MemcachedTaskServer::ProcessMsg(const IoInfo& ioinfo,const InMsg& msg) {
  Task task;
  MemcachedTaskProcessTask::CreateTask(this, msg, ioinfo, task);//创建task
  if (timeout_ > 0) {
    task.expire_time = TimeUtil::GetTickMs() + timeout_;
  } else {
    task.expire_time = 0;
  }
  boost::mutex::scoped_lock guard(task_queue_mutex_);
  task_queue_.push(task);//添加到任务队列
  return 0;
}

int MemcachedTaskServer::PopValidTask(Task &task) {
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


int MemcachedTaskServer::RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  uint64_t io_id = ioinfo.get_id();
  int ret = 0;
  std::string command;
  command.assign(reinterpret_cast<char*>(msg.data));
  ret = ProcessCommand(ioinfo, const_cast<char *>(command.c_str()));      
  if (msg.fn_free) {
    msg.fn_free(msg.data,msg.free_ctx);
  }
  if (ret != 0) {
    LOGGER_ERROR(g_log_framework, 
                 "act=RealProcessMsg,err=ProcessCommand fail,command=%s",
                 command.c_str());
    framework_.CloseIO(io_id);
    return -1;
  }
  return 0;
}

void MemcachedTaskServer::ParseRequest(const std::string &request, std::string &key) {
  key = request;
  size_t pos_end = key.find("\r\n");
  if (std::string::npos != pos_end) {
    key.erase(pos_end, 2);
  }
}

} // namespace blitz
