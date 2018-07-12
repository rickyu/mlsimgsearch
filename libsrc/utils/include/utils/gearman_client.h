#ifndef UTILS_GEARMAN_CLIENT_H_
#define UTILS_GEARMAN_CLIENT_H_

#include <string>
#include <libgearman/gearman.h>
#include "constant.h"

class GearmanClient {
 public:
  GearmanClient();
  ~GearmanClient();

  const int Init();
  const int AddServer(const std::string& addr,
                      const uint32_t port);
  // The format for this is SERVER[:PORT][,SERVER[:PORT]]...
  const int AddServers(const std::string& servers);
  const int SetNonBlocking();
  const int AddTaskBackground(const void* context,
                              const char* func_name,
                              const char* unique,
                              const void* workload,
                              const size_t workload_size);
  const int RunTasks();

 private:
  gearman_client_st* client_;
};

#endif
