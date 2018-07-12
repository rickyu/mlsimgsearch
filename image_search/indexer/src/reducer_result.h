#ifndef IMAGE_SEARCH_INDEXER_REDUCER_RESULT_H_
#define IMAGE_SEARCH_INDEXER_REDUCER_RESULT_H_

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <bof_searcher/indexer.h>
#include <bof_searcher/index_hinter.h>
#include <bof_feature_analyzer/bof_weighter.h>

class ReducerBucketResult {
 public:
  ReducerBucketResult(int bucket_id);
  ~ReducerBucketResult();

  // for reducer
  const int Init();
  const int AddLine(const int64_t pic_id,
                    const float q,
                    const std::string &words1, 
                    const std::string &words2);
  void PrintResult();

 private:
  ReducerBucketResult() {}

 private:
  int bucket_id_;
  BOFIndexBucket *bucket_;
};


class ReducerResultResolver {
 public:
  ReducerResultResolver(const int bucket_count);
  ~ReducerResultResolver();

  const int Init(const std::string &index_data_path,
                 const std::string &index_hint_path);
  const int ParseFile(const std::string &file_path, 
                      BOFWeighter * weighter);
  void SaveIndex();

 private:
  const int Normalize(const std::string &image_quantized_info,
                      const BOFWeighter *weighter,
                      int bucket_id,
                      int feature_num,
                      float &q);

 private:
  std::fstream fs_data_;
  std::fstream fs_hint_;
  int64_t cur_file_pos_;
  IndexHinter hinter_;
};

#endif
