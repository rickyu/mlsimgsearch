#include "url_util.h"
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

static unsigned char hexchars[] = "0123456789ABCDEF";

void url_free(url_info *theurl)
{
  if (theurl->scheme)
    free(theurl->scheme);
  if (theurl->user)
    free(theurl->user);
  if (theurl->pass)
    free(theurl->pass);
  if (theurl->host)
    free(theurl->host);
  if (theurl->path)
    free(theurl->path);
  if (theurl->query)
    free(theurl->query);
  if (theurl->fragment)
    free(theurl->fragment);
  free(theurl);
}

char *replace_controlchars_ex(char *str, int len)
{
  unsigned char *s = (unsigned char *)str;
  unsigned char *e = (unsigned char *)str + len;
        
  if (!str) {
    return (NULL);
  }
        
  while (s < e) {
            
    if (iscntrl(*s)) {
      *s='_';
    }   
    s++;
  }
        
  return (str);
} 
/* }}} */

char *replace_controlchars(char *str) {
  return replace_controlchars_ex(str, (int)strlen(str));
} 


url_info *url_parse(char const *str) {
  return url_parse_ex(str, (int)strlen(str));
}

url_info *url_parse_ex(char const *str, int length)
{
  char port_buf[6];
  url_info *ret = (url_info*)calloc(1, sizeof(url_info));
  char const *s, *e, *p, *pp, *ue;
                
  s = str;
  ue = s + length;

  /* parse scheme */
  if ((e = (char*)memchr(s, ':', length)) && (e - s)) {
    /* validate scheme */
    p = s;
    while (p < e) {
      /* scheme = 1*[ lowalpha | digit | "+" | "-" | "." ] */
      if (!isalpha(*p) && !isdigit(*p) && *p != '+' && *p != '.' && *p != '-') {
        if (e + 1 < ue) {
          goto parse_port;
        } else {
          goto just_path;
        }
      }
      p++;
    }
        
    if (*(e + 1) == '\0') { /* only scheme is available */
      ret->scheme = strndup(s, (e - s));
      replace_controlchars_ex(ret->scheme, (int)(e - s));
      goto end;
    }

    /* 
     * certain schemas like mailto: and zlib: may not have any / after them
     * this check ensures we support those.
     */
    if (*(e+1) != '/') {
      /* check if the data we get is a port this allows us to 
       * correctly parse things like a.com:80
       */
      p = e + 1;
      while (isdigit(*p)) {
        p++;
      }
                        
      if ((*p == '\0' || *p == '/') && (p - e) < 7) {
        goto parse_port;
      }
                        
      ret->scheme = strndup(s, (e-s));
      replace_controlchars_ex(ret->scheme, (int)(e - s));
                        
      length -= (int)(++e - s);
      s = e;
      goto just_path;
    } else {
      ret->scheme = strndup(s, (e-s));
      replace_controlchars_ex(ret->scheme, (int)(e - s));
                
      if (*(e+2) == '/') {
        s = e + 3;
        if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
          if (*(e + 3) == '/') {
            /* support windows drive letters as in:
               file:///c:/somedir/file.txt
            */
            if (*(e + 5) == ':') {
              s = e + 4;
            }
            goto nohost;
          }
        }
      } else {
        if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
          s = e + 1;
          goto nohost;
        } else {
          length -= (int)(++e - s);
          s = e;
          goto just_path;
        }       
      }
    }   
  } else if (e) { /* no scheme; starts with colon: look for port */
  parse_port:
    p = e + 1;
    pp = p;

    while (pp-p < 6 && isdigit(*pp)) {
      pp++;
    }

    if (pp - p > 0 && pp - p < 6 && (*pp == '/' || *pp == '\0')) {
      long port;
      memcpy(port_buf, p, (pp - p));
      port_buf[pp - p] = '\0';
      port = strtol(port_buf, NULL, 10);
      if (port > 0 && port <= 65535) {
        ret->port = (unsigned short) port;
      } else {
        free(ret->scheme);
        free(ret);
        return NULL;
      }
    } else if (p == pp && *pp == '\0') {
      free(ret->scheme);
      free(ret);
      return NULL;
    } else {
      goto just_path;
    }
  } else {
  just_path:
    ue = s + length;
    goto nohost;
  }
        
  e = ue;
        
  if (!(p = (char*)memchr(s, '/', (ue - s)))) {
    char *query, *fragment;

    query = (char*)memchr(s, '?', (ue - s));
    fragment = (char*)memchr(s, '#', (ue - s));

    if (query && fragment) {
      if (query > fragment) {
        p = e = fragment;
      } else {
        p = e = query;
      }
    } else if (query) {
      p = e = query;
    } else if (fragment) {
      p = e = fragment;
    }
  } else {
    e = p;
  }     
                
  /* check for login and password */
  if ((p = (char*)memrchr(s, '@', (e-s)))) {
    if ((pp = (char*)memchr(s, ':', (p-s)))) {
      if ((pp-s) > 0) {
        ret->user = strndup(s, (pp-s));
        replace_controlchars_ex(ret->user, (int)(pp - s));
      } 
                
      pp++;
      if (p-pp > 0) {
        ret->pass = strndup(pp, (p-pp));
        replace_controlchars_ex(ret->pass, (int)(p-pp));
      } 
    } else {
      ret->user = strndup(s, (p-s));
      replace_controlchars_ex(ret->user, (int)(p-s));
    }
                
    s = p + 1;
  }

  /* check for port */
  if (*s == '[' && *(e-1) == ']') {
    /* Short circuit portscan, 
       we're dealing with an 
       IPv6 embedded address */
    p = s;
  } else {
    /* memrchr is a GNU specific extension
       Emulate for wide compatability */
    for(p = e; *p != ':' && p >= s; p--);
  }

  if (p >= s && *p == ':') {
    if (!ret->port) {
      p++;
      if (e-p > 5) { /* port cannot be longer then 5 characters */
        free(ret->scheme);
        free(ret->user);
        free(ret->pass);
        free(ret);
        return NULL;
      } else if (e - p > 0) {
        long port;
        memcpy(port_buf, p, (e - p));
        port_buf[e - p] = '\0';
        port = strtol(port_buf, NULL, 10);
        if (port > 0 && port <= 65535) {
          ret->port = (unsigned short)port;
        } else {
          free(ret->scheme);
          free(ret->user);
          free(ret->pass);
          free(ret);
          return NULL;
        }
      }
      p--;
    }   
  } else {
    p = e;
  }
        
  /* check if we have a valid host, if we don't reject the string as url */
  if ((p-s) < 1) {
    free(ret->scheme);
    free(ret->user);
    free(ret->pass);
    free(ret);
    return NULL;
  }

  ret->host = strndup(s, (p-s));
  replace_controlchars_ex(ret->host, (int)(p - s));
        
  if (e == ue) {
    return ret;
  }
        
  s = e;
        
 nohost:
        
  if ((p = (char*)memchr(s, '?', (ue - s)))) {
    pp = strchr(s, '#');

    if (pp && pp < p) {
      if (pp - s) {
        ret->path = strndup(s, (pp-s));
        replace_controlchars_ex(ret->path, (int)(pp - s));
      }
      p = pp;
      goto label_parse;
    }
        
    if (p - s) {
      ret->path = strndup(s, (p-s));
      replace_controlchars_ex(ret->path, (int)(p - s));
    }   
        
    if (pp) {
      if (pp - ++p) { 
        ret->query = strndup(p, (pp-p));
        replace_controlchars_ex(ret->query, (int)(pp - p));
      }
      p = pp;
      goto label_parse;
    } else if (++p - ue) {
      ret->query = strndup(p, (ue-p));
      replace_controlchars_ex(ret->query, (int)(ue - p));
    }
  } else if ((p = (char*)memchr(s, '#', (ue - s)))) {
    if (p - s) {
      ret->path = strndup(s, (p-s));
      replace_controlchars_ex(ret->path, (int)(p - s));
    }   
                
  label_parse:
    p++;
                
    if (ue - p) {
      ret->fragment = strndup(p, (ue-p));
      replace_controlchars_ex(ret->fragment, (int)(ue - p));
    }   
  } else {
    ret->path = strndup(s, (ue-s));
    replace_controlchars_ex(ret->path, (int)(ue - s));
  }
 end:
  return ret;
}

#define TEST_CHAR(c, f)	(test_char_table[(unsigned)(c)] & (f))

static const char c2x_table[] = "0123456789abcdef";
static unsigned char *c2x(unsigned what, unsigned char *where) {
#ifdef CHARSET_EBCDIC
    what = os_toascii[what];
#endif /*CHARSET_EBCDIC*/
    *where++ = '%';
    *where++ = c2x_table[what >> 4];
    *where++ = c2x_table[what & 0xf];
    return where;
}

static const unsigned char test_char_table[256] = {
    0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,  /*0x00...0x07*/
    0x1e, 0x1e, 0x1f, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,  /*0x08...0x0f*/
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,  /*0x10...0x17*/
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,  /*0x18...0x1f*/
    0x0e, 0x00, 0x17, 0x06, 0x01, 0x06, 0x01, 0x01,  /*0x20...0x27*/
    0x09, 0x09, 0x01, 0x00, 0x08, 0x00, 0x00, 0x0a,  /*0x28...0x2f*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x30...0x37*/
    0x00, 0x00, 0x08, 0x0f, 0x0f, 0x08, 0x0f, 0x0f,  /*0x38...0x3f*/
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x40...0x47*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x48...0x4f*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x50...0x57*/
    0x00, 0x00, 0x00, 0x0f, 0x1f, 0x0f, 0x07, 0x00,  /*0x58...0x5f*/
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x60...0x67*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x68...0x6f*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*0x70...0x77*/
    0x00, 0x00, 0x00, 0x0f, 0x07, 0x0f, 0x01, 0x1e,  /*0x78...0x7f*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0x80...0x87*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0x88...0x8f*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0x90...0x97*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0x98...0x9f*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xa0...0xa7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xa8...0xaf*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xb0...0xb7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xb8...0xbf*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xc0...0xc7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xc8...0xcf*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xd0...0xd7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xd8...0xdf*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xe0...0xe7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xe8...0xef*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,  /*0xf0...0xf7*/
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16   /*0xf8...0xff*/
    
};

char * url_encode(const char *src, const int len /*= 0*/) {
  size_t src_len = 0;
  if (0 == len) {
    src_len = strlen(src);
  }
  char *dst = (char *) malloc(3 * src_len + 3);
  const unsigned char *s = (const unsigned char *)src;
  unsigned char *d = (unsigned char *)dst;
  unsigned c;

  while ((c = *s)) {
    if (TEST_CHAR(c, 0x04)) {
      d = c2x(c, d);
    }
    else {
      *d++ = (unsigned char)c;
    }
    ++s;
  }
  *d = '\0';
  return dst;
}

const char* URL_CHARS = "!@#$&*()=:/;?+'%";
static bool is_url_char(char c) {
  int chars_num = strlen(URL_CHARS);
  for (int i = 0; i < chars_num; ++i) {
    if (URL_CHARS[i] == c) {
      return true;
    }
  }
  return false;
}

char * uri_encode(const char *src, const int len /*= 0*/) {
  size_t src_len = 0;
  if (0 == len) {
    src_len = strlen(src);
  }
  char *dst = (char *) malloc(3 * src_len + 3);
  const unsigned char *s = (const unsigned char *)src;
  unsigned char *d = (unsigned char *)dst;
  unsigned c;

  while ((c = *s)) {
    if (!is_url_char(c) && TEST_CHAR(c, 0x04)) {
      d = c2x(c, d);
    }
    else {
      *d++ = (unsigned char)c;
    }
    ++s;
  }
  *d = '\0';
  return dst;
}

static int _htoi(char *s)
{
  int value;
  int c;

  c = ((unsigned char *)s)[0];
  if (isupper(c))
    c = tolower(c);
  value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

  c = ((unsigned char *)s)[1];
  if (isupper(c))
    c = tolower(c);
  value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

  return (value);
}

int url_decode(char *str, int len)
{
  char *dest = str;
  char *data = str;

  while (len--) {
    if (*data == '+') {
      *dest = ' ';
    }
    else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
             && isxdigit((int) *(data + 2))) {
      *dest = (char) _htoi(data + 1);
      data += 2;
      len -= 2;
    } else {
      *dest = *data;
    }
    data++;
    dest++;
  }
  *dest = '\0';
  return (int)(dest - str);
}
