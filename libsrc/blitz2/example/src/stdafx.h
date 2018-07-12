#ifndef __BLITZ2_EXAMPLE_STDAFX_H_
#define __BLITZ2_EXAMPLE_STDAFX_H_ 1
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <blitz2/blitz2.h>
#include <blitz2/thread_driver.h>
#include <blitz2/line_msg.h>
#include <blitz2/base_server.h>
#include <blitz2/zookeeper.h>
#include <blitz2/state_machine.h>
#include <blitz2/stress_tester.h>
#include <blitz2/redis_pool.h>
#include <blitz2/thrift.h>
using std::cout;
using std::endl;
using std::string;
using std::vector;
using blitz2::ThreadDriver;
using blitz2::Configure;
using blitz2::Acceptor;
using blitz2::LoggerManager;
using blitz2::SharedLine;
using blitz2::SharedPtr;
using blitz2::LineMsgDecoder;
using blitz2::StreamSocketChannel;
using blitz2::MsgData;
using blitz2::OutMsg;
using blitz2::BaseServer;
using blitz2::Reactor;
using blitz2::Logger;
using blitz2::StressTester;
using blitz2::ZookeeperPool;
using blitz2::ZookeeperEventFire;
using blitz2::StateMachine;
using blitz2::state_id_t;
using blitz2::action_result_t;
using blitz2::ActionArcs;
using blitz2::Event;
using blitz2::ZookeeperStringEvent;
using blitz2::ThriftServer;
typedef blitz2::RedisPool<blitz2::RandomSelector> RedisPool;


namespace po = boost::program_options;
#endif 
