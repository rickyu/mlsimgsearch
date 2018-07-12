#include "gearman_client.h"

GearmanClient::GearmanClient()
  : client_(NULL) {
}

GearmanClient::~GearmanClient() {
  if (client_) {
    gearman_client_free(client_);
    client_ = NULL;
  }
}

const int GearmanClient::Init() {
  client_ = gearman_client_create(NULL);
  if (!client_) {
    return CR_GEARMAN_INIT_FAILED;
  }

  return CR_SUCCESS;
}

const int GearmanClient::AddServer(const std::string& addr,
                                   const uint32_t port) {
  gearman_return_t ret = gearman_client_add_server(client_, addr.c_str(), port);
  if (gearman_failed(ret)) {
    return CR_GEARMAN_ADD_SERVER_FAILED;
  }

  return CR_SUCCESS;
}

const int GearmanClient::AddServers(const std::string& servers) {
  gearman_return_t ret = gearman_client_add_servers(client_, servers.c_str());
  if (gearman_failed(ret)) {
    return CR_GEARMAN_ADD_SERVER_FAILED;
  }

  return CR_SUCCESS;
}

const int GearmanClient::SetNonBlocking() {
  gearman_client_set_options(client_, GEARMAN_CLIENT_NON_BLOCKING);
  return CR_SUCCESS;
}

const int GearmanClient::AddTaskBackground(const void* context,
                                           const char* func_name,
                                           const char* unique,
                                           const void* workload,
                                           const size_t workload_size) {
  gearman_return_t ret;
  gearman_task_st *task;

  task = gearman_client_add_task_background(client_,
                                            NULL,
                                            (void*)context,
                                            func_name,
                                            unique,
                                            workload,
                                            workload_size,
                                            &ret);
  if (gearman_failed(ret)) {
    return CR_GEARMAN_ADD_TASK_FAILED;
  }

  return CR_SUCCESS;
}

const int GearmanClient::RunTasks() {
  gearman_return_t ret = gearman_client_run_tasks(client_);
  if (gearman_failed(ret)) {
    return CR_GEARMAN_RUN_TASK_FAILED;
  }

  gearman_client_task_free_all(client_);

  return CR_SUCCESS;
}
