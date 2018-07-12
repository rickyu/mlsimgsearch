#include "gearman_worker.h"
#include <boost/lexical_cast.hpp>

CLogger& GearmanWorker::logger_ = CLogger::GetInstance("gearman");

GearmanWorker::GearmanWorker()
  : worker_(NULL)
  , function_name_("") {
}

GearmanWorker::~GearmanWorker() {
  gearman_job_st *job = NULL;
  while ((job = const_cast<gearman_job_st*>(PopJob()))) {
    FreeJob(job);
  }
  // if (!function_name_.empty()) {
  //   gearman_worker_unregister(worker_, function_name_.c_str());
  // }
  if (worker_) {
    gearman_worker_free(worker_);
    worker_ = NULL;
  }
}

const int GearmanWorker::Init() {
  worker_ = gearman_worker_create(NULL);
  if (NULL == worker_) {
    return CR_GEARMAN_CREATE_WORKER_FAILED;
  }
  return CR_SUCCESS;
}

const int GearmanWorker::AddServer(const std::string &addr,
                                   const uint32_t port) {
  gearman_return_t gret = gearman_worker_add_server(worker_, addr.c_str(), port);
  if (gearman_failed(gret)) {
    LOGGER_ERROR(logger_, "gearman_worker_add_server failed(%s:%d,ret=%d)",
                 addr.c_str(), port, gret);
    return CR_GEARMAN_ADD_SERVER_FAILED;
  }
  return CR_SUCCESS;
}

const int GearmanWorker::AddServers(const std::string &addr) {
  gearman_return_t gret = gearman_worker_add_servers(worker_, addr.c_str());
  if (gearman_failed(gret)) {
    LOGGER_ERROR(logger_, "gearman_worker_add_servers failed(%s, ret=%d)",
                 addr.c_str(), gret);
    return CR_GEARMAN_ADD_SERVER_FAILED;
  }
  return CR_SUCCESS;
}

const int GearmanWorker::SetGrabUnique() {
  gearman_worker_set_options(worker_, GEARMAN_WORKER_GRAB_UNIQ);
  return CR_SUCCESS;
}

const int GearmanWorker::SetTimeoutReturn() {
  gearman_worker_set_options(worker_, GEARMAN_WORKER_TIMEOUT_RETURN);
  return CR_SUCCESS;
}

void GearmanWorker::SetIOTimeout(const int ms_timeout /*= -1*/) {
  gearman_worker_set_timeout(worker_, ms_timeout);
}

const int GearmanWorker::RegisterFunction(const std::string &func_name,
                                          uint32_t timeout /*=0*/) {
  gearman_return_t gret = gearman_worker_register(worker_,
                                                  func_name.c_str(),
                                                  timeout);
  if (gearman_failed(gret)) {
    LOGGER_ERROR(logger_, "gearman_worker_register failed, ret=%d", gret);
    return CR_GEARMAN_ADD_FUNCTION_FAILED;
  }
  function_name_ = func_name;
  return CR_SUCCESS;
}

const int GearmanWorker::GrabJobs(int num) {
  gearman_job_st *job = NULL;
  gearman_return_t gret;
  for (int i = 0; i < num; i++) {
    job = gearman_worker_grab_job(worker_, NULL, &gret);
    if (gearman_failed(gret)) {
      LOGGER_ERROR(logger_, "gearman_worker_grab_job failed, ret=%d", gret);
      return CR_GEARMAN_GRAB_JOB_FAILED;
    }
    PushJob(job);
  }
  return CR_SUCCESS;
}

const gearman_job_st* GearmanWorker::GrabJob() {
  gearman_job_st *job = NULL;
  gearman_return_t gret;
  try {
    gearman_job_free(worker_->work_job);
    worker_->work_job = NULL;
    job = gearman_worker_grab_job(worker_, NULL, &gret);
    if (gearman_failed(gret) || NULL == job) {
      LOGGER_ERROR(logger_, "gearman_worker_grab_job failed, ret=%d", gret);
      return NULL;
    }
    return job;
  }
  catch (const std::exception &e) {
    LOGGER_ERROR(logger_, "catch a std::exception: %s!", e.what());
  }
  catch (...) {
    LOGGER_ERROR(logger_, "catch a unknown exception!");
  }
  return NULL;
}

const size_t GearmanWorker::GetJobQueueSize() {
  UniqueLock<Mutex> lock(mutex_);
  return jobs_.size();
}

const gearman_job_st* GearmanWorker::PopJob() {
  UniqueLock<Mutex> lock(mutex_);
  gearman_job_st *job = NULL;
  if (jobs_.empty())
    return NULL;
  job = jobs_.front();
  jobs_.pop();
  return job;
}

const int GearmanWorker::SendJobResponse(const gearman_job_st *job,
                                         const bool succ,
                                         const void *result /*= NULL*/,
                                         const size_t result_size /*= 0*/) {
  gearman_return_t ret;
  if (succ) {
    ret = gearman_job_send_complete(const_cast<gearman_job_st*>(job),
                                    result,
                                    result_size);
  }
  else {
    ret = gearman_job_send_fail(const_cast<gearman_job_st*>(job));
  }
  if (gearman_failed(ret)) {
    LOGGER_ERROR(logger_, "Gearman send job response failed, ret=%d", ret);
    return CR_GEARMAN_SEND_RESPONSE_FAILED;
  }
  return CR_SUCCESS;
}

void GearmanWorker::PushJob(const gearman_job_st *job) {
  UniqueLock<Mutex> lock(mutex_);
  jobs_.push(const_cast<gearman_job_st*>(job));
}

void GearmanWorker::FreeJob(gearman_job_st *&job) {
  if (!job)
    return;
  gearman_job_free(job);
  job = NULL;
}

void GearmanWorker::GetJobUniqueIDString(const gearman_job_st *job,
                                         std::string &id) {
  id.assign(gearman_job_unique(job));
}

int GearmanWorker::GetJobUniqueIDInt(const gearman_job_st *job) {
  try {
    return boost::lexical_cast<int>(gearman_job_unique(job));
  }
  catch (boost::bad_lexical_cast& e) {
    LOGGER_ERROR(logger_, "act=GearmanWorker::GetJobUniqueIDInt failed");
    return -1;
  }
}

int64_t GearmanWorker::GetJobUniqueIDInt64(const gearman_job_st *job) {
  try {
   return boost::lexical_cast<int64_t>(gearman_job_unique(job));
  }
  catch (boost::bad_lexical_cast& e) {
    LOGGER_ERROR(logger_, "act=GearmanWorker::GetJobUniqueIDInt64 failed");
    return -1;
  }
}

long GearmanWorker::GetJobUniqueIDLong(const gearman_job_st *job) {
  try {
    return boost::lexical_cast<long>(gearman_job_unique(job));
  }
  catch (boost::bad_lexical_cast& e) {
    LOGGER_ERROR(logger_, "act=GearmanWorker::GetJobUniqueIDLong failed");
    return -1L;
  }
}

void GearmanWorker::GetJobWorkloadString(const gearman_job_st *job,
                                         std::string &work_load) {
  void *wl;
  size_t wl_size;
  GetJobWorkload(job, wl, wl_size);
  work_load.assign(static_cast<char*>(wl), wl_size);
}

void GearmanWorker::GetJobWorkload(const gearman_job_st *job,
                                   void *&wl,
                                   size_t &wl_size) {
  wl_size = gearman_job_workload_size(job);
  wl = const_cast<void*>(gearman_job_workload(job));
}

const int GearmanWorker::AddFunction(const std::string &func_name,
                                     const gearman_worker_callback_fn callback,
                                     const int timeout,
                                     void *context) {
  gearman_return_t ret = gearman_worker_add_function(worker_,
                                                     func_name.c_str(),
                                                     timeout,
                                                     callback,
                                                     context);
  if (ret != GEARMAN_SUCCESS) { 
    LOGGER_ERROR(logger_, "act=GearmanWorker::AddFunction failed, ret=%d, msg=%s",
                 ret, gearman_worker_error(worker_));
    return CR_GEARMAN_ADD_FUNCTION_FAILED;
  }
  return CR_SUCCESS;
}

void GearmanWorker::Quit() {
  quit_ = true;
}

void GearmanWorker::Start() {
  gearman_return_t ret = GEARMAN_SUCCESS;
  while (!quit_) {
    ret = gearman_worker_work(worker_);
    if (ret != GEARMAN_SUCCESS) {
      LOGGER_ERROR(logger_, "act=gearman_worker_work failed, ret=%d, msg=%s",
                   ret, gearman_worker_error(worker_));
    }
  }
  LOGGER_ERROR(logger_, "GearmanWorker quit now");
}
