// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// AcceptorManager. 
#ifndef __BLITZ2__ACCEPT_POLLER_H_
#define __BLITZ2__ACCEPT_POLLER_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/acceptor.h>
#include <blitz2/logger.h>
namespace blitz2 {
class AcceptorInfo {
 public:
  AcceptorInfo() {
    observer_ = NULL;
    tag_ = -1;
  }
  AcceptorInfo(const std::string& addr, int tag)
              :tag_(tag), addr_(addr) {
    observer_ = NULL;
  }
  const std::string& addr() const { return addr_; }
  std::string& addr() { return addr_; }
  int tag() const { return tag_;}
  int& tag() { return tag_;}
  Acceptor::Observer* get_observer() { return observer_; }
  void set_observer(Acceptor::Observer* observer) {observer_ = observer; }
 protected:
  int tag_;
  std::string addr_;
  Acceptor::Observer* observer_;
};

template<typename Mutex>
class AcceptPoller {
 public:
  AcceptPoller() 
    :logger_(Logger::GetLogger("blitz2::Acceptor")) {
  }
  ~AcceptPoller() {
    Close();
  }

  int Init();
  int Bind(const string& addr, int tag);
  void TryAccept(int* fd, int* tag);
  template<typename NewConnFunc>
  void BatchAccept(NewConnFunc func);
  void Close();
 private:
  void Rebind(int index);
 private:
  Mutex mutex_;
  vector<pollfd> fds_;
  vector<AcceptorInfo> acceptor_infos_;
  Logger& logger_;
};
template<typename Mutex>
int AcceptPoller<Mutex>::Init() {
  return 0;
}
template<typename Mutex>
int AcceptPoller<Mutex>::Bind(const string& addr, int tag) {
  IoAddr ioaddr;
  if (ioaddr.Parse(addr) != 0) {
    return -1;
  }
  int fd = Acceptor::BindAndListen(ioaddr);
  if (fd<0) {
    return  -1;
  }
  struct pollfd pollfd;
  pollfd.fd = fd;
  pollfd.events = POLLIN;
  pollfd.revents = 0;
  fds_.push_back(pollfd);
  acceptor_infos_.push_back(AcceptorInfo(addr, tag));
  return 0;
}
template<typename Mutex>
template<typename NewConnFunc>
void AcceptPoller<Mutex>::BatchAccept(NewConnFunc func) {
  int count = static_cast<int>(fds_.size());
  if (count == 0) {
    return ;
  }
  {
    typename Mutex::scoped_try_lock lock(mutex_);
    if (!lock) {
      return ;
    }
    struct pollfd* fds = &fds_[0];
    int ret = poll(fds, count, 0);
    if (ret < 0) {
      LOG_ERROR(logger_, "poll errno=%d", errno);
      // error
    } else if (ret == 0) {
      LOG_DEBUG(logger_, "poll no_event");
    } else {
      for (int i=0; i<ret; ++i) {
        if (fds[i].revents == 0) {
          continue;
        }
        bool end_loop = false;
        int tag = acceptor_infos_[i].tag();
        if (fds[i].revents & POLLIN) {
          int new_fd = -1;
          do {
            new_fd = ::accept(fds[i].fd, NULL, 0);
            if (new_fd > 0) {
              if (func(new_fd, tag) != 0) {
                end_loop = true;
                break;
              }
            } else  {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
              } else if (errno == EINTR) {
                new_fd = 0;
                continue;
              } else {
                LOG_ERROR(logger_, "accept errno=%d", errno);
                Rebind(i);
                break;
              }
            }
            break;
          } while (new_fd != -1);
        }
        if (fds[i].revents & POLLERR) {
          Rebind(i);
        }
        if (end_loop) {
          break;
        }
      }
    }
  }
}
template<typename Mutex>
void AcceptPoller<Mutex>::TryAccept(int* fd, int* tag) {
  struct NewConnFunctor {
    NewConnFunctor(int *fd, int *tag) {
      this->fd = fd;
      this->tag = tag;
    }
    int operator()(int new_fd, int new_tag) {
      *fd = new_fd;
      *tag = new_tag;
      return 1;
    }
    int *fd;
    int *tag;
  };
  BatchAccept(NewConnFunctor(fd, tag));
}
template<typename Mutex>
void AcceptPoller<Mutex>::Rebind(int index) {
  ::close(fds_[index].fd);
  fds_[index].fd = -1;
  int fd = Bind(acceptor_infos_[index].addr(), acceptor_infos_[index].tag());
  if (fd < 0) {
    return;
  }
  fds_[index].fd = fd;
}
template<typename Mutex>
void AcceptPoller<Mutex>::Close() {
  for (auto it=fds_.begin(); it!=fds_.end(); ++it) {
    ::close(it->fd);
  }
  fds_.clear();
  acceptor_infos_.clear();
}

} // namespace blitz2.
#endif // __BLITZ2__ACCEPT_POLLER_H_
