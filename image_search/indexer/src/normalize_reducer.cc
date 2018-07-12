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

const int BUFFER_SIZE = 8192;
CLogger& g_logger = CLogger::GetInstance("indexer");

struct ImageQuanzitedInfo {
  int word_id_;
  float weight_;
  std::vector<std::string> word_info_;
};

typedef std::vector<ImageQuanzitedInfo> ImageQuanzitedInfos;

static inline void AddImageQuantizedInfo(int word_id,
                                         float weight,
                                         const std::vector<std::string> &word_info,
                                         ImageQuanzitedInfos &infos) {
  ImageQuanzitedInfo info;
  info.word_id_ = word_id;
  info.weight_ = weight;
  info.word_info_ = word_info;
  infos.push_back(info);
}

static inline void PrintImageNormalizedInfo(int64_t pic_id,
                                            const ImageQuanzitedInfos &infos) {
  float sum = 0.f;
  for (size_t i = 0; i < infos.size(); ++i) {
    sum += infos[i].weight_;
  }
  for (size_t i = 0; i < infos.size(); ++i) {
    float weight = 0.f;
    if (sum > 0.f) {
      weight = sqrt(infos[i].weight_/sum);
    }
    std::cout << infos[i].word_id_ << "\t"
              << pic_id << "\t"
              << weight << "\t"
              << infos[i].word_info_[0] << "\t"
              << infos[i].word_info_[1] << std::endl;
  }
}

int main(int argc, char **argv, char **env) {
  Configure::GetInstance()->Init(argv[1]);

  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    return -1;
  }

  char buf[BUFFER_SIZE];
  int64_t cur_picid  = -1, prev_picid = -1;
  ImageQuanzitedInfos infos;
  while (fgets(buf, BUFFER_SIZE-1, stdin)) {
    try {
      std::vector<std::string> fields;
      boost::algorithm::split(fields, buf, boost::algorithm::is_any_of("\t"));
      if (fields.size() < 2 ) {
        LOGGER_ERROR(g_logger, "Wrong line: %s", buf);
        continue;
      }
      int64_t cur_picid = boost::lexical_cast<int>(fields[0]);
      if (cur_picid != prev_picid && prev_picid != -1) {
        PrintImageNormalizedInfo(prev_picid, infos);
        infos.clear();
      }
      else {
        int word_id = boost::lexical_cast<int>(fields[1]);
        float q = boost::lexical_cast<float>(fields[2]);
        std::vector<std::string> word_info;
        word_info.push_back(fields[3]);
        word_info.push_back(fields[4]);
        AddImageQuantizedInfo(word_id, q, word_info, infos);
      }
      prev_picid = cur_picid;
    }
    catch (...) {
      LOGGER_FATAL(g_logger, "Process line except: %s", buf);
      continue;
    }
  }

  if (infos.size() > 0) {
    PrintImageNormalizedInfo(cur_picid, infos);
  }

  return 0;
}
