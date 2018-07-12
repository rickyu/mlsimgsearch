#include <iostream>
#include "image_quantizer.h"
#include <string>
#include <common/configure.h>
#include <beansdb_client/beansdb_clusters.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <sys/time.h>
#include "image_quantized_serializer.h"

CLogger& g_logger = CLogger::GetInstance("indexer");

int main(int argc, char **argv, char **env) {
  Configure::GetInstance()->Init(argv[1]);

  if (BCR_SUCCESS != BeansdbClusters::GetInstance()->AddClusters(Configure::GetInstance()->GetStrings("beansdb_clusters"))) {
    return -1;
  }

  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    return -1;
  }

  int64_t pic_id;
  std::string pic_key;
  BOFQuantizeResultMap result;
  size_t features_num = 0;

  CreateObjectFactory();

  ImageQuantizer img_quantizer;
  if (0 != img_quantizer.Init()) {
    LOGGER_ERROR(g_logger, "quantizer init failed");
    return -2;
  }

  std::string quantizer ="";
  QuantizerFlag label = QUANTIZER_FLAG_HE;
  int sum = 0;
  while (std::cin >> pic_id >> pic_key) {
    try {
      sum ++;
      timeval tv1, tv2;
      gettimeofday(&tv1, NULL);
      if (0 != img_quantizer.Analyze(pic_id, pic_key, result,features_num,label)) {
        LOGGER_ERROR(g_logger, "analyze picture failed. (%ld, %s, %ld)",
                     pic_id, pic_key.c_str(), features_num);
        continue;
      }
      BOFQuantizeResultMap::iterator it = result.begin();
      quantizer = "";
      while (it != result.end()) {
        quantizer += boost::lexical_cast<std::string>(it->first)+";";
        std::cout << it->first << "\t"
                  << pic_id << "\t"
                  << it->second.q_ / features_num << "\t"
                  << (it->second.word_info_[0].empty() ? "+" : it->second.word_info_[0]) << "\t" 
                  << (it->second.word_info_[1].empty() ? "+" : it->second.word_info_[1]) << std::endl;
        ++it;
      }
      gettimeofday(&tv2, NULL);
      unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
      LOGGER_INFO(g_logger, "process (%ld:%s:%ld) success! (time=%ld)  quantizer(%s) ",
                  pic_id, pic_key.c_str(), features_num, ts,quantizer.c_str());
    }
    catch (...) {
      LOGGER_FATAL(g_logger, "Got an exception, %ld:%s", pic_id, pic_key.c_str());
    }
  }
  std::cout << "-2" << "\t"
            << sum << std::endl;
  return 0;
}
