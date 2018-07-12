#ifndef IMAGE_SERVICE_COMMON_CONFIGURE_H_
#define IMAGE_SERVICE_COMMON_CONFIGURE_H_

#include <utils/textconfig.h>
#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>


class Configure {
 public:
  Configure();
  ~Configure();

  int Init(const std::string &conf_file);
  void Release();
  static Configure* GetInstance();

  void GetString(const std::string &key, std::string &value);
  void GetStrings(const std::string &key, std::vector<std::string> &values);

  std::string GetString(const std::string &key);
  std::vector<std::string> GetStrings(const std::string &key);

  template<typename T> T GetNumber(const std::string &key, const T default_value = (T)0) {
    std::string val("0");
    try {
      GetString(key, val);
      return boost::lexical_cast<T>(val);
    }
    catch (...) {
    }
    return boost::lexical_cast<T>(default_value);
  }

 private:
  CTextConfig *config_;
  static Configure *instance_;
};

#endif
