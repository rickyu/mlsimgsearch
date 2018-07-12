// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// reactor. 
#ifndef __BLITZ2_REACTOR_H_
#define __BLITZ2_REACTOR_H_ 1
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <functional>
#include <event.h>
#include <blitz2/ioobject.h>
namespace blitz2 
{
class Reactor {
 public:
  typedef std::function<void(uint32_t)> TTimerCallback;
  typedef std::function<void()> TTask;
  typedef std::function<void()> TSignalCallback;
  Reactor() {
    running_ = false;
    timer_id_ = 0;
    event_base_ = NULL;
  }
  ~Reactor() {
    Uninit();
  }
  int Init();
  void Uninit();
  void RemoveAllEventSource();
  void PostTask(const TTask& task) {tasks_.push(task);};
  int RegisterIoObject(IoObject* io_obj);
  int RemoveIoObject(IoObject* io_obj);
  /**
   * @return timerid, 0 means fail.
   */
  uint32_t  AddTimer(uint32_t timeout, 
      const TTimerCallback& callback,
      bool repeat);
  int RemoveTimer(uint32_t timer_id);
  int Signal(int sig, const TSignalCallback& cb);
  int HandleEvents();
  void EventLoop(); 
  void Stop() {
    running_ = false;
  }
  void AfterFork();
  bool running() const { return running_; }
 protected:
  class IoObjectWrap {
   public:
    IoObjectWrap() {Clear();}
    IoObject* io_obj() { return io_obj_;}
    void SetIoObj(IoObject* obj) { io_obj_ = obj; }
    struct event* event() { return &event_; }
    void Clear() { io_obj_ = NULL; memset(&event_, 0, sizeof(event_));}
   private:
    IoObject* io_obj_;
    struct event event_;

  };
  class TimerWrap {
   public:
    enum {
      FLAG_REMOVED = 0x01,
      FLAG_REPEAT = 0x02
    };
    TimerWrap(uint32_t id, const TTimerCallback& callback,
        Reactor* reactor, uint32_t timeout, bool repeat) {
      id_ = id;
      callback_ = callback;
      reactor_ = reactor;
      tv_.tv_sec = timeout /1000;
      tv_.tv_usec = (timeout % 1000) * 1000;
      flags_ = 0;
      if(repeat) {
        flags_ |= FLAG_REPEAT;
      }
    }
    uint32_t id() const { return id_;}
    struct event* event() { return &event_;}
    Reactor* reactor() { return reactor_;}
    timeval* tv() { return &tv_;}
    bool IsRemoved() const { return flags_ & FLAG_REMOVED ;}
    void SetRemoved() { flags_ |= FLAG_REMOVED;}
    bool NeedRepeat() const { return flags_ & FLAG_REPEAT;}
    void Callback() { callback_(id_);}
   private:
    uint32_t id_;
    TTimerCallback callback_;
    struct event event_;
    Reactor* reactor_;
    uint32_t flags_;
    timeval tv_;
  };
  typedef std::unordered_map<uint32_t, TimerWrap*> TimerMap;
  class SignalWrap {
   public:
    SignalWrap(int sig, const TSignalCallback& callback) 
      :sig_(sig), callback_(callback) {

    }
    struct event* event() { return &event_;}
    void callback() { callback_(); }
   private:
    int sig_;
    TSignalCallback callback_;
    struct event event_;
  };
  typedef std::unordered_map<int, SignalWrap*> TSignalMap;
 private:
  static void HandleIoEvent(int fd, short event, void* arg);
  static void HandleTimerEvent(int fd, short event, void* arg);
  static void HandleSignalEvent(int fd, short event, void* arg);
 private:
  struct event_base* event_base_;
  std::vector<IoObjectWrap> io_objs_;
  TimerMap timers_;
  uint32_t timer_id_;
  bool running_;
  std::queue<TTask> tasks_;
  TSignalMap signals_;
};

} // namespace blitz2
#endif // __BLITZ2_REACTOR_H_
