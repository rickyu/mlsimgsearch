#ifndef LIBSRC_UTILS_URL_UTIL_H_
#define LIBSRC_UTILS_URL_UTIL_H_

#include <stdlib.h>
#include <string>

typedef struct _url_info {
  char *scheme;
  char *user;
  char *pass;
  char *host;
  unsigned short port;
  char *path;
  char *query;
  char *fragment;
} url_info;

void url_info_free(url_info *theurl);
url_info *url_parse(char const *str);
url_info *url_parse_ex(char const *str, int length);

char * url_encode(const char *src, const int len);
int url_decode(char *str, int len);

char* uri_encode(const char *src, const int len);

#endif
