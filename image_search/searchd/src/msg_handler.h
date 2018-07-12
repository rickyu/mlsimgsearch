#ifndef IMAGE_SEARCH_SEARCHD_MSG_HANDLER_H_
#define IMAGE_SEARCH_SEARCHD_MSG_HANDLER_H_

#include <iostream>
#include <ImageSearchMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <blitz/framework.h>
#include <blitz/line_msg.h>
#include "service_base.h"


class ImageSearchMsgHandler : virtual public ImageSearchMsgIf {
 public:
  ImageSearchMsgHandler();
  ~ImageSearchMsgHandler();

  int Init();

 private:
  void Search(ImageSearchResponse& _return, const ImageSearchRequest& request);

 private:
  //conf
  Configure *conf_;
};

#endif
