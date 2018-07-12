#include <common/http_client.h>
#include <string.h>
#include <memory>
#include <utils/md5.h>

CLogger& HttpClient::logger_ = CLogger::GetInstance("HTTP");


HttpClient::HttpClient()
  : curl_(NULL)
  , auto_redirect_(true)
  , kEachReallocSize(102400) {
  url_prefix_.clear();
}

HttpClient::~HttpClient() {
  if (curl_)
    curl_easy_cleanup(curl_);
}

int HttpClient::GlobalInit() {
  int ret = CR_SUCCESS;
  if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
    return CR_HTTP_GLOBAL_INIT_FAILED;
  }
  return ret;
}

int HttpClient::GlobalFinalize() {
  curl_global_cleanup();
  return CR_SUCCESS;
}

int HttpClient::Init() {
  int ret = CR_SUCCESS;
  curl_ = curl_easy_init();
  if (!curl_) {
    ret = CR_HTTP_CURL_INIT_FAILED;
  }

  return ret;
}

int HttpClient::SendRequest(HttpMethod method,
                            const char* url,
                            char **recv_buf /*= NULL*/,
                            size_t *recvd /*= NULL*/,
                            const char* option_data /*= NULL*/,
                            size_t option_data_len /*= 0*/,
                            const int connect_timeout /*= 0*/,
                            const int recv_timeout /*= 0 */,
                            char **header_buf /*= NULL*/,
                            size_t *header_buf_len /*= 0*/
                            ) {
  int ret = CR_SUCCESS;
  CURLcode code;
  
  std::string real_url("");
  if (!url_prefix_.empty()) {
    real_url.assign(url_prefix_);
  }
  real_url.append(url);
  curl_easy_setopt(curl_, CURLOPT_URL, real_url.c_str());
  curl_easy_setopt(curl_, CURLOPT_ENCODING, "gzip,deflate");
  curl_easy_setopt(curl_, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
  //curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
  if (auto_redirect_) {
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 5);
  }
  if (recv_timeout) {
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, recv_timeout);
  }
  if (connect_timeout) {
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout);
  }

  switch (method) {
  case HTTP_POST:
    if (!option_data || !option_data_len) {
      ret = CR_HTTP_WRONG_POST_DATA;
      return ret;
    }
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, option_data);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, option_data_len);
    break;

  case HTTP_UPLOAD:
    break;

  case HTTP_HEAD:
    curl_easy_setopt(curl_, CURLOPT_NOBODY, 1);
    break;

  case HTTP_GET:
  default:
    break;
  }

  // set receive callback and receive data buffer here
  if (HTTP_HEAD != method) {

    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, RecvDataCallback);
  }
  recv_buf_.buf = recv_buf;
  if (header_buf) {
    header_buf_.buf = header_buf;
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, RecvHeaderCallback);
  }
   
  code = curl_easy_perform(curl_);
  if (CURLE_OK != code) {
    ret = CR_HTTP_CURL_PERFORM_FAILED;
    if (CURLE_HTTP_RETURNED_ERROR == code)
    {
      ret = CR_HTTP_RETURN_ERROR;
    }
    LOGGER_ERROR(logger_, "curl_easy_perform failed, error code is %d, url=%s",
                 code, url);
  }
  if (recvd) {
    *recvd = recv_buf_.used;
  }
  if (header_buf_len) {
    *header_buf_len = header_buf_.used;
  }
  return ret;
}

int HttpClient::PostMultipartFormData(const char *url,
                                      const std::vector<multipart_formdata_item_st> &items,
                                      char **recv_buf /*= NULL*/, size_t *recvd /*= NULL*/,
                                      const int connect_timeout /*= 0*/,
                                      const int recv_timeout /*= 0*/) {
  int ret = CR_SUCCESS;
  CURLcode code;
  
  std::string real_url("");
  if (!url_prefix_.empty()) {
    real_url.assign(url_prefix_);
  }
  real_url.append(url);
  curl_easy_setopt(curl_, CURLOPT_URL, real_url.c_str());
  curl_easy_setopt(curl_, CURLOPT_ENCODING, "gzip,deflate");
  curl_easy_setopt(curl_, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
  if (recv_timeout) {
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, recv_timeout);
  }
  if (connect_timeout) {
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout);
  }


  if (0 == items.size()) {
    ret = CR_HTTP_WRONG_POST_DATA;
    return ret;
  }
  struct curl_httppost* post = NULL;
  struct curl_httppost* last = NULL;
  std::vector<multipart_formdata_item_st>::const_iterator itCur = items.begin();
  while (itCur != items.end()) {
    multipart_formdata_item_st item = *itCur;
    if (item.content_type) {
      char md5[40] = {0};
      MD5_str((uint8_t*)item.data, item.data_len, md5, sizeof(md5));
      curl_formadd(&post, &last,
                   CURLFORM_COPYNAME, md5,
                   CURLFORM_BUFFER, item.name,
                   CURLFORM_BUFFERPTR, item.data,
                   CURLFORM_BUFFERLENGTH, item.data_len,
                   //CURLFORM_CONTENTTYPE, item.content_type,
                   CURLFORM_END);
      /*curl_formadd(&post, &last,
                   CURLFORM_COPYNAME, item.name,
                   CURLFORM_PTRCONTENTS, item.data,
                   CURLFORM_CONTENTSLENGTH, item.data_len,
                   CURLFORM_CONTENTTYPE, item.content_type,
                   CURLFORM_END);*/
      printf("buffer length=%ld\n", item.data_len);
    }
    else {
      curl_formadd(&post, &last,
                   CURLFORM_COPYNAME, item.name,
                   CURLFORM_PTRCONTENTS, item.data,
                   CURLFORM_CONTENTSLENGTH, item.data_len,
                   CURLFORM_END);
    }
    ++itCur;
  }

  curl_easy_setopt(curl_, CURLOPT_HTTPPOST, post);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, RecvDataCallback);
  recv_buf_.buf = recv_buf;

  code = curl_easy_perform(curl_);
  if (CURLE_OK != code) {
    ret = CR_HTTP_CURL_PERFORM_FAILED;
    if (CURLE_HTTP_RETURNED_ERROR == code)
    {
      ret = CR_HTTP_RETURN_ERROR;
    }
    LOGGER_ERROR(logger_, "curl_easy_perform failed, error code is %d, url=%s",
                 code, url);
  }
  if (recvd) {
    *recvd = recv_buf_.used;
  }
  curl_formfree(post);
  return ret;
}

size_t HttpClient::RecvDataCallback(void *buffer,
                                    size_t size,
                                    size_t nmemb,
                                    void *userp) {
  HttpClient *pclient = static_cast<HttpClient*>(userp);
  size_t recv_data_len = size*nmemb;

  if (NULL == pclient->recv_buf_.buf) {
    return recv_data_len;
  }
  
  if (NULL == *(pclient->recv_buf_.buf)) {
    pclient->recv_buf_.size = 
      recv_data_len>pclient->kEachReallocSize ? recv_data_len : pclient->kEachReallocSize;
    *(pclient->recv_buf_.buf) = (char*)malloc(pclient->recv_buf_.size);
    pclient->recv_buf_.used = 0;
  }
  else {
    if (pclient->recv_buf_.size - pclient->recv_buf_.used < recv_data_len) {
      pclient->recv_buf_.size +=
        recv_data_len>pclient->kEachReallocSize ? recv_data_len : pclient->kEachReallocSize;
      *(pclient->recv_buf_.buf) = (char*)realloc((void*)(*(pclient->recv_buf_.buf)),
                                                 pclient->recv_buf_.size);
    }
  }

  memcpy((*(pclient->recv_buf_.buf))+pclient->recv_buf_.used,
         buffer,
         sizeof(char)*recv_data_len);
  pclient->recv_buf_.used += recv_data_len;

  return recv_data_len;
}

size_t HttpClient::RecvHeaderCallback(void *buffer,
                                      size_t size,
                                      size_t nmemb,
                                      void *userp) {
  HttpClient *pclient = static_cast<HttpClient*>(userp);
  size_t recv_data_len = size*nmemb;
  if (NULL == pclient->header_buf_.buf) {
    return recv_data_len;
  }
  
  if (NULL == *(pclient->header_buf_.buf)) {
    pclient->header_buf_.size = 
      recv_data_len>pclient->kEachReallocSize ? recv_data_len : pclient->kEachReallocSize;
    *(pclient->header_buf_.buf) = (char*)malloc(pclient->header_buf_.size);
    pclient->header_buf_.used = 0;
  }
  else {
    if (pclient->header_buf_.size - pclient->header_buf_.used < recv_data_len) {
      pclient->header_buf_.size +=
        recv_data_len>pclient->kEachReallocSize ? recv_data_len : pclient->kEachReallocSize;
      *(pclient->header_buf_.buf) = (char*)realloc((void*)(*(pclient->header_buf_.buf)),
                                                 pclient->header_buf_.size);
    }
  }

  memcpy((*(pclient->header_buf_.buf))+pclient->header_buf_.used,
         buffer,
         sizeof(char)*recv_data_len);
  pclient->header_buf_.used += recv_data_len;

  return recv_data_len;
}

bool HttpClient::EnableCookie(const char* cookie_file_path) {
  CURLcode ret;
  if (!cookie_file_path) {
    ret = curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, ""); // just to start cookie engine
  }
  else {
    ret = curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, cookie_file_path);
  }

  return (CURLE_OK==ret);
}

bool HttpClient::SetCookie(const char* cookie_line) {
  CURLcode ret = curl_easy_setopt(curl_, CURLOPT_COOKIE, cookie_line);
  if (CURLE_OK != ret) {
    return false;
  }

  return true;
}

bool HttpClient::SetHttpServerAuth(const char* user_name, const char* pwd) {
  if (CURLE_OK != curl_easy_setopt(curl_, CURLOPT_USERNAME, user_name))
    return false;
  if (CURLE_OK != curl_easy_setopt(curl_, CURLOPT_PASSWORD, pwd))
    return false;

  return true;
}

bool HttpClient::SetProxyServerAuth(const char* user_name, const char* pwd) {
  return SetServerAuth(false, user_name, pwd);
}

bool HttpClient::SetServerAuth(bool is_http_auth,
                               const char* user_name,
                               const char* pwd) {
  bool ret = false;
  int field_len = strlen(user_name)+strlen(pwd)+2;
  char *pauthfield = new char[field_len];

  memset(pauthfield, 0, field_len);
  snprintf(pauthfield, field_len, "%s:%s", user_name, pwd);
  if (CURLE_OK == curl_easy_setopt(curl_,
                                   is_http_auth ? CURLOPT_USERPWD : CURLOPT_PROXYUSERPWD,
                                   pauthfield))
    {
      delete[] pauthfield;
      return true;
    }else{
    delete[] pauthfield;
    return false;
  }

  if (CURLE_OK == curl_easy_setopt(curl_,
                                   is_http_auth ? CURLOPT_HTTPAUTH : CURLOPT_PROXYAUTH,
                                   CURLAUTH_ANY)) // libcurl will choose the best one
    ret = true;
  else
    ret = false;

  return ret;  
}

bool HttpClient::GetResponceCookieList(std::vector<std::string>& cookie_list) {
  bool ret = false;
  CURLcode curl_ret;
  struct curl_slist *cookies;
  struct curl_slist *nc;

  curl_ret = curl_easy_getinfo(curl_, CURLINFO_COOKIELIST, &cookies);
  if (CURLE_OK != curl_ret) {
    return ret;
  }

  cookie_list.clear();
  nc = cookies;
  while (nc) {
    cookie_list.push_back(nc->data);
    nc = nc->next;
  }

  curl_slist_free_all(cookies);
  ret = true;

  return ret;
}

bool HttpClient::SetHttpProxy(const char *proxy) {
  curl_easy_setopt(curl_, CURLOPT_HTTPPROXYTUNNEL, 1);
  return curl_easy_setopt(curl_, CURLOPT_PROXY, proxy) == CURLE_OK
    ? true : false;
}

bool HttpClient::SetUrlPrefix(const char *prefix) {
  url_prefix_.assign(prefix);
  return true;
}

void HttpClient::SetAutoRedirect(const bool auto_redirect) {
  auto_redirect_ = auto_redirect;
}

int HttpClient::GetResponseCode(int &response_code) {
  response_code = 0;
  if (NULL == curl_) {
    return CR_HTTP_CURL_INIT_FAILED;
  }
  long lcode;
  CURLcode code = curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &lcode);
  if (CURLE_OK != code) {
    return CR_HTTP_CURL_PERFORM_FAILED;
  }
  response_code = static_cast<int>(lcode);
  return CR_SUCCESS;
}
