// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// IoObject. 
#ifndef __BLITZ2_IOOBJECT_H_
#define __BLITZ2_IOOBJECT_H_ 1
namespace blitz2 {

class IoEvent {
 public:
  static const int kEventMaskRead = 0x01;
  static const int kEventMaskWrite = 0x02;
  IoEvent() {event_ = 0;}
  explicit IoEvent(int  event) { event_= event; }
  void setRead() { event_ |= kEventMaskRead; }
  void setWrite() { event_ |= kEventMaskWrite; }
  bool haveRead() const { return event_ & kEventMaskRead;}
  bool haveWrite() const { return event_ & kEventMaskWrite;}
  int event() const { return event_;}
 private:
  int event_;
};
/**
 * base class for io object.
 */
class IoObject {
 public:
  IoObject() {fd_=-1;}
  virtual void ProcessEvent(const IoEvent& event) = 0;
  int fd() {
    return fd_;
  }
 protected:
  int fd_;
};

} // namespace blitz2.
#endif // __BLITZ2_IOOBJECT_H_
