#include <utils/time_util.h>
#include <utils/logger.h>
#include <string>
#include <sys/time.h>
#include <zlib.h>
#include <boost/lexical_cast.hpp>
#include "blitz/imgdbproxy_server.h"
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

class ImgdbProxyTaskProcessTask {
 public:
  static int CreateTask(ImgdbProxyTaskServer* server,
                        const InMsg& msg,
                        const IoInfo& ioinfo,
                        Task& task) {
    task.fn_callback = TaskCallback;
    task.fn_delete = TaskFreeArg;
    task.args.arg1.ptr = new ImgdbProxyTaskProcessTask(server, msg, ioinfo);
    task.args.arg2.u64 = 0;
    task.expire_time = 0;
    return 0;
  }
  static void TaskCallback(const RunContext& run_context,
                          const TaskArgs& args) {

    LOGGER_INFO(g_log_framework, "act=TaskCallback");
    AVOID_UNUSED_VAR_WARN(run_context);
    ImgdbProxyTaskProcessTask* task = reinterpret_cast<ImgdbProxyTaskProcessTask*>(args.arg1.ptr);
    // InMsg代表的内存由ProcessMsg释放.
    task->server_->RealProcessMsg(task->ioinfo_, task->msg_);
    delete task;
  }
  static void TaskFreeArg(const TaskArgs& args) {
    ImgdbProxyTaskProcessTask* task = reinterpret_cast<ImgdbProxyTaskProcessTask*>(args.arg1.ptr);
    if(task->msg_.fn_free) {
      task->msg_.fn_free(task->msg_.data,task->msg_.free_ctx);
    }
    delete task;
  }
  ImgdbProxyTaskProcessTask(ImgdbProxyTaskServer* server,
                           const InMsg& msg,
                           const IoInfo& ioinfo)
    : server_(server),
      msg_(msg),ioinfo_(ioinfo) {
  }
  ImgdbProxyTaskServer* server_;
  InMsg msg_;
  IoInfo ioinfo_;
};

int ImgdbProxyTaskServer::EchoString(const IoInfo& ioinfo,
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

int ImgdbProxyTaskServer::EchoData(const IoInfo& ioinfo,
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
int ImgdbProxyTaskServer::mmc_uncompress(const char *data, unsigned long data_len, char **result, unsigned long *result_len) {
	int status = 1;
  int factor = 1;
	do {
		*result_len = data_len * (1 << factor++);
		*result = (char *)realloc(*result, *result_len + 1);
		status = uncompress((unsigned char *)*result, result_len, (unsigned const char *)data, data_len);
	} while (status == Z_BUF_ERROR && factor < 16);

  LOGGER_INFO(g_log_framework, "factor=%d,result_len=%d,status=%d,Z_MEM_ERROR=%d,Z_BUF_ERROR=%d,Z_DATA_ERROR=%d", factor, *result_len,status,Z_MEM_ERROR,Z_BUF_ERROR,Z_DATA_ERROR);
	if (status == Z_OK) {
    LOGGER_INFO(g_log_framework, "mmc_uncompress=%s", "ok");
		return 0;
	}
	free(*result);
	return -1;
}

size_t ImgdbProxyTaskServer::TokenizeCommand(char *command,
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
    else if (*e == '\r' && *(e + 1) == '\n') {
      tokens[ntokens].value = s;
      tokens[ntokens].length = e - s;
      ntokens++;
      *e = '\0';
      ++e;
      ++e;
      s = e;
      //解析第一行，然后退出
      break;
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
  if(strcmp(tokens[COMMAND_TOKEN].value, "set") == 0) {
    unsigned long data_len = boost::lexical_cast<int>(tokens[4].value);
    int flag = boost::lexical_cast<int>(tokens[2].value); //compressed or not
    LOGGER_INFO(g_log_framework, "setflag=%d", flag); 
    if(flag == 0) { 
      LOGGER_INFO(g_log_framework, "flag=%d,value=%s, length=%s", flag, s, tokens[4].value); 
      tokens[ntokens].value = s;
      tokens[ntokens].length = boost::lexical_cast<int>(tokens[4].value);
    }
    else if(flag == 2) { //compresssed data
      LOGGER_INFO(g_log_framework, "data_len=%u",data_len); 
      char *result = NULL;
      unsigned long result_len = 0;
      //*(s + data_len) = '\0';
      //*(s + data_len + 1) = '\0';
      //char *source = new char[data_len + 2];
      //strncpy(source, s, data_len);
      //source[data_len] = '\r';
      //source[data_len + 1] = '\n';
      int ret = mmc_uncompress(s, data_len, &result, &result_len);
      if(0 != ret) {
        LOGGER_ERROR(g_log_framework, "retofuncompress=%d", ret);
      }
      tokens[ntokens].value = (char *)malloc(result_len);
      //strncpy(tokens[ntokens].value, result, result_len);
      memcpy(tokens[ntokens].value, result, result_len);
      tokens[ntokens].length = result_len;
      LOGGER_INFO(g_log_framework, "result_len=%u",result_len); 
    }
    e = e + boost::lexical_cast<int>(data_len);
    *e = '\0';
    ++e;
    *e = '\0';
    ntokens++;
  }
  tokens[ntokens].value =  *e == '\0' ? NULL : e;
  tokens[ntokens].length = 0;
  ntokens++;

  return ntokens;
}

//处理memcached的get请求,只有get key一种情况，不支持get多个key
int ImgdbProxyTaskServer::ProcessGetCommand(const IoInfo& ioinfo,
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
  ret = processor_->Get(key, data, data_len);
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

//处理memcached的set请求
int ImgdbProxyTaskServer::ProcessSetCommand(const IoInfo& ioinfo,
                                           token_t *tokens,
                                           size_t ntokens) {
  timeval t1,t2;
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  std::string request,key;
  size_t nkey;
  //char *data = NULL; 
  size_t data_len = 0;
  int ret;
  if (ntokens < 2) {
    return 0;
  }
  if (tokens[KEY_TOKEN].value == NULL || tokens[KEY_TOKEN].length == 0){
    ret = EchoString(ioinfo, "ERROR");
    return ret;
  }
  request = tokens[KEY_TOKEN].value; 
  nkey = tokens[KEY_TOKEN].length;
  if(nkey > KEY_MAX_LENGTH) {
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
    return ret;
  }
  //计算len和data
  //data_len = tokens[5].length;
  //data_len = boost::lexical_cast<int>(tokens[4].value);
  //data = tokens[5].value;
  LOGGER_INFO(g_log_framework, 
      "tokens[5].value=%s,tokens[5].length=%ld, tokens=%ld, tokens[4].value=%s", tokens[5].value, tokens[5].length, ntokens, tokens[4].value);
  ParseRequest(request, key);
  gettimeofday(&t1, NULL);
  //对set返回值的判断
  ret = processor_->Set(key, tokens[5].value, tokens[5].length);
  gettimeofday(&t2, NULL);
  int time = t2.tv_sec*1000000 + t2.tv_usec - t1.tv_sec*1000000 - t1.tv_usec;
  LOGGER_INFO(g_log_framework, "act=processor_->Set time = %d", time);
  if (0 != ret) {
    LOGGER_ERROR(g_log_framework, 
                 "act=ProcessSetCommand,err=Set fail,key=%s",
                 key.c_str());
    ret = EchoString(ioinfo, "SERVER_ERROR");
    return ret;
  }
  LOGGER_INFO(g_log_framework, 
              "act=ProcessSetCommand,info=Set,data_len=%ld,key=%s",
              data_len, key.c_str());
  ret = EchoString(ioinfo, "STORED");
  return ret;
}

int ImgdbProxyTaskServer::ProcessCommand(const IoInfo& ioinfo,
                                        const char* command) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  token_t tokens[MAX_TOKENS];
  size_t ntokens;
  int ret = -1;
  ntokens = TokenizeCommand(const_cast<char *>(command), tokens, MAX_TOKENS);
  LOGGER_INFO(g_log_framework, "act=ProcessCommand, ntokens=%ld, tokens=", ntokens);
  for(size_t i=0; i < ntokens; ++i) {
    LOGGER_INFO(g_log_framework, "%s;", tokens[i].value);
  }
  if (ntokens < 3 ) { 
    LOGGER_ERROR(g_log_framework, "act=ProcessCommand,err=ntokens < 3,command=%s",command);
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
    return ret;
  }
  if (strcmp(tokens[COMMAND_TOKEN].value, "get") == 0)  {
    ret = ProcessGetCommand(ioinfo, tokens, ntokens);
    //如果是set命令
  } else if (strcmp(tokens[COMMAND_TOKEN].value, "set") == 0)  {
    ret = ProcessSetCommand(ioinfo, tokens, ntokens);
  } else {
    LOGGER_ERROR(g_log_framework, "act=ProcessCommand,err=token has not been supported,command=%s", command);
    ret = EchoString(ioinfo, "CLIENT_ERROR bad command line format");
  }      
  return ret;
}

int ImgdbProxyTaskServer::Bind(const char* addr) {
  return framework_.Bind(addr, &decoder_factory_, this, NULL);
}

int ImgdbProxyTaskServer::ProcessMsg(const IoInfo& ioinfo,const InMsg& msg) {
  Task task;
  ImgdbProxyTaskProcessTask::CreateTask(this, msg, ioinfo, task);//创建task
  if (timeout_ > 0) {
    task.expire_time = TimeUtil::GetTickMs() + timeout_;
  } else {
    task.expire_time = 0;
  }
  boost::mutex::scoped_lock guard(task_queue_mutex_);
  task_queue_.push(task);//添加到任务队列
  return 0;
}

int ImgdbProxyTaskServer::PopValidTask(Task &task) {
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


int ImgdbProxyTaskServer::RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  uint64_t io_id = ioinfo.get_id();
  int ret = 0;
  std::string command(reinterpret_cast<char*>(msg.data), msg.bytes_length);
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

void ImgdbProxyTaskServer::ParseRequest(const std::string &request, std::string &key) {
  key = request;
  size_t pos_end = key.find("\r\n");
  if (std::string::npos != pos_end) {
    key.erase(pos_end, 2);
  }
}

} // namespace blitz
