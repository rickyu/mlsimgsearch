
// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// Configure related class implementation.
//
#include <blitz2/configure.h>
namespace blitz2 {
bool ConfigureLoader::LoadStream(Configure& config, std::istream& stream) {
   using boost::property_tree::info_parser_error;
  try {
    read_info(stream, config);
  } catch(info_parser_error& e) {
    last_error_ = e.what();
    return false;
  }
  return true;

 }
 bool ConfigureLoader::LoadFile(Configure& config, const std::string& path) {
  using boost::property_tree::info_parser_error;
  try {
    read_info(path, config);
  } catch(info_parser_error& e) {
    last_error_ = e.what();
    return false;
  }
  return true;
 }
 bool ConfigureLoader::WriteFile(const Configure& config,
     const std::string& path) {
  using boost::property_tree::info_parser_error;
  try {
   write_info(path, config);
  } catch(info_parser_error& e) {
    last_error_ = e.what();
    return false;
  }
  return true;
 }
 bool ConfigureLoader::WriteStream(const Configure& config,
     std::ostream& stream) {
   using boost::property_tree::info_parser_error;
  try {
   write_info(stream, config);
  } catch(info_parser_error& e) {
    last_error_ = e.what();
    return false;
  }
  return true;
 }
} // namespace blitz2.
