#ifndef IMAGE_SEARCH_SEARCHD_EXEC_THREADS_H_
#define IMAGE_SEARCH_SEARCHD_EXEC_THREADS_H_

#include <boost/thread/thread.hpp>
#include <boost/array.hpp>
#include <array>
#include <boost/foreach.hpp>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_searcher/searcher.h>
#include <common/configure.h>


typedef struct {
  Searcher searcher; 
  BOFQuantizeResultMap img_qdata;
} PartMerge;

typedef std::vector<PartMerge> PartMergeVector;
typedef boost::unique_future<SearchResultSimilar> UniqueFutureSearch;
typedef boost::promise<SearchResultSimilar> PromiseSearch;

class ExecThreads {
 public:
  ExecThreads(){};

 public:
  int BeginMerge(int step,
                 BOFQuantizeResultMap img_qdata,
                 PartMergeVector &pmv);
  int ExecMerge(int max_bucket_count,
                int He_Threshold,
                PartMergeVector &pmv,
                boost::promise<SearchResultSimilar> *promise_result_similar);
 private:
  int step_;

  BOFQuantizeResultMap img_qdata_;
  SearchResultSimilar result_;
  PartMergeVector part_merge_vector_;
};

#endif
