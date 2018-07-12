#ifndef __LIBSRC_UTILS_MATH_H_
#define __LIBSRC_UTILS_MATH_H_

#define ERROR_DOUBLE 0.0000001

#include <math.h>

class MathUtil {
  public:
    static bool IsDoubleEqual(double a, double b) {
      return fabs(a - b) <= ERROR_DOUBLE;
    }
};

#endif //__LIBSRC_UTILS_MATH_H_
