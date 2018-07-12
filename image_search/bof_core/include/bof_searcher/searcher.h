#ifndef IMAGE_SEARCH_BOF_SEARCHER_SEARCHER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_SEARCHER_H_

#include <bof_searcher/indexer.h>
#include <bof_searcher/search_result.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_feature_analyzer/weak_geometry_consistency.h>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

typedef std::vector<std::string> RankInfo;

typedef struct _MergeSimilarInfo {
 public:
  _MergeSimilarInfo() {
    sim_ = 0.0f;
    pow_ = 0.0f;
    pic_id_ = 0;
  }
  float sim_;
  float pow_;
  int64_t pic_id_;
  BOFWgc wgc_angle_;
 private:
  friend class boost::serialization::access;
  template<class Archive> 
  void serialize(Archive &ar,
                 const unsigned int version) {
    ar & pic_id_;
    ar & sim_;
    ar & pow_;
  }
} MergeSimilarInfo;
typedef std::list<MergeSimilarInfo> SearchResultSimilar;

class Searcher {
 public:
  Searcher();
  virtual ~Searcher();

  virtual int SetQueryData(const BOFQuantizeResultMap *result);
  virtual int SetBeginBucketId(int begin_bucket_id);
  virtual int SetParam(int he_t);
  virtual int Merge(int max_bucket_count, std::vector<int64_t> &result);
  virtual void MergeByThread(const BOFQuantizeResultMap *query_data,
                             boost::promise<SearchResultSimilar> *promise_result);
  virtual int GetRankInfo(RankInfo &rank_info);
  virtual int Rank(const RankInfo &rank_info,
                   std::vector<int64_t> &result);
  virtual int GetResult(SearchResult &result);
  virtual int GetResultSimilar(SearchResultSimilar *&result_similar);
  virtual int SetResultSimilar(const SearchResultSimilar &result_similar);
  virtual int SetQueryPow(const float query_pow);
  virtual int GetQueryPow(float &query_pow);

 private:
  int MergeBucketResult(int word);
  unsigned long sum_he_time_;
  unsigned long sum_angle_time_;
 protected:
  BOFQuantizeResultMap *query_data_;
  SearchResultSimilar result_similar_;
  float query_pow_;
  int he_threshold_;
  SearchResult result_;
  static CLogger &logger_;
  int begin_bucket_id_;
};

#endif
