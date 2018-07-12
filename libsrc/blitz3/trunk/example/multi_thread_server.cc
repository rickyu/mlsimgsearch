#include <string>
#include <queue>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/yield.hpp>
#include "redis_client.hpp"
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::asio::coroutine;
using boost::shared_ptr;
using boost::thread;
using boost::asio::const_buffer;
typedef boost::system::error_code error_code_t;

class GetProcessor {
 public:
  GetProcessor(io_service& io) 
    : io_(io) , timer_(io), redis_(io, "172.16.0.117",  6379) {
      reply_ = NULL;
  }
  ~GetProcessor() {
  }
  template<typename Handler>
  void Process(Handler handler) {
    handler_ = handler;
    Loop(error_code_t(), 0);
  }
  string& key() {
    return key_;
  }
  const string& value1() const {
    return value1_;
  }
  const string& value2() const {
    return value2_;
  }
  void Loop(const error_code_t& ec, std::size_t sz) {
    reenter(co_) {
      reply_ = NULL;
      yield redis_.Get(key_.c_str(), &reply_, boost::bind(&GetProcessor::Loop, this, _1, 0));
      cout << ec << endl;
      cout << reply_ << endl;
      if (reply_) {
        cout << "redis reply type " << reply_->type << endl;
        if (reply_->type == REDIS_REPLY_STRING || 
            reply_->type == REDIS_REPLY_ERROR ) {
          value1_ = key_ + ":" + reply_->str;
        } 
        freeReplyObject(reply_);
        reply_ = NULL;
      } else {
          value1_ = key_ + ":error";
      }
      timer_.expires_from_now(boost::posix_time::milliseconds(40));
      yield timer_.async_wait(boost::bind(&GetProcessor::Loop, this, _1, 0));
      value2_ = key_ + ":hehehe";
      handler_(error_code_t(), 0);
    }
  }

 private:
  io_service& io_;
  boost::asio::deadline_timer timer_;
  string key_;
  string value1_;
  string value2_;
  boost::asio::coroutine co_;
  boost::function<void(const error_code_t&, std::size_t)> handler_;
  RedisClient redis_;
  redisReply* reply_;
};




class ServerBase {
 public:
  ServerBase(io_service& io, tcp::endpoint endpoint)
    :acceptor_(io), endpoint_(endpoint) {
  }
  virtual ~ServerBase() {
  }
  void Start() {
    acceptor_.open(endpoint_.protocol());
    boost::asio::socket_base::reuse_address option(true);
    acceptor_.set_option(option);
    acceptor_.bind(endpoint_);
    acceptor_.listen();
    Loop(boost::system::error_code());
  }
  virtual void Loop(const boost::system::error_code& ec) = 0; 

 protected:
  tcp::acceptor acceptor_;
  tcp::endpoint endpoint_;
};
template<typename Session, typename App>
class Server : public ServerBase {
 public:
  Server(io_service& io, App& app, const tcp::endpoint& endpoint)
    :ServerBase(io, endpoint), app_(app) {

  }
  virtual ~Server() {
  }

  void Loop(const boost::system::error_code& error) {
    reenter(co_) {
      for (;;) {
        session_ = new Session(app_.get_worker());
        yield acceptor_.async_accept(session_->socket(),
                boost::bind(&Server::Loop, this, _1));
        if (!error) {
          session_->Start(boost::bind(OnSessionExit, session_));
          session_ = NULL;
        } else {
          delete session_;
          session_ = NULL;
        }
      }
    }
  }
  static void OnSessionExit(Session* s) {
    delete s;
  }
 protected:
  Session* session_;
  App& app_;
  coroutine co_;
};



template<typename Worker>
class ThreadApp {
 public:
  typedef ThreadApp<Worker> Self;
  ThreadApp() {
    io_service_ = shared_ptr<io_service>(new io_service());
  }
  template<typename Session>
  void Listen(tcp::endpoint endpoint) {
    shared_ptr<ServerBase> s(new Server<Session, Self>(
          get_io_service(), *this, endpoint));
    servers_.push_back(s);
  }
  void StartWorker(Worker& worker)  {
    worker.BeforeRun();
    worker.get_io_service().run();
    worker.AfterRun();
  }
  struct WorkerThreadArg {
    Self* self;
    Worker* worker;

  };
  static void* worker_thread_func(void* args) {
    WorkerThreadArg* arg = (WorkerThreadArg*)args;
    arg->self->StartWorker(*(arg->worker));
    delete arg;

  }

  void Run(int worker_count = 0) {
    if (worker_count == 1) {
      // 单线程模式.
      worker_io_services_.push_back(io_service_);
      shared_ptr<Worker> worker(new Worker(*io_service_));
      workers_.push_back(worker);
    } else {
      for (int i = 0; i < worker_count; ++i) {
        shared_ptr<io_service> io(new io_service());
        worker_io_services_.push_back( io );
        shared_ptr<io_service::work> work(new io_service::work(*io));
        worker_io_service_works_.push_back(work);
        workers_.push_back(shared_ptr<Worker>(new Worker(*io)));
      }
    }
    for (size_t i = 0; i < servers_.size(); ++i) {
      servers_[i]->Start();
    }
    if (workers_.size() == 1) {
      StartWorker(*workers_[0]);

    } else {
      vector< pthread_t > threads;
      for (int i = 0; i < worker_count; ++i) {
        pthread_t h;
        WorkerThreadArg* arg = new WorkerThreadArg;
        arg->self = this;
        arg->worker = workers_[i].get();
        pthread_create(&h, NULL, worker_thread_func, arg );
        threads.push_back(h);
      }
      io_service_->run();
      for (size_t i = 0; i < threads.size(); ++i) {
        pthread_join(threads[i], NULL);
      }
    }
    
  }
  
  io_service& get_io_service() { 
    return *io_service_;
  }
  Worker& get_worker() {
    return *workers_[++cursor_%workers_.size()];
  }
 private:
  shared_ptr<io_service> io_service_;
  vector< shared_ptr<ServerBase> > servers_;
  vector< shared_ptr<io_service> > worker_io_services_;
  vector< shared_ptr<io_service::work> > worker_io_service_works_;
  size_t cursor_;
  vector< shared_ptr<Worker> > workers_;
  vector< shared_ptr<thread> > threads_;
  
  
};


// 每个线程/进程有一个该类的对象，用于保存全局变量.
class MyWorker {
 public:
  MyWorker(io_service& io) 
    :io_(io) {
  }
  void BeforeRun() {
  }
  void AfterRun() {
  }
 io_service& get_io_service() {
   return io_;
 }
 private:
  io_service& io_;
  // boost::property_tree config_;
  // RedisPool redis_pool_;
  // MysqlPool mysql_pool_;
};
// 表示一个客户端连接.
class MemcSession {
 public:
  // Session必须要的构造函数.
  MemcSession(MyWorker& worker) 
      : io_(worker.get_io_service()), socket_(worker.get_io_service()) {
  }
  ~MemcSession() {
    cout << "session destructor" << endl;
  }
  // Session必须要的函数.
  tcp::socket& socket() {
    return socket_;
  }
  // Session必须要的函数.
  template<typename ExitHandler>
  void Start(ExitHandler handler) {
    on_exit_ = handler;
    buf_.prepare(8192);
    Loop(error_code_t(), 0);
  }

 private:
  void Exit() {
    if (on_exit_) {
      io_.post(on_exit_);
      on_exit_ = boost::function<void()>();
    }
  }

  void Loop( const error_code_t& ec, std::size_t size) {
    reenter (co_) {
      for (;;) {
        yield boost::asio::async_read_until(socket_, buf_, '\n',
            boost::bind(&MemcSession::Loop, this,  _1, _2));
        if (ec) {
          cout << "recv line fail" << endl;
          Exit();
          return;
        }
        PrepareProcessor(size);
        if (!processor_) {
          resp_bufs_.push_back(const_buffer("unsupport\n", 10));
        } else {
          yield processor_->Process(boost::bind(&MemcSession::Loop, this, _1, _2));
          ProcessResult();
        }
        yield boost::asio::async_write(socket_, resp_bufs_, 
               boost::bind(&MemcSession::Loop, this,  _1, _2));
        DestroyProcessor();
        if (ec) {
          Exit();
          return;
        }
      }
    }
  }
  void ProcessResult() {
    resp_bufs_.push_back(const_buffer(processor_->value1().c_str(), processor_->value1().size()));
    resp_bufs_.push_back(const_buffer("\r\n", 2));
    resp_bufs_.push_back(const_buffer(processor_->value2().c_str(), processor_->value2().size()));
    resp_bufs_.push_back(const_buffer("\r\n", 2));
  }
  void PrepareProcessor(std::size_t sz) {
    std::size_t old_size = buf_.size() ;
    cout << "sz = " << sz <<endl;
    istream is(&buf_);
    string name;
    is >> name;
    if (name == "get") {
      command_ = 1;
      processor_ = new GetProcessor(io_);
      is >> processor_->key();
      cout << "consumed " << is.gcount() << endl;
      cout << "tellg " << is.tellg() << endl;
      cout << "buf size " << buf_.size() << endl;
      std::size_t consumed  =  old_size - buf_.size() ;
      if (consumed < sz) {
        buf_.consume(sz - consumed);
      }
    } else {
      command_ = 0;
    }
  }
  void DestroyProcessor() {
    resp_bufs_.clear();
    if (processor_) {
      delete processor_;
      processor_ = NULL;
    }
    
  }
  
 private:
  boost::asio::streambuf buf_;
  io_service& io_;
  tcp::socket socket_;
  coroutine co_;
  boost::function<void()>  on_exit_;
  GetProcessor* processor_;
  int command_;
  vector<const_buffer> resp_bufs_;
};

int main(int argc, char* argv[]) {
  try {
    ThreadApp<MyWorker> app;
    //ProcessApp<MyWorker> app;
    app.Listen< MemcSession >(tcp::endpoint(tcp::v4(), 1200));
    app.Listen< MemcSession >(tcp::endpoint(tcp::v4(), 1201));
    //app.Listen< MysqlSession >(tcp::endpoint(tcp::v4(), 1202));
    app.Run(2);
  }
  catch (std::exception& e) {
    cerr << "Exception:" << e.what() << endl;
  }
  return 0;
}
