#ifndef __VIRGO_FEATURE_RANSAC_RANKER_H_
#define __VIRGO_FEATURE_RANSAC_RANKER_H_ 1

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>

class RansacRanker {
 public:
  RansacRanker();
  float ComputeRank(const std::vector<cv::KeyPoint> &query_points,
                    const cv::Mat &query_descriptors,
                    const std::vector<cv::KeyPoint>& doc_points,
                    const cv::Mat& doc_descriptors) ;
  void set_estimate_times(int v) {
    estimate_times_ = v;
  }
  int estimate_times() const { 
    return estimate_times_;
  }
  void set_inlier_threshold(int v) {
    inlier_threshold_ = v;
  }
  int inlier_threshold() const {
    return inlier_threshold_;
  }

 private:
  int ComputeInliers(const std::vector<cv::KeyPoint> &query_points,
                     const cv::Mat &query_descriptors,
                     const std::vector<cv::KeyPoint>& doc_points,
                     const cv::Mat& doc_descriptors) ;
  int GetRandomNumbers(const int max_value, const int num,
                       std::vector<int> &numbers) const;

 private:
  int estimate_times_;
  int inlier_threshold_;
  int min_inliers_number_;
};

#endif // __VIRGO_FEATURE_RANSAC_RANKER_H_
