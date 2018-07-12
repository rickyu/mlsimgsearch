#include "service_base.h"
#include <common/http_client.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>

extern CLogger& g_logger;

SearchServiceBase::SearchServiceBase() {
}

SearchServiceBase::~SearchServiceBase() {
}

int SearchServiceBase::Exec(const ImageSearchRequest &request, std::string &result) {
  return Process(request, result);
}


///////////////////////////// SearchServiceDispatcher ////////////////////////////////
SearchServiceDispatcher* SearchServiceDispatcher::instance_ = NULL;

SearchServiceDispatcher::SearchServiceDispatcher() {
}

int SearchServiceDispatcher::Init() {
  return 0;
}

SearchServiceDispatcher::~SearchServiceDispatcher() {
}

SearchServiceDispatcher* SearchServiceDispatcher::GetInstance() {
  if (NULL == instance_) {
    instance_ = new SearchServiceDispatcher();
  }
  return instance_;
}

void SearchServiceDispatcher::Release() {
  if (instance_) {
    delete instance_;
    instance_ = NULL;
  }
}

SearchServiceBase* SearchServiceDispatcher::GetService(int type) {
  SearchServiceMember::iterator it = reg_members_.find(type);
  if (reg_members_.end() == it) {
    return NULL;
  }
  SearchServiceBase *service = it->second;
  if (service) {
    return service->Clone();
  }
  return NULL;
}

void SearchServiceDispatcher::ReleaseService(SearchServiceBase *&service) {
  if (service) {
    service->Clear();
    LOGGER_INFO(g_logger, "SearchServiceDispatcher release service success");
    delete service;
    service = NULL;
  }
}
