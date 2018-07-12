#ifndef __BLITZ_THREAD_DRIVER_H_
#define __BLITZ_THREAD_DRIVER_H_ 1
#include <blitz2/stdafx.h>
#include <blitz2/logger.h>
#include <blitz2/acceptor.h>
#include <blitz2/configure.h>
#include <blitz2/accept_poller.h>

namespace blitz2 
{

class DriverBase {
 public:
  typedef std::unordered_map<std::string,int> TAddrMap;
  int AddAcceptor(const std::string& addr) {
    if(acceptor_addrs_.find(addr) != acceptor_addrs_.end()) {
      return 1;
    }
    int tag = static_cast<int>(acceptor_infos_.size());
    acceptor_infos_.push_back(AcceptorInfo(addr,tag));
    acceptor_addrs_[addr] = tag;
    return 0;
  }
  void AddAcceptors(const std::vector<std::string>& addrs) {
    for (int i=0; i<static_cast<int>(addrs.size()); ++i) {
      AddAcceptor(addrs[i]);
    }
  }
  const std::vector<AcceptorInfo>& get_acceptor_infos() const {
    return acceptor_infos_;
  }
  virtual ~DriverBase() {}
 protected:
  TAddrMap acceptor_addrs_;
  std::vector<AcceptorInfo> acceptor_infos_;
};

/**
 * app will run in multi thread.
 * app must have functions:
 * class App {
 *  public:
 *   int Init(const Configuration& conf);
 *   Reactor& GetReactor();
 *   Acceptor::Observer* GetAcceptObserver(const std::string& addr);
 *   void Uninit();
 * };
 *
 */
template<typename App>
class WorkerThread;
template<typename App>
class ThreadDriver : public DriverBase
{
 public:
  typedef App TApp;
  typedef typename App::TConfigure TConfigure;
  typedef WorkerThread<App> TWorkerThread;
  typedef ThreadDriver<App> TDriver;
  ThreadDriver():logger_(Logger::GetLogger("blitz2::ThreadDriver")) {
    running_ = false;
    worker_selector_ = 0;
    log_rotate_hour_ = 0;
  }
  int Start(int worker_count, const TConfigure& conf);
  void OnConnectAccepted(Acceptor* acceptor, int fd, int tag) {
    (void)(acceptor);
    SelectIdleWorker()->PostAcceptedFd(fd, tag);
  }
  /// ctrl+c exit.
  void OnSigInt();
  const std::vector<TWorkerThread*>& workers() const {
    return workers_;
  }
 protected:
  int StartAcceptor();
  void StopAcceptor();
  int SetupSignal();
  void WaitWorkerExit();
  TWorkerThread* SelectIdleWorker() {
    return workers_[worker_selector_++%workers_.size()];
  }
 protected:
  Logger& logger_;
  int worker_selector_;
  std::vector<TWorkerThread*> workers_;
  bool running_;
  Reactor reactor_;
  int log_rotate_hour_;
  TConfigure conf_;
  AcceptPoller<mutex> acceptor_poller_;
};
template<typename App>
int ThreadDriver<App>::Start(int worker_count, const TConfigure& conf) {
  conf_ = conf;
  reactor_.Init();
  assert(worker_count>0);
  time_t now = ::time(NULL);
  struct tm the_time;
  localtime_r(&now, &the_time);
  log_rotate_hour_ = the_time.tm_hour;
  if (StartAcceptor() != 0) {
    cout << "fail to listen some port" << endl;
    return -1;
  }
  SetupSignal();
  workers_.resize(worker_count);
  for(size_t i=0; i<static_cast<size_t>(worker_count); ++i) {
    workers_[i] = new TWorkerThread(this, acceptor_infos_,
                                    &acceptor_poller_, conf_);
    workers_[i]->Start();
  }
  // init master and enter event loop
  reactor_.EventLoop();
  StopAcceptor();
  WaitWorkerExit();
  reactor_.Uninit();
  return 0;
}

template<typename App>
int ThreadDriver<App>::StartAcceptor() {
  auto it = acceptor_infos_.begin();
  auto end = acceptor_infos_.end();
  for(; it!=end; ++it) {
    int retry;
    for (retry=0; retry<3; ++retry) {
      if (acceptor_poller_.Bind(it->addr(), it->tag()) != 0) {
        LOG_ERROR(logger_, "bind fail, retry=%d addr=%s",
            retry, it->addr().c_str());
        continue;
      }
      break;
    }
    if (retry == 3) {
      return -1;
    }
  }
  return 0;
}
template<typename App>
void ThreadDriver<App>::StopAcceptor() {
  acceptor_poller_.Close();
}
template<typename App>
int ThreadDriver<App>::SetupSignal() {
  reactor_.Signal(SIGINT, std::bind(&TDriver::OnSigInt, this));
  return 0;
}
template<typename App>
void ThreadDriver<App>::OnSigInt() {
  reactor_.Stop();
}

template<typename App>
void ThreadDriver<App>::WaitWorkerExit() {
  for(auto it=workers_.begin(); it!=workers_.end(); ++it) {
    (*it)->Stop();
  }
  for(auto it=workers_.begin(); it!=workers_.end(); ++it) {
    (*it)->Wait(5);
    delete (*it);
  }
  workers_.clear();
}
/**
 * App will run in thread.
 */

template<typename App>
class WorkerThread {
 public:
  typedef App TApp;
  typedef typename App::TConfigure TConfigure;
  typedef WorkerThread<App> TWorkerThread;
  typedef ThreadDriver<App> TDriver;
  WorkerThread(TDriver* driver, const std::vector<AcceptorInfo>& acceptor_infos,
               AcceptPoller<mutex>* accept_poller, const TConfigure& conf) 
        : on_new_conn_func_(this) {
    driver_ = driver;
    thread_ = 0;
    running_ = false;
    acceptor_infos_ = acceptor_infos;
    acceptor_poller_ = accept_poller;
    conf_ = conf;
  }
  void Start();
  void Stop() {running_ = false; }
  void Wait(int seconds);
  void PostAcceptedFd(int fd, int tag);
  pthread_t GetHandle() const {
    return thread_;
  }
 private:
  static void* ThreadFunc(void* arg) {
    TWorkerThread* this_ptr = reinterpret_cast<TWorkerThread*>(arg);
    this_ptr->Run();
    return NULL;
  }
  void Run();
  void OnNewConn(int fd, int tag) {
    acceptor_infos_[tag].get_observer()->OnConnectAccepted(NULL, fd);
  }
 private:
  struct OnNewConnFunc {
    OnNewConnFunc(WorkerThread<App>* owner) {
      this->owner = owner;
    }
    int operator()(int fd, int tag) {
      owner->OnNewConn(fd, tag);
      return 0;
    }
    WorkerThread<App>* owner;
  };
  friend struct OnNewConnFunc;
 private:
  pthread_t thread_;
  TDriver* driver_;
  TConfigure conf_;
  bool running_;
  std::vector<AcceptorInfo> acceptor_infos_;
  AcceptPoller<mutex>* acceptor_poller_;
  OnNewConnFunc on_new_conn_func_;
};
template<typename App>
void WorkerThread<App>::Start() {
  pthread_create(&thread_, NULL, ThreadFunc, this);
  running_  = true;
}

template<typename App>
void WorkerThread<App>::Wait(int seconds) {
  (void)(seconds);
  pthread_join(thread_, NULL);
}
template<typename App>
void WorkerThread<App>::Run() {
  TApp app;
  app.Init(conf_);
  for(auto it=acceptor_infos_.begin(); it!=acceptor_infos_.end(); ++it) {
    it->set_observer(app.GetAcceptorObserver(it->addr()));
  }
  Reactor& reactor = app.GetReactor();
  while(running_) {
    acceptor_poller_->BatchAccept(on_new_conn_func_);
    reactor.HandleEvents();
    usleep(1);
  }
  for(auto it=acceptor_infos_.begin(); it!=acceptor_infos_.end(); ++it) {
    it->set_observer(NULL);
  }
  app.Uninit();
}

} // namespace blitz2
#endif // __BLITZ_THREAD_DRIVER_H_
