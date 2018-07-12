// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// stdafx.h include all used system header and namespace.

#ifndef __BLITZ2_STDAFX_H_
#define __BLITZ2_STDAFX_H_ 1
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <stdint.h>
#include <cstddef>
#include <memory>
#include <poll.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unordered_map>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
namespace blitz2 {
using std::vector;
using std::string;
using std::cout;
using std::endl;
using boost::mutex;
} // namespace blitz2.
#endif // __BLITZ2_STDAFX_H_ 
