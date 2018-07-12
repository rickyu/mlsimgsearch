#include <common/configure.h>
#include <blitz2/blitz2.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <boost/program_options.hpp>
#include "ransac_ranker.h"
#include "feature_data_container.h"

using blitz2::Logger;
using blitz2::LoggerManager;
using blitz2::LogId;

CLogger& g_logger = CLogger::GetInstance("feature_loader");

int main(int argc, char **argv) {
  boost::program_options::options_description options_desc("supported command line");
  options_desc.add_options()
      ("help,h","list all arguments")
      ("log,l",boost::program_options::value<std::string>(),"log file path")
      ("key,k",boost::program_options::value<std::string>(),"key words")
      ("url,u",boost::program_options::value<std::string>(),"image url")
      ("host,H",boost::program_options::value<std::string>(),"ssdb host")
      ("port,P",boost::program_options::value<std::string>(),"ssdb port");
  boost::program_options::variables_map conf_map;
  boost::program_options::store(boost::program_options::parse_command_line(argc,argv,options_desc),conf_map);
  boost::program_options::notify(conf_map);
  if (conf_map.count("help")) {
    std::cout << options_desc << std::endl;
    return 1;
  }
  if (!conf_map.count("key")) {
    return 2;
  }
  if (!conf_map.count("url")) {
    return 3;
  }
  if (!conf_map.count("host")) {
    return 4;
  }
  if (!conf_map.count("port")) {
    return 5;
  }

  std::string key = conf_map["key"].as<std::string>();
  std::string url = conf_map["url"].as<std::string>();
  std::string host = conf_map["host"].as<std::string>();
  int port = boost::lexical_cast<int>(conf_map["port"].as<std::string>());
  std::string log_path = conf_map["log"].as<std::string>();
  LoggerManager::GetInstance().InitFromLoggerConf(log_path);
  ImageFeatureLoader feature_loader;
  if (feature_loader.ConnectSSDB(host, port) != 0) {
    LOGGER_ERROR(g_logger, "connect ssdb failed! [%s:%d]", host.c_str(), port);
    return -1;
  }
  if (feature_loader.LoadFeatureFromUrl(url) != 0) {
    LOGGER_ERROR(g_logger, "load image feature failed! image_url=%s", url.c_str());
    return -2;
  }
  if (feature_loader.SaveFeatures(key) != 0) {
    return -3;
  }
  // feature_loader.GetKeyPoints(img1_points);
  // feature_loader.GetDescriptors(img1_descs);
  // feature_loader.LoadFeatureFromUrl(img2_url);
  // feature_loader.GetKeyPoints(img2_points);
  // feature_loader.GetDescriptors(img2_descs);
  // RansacRanker rr;
  // float sim = rr.ComputeRank(img1_points, img1_descs, img2_points, img2_descs);
  // std::cout << sim << std::endl;
  
  return 0;
}
