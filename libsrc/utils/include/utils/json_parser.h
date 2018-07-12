#ifndef IMAGE_SERVICE_COMMON_JSON_PARSER_H_
#define IMAGE_SERVICE_COMMON_JSON_PARSER_H_

#include <stdint.h>
#include <string>
#include <json/json.h>

class JsonParser {
 public:
  JsonParser();
  ~JsonParser();

  int LoadMem(const char *data, const size_t len);
  int LoadFile(const char *file);

  /**
     
   */
  int GetInt(const char *xpath, int &val);
  int GetInt64(const char *xpath, int64_t &val);
  int GetFloat(const char *xpath, float &val);
  int GetString(const char *xpath, std::string &val);
  int GetBool(const char *xpath, bool &val);
  int GetArrayNodeLength(const char *xpath, int &len);

 private:
  struct json_tokener *j_tokener_;
  struct json_object *j_root_obj_;
};

#endif
