#ifndef IMAGE_SERVICE_COMMON_UTIL_H_
#define IMAGE_SERVICE_COMMON_UTIL_H_

#include <signal.h>
#include <common/constant.h>

typedef void isighandler_t(int);
isighandler_t* img_signal(int signo,  isighandler_t *func);

#endif
