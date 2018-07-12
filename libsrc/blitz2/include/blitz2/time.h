// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// time related function.. 
#ifndef __BLITZ2_TIME_H_
#define __BLITZ2_TIME_H_ 1

#include <stdint.h>
#include <sys/time.h>
namespace blitz2 { namespace time {
inline uint64_t GetTickMs() {
  timeval use;
  gettimeofday(&use, 0);
  return use.tv_sec * 1000 + use.tv_usec / 1000;
}
inline uint64_t GetTickUs() {
  struct timeval cur_time;
  gettimeofday(&cur_time, 0);
  return cur_time.tv_sec * 1000000 + cur_time.tv_usec;
}

}} // namespace blitz2::time.
#endif // __BLITZ2_TIME_H_.

