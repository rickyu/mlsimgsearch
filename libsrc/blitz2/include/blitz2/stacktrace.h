// Copyright 2013 meilishuo Inc.
// Author: 余佐
#ifndef __BLITZ2_STACKTRACE_H_
#define __BLITZ2_STACKTRACE_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <ostream>
#include <cxxabi.h>
namespace blitz2 {
template<typename TOutput>
int GetStacktrace(TOutput output, unsigned int max_frames)
{
  // storage array for stack trace address data
  void** addrlist = reinterpret_cast<void**>(
      malloc(sizeof(void*) * (max_frames+1)));
  // retrieve current stack addresses
  int addrlen = backtrace(addrlist, max_frames+1);
  if (addrlen == 0) {
    return -1;
  }
  // resolve addresses into strings containing "filename(function+address)",
  // this array must be free()-ed
  char** symbollist = backtrace_symbols(addrlist, addrlen);
  // allocate string which will be filled with the demangled function name
  size_t funcnamesize = 256;
  char* funcname =  new char[funcnamesize];
  // iterate over the returned symbol lines. skip the first, it is the
  // address of this function.
  for (int i = 1; i < addrlen; ++i) {
    char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

    // find parentheses and +address offset surrounding the mangled name:
    // ./module(function+0x15c) [0x8048a6d]
    for (char *p = symbollist[i]; *p; ++p) {
        if (*p == '(') {
          begin_name = p;
        } else if (*p == '+') {
          begin_offset = p;
        } else if (*p == ')' && begin_offset) {
          end_offset = p;
          break;
        }
    }

    if (begin_name && begin_offset && end_offset
        && begin_name < begin_offset) {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        // mangled name is now in [begin_name, begin_offset) and caller
        // offset in [begin_offset, end_offset). now apply
        // __cxa_demangle():

        int status;
        char* ret = abi::__cxa_demangle(begin_name,
                funcname, &funcnamesize, &status);
        if (status == 0) {
          funcname = ret; // use possibly realloc()-ed string
          // output (module, funcname, offset);
          output(symbollist[i], funcname, begin_offset);
        }
        else {
          // demangling failed. Output function name as a C function with
          // no arguments.
          output(symbollist[i], funcname, begin_offset);
        }
    }
    else {
        output(symbollist[i], "", "");
    }
  }
  free(funcname);
  free(symbollist);
  free(addrlist);
  return 0;
}
struct StacktraceOutput {
  StacktraceOutput() {
    os_ = NULL;
  }
  StacktraceOutput(std::ostream& os) {
    os_ = &os;
  }

  int operator()(const char* mod, const char* func, const char* offset) {
    (*os_) << mod << ':' << func << '+' << offset << std::endl;
    return 0;
  }
  std::ostream* os_;
};

int PrintStacktrace(std::ostream& os, unsigned int frames) {

  StacktraceOutput output(os);
  return GetStacktrace(output, frames);
}
} // namespace blitz2
#endif // __BLITZ2_STACKTRACE_H_
