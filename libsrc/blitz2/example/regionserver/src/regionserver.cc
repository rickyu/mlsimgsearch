#include "stdafx.h"
using ::apache::thrift::protocol::TBinaryProtocol;
using ::apache::thrift::protocol::TProtocol;
using ::apache::thrift::transport::TTransport;
using blitz2::ThriftMsg;
using blitz2::RegionServer;
using blitz2::ThriftFramedMsgDecoder;
using blitz2::ThriftMemoryOutTransport;
using blitz2::Buffer;
using blitz2::IoAddr;
using blitz2::OutMsg;
using blitz2::SharedPtr;
using blitz2::SharedBuffer;
using blitz2::MsgData;
using blitz2::SharedObject;
using blitz2::BytesMsg;
using blitz2::EQueryResult;
using blitz2::kQueryResultSuccess;
using blitz2::ThriftMemoryInTransport;
using boost::shared_ptr;
using namespace apache::hadoop::hbase::thrift;

typedef SharedObject<BytesMsg> ThriftMsg;

SharedPtr<OutMsg> CreateOutMsg(ThriftMemoryOutTransport* trans) {
  SharedPtr<OutMsg>  out_msg = OutMsg::CreateInstance();
  uint8_t* reply_data = NULL;
  uint32_t reply_data_len  = 0;
  trans->Detach(&reply_data, &reply_data_len);
  if (reply_data != NULL && reply_data_len != 0) {
    SharedPtr<SharedBuffer> shared_buffer = SharedBuffer::CreateInstance();
    shared_buffer->Attach(reply_data, reply_data_len, Buffer::DestroyFreeImpl);
    out_msg->AppendData(MsgData(shared_buffer, 0, reply_data_len));
  }
  return out_msg;
}
void GetCallback(EQueryResult result, ThriftMsg* msg) {
  cout << "GetCallback" << endl;
  if (result == kQueryResultSuccess) {
    shared_ptr<TTransport> transport(new ThriftMemoryInTransport(
          msg->data(), msg->data_len()));
    shared_ptr<TBinaryProtocol> proto(new TBinaryProtocol(transport));
     HbaseClient client(proto);
     std::vector<TCell> cells;
     try {
       client.recv_get(cells);
     } catch(...) {
       cout << "kQueryResultInvalidResp" << endl;
     }
  }
  cout << "result != kQueryResultSuccess" << endl;
}
int main() {
  Reactor reactor;
  reactor.Init();
  ThriftFramedMsgDecoder::TInitParam param; 
  char hostname[] = "localhost.localdomain";
  struct hostent *ht;
  if((ht=gethostbyname(hostname))==NULL) {
    cout << "Not get host by name" << endl;
    exit(1);
  }
  cout << "ip=" << inet_ntoa(*((struct in_addr *)ht->h_addr)) << endl;
  IoAddr addr;
  addr.Parse("tcp://127.0.0.1:9091");
  //addr.Parse("tcp://localhost.localdomain:9091");
  RegionServer *regionserver = new RegionServer(reactor, addr, param);
  ThriftMemoryOutTransport* mem_trans = new ThriftMemoryOutTransport();
  assert(mem_trans!=NULL);
  shared_ptr<TTransport> out_transport(mem_trans);
  shared_ptr<TProtocol> oprot(new TBinaryProtocol(out_transport));
  HbaseClient client(oprot);
  std::vector<Mutation> mutations;
  mutations.push_back(Mutation());
  mutations.back().column = "body:img";
  mutations.back().value = "xv100";
  std::map<Text,Text> attributes;
  //client.send_get("picture", "1", "body:img", attributes);
  client.send_mutateRow("picture", "xk100", mutations, attributes);
  SharedPtr<OutMsg> outmsg = CreateOutMsg(mem_trans);
  regionserver->QueryWithExpire(outmsg, NULL, 1000000);
}
