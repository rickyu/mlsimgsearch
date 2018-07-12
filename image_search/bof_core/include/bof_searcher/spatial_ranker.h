#ifndef IMAGE_SEARCH_BOF_SEARCHER_SPATIAL_RANKER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_SPATIAL_RANKER_H_

#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_searcher/search_result.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <bof_feature_analyzer/feature_descriptor_extractor.h>
#include <bof_common/object_selector.h>

class BOFSpatialRanker {
 public:
  BOFSpatialRanker();
  virtual ~BOFSpatialRanker();

  virtual std::string GetName() const = 0;
  virtual BOFSpatialRanker* Clone() = 0;
  virtual void SetConf(const void *conf) = 0;

  /**
   * @brief 剔除不符合要求的结果，并按照空间校验结果排序
   * @param key_points [in] 查询图像的特征点
   * @param descriptors [in] 查询图像的特征向量
   * @param result [in|out] Merge后的结果，一些不符合要求的结果会被排除，
   *                        符合要求的结果会被按照空间检验后重新排序
   * @return 如果成功返回0， 否则返回小于0的数
   */
  virtual int Rank(const std::vector<cv::KeyPoint> &key_points,
                   const cv::Mat &descriptors,
                   SearchResult &result,
                   const void *context = NULL) = 0;

 protected:
  static CLogger &logger_;
};

typedef ObjectSelector<BOFSpatialRanker> BOFSpatialRankerSelector;

#endif
