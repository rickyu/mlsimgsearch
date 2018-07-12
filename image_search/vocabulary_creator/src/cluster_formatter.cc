#include "kmeans_tools.h"
#include <opencv2/opencv.hpp>

/**
 * cluster_formatter cluster_src_data save_path
 */
int main(int argc, char **argv, char **env) {
  if (argc < 3) {
    printf("cluster_formatter cluster_src_data save_path\n");
    return -1;
  }

  std::string src_path(argv[1]), save_path(argv[2]);
  KmeansCentroids centroids;
  int ret = KmeansTools::LoadCentroids(src_path, centroids);
  if (0 != ret) {
    printf("Load centroids failed. (%d)\n", ret);
    return -2;
  }

  size_t rows = centroids.size(), cols = centroids[0].size();
  cv::Mat mat(rows, cols, CV_32F);
  for (size_t r = 0; r < rows; ++r) {
    for (size_t c = 0; c < cols; ++c) {
      mat.at<float>(r, c) = centroids[r][c];
    }
  }
  cv::Mat vocabulary;
  cv::normalize(mat, vocabulary);

  cv::FileStorage fs(save_path.c_str(), cv::FileStorage::WRITE);
  fs << "vocabulary" << mat;
  fs.release();

  return 0;
}
