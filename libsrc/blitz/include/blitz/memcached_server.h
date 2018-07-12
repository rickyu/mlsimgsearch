#ifndef __BLITZ_MEMCACHED_SERVER_H_
#define __BLITZ_MEMCACHED_SERVER_H_ 1

#include <arpa/inet.h>
#include <unordered_map>
#include <boost/asio.hpp>
#include <string>
#include "memcached_server_msg.h"
#include "framework.h"

namespace blitz {
#define MAX_TOKENS 8
#define KEY_TOKEN 1
#define COMMAND_TOKEN 0
#define SUBCOMMAND_TOKEN 1
#define KEY_MAX_LENGTH 250
#define HEAD_MAX_LENGTH 200
#define DATA_BUFFER_SIZE 2048

typedef struct token_s {
  char *value;
  size_t length;
} token_t;

typedef enum {
  MEMCACHED_PROCESS_SUCCESS, //0
  MEMCACHED_PROCESS_PROTOCOL_ERROR,
  MEMCACHED_PROCESS_SERVER_ERROR,
  MEMCACHED_PROCESS_DATA_ERROR,
} memc_server_process_t;

class Framework;
class MemcachedProcessor {
 public:
  virtual int GetItemData(char* &data, size_t &data_len, const std::string &key) = 0;
};


//使用者需要注册自己的processor
class MemcachedTaskServer : public MsgProcessor {
 public:
  MemcachedTaskServer(Framework& framework,MemcachedProcessor* processor)
      :framework_(framework),processor_(processor) {
        timeout_ = 0;
  }
  ~MemcachedTaskServer() {}
  int Bind(const char *addr);
  int ProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  int RealProcessMsg(const IoInfo& ioinfo, const InMsg& msg);
  int PopValidTask(Task &task);
  void set_timeout(int v) { timeout_ = v; }
  int get_timeout() const { return timeout_; }

 private:
  int ProcessCommand(const IoInfo& ioinfo,
                     const char* command);

  size_t TokenizeCommand(char *command,
                         token_t *tokens,
                         const size_t max_tokens);

  int ProcessGetCommand(const IoInfo& ioinfo,
                        token_t *tokens,
                        size_t ntokens);

  int EchoString(const IoInfo& ioinfo,
                 const char *str);

  int EchoData(const IoInfo& ioinfo,
                char* data, 
                size_t data_len,
                std::string &key);

  void ParseRequest(const std::string &request, std::string &key);

 protected:
  static void ProcessMsgTask(const RunContext& run_context,
                             const TaskArgs& task);

 protected:
  Framework& framework_;
  MemcachedProcessor* processor_; //根据相应的图片key返回图片数据
  MemcachedServerMsgDecoderFactory decoder_factory_;
  int timeout_;
  boost::mutex task_queue_mutex_;
  std::queue<Task> task_queue_;
};

}

#endif
