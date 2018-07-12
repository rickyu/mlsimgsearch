#ifndef __BLITZ_MULTIPROCESS_H_
#define __BLITZ_MULTIPROCESS_H_ 1
#include <sys/types.h>
#include <sys/wait.h>
#include <blitz2/blitz2.h>
namespace blitz2
{
class AcceptorManager:public Acceptor::Observer
{
 public:
  void SetObserver(int type, Acceptor::Observer* ob) {
    observers_[type] = ob;
  }
  void OnFdRecved(ControlChannel* , int fd ,int32_t type) {
    auto iter = observers_.find(type);
    Acceptor::Observer* ob =  NULL;
    if(iter != observers_.end()) {
      ob = iter->second;
    }
    if(ob) {
      ob->OnConnectAccepted(NULL, fd);
    } else {
      ::close(fd);
    }
  }
 private:
  typedef std::unordered_map<int,Acceptor::Observer*> TAcceptorObserverMap;
  TAcceptorObserverMap observers_;
};
/**
 * 
 */
class Process 
{
 public:
  void OnControlChannelClosed(ControlChannel* channel, int reason) {
    (void)(reason);
    if(channel == control_channel_) {
      control_channel_ = NULL;
    }
    delete channel;
  }
  Process(Reactor& reactor, Logger& log)
    :reactor_(reactor), logger_(log) {
    pid_ = -1;
    control_channel_ = NULL;
  }
  ~Process() {
    if(control_channel_) {
      delete control_channel_;
      control_channel_ = NULL;
    }
  }
  pid_t pid() const { return pid_; }
  void SetPid(pid_t pid) { pid_ = pid; }
  void SetControlPipeFd(int pipefd) {
    control_channel_ = new ControlChannel(reactor_, logger_);
    control_channel_->Init(pipefd);
    control_channel_->SetClosedCallback(
        std::bind(&Process::OnControlChannelClosed, this, std::placeholders::_1, std::placeholders::_2));
  }
  void Clear() {
    if(control_channel_) {
      // control_channel_->ResetAllCallback();
      control_channel_->ForceClose();
      delete control_channel_;
      control_channel_ = NULL;
    }
    pid_ = -1;
  }
  int Wait(bool hang=false) {
    int ret =::waitpid(pid_, NULL, hang?0:WNOHANG);
    if(ret == 0) {
      return 0;
    } else if(ret==pid_) {
      return 1;
    } else {
      return -1;
    }
  }
  
  int Kill(int sig) {
    return ::kill(pid_, sig);
  }
  void SendFd(int fd_to_send, int type) {
    control_channel_->SendFd(fd_to_send, type);
  }
 protected:
  Reactor& reactor_;
  Logger& logger_;
  ControlChannel* control_channel_;
  pid_t pid_;
};

class AcceptorObserverCreator {
 public:
  virtual Acceptor::Observer* CreateObserver() = 0;
  virtual void DestroyObserver(Acceptor::Observer* observer) = 0;

};

template<typename App>
class ProcessDriver : public DriverBase
{
 public:
  typedef App TApp;
  typedef ProcessDriver<App> TDriver;
  typedef std::unordered_map<std::string,AcceptorObserverCreator*> TAddrMap;
  enum EnumRole {
    kRoleMaster = 1,
    kRoleWorker = 2
  };
  ProcessDriver():logger_(logger_manager_.GetLogger("framework")) {
    process_idx_ = 0;
    exiting_ = false;
    role_ = kRoleMaster;
    worker_forking_ = false;
    master_pipe_ = -1;
    log_rotate_hour_ = -1;
  }
  int Start(int worker_count, const Configure& config);
  ~ProcessModel() {
    Uninit();
  }
  int Init() {
    return 0;
  }
  void Uninit();
  int AddAcceptor(const std::string& addr, AcceptorObserverCreator* creator) {
    if(addrs_.find(addr) != addrs_.end()) {
      return 1;
    }
    addrs_[addr] = creator;
    return 0;
  }

  LoggerManager* logger_manager() { return &logger_manager_;}
 protected:
  class AcceptorForwarder:public Acceptor::Observer
  {
   public:
    AcceptorForwarder(Reactor& reactor, TMyself* model,  Logger& log, int type)
      :logger_(log), acceptor_(reactor, log) {
        type_ = type;
        acceptor_.SetObserver(this);
        process_model_ = model;
      }
    int Bind(const std::string& addr) {
      return acceptor_.Bind(addr);
    }
    void Close() {
      acceptor_.SetObserver(NULL);
      acceptor_.Close();
    }
    virtual void OnConnectAccepted(Acceptor* acceptor, int fd) {
      //Forward to worker process/thread.
      LOGGER_DEBUG(logger_, "GotOneFd fd=%d type=%d", fd, type_);
      process_model_->ForwardFd(fd, type_);
    }
    virtual void OnClosed(Acceptor* acceptor) { 
      acceptor_.Bind();
    }
   protected:
    Logger& logger_;
    int type_;
    Acceptor acceptor_;
    TMyself* process_model_;
  };

  void StartWorker(size_t i ) {
    int sockfd[2];
    socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
    pid_t pid = fork();
    if(pid>0) {
      // parent
      ::close(sockfd[1]);
      worker_processes_[i]->SetControlPipeFd(sockfd[0]);
      worker_processes_[i]->SetPid(pid);

    } else if(pid==0)  {
      logger_manager_.AfterFork();
      ::close(sockfd[0]);
      // Release Master
      worker_forking_ = true;
      master_pipe_ = sockfd[1];
      role_ = kRoleWorker;
      if(reactor_.running()) {
        LOG_DEBUG(logger_, "desc=reactor_running_stop_first");
        reactor_.Stop();
        return;
      } else {
         WorkerRoutine1(sockfd[1]);
         exit(0);
      }
    } else {
        LOGGER_ERROR(logger_, "fork i=%lu", i);
        abort();
    }
  }
  void Tick(uint32_t ) {
    // we try to rotate logs every hour.
    time_t now = time(NULL);
    struct tm the_time;
    localtime_r(&now, &the_time);
    if(the_time.tm_hour != log_rotate_hour_) {
      logger_manager_.RotateLogFiles();
      for(size_t i=0; i<worker_processes_.size(); ++i) {
        worker_processes_[i]->Kill(SIGUSR1);
      }
      log_rotate_hour_ = the_time.tm_hour;
    }

  }
  void WaitWorkerExit() {
    for(size_t i=0; i<worker_processes_.size(); ++i) {
      worker_processes_[i]->Wait(true) ;
      delete worker_processes_[i];
    }
    worker_processes_.clear();
  }
  void OnSigint() {
    exiting_ = true;
    reactor_.Stop();
    LOG_LOG(logger_, "SIGINT");
    if(role_ == kRoleMaster) {
      for(size_t i=0; i<worker_processes_.size(); ++i) {
        worker_processes_[i]->Kill(SIGINT);
      }
    }
  }
  void OnSIGCHLD() {
    int status = 0;
    pid_t pid = ::waitpid(-1, &status, WNOHANG);
    LOG_WARN(logger_, "SIGCHLD pid=%d", pid);
    if(role_ == kRoleMaster) {
      if(exiting_) {
        return;
      }
      for(size_t i=0; i<worker_processes_.size(); ++i) {
        if(worker_processes_[i]->pid() == pid) {
          worker_processes_[i]->Clear();
          StartWorker(i);
          break;
        }
      }
    } else {
      LOG_ERROR(logger_, "reason=ProcessIsNotMaster");
    }
  }
  void OnSIGUSR1() {
    LOG_LOG(logger_, "SIGUSR1 rotate_logs");
    if(role_ == kRoleWorker) {
      logger_manager_.RotateLogFiles();
    }
  }
  void OnSIGUSR2() {
    LOG_LOG(logger_, "SIGUSR2 reopen");
    if(role_ == kRoleMaster) {
      Reopen();
    }
  }
  void OnTimerStop(uint32_t ) {
    reactor_.Stop();
  }
  void OnSIGQUIT() {
    if(role_ == kRoleWorker) {
      LOG_WARN(logger_, "SIGQUIT action=exit_after_5_seconds");
      reactor_.AddTimer(5000,
          std::bind(&TMyself::OnTimerStop, this, std::placeholders::_1),
          NULL, false);
    }
  }
   
  void MasterRoutine() {
    reactor_.EventLoop();
    if(worker_forking_) {
      LOG_DEBUG(logger_, "desc=forking_worker");
      //FreeMaster();
      WorkerRoutine1(master_pipe_);
    } else {
      WaitWorkerExit();
      reactor_.Uninit();
    }
  }
  void InitMaster(int worker_count);
  void FreeMaster();
  void WorkerRoutine(int master_pipe);
  void WorkerRoutine1(int master_pipe);
  void ForwardFd(int fd, int type) {
    size_t i = process_idx_;
    process_idx_ = (process_idx_+1) % worker_processes_.size();
    worker_processes_[i]->SendFd(fd, type);
  }
  void Reopen();
 protected:
  LoggerManager logger_manager_;
  Reactor reactor_;
  Logger& logger_;
  // master 
  TAddrMap addrs_;
  std::vector<Process*> worker_processes_;
  std::vector<Process*> wait_close_workers_;
  size_t process_idx_;
  std::vector<std::string> argv_;
  std::vector<AcceptorForwarder*> acceptors_;
  bool exiting_;
  int log_rotate_hour_;

  // worker
  Reactor reactor_worker_;
  int master_pipe_;
  bool worker_forking_;
  EnumRole role_;
};
template<typename App>
int ProcessDriver<App>::Start(int worker_count,  const Configure& conf) {
  assert(worker_count>0);
  time_t now = time(NULL);
  struct tm the_time;
  localtime_r(&now, &the_time);
  log_rotate_hour_ = the_time.tm_hour;
  InitMaster(worker_count);
  for(size_t i=0; i<static_cast<size_t>(worker_count); ++i) {
    StartWorker(i);
  }
  MasterRoutine();
  return 0;
}
template<typename Worker>
void ProcessModel<Worker>::Uninit() {
  addrs_.clear();
  for(size_t i=0; i<worker_processes_.size(); ++i) {
    delete worker_processes_[i];
  }
  worker_processes_.clear();
  for(size_t i=0; i<acceptors_.size(); ++i) {
    delete acceptors_[i];
  }
  acceptors_.clear();
  argv_.clear();
  process_idx_ = 0;
  master_pipe_ = -1;
  worker_forking_ = false;
}
template<typename Worker>
void ProcessModel<Worker>::InitMaster(int worker_count) {
  reactor_.Init();
  for(size_t i=0; i<static_cast<size_t>(worker_count); ++i) {
      Process* p = new Process(reactor_, logger_);
      worker_processes_.push_back(p);
  }
  reactor_.AddTimer(5000,
       std::bind(&TMyself::Tick, this, std::placeholders::_1), NULL, true);
  reactor_.Signal(SIGINT, std::bind(&TMyself::OnSigint, this));
  reactor_.Signal(SIGCHLD, std::bind(&TMyself::OnSIGCHLD, this));
  reactor_.Signal(SIGUSR1, std::bind(&TMyself::OnSIGUSR1, this));
  reactor_.Signal(SIGUSR2, std::bind(&TMyself::OnSIGUSR2, this));
  for(auto it=addrs_.begin(); it!=addrs_.end();++it) {
    AcceptorForwarder* acceptor = new AcceptorForwarder(reactor_, this,
        logger_, it->second);
    acceptors_.push_back(acceptor);
    if(acceptor->Bind(it->first) != 0) { 
      LOGGER_ERROR(logger_, "bind addr=%s", it->first.c_str());
    }
  }
}
template<typename Worker>
void ProcessModel<Worker>::FreeMaster() {
  for(size_t i=0; i<worker_processes_.size(); ++i) {
    delete worker_processes_[i];
  }
  worker_processes_.clear();
  for(size_t i=0; i<acceptors_.size(); ++i) {
    acceptors_[i]->Close();
    delete acceptors_[i];
  }
  acceptors_.clear();
  reactor_.RemoveAllEventSource();
}
template<typename Worker>
void ProcessModel<Worker>::WorkerRoutine(int master_pipe) {
  logger_manager_.AfterFork();
  // FreeMaster();
  reactor_worker_.Init();
  reactor_worker_.Signal(SIGINT, std::bind(&TMyself::OnSigintWorker, this));
  ControlChannel control_channel(reactor_worker_, logger_);
  control_channel.Init(master_pipe);
  TWorker worker(reactor_worker_, logger_manager_);
  AcceptorManager acm;
  control_channel.SetRecvFdCallback(
      std::bind(&AcceptorManager::OnFdRecved, &acm, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
  int argc = static_cast<int>(argv_.size());
  const char **argv = new const char*[argc];
  for(int i=0; i<argc;++i) {
    argv[i] = argv_[i].c_str();
  }
  worker.Init(argc, argv, &acm);
  delete [] argv;
  LOG_LOG(logger_, "EnterLoop");
  reactor_worker_.EventLoop();
  LOG_LOG(logger_, "ExitLoop");
  worker.Uninit();
  reactor_worker_.Uninit();
  LOG_LOG(logger_, "ReactorUninit");
} 
template<typename Worker>
void ProcessModel<Worker>::WorkerRoutine1(int master_pipe) {
  logger_manager_.AfterFork();
  reactor_.AfterFork();
  FreeMaster();
  //reactor_.Init();
  reactor_.Signal(SIGINT, std::bind(&TMyself::OnSigint, this));
  reactor_.Signal(SIGUSR1, std::bind(&TMyself::OnSIGUSR1, this));
  reactor_.Signal(SIGQUIT, std::bind(&TMyself::OnSIGQUIT, this));
 
  ControlChannel control_channel(reactor_, logger_);
  control_channel.Init(master_pipe);
  TWorker worker(reactor_, logger_manager_);
  //TWorker worker(reactor_);
  AcceptorManager acm;
  control_channel.SetRecvFdCallback(
      std::bind(&AcceptorManager::OnFdRecved, &acm, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
  int argc = static_cast<int>(argv_.size());
  const char **argv = new const char*[argc];
  for(int i=0; i<argc;++i) {
    argv[i] = argv_[i].c_str();
  }
  worker.Init(argc, argv, &acm);
  delete [] argv;
  LOG_LOG(logger_, "EnterLoop");
  reactor_.EventLoop();
  LOG_LOG(logger_, "ExitLoop");
  worker.Uninit();
  reactor_.Uninit();
  LOG_LOG(logger_, "ReactorUninit");
} 

template<typename Worker>
void ProcessModel<Worker>::Reopen() {
  for(size_t i=0; i<worker_processes_.size(); ++i) {
    worker_processes_[i]->Kill(SIGQUIT);
    worker_processes_[i]->Clear();
    StartWorker(i);
  }
  // TODO: reload Loggers.
  // TODO: reload listeners.
}
} // namespace blitz.
#endif // __BLITZ_MULTIPROCESS_H_ 
