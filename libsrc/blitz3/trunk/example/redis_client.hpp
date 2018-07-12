#ifndef __REDIS_CLIENT_HPP_
#define __REDIS_CLIENT_HPP_

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <hiredis/hiredis.h>
using boost::asio::ip::tcp;
class RedisClient {
 public:
  RedisClient(boost::asio::io_service& io, const std::string& ip, uint16_t port) 
    : io_(io), socket_(io), ip_(ip), port_(port) {
      reader_ = NULL;
      connected_ = false;
      reply_ = NULL;
      command_ = NULL;
      command_len_ = 0;
  }
  template<typename Handler>
  void Get(const char* key, redisReply** reply, Handler handler) {
    if (InRequest()) {
      io_.post(boost::bind(handler, boost::system::error_code(), 0));
      return;
    }
    handler_ = handler;
    command_len_ = redisFormatCommand(&command_, "GET %s", key);
    reply_ = reply;
    Loop(boost::system::error_code(), 0);
  }
  bool InRequest() const {
    return reply_ != NULL;
  }
  const std::string& ip() const {
    return ip_;
  }
  uint16_t port() const {
    return port_;
  }
  
 private:
  void FinishRequest(const boost::system::error_code& ec) {
    handler_(ec, 0);
    ClearRequest();

  }
  void ClearRequest() {
    if (command_) {
      free(command_);
      command_ = NULL;
    }
    reply_ = NULL;
    handler_ = boost::function<void(const boost::system::error_code&, std::size_t)>();
    command_len_ = 0;
  }
  void CloseSocket() {
     socket_.close();
     connected_ = false;
     if (reader_ ) {
      reader_ = redisReplyReaderCreate();
      reader_ = NULL;
     }
  }
  void Loop(const boost::system::error_code& ec, std::size_t sz) {
    reenter(co_) {
      // connect
      if (!connected_) {
        reader_ = redisReplyReaderCreate();
        yield socket_.async_connect(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(ip_), port_) , 
               boost::bind(&RedisClient::Loop, this, _1, 0));
        if (ec) {
          FinishRequest(ec);
          CloseSocket();
          return;
        }
        connected_ = true;
        std::cout << "redis connected" << std::endl;
      }
      // send request
      yield boost::asio::async_write(socket_, 
            boost::asio::buffer(command_, command_len_), 
            boost::bind(&RedisClient::Loop, this, _1, _2));
      if (ec) {
          FinishRequest(ec);
          CloseSocket();
        return ;
      }
      std::cout << "redis request sent" << std::endl;
      // recv response
      for ( ;; ) {
        yield socket_.async_read_some( 
            boost::asio::buffer(resp_, sizeof(resp_)),
            boost::bind(&RedisClient::Loop, this, _1, _2));
        redisReplyReaderFeed(reader_, resp_, sz);
        if (redisReplyReaderGetReply(reader_, (void**)reply_) != REDIS_OK) {
          FinishRequest(ec);
          CloseSocket();
          return;
        }
        if (*reply_ != NULL) {
          std::cout << "redis got response" << std::endl;
          break;
        }
      }
      FinishRequest(boost::system::error_code());
    }
  }
 private:
  boost::asio::io_service& io_;
  tcp::socket socket_;
  std::string ip_;
  uint16_t port_;
  redisReader* reader_;
  bool connected_;
  redisReply** reply_;
  char resp_[2048];
  char* command_;
  int command_len_;
  boost::asio::coroutine co_;
  boost::function<void(const boost::system::error_code& , std::size_t )> handler_;
};
#endif
