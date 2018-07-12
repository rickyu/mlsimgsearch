#ifndef __TAURUS_STDAFX_H_
#define __TAURUS_STDAFX_H_ 1
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <blitz2/blitz2.h>
#include <blitz2/outmsg.h>
#include <blitz2/configure.h>
#include <blitz2/memcached.h>
#include <blitz2/hbase.h>
#include <blitz2/thread_driver.h>
#include <blitz2/state_machine.h>
namespace po=boost::program_options;
using std::string;
using std::cout;
using std::endl;
using std::vector;
using blitz2::Configure;
using blitz2::ConfigureLoader;
using blitz2::Reactor;
using blitz2::BaseServer;
using blitz2::OutMsg;
using blitz2::Logger;
using blitz2::LoggerManager;
using blitz2::Acceptor;
//using blitz2::MemcachedServer;
using blitz2::MemcachedReq;
using blitz2::MemcachedGetReq;
using blitz2::MemcachedSetReq;
using blitz2::MemcachedReqDecoder;
using blitz2::MemcachedResp;
using blitz2::MemcachedRespDecoder;
using blitz2::HbasePool;
using blitz2::SharedStreamSocketChannel;
using blitz2::SharedPtr;
using blitz2::ThreadDriver;
using blitz2::StateMachine;
using blitz2::Event;
using blitz2::EQueryResult;
using blitz2::kQueryResultInvalidResp;
using blitz2::LogId;
using blitz2::HbaseGetEvent;
using blitz2::HbaseSetEvent;
using blitz2::RandomSelector;
using blitz2::RegionServer;
//using blitz2::RangeSelector;
using blitz2::state_id_t;
using blitz2::action_result_t;
using blitz2::HbaseEventFire;
using namespace apache::hadoop::hbase::thrift;
using boost::shared_ptr;
//typedef blitz2::MemcachedPool<RandomSelector> MemcachedPool;
#endif // __TAURUS_STDAFX_H_

