#include <iostream>
#include <boost/shared_ptr.hpp>
#include "blitz/framework.h"
#include "blitz/thrift_msg.h"
#include <protocol/TBinaryProtocol.h>
#include "utils/logger_loader.h"

class ThriftMsgProcessor:public blitz::MsgProcessor {
  public:
    ThriftMsgProcessor(blitz::Framework& framework):
      framework_(framework) {
      }
    int ProcessMsg(const blitz::IoInfo& ioinfo, const blitz::InMsg& msg){
      // boost::shared_ptr<blitz::MemoryInTransport> transport(
      //     new blitz::MemoryInTransport(msg.data, msg.bytes_length));
      // boost::shared_ptr<blitz::MemoryOutTransport> out;
      // apache::thrift::protocol::TBinaryProtocol iprot(transport);
      // apache::thrift::protocol::TBinaryProtocol oprot(out);
      
      blitz::OutMsg* out_msg = NULL;
      blitz::OutMsg::CreateInstance(&out_msg);
      out_msg->AppendData(msg.data,msg.bytes_length,msg.fn_free,msg.free_ctx);
      framework_.SendMsg(ioinfo.get_id(), out_msg);
      out_msg->Release();
      return 0;
    }
 private:
  blitz::Framework& framework_;
};

int main(int argc,char* argv[]){
  std::string log_str1("framework,%L %T %N %P:%p %f(%F:%l) - %M,thrift_server.log,,"); 
  log_str1 += "10";
  InitLogger("./", log_str1.c_str());
  blitz::Framework framework;
  framework.Init();
  ThriftMsgProcessor processor(framework);
  blitz::ThriftFramedMsgFactory parser_factory;
  uint64_t io_id = 0;
  framework.Bind("tcp://*:5556", &parser_factory, &processor, &io_id);
  framework.Start();
  framework.Uninit();
  return 0;
}
