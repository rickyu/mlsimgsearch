#include <iostream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <common/configure.h>
#include <sstream>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include "reducer_result.h"

#define BUFFER_SIZE 8192

CLogger& g_logger = CLogger::GetInstance("indexer");

typedef struct {
  int64_t words_id_;
  int count_;
} IDFReduceResult;

int main(int argc, char **argv, char **env) {
  Configure::GetInstance()->Init(argv[1]);

  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    return -1;
  }

  int cur_bucket;
  int prev_bucket = -1;

  int word_num = 0;//idf
  std::vector<IDFReduceResult> words_stat;

  char buf[BUFFER_SIZE];
  ReducerBucketResult *result = NULL;
  int ret = 0;

  while (fgets(buf, BUFFER_SIZE-1, stdin)) {
    try {
      std::vector<std::string> fields;
      boost::algorithm::split(fields, buf, boost::algorithm::is_any_of(" \t"));
      if (fields.size() < 5 ) {
        LOGGER_ERROR(g_logger, "Wrong line: %s", buf);
        continue;
      }
      cur_bucket = boost::lexical_cast<int>(fields[0]);
      if (cur_bucket < 0) {
        continue;
      }
      LOGGER_INFO(g_logger, "cur bucket %d ",cur_bucket); 
      if (cur_bucket != prev_bucket) {
        if (result) {
          LOGGER_LOG(g_logger, "Process a bucket done. (%d) word total = %d", 
                     prev_bucket, word_num);

          IDFReduceResult word_r;
          word_r.words_id_ = prev_bucket;
          word_r.count_ = word_num ;
          word_num = 0;
          words_stat.push_back(word_r);

          result->PrintResult();
          delete result;
          result = NULL;
        }
        result = new ReducerBucketResult(cur_bucket);
        ret = result->Init();
        if (0 != ret) {
          LOGGER_ERROR(g_logger, "ReducerBucketResult init failed! (%d)", ret);
          continue;
        }
        LOGGER_LOG(g_logger, "new ReducerBucketResult init success, bucket=%d", cur_bucket);
      }
      boost::algorithm::trim(fields[4]);
      result->AddLine(boost::lexical_cast<int64_t>(fields[1]),
                      boost::lexical_cast<float>(fields[2]),
                      fields[3],fields[4]);
      prev_bucket = cur_bucket;
    }
    catch (...) {
      LOGGER_FATAL(g_logger, "Process line except: %s", buf);
      continue;
    }
  }

  try {
    // last bucket
    if (result) {
      result->PrintResult();
      delete result;
      result = NULL;

      IDFReduceResult word_r;
      word_r.words_id_ = prev_bucket;
      word_r.count_ = word_num ;
      word_num = 0;
      words_stat.push_back(word_r);
      std::cout << std::endl;
    }
  }
  catch (...) {
  }
  LOGGER_LOG(g_logger, "all finish");

  return 0;
}
