#ifndef IMAGE_SEARCH_SEARCHD_COMMON_SEARCH_H_
#define IMAGE_SEARCH_SEARCHD_COMMON_SEARCH_H_

#include "service_base.h"
#include <bof_searcher/searcher.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <bof_feature_analyzer/feature_descriptor_extractor.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_feature_analyzer/bof_weighter.h>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/foreach.hpp>

class CommonSearch : public SearchServiceBase {
 private:
  typedef struct {
    std::string ip_;
    unsigned int port_;
  } IPAddress;
  typedef struct {
    int begin_id_;
    int end_id_;
    std::vector<IPAddress> servers_;
  } MergeConf;
  typedef struct {
    std::vector<IPAddress> servers_;
    BOFQuantizeResultMap img_qdata_;
  } ServerToData;
  typedef std::vector<MergeConf> ExecMergeConf;
  typedef std::vector<ServerToData> ServersToDatas;
 public:
  CommonSearch();
  ~CommonSearch();

  SearchServiceType get_type() const;
  SearchServiceBase* Clone();
  int Init();
  void Clear();
    
 protected:
  int Process(const ImageSearchRequest &request, std::string &result);

 private:
  int InitExecMergeConf();
  void ResultFormat(const SearchResult &sr, std::string &result);

  static int SpatialRankerCallback(void *context, int thread_index);
  int SpatialRanker(int thread_index);
  int DispatchData(const BOFQuantizeResultMap &img_qdata);
  static int ExecMergeCallback(void *context, int thread_index);
  int ExecMerge(int thread_index);
  int MergeThreadsResult(const SearchResultSimilar *thread_result);
  int ClearMergeData();

  //int ExecMerge(const BOFQuantizeResultMap &img_qdata,
  //              SearchResultSimilar &result_similar);

 protected:
  Searcher *searcher_;
  ImageFeatureDetector *detector_;
  ImageFeatureDescriptorExtractor *extractor_;
  BOFQuantizer *quantizer_;
  BOFWeighter *weighter_;
  CvMat *cur_mat_;
  char *request_image_data_;
  size_t request_image_data_len_;
  std::string spatial_ranker_name_;
  std::string image_service_server_;
  int he_threshold_;
  bool need_extractor_;
  static int searcher_ref_count_;
  std::vector<cv::KeyPoint> points_;
  cv::Mat descriptors_;
  SearchResult search_result_;
  int spatial_request_num_;
  int spatial_request_parts_;
  int max_response_result_count_;
  std::vector<IPAddress> spatial_ranker_servers_;

  //分布式合并索引相关配置
  int exec_merge_request_num_;
  ServersToDatas servers_to_datas_;
  ExecMergeConf exec_merge_conf_;
  SearchResultSimilar result_similar_;
  float query_pow_;
  boost::mutex io_mutex_;
};

#endif
