#include <TApplicationException.h>
// #include <utils/time_util.h>
#include <blitz2/logger.h>
#include <blitz2/thrift.h>
namespace blitz2 
{
void ThriftProcessor::operator()(ThriftFramedMsgDecoder::TMsg* msg, 
    SharedPtr<TSharedChannel> channel) {
  using boost::shared_ptr;
  using apache::thrift::protocol::TBinaryProtocol;
  uint8_t* data = msg->data();
  uint32_t length = *((uint32_t*)data);
  shared_ptr<ThriftMemoryInTransport> in_transport(
      new ThriftMemoryInTransport(data + 4, length));
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
    SendReply(channel, x, fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    return ;
  }
  THandler* handler = GetHandler(fname.c_str());
  if (handler) {
    (*handler)(iprot.get(), seqid, channel);
  } else {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(
        ::apache::thrift::TApplicationException::UNKNOWN_METHOD,
        "Invalid method name: '"+fname+"'");
    SendReply(channel, x, fname,
              ::apache::thrift::protocol::T_EXCEPTION, seqid);
  }
}

} // namespace blitz2
