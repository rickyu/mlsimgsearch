#include "msg_handler.h"
#include <boost/lexical_cast.hpp>
#include <utils/logger.h>
#include <utils/logger_loader.h>

extern CLogger& g_logger;

/////////////////////// start ImageSearchMsgHandler /////////////////////////////////
ImageSearchMsgHandler::ImageSearchMsgHandler() {
}

ImageSearchMsgHandler::~ImageSearchMsgHandler() {
}

int ImageSearchMsgHandler::Init() {
  return 0;
}

void ImageSearchMsgHandler::Search(ImageSearchResponse& _return,
                                   const ImageSearchRequest& request) {
  SearchServiceBase *service =
    SearchServiceDispatcher::GetInstance()->GetService(request.type);
  int ret = 0;
  std::string result;
  if (service) {
    LOGGER_LOG(g_logger, "act=ImageSearchMsgHandler::ServiceProcess, type:%d, key:%s, workload:%s",
               request.type, request.key.c_str(), request.workload.c_str());
    ret = service->Exec(request, result);
    if (0 == ret) {
      if (0 == ret) {
        _return.return_code = ret;
        _return.img_data.assign(result.c_str());
        _return.img_data_len = result.length();
        SearchServiceDispatcher::GetInstance()->ReleaseService(service);
        LOGGER_LOG(g_logger, "act=ImageSearchMsgHandler::GetResult Success data_len= %d",
                   _return.img_data_len);
        return;
      }
    }
  }
  else {
    ret = -2;
  }
  _return.return_code = ret;
  _return.img_data_len = -1;
  SearchServiceDispatcher::GetInstance()->ReleaseService(service);
}
