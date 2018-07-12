#include <iostream>
#include <boost/shared_ptr.hpp>
#include "blitz/framework.h"
#include "blitz/thrift_msg.h"
#include <protocol/TBinaryProtocol.h>
#include "utils/logger_loader.h"
#include "blitz/buffer.h"

class ThriftMsgProcessor:public blitz::MsgProcessor,
                         public blitz::IoStatusObserver {
  public:
    ThriftMsgProcessor(blitz::Framework& framework):
      framework_(framework) {
        send_count_ = 0;
        recv_count_ = 0;
      }
    int ProcessMsg(const blitz::IoInfo& ioinfo, const blitz::InMsg& msg) {
      ++ recv_count_;
      framework_.CloseIO(ioinfo.get_id());
      if (recv_count_ % 100 == 0) {
        printf("recv %d\n", recv_count_);
      }
      return 0;
    }
    int OnOpened(const blitz::IoInfo& ioinfo) {
      blitz::OutMsg* out_msg = NULL;
      blitz::OutMsg::CreateInstance(&out_msg);
      int r = rand() % 1024;
      if (r <= 4) { r = 4; }
      char* buffer = reinterpret_cast<char*>(blitz::Alloc::Malloc(r));
      out_msg->AppendData(buffer, r, blitz::Alloc::Free, NULL);
      int ret = framework_.SendMsg(ioinfo.get_id(), out_msg);
      out_msg->Release();
      if (ret != 0) {
        printf("Send fail \n");
      } else {
        ++ send_count_;
      }
      if (send_count_ % 100 == 0) {
        printf("sent %d\n", send_count_);
      }
      return 0;
    }
    int OnClosed(const blitz::IoInfo& ioinfo)  {
      return 0;
    }
    void Attack() {
      uint64_t io_id = 0;
      int ret = framework_.Connect(server_, &decoder_factory_, this, this, &io_id);
      if (ret != 0) {
        printf("Connect fail \n");
        return;
      }
    }
    void PostTask() {
      blitz::Task task;
      task.fn_callback   = MyTask;
      task.fn_delete = NULL;
      task.args.arg1.ptr = this;
      task.args.arg2.u64 = 0;
      usleep(10000);
      int ret = framework_.PostTask(task);
      if (ret != 0) {
        printf("Post Task Fail\n");
      }

    }
    static void MyTask( const blitz::RunContext& run_context, const blitz::TaskArgs& args) {
      ThriftMsgProcessor* self = reinterpret_cast<ThriftMsgProcessor*>(args.arg1.ptr);
      self->Attack();
      self->PostTask();
    }
    void set_server(const std::string& server) { server_ = server; }
 private:
  blitz::Framework& framework_;
  std::string server_;
  int send_count_;
  int recv_count_;
  blitz::ThriftFramedMsgFactory decoder_factory_;
};

int main(int argc,char* argv[]){
  if (argc < 2) {
    printf("please set server\n");
    return 1;
  }
  srand(time(NULL));
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,evil_client.log,,"); 
  log_str1 += "10";
  InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  framework.Init();
  ThriftMsgProcessor processor(framework);
  processor.set_server(argv[1]);
  processor.PostTask();
  framework.Start();
  framework.Uninit();
  return 0;
}
