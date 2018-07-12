#ifndef IMAGE_SEARCH_SEARCHD_MSG_HANDLER_H_
#define IMAGE_SEARCH_SEARCHD_MSG_HANDLER_H_

#include <iostream>
#include <SpatialRankerMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <blitz/framework.h>
#include <blitz/line_msg.h>
#include <common/configure.h>

class SpatialRankerMsgHandler : virtual public SpatialRankerMsgIf {
 public:
  SpatialRankerMsgHandler();
  ~SpatialRankerMsgHandler();

  int Init();

 private:
  void Rank(SpatialRankerResponse& _return, const SpatialRankerRequest& request);

 private:
  //conf
  Configure *conf_;
};

#endif
