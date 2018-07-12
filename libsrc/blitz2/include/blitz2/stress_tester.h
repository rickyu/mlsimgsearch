// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// StressTester. 
#ifndef __BLITZ2_STRESS_TESTER_H_
#define __BLITZ2_STRESS_TESTER_H_ 1
#include <ostream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <blitz2/half_duplex_service.h>
#include <blitz2/time.h>
namespace blitz2 {

SharedPtr<OutMsg> GenNextRequest(size_t idx);

class RequestInfo {
 public:
  RequestInfo() { 
    time_ = 0;
    finished_ = false;
  }
  void set_finished(uint32_t t) { time_ = t;finished_ = true;}
  bool finished() const { return finished_ ;}
  bool operator<(const RequestInfo& rhs) const {
    return time_<rhs.time_;
  }
 private:
  uint32_t time_;
  bool finished_;
};
template<typename Decoder>
class StressTester {
 public:
  typedef StressTester<Decoder> TStressTester;
  typedef typename Decoder::TInitParam TDecoderInitParam;
  typedef HalfDuplexService<Decoder> TService;
  typedef typename Decoder::TMsg TMsg;
  typedef std::function<SharedPtr<OutMsg>(size_t)> TReqGeneratorFunc;
  typedef std::function<bool(TStressTester*, size_t)> TProgressFunc;
  // typedef std::function<bool(size_t, TMsg*)> TRespCheckFunc;
  StressTester();
  ~StressTester();
  void Start(const std::string& addr, const TReqGeneratorFunc& req_gen_func,
      const TDecoderInitParam& decoder_init_param);
  std::ostream& Print(std::ostream& os);
  size_t req_count() const { return req_count_; }
  void set_req_count(uint32_t c) {req_count_ = c; }
  size_t concurrency() const { return concurrency_; }
  void set_concurrency(uint32_t c) { concurrency_ = c; }
  void set_timeout(uint32_t timeout) { timeout_ = timeout; }
  void set_progress_reporter(const TProgressFunc& p) { progress_reporter_ = p;}
  size_t recved_resp_count() const { return recved_resp_count_;}
  size_t issued_req_count() const { return issued_req_count_;}
 protected:
  void QueryCallback(EQueryResult, TMsg* msg,
                      uint64_t start_time, size_t idx);
  void OnFinished();
 protected:
  size_t req_count_;
  size_t concurrency_;
  uint32_t timeout_;
  size_t issued_req_count_;
  size_t recved_resp_count_;
  size_t succeed_count_;
  uint64_t req_bytes_; ///< total bytes of request sent.
  uint64_t resp_bytes_; ///< total bytes of response recieved.
  std::vector<RequestInfo> req_infos_;
  uint64_t start_time_;
  uint64_t stop_time_;
  TService* service_;
  Reactor reactor_;
  TReqGeneratorFunc req_generator_;
  TProgressFunc progress_reporter_;
  // TRespCheckFunc resp_checker_;
};
template<typename Decoder>
StressTester<Decoder>::StressTester() {
  req_count_ = 1000;
  concurrency_ = 1;
  timeout_ = 10000;
  issued_req_count_ = 0;
  recved_resp_count_ = 0;
  succeed_count_ = 0;
  start_time_ = 0;
  stop_time_ = 0;
  service_ = NULL;
  req_bytes_ = 0;
  resp_bytes_ = 0;
}
template<typename Decoder>
StressTester<Decoder>::~StressTester() {
  if (service_) {
    delete service_;
    service_ = NULL;
  }
}
template<typename Decoder>
void StressTester<Decoder>::Start(const std::string& addr,
    const TReqGeneratorFunc& req_gen_func,
    const TDecoderInitParam& decoder_init_param) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  reactor_.Init();
  if (req_count_ < concurrency_) {
    req_count_ = concurrency_;
  }
  req_infos_.resize(req_count_);
  req_generator_ = req_gen_func;
  IoAddr ioaddr;
  ioaddr.Parse(addr);
  start_time_ = time::GetTickMs();
  service_ = new TService(reactor_, ioaddr, decoder_init_param);
  service_->Init();
  for (size_t c=0; c<concurrency_; ++c) {
    SharedPtr<OutMsg> outmsg  = req_generator_(issued_req_count_);
    service_->Query(outmsg,
        std::bind(&StressTester::QueryCallback, this,
          _1, _2, time::GetTickMs(), issued_req_count_),
        timeout_);
    ++issued_req_count_;
  }
  reactor_.EventLoop();
}
template<typename Decoder>
void StressTester<Decoder>::QueryCallback(EQueryResult result, TMsg* msg,
    uint64_t start_time, size_t idx) {
  (void)(result);
  using std::placeholders::_1;
  using std::placeholders::_2;
  uint64_t stop_time = time::GetTickMs();
  req_infos_[idx].set_finished(stop_time_ - start_time);
  ++recved_resp_count_;
  if (progress_reporter_) {
    progress_reporter_(this, idx);
  }
  if (msg != NULL) {
    ++succeed_count_;
  }
  if (issued_req_count_ < req_count_) {
    SharedPtr<OutMsg> outmsg  = req_generator_(issued_req_count_);
    service_->Query(outmsg,
        std::bind(&StressTester::QueryCallback, this,
          _1, _2, time::GetTickMs(), issued_req_count_),
        timeout_);
    ++issued_req_count_;
  } else {
    if (recved_resp_count_ == issued_req_count_) {
      stop_time_ = stop_time;
      OnFinished();
    }
  }
}
template<typename Decoder>
void StressTester<Decoder>::OnFinished() {
  reactor_.Stop();
  std::sort(req_infos_.begin(), req_infos_.end());
}
template<typename Decoder>
std::ostream& StressTester<Decoder>::Print(std::ostream& os) {
  using namespace std;
  os<<"stress tester result:"<<endl;
  os<<"concurrency:"<<concurrency_<<",request count:"<<issued_req_count_<<endl;
  os<<"total time consumed:"<<stop_time_-start_time_<<"ms"<<endl;
  os<<"req/seconds:"<<recved_resp_count_*1000.0/(stop_time_-start_time_)<<endl;
  os<<"failed count:"<<recved_resp_count_-succeed_count_<<endl;
  os<<"unfinished req"<<endl;
  for (size_t idx=0; idx<req_infos_.size(); ++idx) {
    if (!req_infos_[idx].finished()) {
      cout<<idx<<' ';
    }
  }
  cout<<endl;
  return os;
}

} // namespace blitz2.

#endif // __BLITZ2_STRESS_TESTER_H_

