#include <event2/event.h>
#include "utils/logger.h"
#include "blitz/fd_manager.h"
namespace blitz {
static CLogger& g_log_framework = CLogger::GetInstance("framework");
int FdManager::Init(int max_count) {
  assert(items_ == NULL);
  if (items_ != NULL) { return 1; }
  max_count_ = max_count;
  items_ =  new Item[max_count_];
  for (int i = 0; i < max_count_; ++i) {
    items_[i].next_salt = 0;
    items_[i].in_use = false;
    memset(&items_[i].descriptor,0,sizeof(FdDescriptor));
  }
  return 0;
}

int FdManager::Uninit() {
  if (!items_) { return 0; }
  for (int fd = 0; fd < max_count_; ++fd) {
    Item& item = items_[fd];
    FdDescriptor& descriptor = item.descriptor;
    if (descriptor.handler && descriptor.fn_free) {
      LOGGER_DEBUG(g_log_framework, "uninit fd=%d", fd);
      descriptor.fn_free(descriptor.handler, descriptor.free_ctx);
    }
    if (descriptor.ev) {
      event_del(descriptor.ev);
      event_free(descriptor.ev);
    }
    memset(&descriptor, 0, sizeof(FdDescriptor));
  }
  delete []items_;
  items_ = NULL;
  max_count_ = 0;
  // count_ = 0;
  return 0;
}
} // namespace blitz.
 
