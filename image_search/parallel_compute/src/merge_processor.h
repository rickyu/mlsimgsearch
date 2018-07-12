#ifndef IMAGE_SEARCH_SEARCHD_MERGE_PROCESSOR_H_
#define IMAGE_SEARCH_SEARCHD_MERGE_PROCESSOR_H_

#include <bof_searcher/searcher.h>
#include <bof_searcher/indexer.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <bof_feature_analyzer/feature_descriptor_extractor.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_feature_analyzer/bof_weighter.h>
#include <ExecMergeMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <boost/thread/thread.hpp>
#include <boost/array.hpp>
#include <array>
#include <boost/foreach.hpp>

typedef struct {
  Searcher searcher; 
  BOFQuantizeResultMap img_qdata;
} PartMerge;

typedef std::vector<PartMerge> PartMergeVector;
typedef boost::unique_future<SearchResultSimilar> UniqueFutureSearch;
typedef boost::promise<SearchResultSimilar> PromiseSearch;

extern CLogger& g_logger;
const size_t PART_NUM_MAX_SIZE = 30;

class MergeProcessor {
 public:
  MergeProcessor();
  ~MergeProcessor();

  int Init();
  int Merge(ExecMergeResponse& response, const ExecMergeRequest& request);

 private:
  int ParseRequest(const ExecMergeRequest& request);
  int ConstructResult(ExecMergeResponse& response);
  int PartitionData();
  int ExecMerge(boost::promise<SearchResultSimilar> *promise_result_similar);

 protected:
  int partition_num_;
  int he_threshold_;
  int begin_word_id_;
  int end_word_id_;
  SearchResultSimilar *result_similar_;
  float query_pow_;
  BOFQuantizeResultMap img_qdata_;
  PartMergeVector pmv_;
};

#endif
