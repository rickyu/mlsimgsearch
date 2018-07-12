#include "json_parser.h"
#include <fstream>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include "constant.h"
#include <string.h>


inline static int _get_array_index(const std::string &name, std::string &node) {
  std::vector<std::string> parts;
  node = name;
  boost::algorithm::split(parts, name, boost::algorithm::is_any_of("[]"));
  if (parts.size() < 3)
    return -1;
  for (size_t i = 0; i < parts[1].length(); i++) {
    if (parts[1][i] > '9' || parts[1][i] < '0')
      return -1;
  }
  node = parts[0];
  return boost::lexical_cast<int>(parts[1]);
}

JsonParser::JsonParser()
  : j_tokener_(NULL)
  , j_root_obj_(NULL) {
}

JsonParser::~JsonParser() {
  if (j_root_obj_) {
    json_object_put(j_root_obj_);
    j_root_obj_ = NULL;
  }
  if (j_tokener_) {
    json_tokener_free(j_tokener_);
    j_tokener_ = NULL;
  }
}

int JsonParser::LoadMem(const char *data, const size_t len) {
  if(NULL == data){
    return CR_JSON_NO_OBJECT;
  }
  //使用之前先删除之前的数据
  if (j_root_obj_) {
    json_object_put(j_root_obj_);
    j_root_obj_ = NULL;
  }
  if (j_tokener_) {
    json_tokener_free(j_tokener_);
    j_tokener_ = NULL;
  }

  j_tokener_ = json_tokener_new();
  if (NULL == j_tokener_) {
    return CR_OUTOFMEMORY;
  }
  j_root_obj_ = json_tokener_parse_ex(j_tokener_, data, len);
  if (NULL == j_root_obj_) {
    return CR_JSON_WRONG_FORMAT;
  }
  return CR_SUCCESS;
}

int JsonParser::LoadFile(const char *file) {
  std::fstream fs;
  fs.open(file, std::ios::binary|std::ios::in);
  if (!fs) {
    return CR_NOFILE;
  }
  fs.seekg(0, std::ios::end);
  size_t file_len = fs.tellg();
  fs.seekg(0, std::ios::beg);
  char *buf = new char[file_len+1];
  fs.read(buf, file_len);
  buf[file_len] = '\0';
  fs.close();
  int ret = LoadMem(buf, file_len);
  delete []buf;
  return ret;
}

int JsonParser::GetInt(const char *xpath, int &val) {
  std::string str_val;
  int ret = GetString(xpath, str_val);
  val = boost::lexical_cast<int>(str_val);
  return ret;
}

int JsonParser::GetInt64(const char *xpath, int64_t &val) {
  std::string str_val;
  int ret = GetString(xpath, str_val);
  val = boost::lexical_cast<int64_t>(str_val);
  return ret;
}

int JsonParser::GetFloat(const char *xpath, float &val) {
  std::string str_val;
  int ret = GetString(xpath, str_val);
  val = boost::lexical_cast<float>(str_val);
  return ret;
}

int JsonParser::GetBool(const char *xpath, bool &val) {
  std::string str_val;
  int ret = GetString(xpath, str_val);
  if(strcasecmp(str_val.c_str(), "true") == 0) {
    val = true;
  }else {
    val = false;
  }
  return ret;
}
 
int JsonParser::GetString(const char *xpath, std::string &val) {
  std::vector<std::string> parts;
  boost::algorithm::split(parts, xpath, boost::algorithm::is_any_of("/"));
  struct json_object *obj_cur = j_root_obj_;
  int arr_idx = -1;
  std::string node("");
  for (size_t i = 1; i < parts.size(); i++) {
    arr_idx = _get_array_index(parts[i], node);
    obj_cur = json_object_object_get(obj_cur, node.c_str());
    if (NULL == obj_cur) {
      return CR_JSON_NO_OBJECT;
    }
    if (json_type_array == json_object_get_type(obj_cur)) {
      if (arr_idx < 0 || json_object_array_length(obj_cur) < arr_idx+1)
        return CR_JSON_WRONG_XPATH;
      obj_cur = json_object_array_get_idx(obj_cur, arr_idx);
      if (NULL == obj_cur) {
        return CR_JSON_NO_OBJECT;
      }
    }
  }
  val.assign(json_object_get_string(obj_cur));
  return CR_SUCCESS;
}

int JsonParser::GetArrayNodeLength(const char *xpath, int &len) {
  std::vector<std::string> parts;
  boost::algorithm::split(parts, xpath, boost::algorithm::is_any_of("/"));
  struct json_object *obj_cur = j_root_obj_;
  int arr_idx = -1;
  std::string node("");
  for (size_t i = 1; i < parts.size(); i++) {
    arr_idx = _get_array_index(parts[i], node);
    obj_cur = json_object_object_get(obj_cur, node.c_str());
    if (NULL == obj_cur) {
      return CR_JSON_NO_OBJECT;
    }
    if (json_type_array == json_object_get_type(obj_cur)) {
      if (arr_idx < 0 || json_object_array_length(obj_cur) < arr_idx+1) {
        if (i+1 == parts.size())
          break;
        return CR_JSON_WRONG_XPATH;
      }
      obj_cur = json_object_array_get_idx(obj_cur, arr_idx);
      if (NULL == obj_cur) {
        return CR_JSON_NO_OBJECT;
      }
    }
  }
  if (json_type_array == json_object_get_type(obj_cur)) {
    len = json_object_array_length(obj_cur);
  }
  else {
    return CR_JSON_NOT_ARRAY;
  }
  return CR_SUCCESS;
}
