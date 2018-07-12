#include "merge_handler.h"
#include <boost/lexical_cast.hpp>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include "merge_processor.h"

extern CLogger& g_logger;

/////////////////////// start ExecMergeMsgHandler /////////////////////////////////
ExecMergeMsgHandler::ExecMergeMsgHandler() {
}

ExecMergeMsgHandler::~ExecMergeMsgHandler() {
}

int ExecMergeMsgHandler::Init() {
  return 0;
}

void ExecMergeMsgHandler::Merge(ExecMergeResponse& _return,
                               const ExecMergeRequest& request) {
  MergeProcessor mp;
  mp.Init();
  mp.Merge(_return, request);
}
