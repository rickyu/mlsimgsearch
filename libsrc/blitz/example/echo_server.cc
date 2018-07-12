#include <iostream>
#include "blitz/framework.h"
#include "blitz/line_msg.h"
#include "utils/logger_loader.h"

class MyMsgProcessor:public blitz::MsgProcessor {
 public:
   MyMsgProcessor(blitz::Framework& framework) 
     :framework_(framework) {
   }
  int ProcessMsg(const blitz::IoInfo& ioinfo, const blitz::InMsg& msg){
    blitz::OutMsg* out_msg = NULL;
    const char* str = reinterpret_cast<char*>(msg.data);
    printf("thread_number=%d ioid=%lu msg=[%s]\n", 
           framework_.GetCurrentThreadNumber(), ioinfo.get_id(), str);
    blitz::OutMsg::CreateInstance(&out_msg);
    out_msg->AppendData(msg.data, 
                        msg.bytes_length, msg.fn_free, msg.free_ctx);
    framework_.SendMsg(ioinfo.get_id(), out_msg);

    out_msg->Release();
    return 0;
  }
  blitz::Framework& framework_;
};
void LongTimeTask(const blitz::RunContext& run_context,
                  const blitz::TaskArgs& task_args) {
  printf("start thread_number=%d, thread_id=%d\n", 
      run_context.thread_number, static_cast<int>(run_context.thread_id));
  sleep(30);
  printf("end thread_number=%d, thread_id=%d\n", 
      run_context.thread_number, static_cast<int>(run_context.thread_id));

}

void TimerPostTask(int32_t id, void* arg) {
  blitz::Task task;
  blitz::TaskUtil::ZeroTask(&task);
  task.fn_callback = LongTimeTask;
  reinterpret_cast<blitz::Framework*>(arg)->PostTask(4, task);
}
int main(int argc,char* argv[]){
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,echoserver.log,,"); 
  log_str1 += "0";
  InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  framework.set_thread_count(5);
  framework.Init();
  char str[80];
  str[0]  = '\0';
  char* p = strerror_r(24, str, sizeof(str));
  printf("err=%s str=%p p=%p\n", p, str, p);
  MyMsgProcessor processor(framework);
  blitz::SimpleMsgDecoderFactory<blitz::LineMsgDecoder> decoder_factory;
  uint64_t io_id = 0;
  framework.Bind("tcp://*:5555",&decoder_factory,&processor, &io_id);
  framework.SetTimer(1000, TimerPostTask, (void*)&framework);
  framework.Start();
  framework.Uninit();
  return 0;
}
