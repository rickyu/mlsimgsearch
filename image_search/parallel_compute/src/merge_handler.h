#ifndef IMAGE_SEARCH_SEARCHD_EXEC_MERGE_MSG_HANDLER_H_
#define IMAGE_SEARCH_SEARCHD_EXEC_MERGE_MSG_HANDLER_H_

#include <iostream>
#include <ExecMergeMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <blitz/framework.h>
#include <blitz/line_msg.h>
#include <common/configure.h>

class ExecMergeMsgHandler : virtual public ExecMergeMsgIf {
 public:
  ExecMergeMsgHandler();
  ~ExecMergeMsgHandler();

  int Init();

 private:
  void Merge(ExecMergeResponse& _return, const ExecMergeRequest& request);

 private:
  Configure *conf_;
};

#endif
