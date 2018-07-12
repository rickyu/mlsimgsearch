#include <iostream>
#include <string>
#include <common/configure.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <sys/time.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <bof_searcher/indexer.h>
#include <bof_feature_analyzer/idf_weighter.h>

CLogger& g_logger = CLogger::GetInstance("indexer");

int main(int argc, char **argv, char **env) {
  Configure::GetInstance()->Init(argv[1]);

  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    return -1;
  }

  std::string vocabulary_file_path(argv[2]),
      idf_word_frequence(argv[3]);
  cv::Mat vocabulary;
  cv::FileStorage fs(vocabulary_file_path, cv::FileStorage::READ);      
  fs["vocabulary"] >> vocabulary;
  fs.release();
    
  IDFWeighter * weighter = new IDFWeighter;
  weighter->SetConf(idf_word_frequence);
  weighter->SetWordsNum(vocabulary.rows);
  weighter->Init();

  std::vector<std::string> fields;
  char line[128] = {0};
  int num = 1;
  while (std::cin.getline(line, sizeof(line))) {
    try {
      fields.clear();
      boost::algorithm::split(fields, line, boost::algorithm::is_any_of("\t"));
      if (0 == strlen(line)) {
        LOGGER_ERROR(g_logger, "read an empty line\n");
        break;
      }
      if (fields.size() < 2 || fields[0].empty()) {
        LOGGER_ERROR(g_logger, "wrong file format, maybe at the end. line=%s\n", line);
        continue;
      }
      boost::algorithm::trim(fields[0]);
      boost::algorithm::trim(fields[1]);
      int64_t bucket_id = boost::lexical_cast<int64_t>(fields[0]);
      if (0 > bucket_id) {
        continue;
      }
      int64_t bucket_data_len = boost::lexical_cast<int64_t>(fields[1]);
      LOGGER_INFO(g_logger, "read line: bucket_id=%ld, bucket_data_len=%ld",
                  bucket_id, bucket_data_len);
      // set hint and write data
      char *data = new char[bucket_data_len+1];
      std::cin.read(data, bucket_data_len);

      BOFIndexBucket bucket;
      std::stringstream ss;
      ss.write(data, bucket_data_len);
      boost::archive::text_iarchive ia(ss);
      ia >> bucket;
    
      std::vector<BOFIndexItem> *pitems;
      bucket.GetItems(pitems);
      std::vector<BOFIndexItem> &items = *pitems;

      size_t item_index = 0;
      while (item_index < items.size()) {
        weighter->Weighting(bucket_id,items[item_index].bof_info_.q_,&num);
        std::cout << items[item_index].pic_id_ << "\t"
                  << bucket_id << "\t"
                  << items[item_index].bof_info_.q_ << "\t"
                  << items[item_index].bof_info_.word_info_[0] << "\t" 
                  << items[item_index].bof_info_.word_info_[1]<< std::endl; 
        item_index ++;
      }
      if (data) {
        delete [] data;
        data = NULL;
      }
    }
    catch (...) {
      LOGGER_FATAL(g_logger, "catch an exception\n");
    }
  }
  LOGGER_LOG(g_logger, "all finish!");

  return 0;
}
