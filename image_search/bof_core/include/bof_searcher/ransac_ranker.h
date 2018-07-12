#ifndef IMAGE_SEARCH_BOF_SEARCHER_RANSAC_RANKER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_RANSAC_RANKER_H_

#include <bof_searcher/spatial_ranker.h>

typedef struct _RansacRankerConf {
  int min_inliers_number_;
  int max_rank_pic_number_;
  ImageFeatureDetector *detector_;
  ImageFeatureDescriptorExtractor *extractor_;
  std::string image_service_server_;
  bool rank_ignore_same_result_;
  bool should_rank_;
  
  _RansacRankerConf() {
    min_inliers_number_ = 8;
    max_rank_pic_number_ = 100;
    detector_ = NULL;
    extractor_ = NULL;
    image_service_server_.clear();
    rank_ignore_same_result_ = true;
    should_rank_ = true;
  }
} RansacRankerConf;

class RansacRanker : public BOFSpatialRanker {
 private:
  class RansacSpatialInfo {
  public:
    RansacSpatialInfo() {
      points_.clear();
    }
    ~RansacSpatialInfo() {
      points_.clear();
    }
    std::vector<cv::KeyPoint> points_;
    cv::Mat descriptors_;
  };

 public:
  RansacRanker();
  ~RansacRanker();

  std::string GetName() const;
  BOFSpatialRanker* Clone();
  void SetConf(const void *conf);

  int Rank(const std::vector<cv::KeyPoint> &key_points,
           const cv::Mat &descriptors,
           SearchResult &result,
           const void *context = NULL);

 private:
  /**
   * 在rank之前估计仿射变换模型
   */
  int BeginRank(const SearchResult &result);
  /**
   * 检查是否有不符合空间一致性的结果，并将之剔除
   */
  int Check(SearchResult &result);
  /**
   * 将符合要求的结果按照空间相关性排序
   */
  int Ranking(SearchResult &result);
  /**
   * 结束后的清除工作
   */
  int EndRank(SearchResult &result);

  int CutHandleRedult(SearchResult &result);
  int GetResultLocalSpatialInfo(const SearchResultItem &item, RansacSpatialInfo &sinfo);

  int ComputeInliers(const RansacSpatialInfo *spatial_info);
  int GetRandomNumbers(const int max_value, const int num, std::vector<int> &numbers) const;

 private:
  int estimate_times_;
  int inlier_threshold_;
  int min_inliers_number_;
  int max_rank_pic_number_;
  bool rank_ignore_same_result_;
  bool should_rank_;
  SearchResult same_result_before_rerank_;
  std::vector<int> ransac_results_;
  std::vector<cv::KeyPoint> *querier_points_;
  cv::Mat *querier_descriptors_;
  std::vector<RansacSpatialInfo> result_spatial_info_;
  std::string image_service_get_if_;
  ImageFeatureDetector *detector_;
  ImageFeatureDescriptorExtractor *extractor_;
  std::string image_service_server_;
};

#endif
