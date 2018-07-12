#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "redis_client.hpp"
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::asio::coroutine;
using boost::shared_ptr;
using boost::asio::const_buffer;
typedef boost::system::error_code error_code_t;

class GetProcessor {
 public:
  GetProcessor(io_service& io) 
    : io_(io) , timer_(io), redis_(io, "127.0.0.1",  6379) {
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

class Session {
 public:
  Session(io_service& io) : io_(io), socket_(io) {
  }
  ~Session() {
    cout << "session destructor" << endl;
  }
  tcp::socket& socket() {
    return socket_;
  }
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
            boost::bind(&Session::Loop, this,  _1, _2));
        if (ec) {
          cout << "recv line fail" << endl;
          Exit();
          return;
        }
        PrepareProcessor(size);
        if (!processor_) {
          resp_bufs_.push_back(const_buffer("unsupport\n", 10));
        } else {
          yield processor_->Process(boost::bind(&Session::Loop, this, _1, _2));
          ProcessResult();
        }
        yield boost::asio::async_write(socket_, resp_bufs_, 
               boost::bind(&Session::Loop, this,  _1, _2));
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



class Server {
 public:
  Server(io_service& io)
    :io_(io),
     acceptor_(io) {

  }
  int Start(uint16_t port) {
    tcp::endpoint endpoint(tcp::v4(),  port);
    acceptor_.open(endpoint.protocol());
    boost::asio::socket_base::reuse_address option(true);
    acceptor_.set_option(option);
    acceptor_.bind(endpoint);
    acceptor_.listen();
    Run(boost::system::error_code());
    return 0;
  }
  void Run(const boost::system::error_code& error) {
    if (error) {
      cout << "accept error" << endl;
      acceptor_.close();
      io_.stop();
      return ;
    }
    reenter(co_) {
      for (;;) {
        new_session_ = new Session(io_);
        yield acceptor_.async_accept(new_session_->socket(),
                boost::bind(&Server::Run, this, _1));
        cout << "get a connect" << endl;
        new_session_->Start(boost::bind(Server::OnSessionExit, new_session_));
      }
    }
  }
 private:
  static void OnSessionExit(Session* session) {
    delete session;
  }
 private:
  boost::asio::io_service& io_;
  tcp::acceptor acceptor_;
  Session* new_session_;
  coroutine co_;
};

int main(int argc, char* argv[]) {
  try {
    boost::asio::io_service io_service;
    Server server(io_service);
    server.Start(1200);
    io_service.run();
  }
  catch (std::exception& e) {
    cerr << "Exception:" << e.what() << endl;
  }
  return 0;
}
