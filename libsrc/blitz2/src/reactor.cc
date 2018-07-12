#include <blitz2/reactor.h>
namespace blitz2 {
int Reactor::Init() {
  if(event_base_) {
    return 1;
  }
  event_base_ = event_base_new();
  io_objs_.resize(20000);
  return 0;
}
void Reactor::Uninit() {
  RemoveAllEventSource();
  if(event_base_) {
    event_base_free(event_base_);
    event_base_ = NULL;
  }
  io_objs_.clear();
}
void Reactor::RemoveAllEventSource() {
  for(auto iter=timers_.begin(); iter!=timers_.end(); ++iter) {
    event_del(iter->second->event());
    delete iter->second;
  }
  timers_.clear();
  timer_id_ = 0;
  for(auto iter=signals_.begin(); iter!=signals_.end(); ++iter) {
    event_del(iter->second->event());
    delete iter->second;
  }
  signals_.clear();
  for(size_t i=0; i<io_objs_.size(); ++i) {
    if(io_objs_[i].io_obj()) {
      event_del(io_objs_[i].event());
      io_objs_[i].Clear();
    }
  }
  while(!tasks_.empty()) {
    tasks_.pop();
  }
}
int Reactor::RegisterIoObject(IoObject* io_obj) {
  if(!io_obj) { return -1;}
  IoObjectWrap* wrap = &io_objs_[io_obj->fd()];
  wrap->SetIoObj(io_obj);
  event_assign(wrap->event(), event_base_, 
      io_obj->fd(), EV_READ|EV_WRITE|EV_PERSIST|EV_ET,
      HandleIoEvent, this);
  event_add(wrap->event(), NULL);
  return 0;
}
int Reactor::RemoveIoObject(IoObject* io_obj) {
  if(!io_obj) { return -1; }
  IoObjectWrap* wrap = &io_objs_[io_obj->fd()];
  if(wrap->io_obj() != io_obj) {
    return -1;
  }
  event_del(wrap->event());
  wrap->Clear();
  return 0;
}
uint32_t Reactor::AddTimer(uint32_t timeout, const TTimerCallback& callback,
    bool need_repeat) {
  TimerWrap* wrap = new TimerWrap(++timer_id_, callback, this,
                           timeout, need_repeat);
  event_assign(wrap->event(), event_base_, -1, 0, HandleTimerEvent, wrap);
  timers_[wrap->id()] = wrap;
  event_add(wrap->event(), wrap->tv());
  return wrap->id();
}
int Reactor::RemoveTimer(uint32_t timer_id) {
  TimerMap::iterator it = timers_.find(timer_id);
  if(it == timers_.end()) {
    return 1;
  }
  it->second->SetRemoved();
  return 0;
}

void Reactor::HandleIoEvent(int fd, short event, void* arg) {
  (void)(arg);
  Reactor* reactor = reinterpret_cast<Reactor*>(arg);
  IoObjectWrap* wrap = &reactor->io_objs_[fd];
  IoObject* io_obj = wrap->io_obj();
  IoEvent ioevent;
  if(event &  EV_READ) {
    ioevent.setRead();
  }
  if(event &  EV_WRITE) {
    ioevent.setWrite(); 
  } 
  io_obj->ProcessEvent(ioevent);
}
void Reactor::HandleTimerEvent(int fd, short event, void* arg) {
  (void)(fd);
  (void)(event);
  TimerWrap* wrap = reinterpret_cast<TimerWrap*>(arg);
  Reactor* reactor = wrap->reactor();
  if(wrap->IsRemoved()) {
    // timer have been removed, we shouldn't call callback function.
    reactor->timers_.erase(wrap->id());
    delete wrap;
    return;
  }
  // invoke callback
  wrap->Callback();
  if(wrap->NeedRepeat()) {
    event_assign(wrap->event(), reactor->event_base_, -1, 0,
        HandleTimerEvent, wrap);
    event_add(wrap->event(), wrap->tv());
  } else {
    reactor->timers_.erase(wrap->id());
    delete wrap;
  }
}
int Reactor::Signal(int sig, const TSignalCallback& cb) {
  (void)(cb);
  (void)(sig);
  auto iter = signals_.find(sig);
  if(iter!=signals_.end()) {
    return 1;
  }
  SignalWrap* wrap = new SignalWrap(sig, cb);
  event_assign(wrap->event(), event_base_, sig, EV_SIGNAL|EV_PERSIST,
      HandleSignalEvent, wrap);
  event_add(wrap->event(), NULL);
  signals_[sig] = wrap;
  return 0;
}
void Reactor::HandleSignalEvent(int fd, short event, void* arg) {
  (void)(fd);
  (void)(event);
  SignalWrap* wrap = reinterpret_cast<SignalWrap*>(arg);
  wrap->callback();
}

int Reactor::HandleEvents() {
  event_base_loop(event_base_, EVLOOP_NONBLOCK);
  while(!tasks_.empty()) {
    TTask& task = tasks_.front();
    task();
    tasks_.pop();
  }
  return 0;
}
void Reactor::EventLoop() {
  if(running_) {
    return;
  }
  running_ = true;
  while(running_) {
    HandleEvents();
    usleep(1000);
  }
}
void Reactor::AfterFork() {
  event_reinit(event_base_);
}

} // namespace blitz2.
