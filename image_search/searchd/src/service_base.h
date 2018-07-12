#ifndef IMAGE_SEARCH_SEARCHD_SERVISE_BASE_H_
#define IMAGE_SEARCH_SEARCHD_SERVISE_BASE_H_


#include <string>
#include <map>
#include <ImageSearchMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <common/configure.h>


typedef enum {
  ISS_COMMON_SEARCH = 1,
} SearchServiceType;

class SearchServiceBase {
 public:
  SearchServiceBase();
  virtual ~SearchServiceBase();

  int Exec(const ImageSearchRequest &request, std::string &result);

  virtual SearchServiceType get_type() const = 0;
  virtual SearchServiceBase* Clone() = 0;
  virtual void Clear() = 0;
  
 protected:
  virtual int Process(const ImageSearchRequest &request, std::string &result) = 0;

  int DownloadPicture(const std::string &url,
                      char *&data,
                      size_t &data_len);

 protected:
};


class SearchServiceDispatcher {
 private:
  typedef std::map<int, SearchServiceBase*> SearchServiceMember;
 public:
  SearchServiceDispatcher();
  ~SearchServiceDispatcher();

  static SearchServiceDispatcher* GetInstance();
  static void Release();

  int Init();
  SearchServiceBase* GetService(int type);
  void ReleaseService(SearchServiceBase *&codec);

  template<typename T> int RegisterService() {
    SearchServiceBase *service = new T();
    if (NULL == service) {
      return -1;
    }
    reg_members_.insert(std::pair<int, SearchServiceBase*>(service->get_type(), service));
    return 0;
  }

 private:
  SearchServiceMember reg_members_;
  static SearchServiceDispatcher *instance_;
};

#define BEGIN_SEARCH_SERVICE_MAP() \
  SearchServiceDispatcher::GetInstance()->Init();

#define REGISTER_SERVICE(service_class) \
  SearchServiceDispatcher::GetInstance()->RegisterService<service_class>();

#define END_SEARCH_SERVICE_MAP

#endif
