#ifndef IMAGE_SERVICE_COMMON_HTTP_CLIENT_H_
#define IMAGE_SERVICE_COMMON_HTTP_CLIENT_H_

#include <curl/curl.h>
#include <vector>
#include <string>
#include <utils/logger.h>
#include <common/constant.h>

const char HTTP_PREFIX[] = "http://";
const char HTTPS_PREFIX[] = "https://";


typedef enum {
  HTTP_GET,
  HTTP_POST,
  HTTP_UPLOAD,
  HTTP_HEAD
}HttpMethod;

typedef struct _multipart_formdata_item_st {
  char *name;
  char *data;
  size_t data_len;
  char *content_type;
  _multipart_formdata_item_st() {
    name = NULL;
    data = NULL;
    data_len = 0;
    content_type = NULL;
  }
} multipart_formdata_item_st;


class HttpClient {
 private:
  typedef struct {
    char **buf;
    size_t size;
    size_t used;
  } HttpRecvBuf;

 public:
  HttpClient();
  ~HttpClient();
  static int GlobalInit();
  static int GlobalFinalize();
  int Init();
  int SendRequest(HttpMethod method,
                  const char* url,
                  char **recv_buf = NULL, size_t *recvd = NULL,
                  const char* option_data = NULL, size_t option_data_len = 0,
                  const int connect_timeout = 0, /* ms */
                  const int recv_timeout = 0 /* ms */,
                  char **header_buf = NULL, size_t *header_buf_len = 0
                  );
  int PostMultipartFormData(const char *url,
                            const std::vector<multipart_formdata_item_st> &items,
                            char **recv_buf = NULL, size_t *recvd = NULL,
                            const int connect_timeout = 0, /* ms */
                            const int recv_timeout = 0 /* ms */);

  bool EnableCookie(const char* cookie_file_path = 0);
  bool SetCookie(const char* cookie_line);
  bool SetHttpServerAuth(const char* user_name, const char* pwd);
  bool SetProxyServerAuth(const char* user_name, const char* pwd);
  bool GetResponceCookieList(std::vector<std::string>& cookie_list);
  bool SetHttpProxy(const char *proxy);
  bool SetUrlPrefix(const char *prefix);
  void SetAutoRedirect(const bool enable_auto_redirect);
  int GetResponseCode(int &response_code);

 protected:
  static size_t RecvDataCallback(void *buffer,
                                 size_t size,
                                 size_t nmemb,
                                 void *userp);
  static size_t RecvHeaderCallback(void *buffer,
                                   size_t size,
                                   size_t nmemb,
                                   void *userp);
  bool SetServerAuth(bool is_http_auth, const char* user_name, const char* pwd);

 private:
  CURL *curl_;
  HttpRecvBuf recv_buf_;
  HttpRecvBuf header_buf_;
  std::string url_prefix_;
  static CLogger &logger_;
  bool auto_redirect_;
  const size_t kEachReallocSize;
};

#endif
