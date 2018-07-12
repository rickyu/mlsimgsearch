#include "ranker_handler.h"
#include <boost/lexical_cast.hpp>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include "ranker_processor.h"

extern CLogger& g_logger;

/////////////////////// start SpatialRankerMsgHandler /////////////////////////////////
SpatialRankerMsgHandler::SpatialRankerMsgHandler() {
}

SpatialRankerMsgHandler::~SpatialRankerMsgHandler() {
}

int SpatialRankerMsgHandler::Init() {
  return 0;
}

void SpatialRankerMsgHandler::Rank(SpatialRankerResponse& _return,
                                   const SpatialRankerRequest& request) {
  RankerProcessor rp;
  rp.Init();
  rp.Rank(_return, request);
}
