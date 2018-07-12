#include <sys/time.h>
#include <opencv2/legacy/legacy.hpp>
#include "ransac_ranker.h"

RansacRanker::RansacRanker()
  : estimate_times_(1000)
  , inlier_threshold_(3)
  , min_inliers_number_(8) {
}

float RansacRanker::ComputeRank(const std::vector<cv::KeyPoint> &query_points,
                       const cv::Mat &query_descriptors,
                       const std::vector<cv::KeyPoint>& doc_points,
                       const cv::Mat& doc_descriptors) {
  int inliers_number = ComputeInliers(query_points, query_descriptors,
                doc_points, doc_descriptors);
  float sim = 0.0f;
  if (inliers_number >= min_inliers_number_) {
    sim = (float)inliers_number / query_points.size();
  }
  return sim;

}


int RansacRanker::GetRandomNumbers(const int max_value,
                                   const int num,
                                   std::vector<int> &numbers) const {
  numbers.clear();
  if (num > max_value) {
    return -1;
  }
  if (max_value <= 0 || num <= 0) {
    return -1;
  }
  int got_num = 0;
  while (got_num < num) {
    timeval t;
    gettimeofday(&t, NULL);
    srand((unsigned)t.tv_usec);
    int rnum = rand() % max_value;
    bool exist = false;
    for (size_t i = 0; i < numbers.size(); ++i) {
      if (numbers[i] == rnum) {
        exist = true;
      }
    }
    if (!exist) {
      numbers.push_back(rnum);
      ++got_num;
    }
  }
  return 0;
}

int RansacRanker::ComputeInliers(const std::vector<cv::KeyPoint>& query_points,
                                 const cv::Mat& query_descriptors,
                                 const std::vector<cv::KeyPoint>& doc_points,
                                 const cv::Mat& doc_descriptors) {
  // 特征点匹配
  std::vector<cv::DMatch> matches;
  cv::BruteForceMatcher<cv::L2<float>> matcher;
  matcher.match(query_descriptors, doc_descriptors, matches);
  std::vector<cv::Point2f> pmat1, pmat2;
  for (size_t i = 0; i < matches.size(); ++i) {
    pmat1.push_back(query_points[matches[i].queryIdx].pt);
    pmat2.push_back(doc_points[matches[i].trainIdx].pt);
  }
  // RANSAC模型匹配
  int inliers_number = 0;
  cv::Mat best_affine;
  for (int i = 0; i < estimate_times_; ++i) {
    int cur_inliers_number = 0;
    std::vector<int> random_nums;
    if (GetRandomNumbers(matches.size(), 3, random_nums) != 0) {
      return 0;
    }
    cv::Point2f ps1[3], ps2[3];
    for (size_t i = 0; i < random_nums.size(); ++i) {
      ps1[i] = pmat1[random_nums[i]];
      ps2[i] = pmat2[random_nums[i]];
    }
    cv::Mat mat_affine( 2, 3, CV_32FC1 );
    mat_affine = cv::getAffineTransform(ps1, ps2);

    cv::Mat A = mat_affine.rowRange(0, 2).colRange(0, 2);
    cv::Mat tmp;
    A.convertTo(tmp, CV_32F);
    A = tmp;
    cv::Mat B = mat_affine.rowRange(0, 2).colRange(2, 3);
    B.convertTo(tmp, CV_32F);
    B = tmp;
    for (size_t i = 0; i < matches.size(); ++i) {
      cv::Mat mat_query(2, 1, CV_32F),
        mat_transformed = cv::Mat::zeros(mat_query.rows,
                                         mat_query.cols,
                                         mat_query.type());
      mat_query.at<float>(0, 0) = pmat1[i].x;
      mat_query.at<float>(1, 0) = pmat1[i].y;
      mat_transformed = A * mat_query + B;
      float dist = sqrt(pow(pmat2[i].x-mat_transformed.at<float>(0, 0), 2.f)
                        + pow(pmat2[i].y-mat_transformed.at<float>(1, 0), 2.f));
      if (dist <= inlier_threshold_) {
        ++cur_inliers_number;
      }
    }
    if (cur_inliers_number > inliers_number) {
      inliers_number = cur_inliers_number;
      best_affine = mat_affine;
    }
  }
  return inliers_number;
}
