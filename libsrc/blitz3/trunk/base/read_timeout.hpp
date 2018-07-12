#ifndef BLIT3_BASE_READ_TIMEOUT_HPP
#define BLIT3_BASE_READ_TIMEOUT_HPP
#include <boost/asio.hpp>
namespace blitz3 {
template<typename ReadHandler>
struct TimerHook {
  deadline_timer timer;
  ReadHandler handler;
  int status;

  TimerHook(io_service& io, ReadHandler& handler, int timeout)
       :handler_(handler) {
    status = 0;
    timer.reset(new deadline_timer(io, boost::posix_time::milliseconds(timeout)));
  }
  static void OnTimeout(shared_ptr<TimerHook> /*hook*/,
      const boost::system::error_code& /*ec*/) {
    if (hook->status != 0) {
      hook->status = 1;
      hook->handler(ec, 0);
    }
  }
  static void OnRead(shared_ptr<TimerHook> hook,
      const boost::system::error_code& ec,
      std::size_t bytes) {
    if (hook->status != 0) {
      hook->status = 2;
      hook->handler(ec, bytes);
    }
  }
  
};
template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until_timeout(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    int timeout,
    char delim, BOOST_ASIO_MOVE_ARG(ReadHandler) handler) {
  typedef TimerHook<ReadHandler> TimerHook_t;
  shared_ptr<TimerHook_t> hook(new TimerHook_t(s.get_io_service(), handler, timeout));
  hook->timer_->async_wait(boost::bind(&TimerHook_t::OnTimeout, hook, _1));
  async_read_until(s, b, delim, boost::bind(&TimerHook_t::OnTimeout, hook, _1, _2));
}
}
#endif
