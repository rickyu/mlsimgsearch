#ifndef  __TIME_H_WANGYUANZHENG_20100717_
#define  __TIME_H_WANGYUANZHENG_20100717_

#include <sys/time.h>
#include <stdint.h>

#define USECS_PER_SEC 1000000

class TimeUtil {
  /// init tick count
  public:
  static uint64_t InitTickCount(void) {
      timeval use;
      gettimeofday(&use, 0);
      return use.tv_sec * 1000000 + use.tv_usec;
  }
  
  /// get current tick count from first call
  static uint64_t CurrentTickCount(void) {
      static uint64_t start = InitTickCount();
      timeval use;
      gettimeofday(&use, 0);
      return use.tv_sec * 1000000 + use.tv_usec - start;
  }
  static uint64_t GetTickMs() {
    timeval use;
    gettimeofday(&use, 0);
    return use.tv_sec * 1000 + use.tv_usec / 1000;
  }
  static uint64_t GetTickUs(void) {
    struct timeval cur_time;
    gettimeofday(&cur_time, 0);
    return cur_time.tv_sec * USECS_PER_SEC + cur_time.tv_usec;
  }
};

#endif  //__TIME_H_WANGYUANZHENG_20100717_
