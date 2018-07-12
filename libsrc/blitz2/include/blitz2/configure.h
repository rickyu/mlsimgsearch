// Copyright 2013 meilishuo Inc.
// Author: Yu Zuo
// Configure related class. 
#ifndef __BLITZ2_CONFIGURE_H_
#define __BLITZ2_CONFIGURE_H_ 1
#include <string>
#include <istream>
#include <ostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
namespace blitz2 {
typedef boost::property_tree::ptree Configure;
class ConfigureLoader {
 public:
   bool LoadStream(Configure& config, std::istream& stream);
   bool LoadFile(Configure& config, const std::string& path);
   bool WriteFile(const Configure& config, const std::string& path); 
   bool WriteStream(const Configure& config,  std::ostream& stream); 
 private:
  std::string last_error_;
};

} // namespace blitz2.
#endif // __BLITZ2_CONFIGURE_H_

