/**
 * Autogenerated by Thrift Compiler (0.7.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#ifndef SpatialRankerMsg_H
#define SpatialRankerMsg_H

#include <TProcessor.h>
#include "spatial_ranker_types.h"



class SpatialRankerMsgIf {
 public:
  virtual ~SpatialRankerMsgIf() {}
  virtual void Rank(SpatialRankerResponse& _return, const SpatialRankerRequest& request) = 0;
};

class SpatialRankerMsgNull : virtual public SpatialRankerMsgIf {
 public:
  virtual ~SpatialRankerMsgNull() {}
  void Rank(SpatialRankerResponse& /* _return */, const SpatialRankerRequest& /* request */) {
    return;
  }
};

typedef struct _SpatialRankerMsg_Rank_args__isset {
  _SpatialRankerMsg_Rank_args__isset() : request(false) {}
  bool request;
} _SpatialRankerMsg_Rank_args__isset;

class SpatialRankerMsg_Rank_args {
 public:

  SpatialRankerMsg_Rank_args() {
  }

  virtual ~SpatialRankerMsg_Rank_args() throw() {}

  SpatialRankerRequest request;

  _SpatialRankerMsg_Rank_args__isset __isset;

  void __set_request(const SpatialRankerRequest& val) {
    request = val;
  }

  bool operator == (const SpatialRankerMsg_Rank_args & rhs) const
  {
    if (!(request == rhs.request))
      return false;
    return true;
  }
  bool operator != (const SpatialRankerMsg_Rank_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SpatialRankerMsg_Rank_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SpatialRankerMsg_Rank_pargs {
 public:


  virtual ~SpatialRankerMsg_Rank_pargs() throw() {}

  const SpatialRankerRequest* request;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SpatialRankerMsg_Rank_result__isset {
  _SpatialRankerMsg_Rank_result__isset() : success(false) {}
  bool success;
} _SpatialRankerMsg_Rank_result__isset;

class SpatialRankerMsg_Rank_result {
 public:

  SpatialRankerMsg_Rank_result() {
  }

  virtual ~SpatialRankerMsg_Rank_result() throw() {}

  SpatialRankerResponse success;

  _SpatialRankerMsg_Rank_result__isset __isset;

  void __set_success(const SpatialRankerResponse& val) {
    success = val;
  }

  bool operator == (const SpatialRankerMsg_Rank_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SpatialRankerMsg_Rank_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SpatialRankerMsg_Rank_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SpatialRankerMsg_Rank_presult__isset {
  _SpatialRankerMsg_Rank_presult__isset() : success(false) {}
  bool success;
} _SpatialRankerMsg_Rank_presult__isset;

class SpatialRankerMsg_Rank_presult {
 public:


  virtual ~SpatialRankerMsg_Rank_presult() throw() {}

  SpatialRankerResponse* success;

  _SpatialRankerMsg_Rank_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class SpatialRankerMsgClient : virtual public SpatialRankerMsgIf {
 public:
  SpatialRankerMsgClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) :
    piprot_(prot),
    poprot_(prot) {
    iprot_ = prot.get();
    oprot_ = prot.get();
  }
  SpatialRankerMsgClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, boost::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) :
    piprot_(iprot),
    poprot_(oprot) {
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void Rank(SpatialRankerResponse& _return, const SpatialRankerRequest& request);
  void send_Rank(const SpatialRankerRequest& request);
  void recv_Rank(SpatialRankerResponse& _return);
 protected:
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class SpatialRankerMsgProcessor : virtual public ::apache::thrift::TProcessor {
 protected:
  boost::shared_ptr<SpatialRankerMsgIf> iface_;
  virtual bool process_fn(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, std::string& fname, int32_t seqid, void* callContext);
 private:
  std::map<std::string, void (SpatialRankerMsgProcessor::*)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*)> processMap_;
  void process_Rank(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  SpatialRankerMsgProcessor(boost::shared_ptr<SpatialRankerMsgIf> iface) :
    iface_(iface) {
    processMap_["Rank"] = &SpatialRankerMsgProcessor::process_Rank;
  }

  virtual bool process(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot, boost::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot, void* callContext);
  virtual ~SpatialRankerMsgProcessor() {}
};

class SpatialRankerMsgMultiface : virtual public SpatialRankerMsgIf {
 public:
  SpatialRankerMsgMultiface(std::vector<boost::shared_ptr<SpatialRankerMsgIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~SpatialRankerMsgMultiface() {}
 protected:
  std::vector<boost::shared_ptr<SpatialRankerMsgIf> > ifaces_;
  SpatialRankerMsgMultiface() {}
  void add(boost::shared_ptr<SpatialRankerMsgIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void Rank(SpatialRankerResponse& _return, const SpatialRankerRequest& request) {
    size_t sz = ifaces_.size();
    for (size_t i = 0; i < sz; ++i) {
      if (i == sz - 1) {
        ifaces_[i]->Rank(_return, request);
        return;
      } else {
        ifaces_[i]->Rank(_return, request);
      }
    }
  }

};



#endif