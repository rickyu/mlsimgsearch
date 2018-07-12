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

CLogger& g_logger = CLogger::GetInstance("goods_similar");

int main(int argc, char **argv) {
  boost::program_options::options_description options_desc("supported command line");
  options_desc.add_options()
      ("help,h","list all arguments")
      ("log,l",boost::program_options::value<std::string>(),"log file path")
      ("key1,k1",boost::program_options::value<std::string>(),"key words1")
      ("key2,k2",boost::program_options::value<std::string>(),"key words2")
      ("host,H",boost::program_options::value<std::string>(),"ssdb host")
      ("port,P",boost::program_options::value<std::string>(),"ssdb port");
  boost::program_options::variables_map conf_map;
  boost::program_options::store(boost::program_options::parse_command_line(argc,argv,options_desc),conf_map);
  boost::program_options::notify(conf_map);
  if (conf_map.count("help")) {
    std::cout << options_desc << std::endl;
    return 1;
  }
  if (!conf_map.count("key1")) {
    return 2;
  }
  if (!conf_map.count("key2")) {
    return 3;
  }
  if (!conf_map.count("host")) {
    return 4;
  }
  if (!conf_map.count("port")) {
    return 5;
  }

  std::string key1 = conf_map["key1"].as<std::string>();
  std::string key2 = conf_map["key2"].as<std::string>();
  std::string host = conf_map["host"].as<std::string>();
  int port = boost::lexical_cast<int>(conf_map["port"].as<std::string>());
  std::string log_path = conf_map["log"].as<std::string>();
  LoggerManager::GetInstance().InitFromLoggerConf(log_path);
  ImageFeatureLoader feature_loader;
  if (feature_loader.ConnectSSDB(host, port) != 0) {
    LOGGER_ERROR(g_logger, "connect ssdb failed! [%s:%d]", host.c_str(), port);
    return -1;
  }
  cv::Mat img_desc1, img_desc2;
  std::vector<cv::KeyPoint> img_points1, img_points2; 
  feature_loader.LoadFeatureFromSSDB(key1, img_desc1, img_points1);
  feature_loader.LoadFeatureFromSSDB(key2, img_desc2, img_points2);
  RansacRanker rr;
  float sim = rr.ComputeRank(img_points1, img_desc1, img_points2, img_desc2);
  std::cout << sim << std::endl;

  // feature_loader.LoadFeatureFromUrl(img1_url);
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






