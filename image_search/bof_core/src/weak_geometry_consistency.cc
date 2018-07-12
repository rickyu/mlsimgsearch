#include "bof_feature_analyzer/weak_geometry_consistency.h"

BOFWgc::BOFWgc() 
  : angle_bin_(0)
  ,histogram_max_num_(0) 
  ,histogram_total_(0) {
  histogram_max_num_ = 0;
}
BOFWgc::~BOFWgc() {
}
int BOFWgc::Init(int angle_bin) {
  if ( angle_bin < 20 ) {
    return -1;
  }

  angle_bin_ = angle_bin;
  int num = 360/angle_bin_;

  for ( int i = 0 ; i < num ; i ++ ) {
    angle_histogram_[i] = 0;
  }
  return 0;
}
int BOFWgc::BuildAngleHistogram(const std::string &str1,
                                const std::string &str2) {
  //每三位一个角度
  int length1 = str1.length();
  int length2 = str2.length();
  int angle1,angle2;
  int n_angle1 = length1 / 3;
  int n_angle2 = length2 / 3;

  if (0 == n_angle1 || 0 == n_angle2 ) {
    return -1;
  }
  int diff_angle = 0;
  for (int i = 0 ; i < n_angle1 ; i++ ) {
    angle1 = (str1[ 3 * i + 0 ] - '0') * 100 +
             (str1[ 3 * i + 1 ] - '0') * 10  +
             (str1[ 3 * i + 2 ] - '0');
    for ( int j = 0 ; j < n_angle2 ; j++ ) {
      angle2 = (str2[ 3 * j + 0 ] - '0') * 100 +
               (str2[ 3 * j + 1 ] - '0') * 10  +
               (str2[ 3 * j + 2 ] - '0');
      diff_angle = angle1 - angle2;
      if (diff_angle < 0 ) {
        diff_angle = - diff_angle;
      }
      BuildAngleHistogram(diff_angle);
    }
  }
  return 0;
}
int BOFWgc::BuildAngleHistogram(const int angle) {
  
  angle_histogram_[angle/angle_bin_] ++;
  histogram_total_ ++;
  if (angle_histogram_[angle/angle_bin_] > histogram_max_num_ ) {
    histogram_max_num_ = angle_histogram_[angle/angle_bin_];
  }
  return 0;
}

int BOFWgc::AverageHistogram() {
  int num  = 360 / angle_bin_ ;
  if ( num < 3 ) {
    return -2 ;
  }

  int tmp[1024];

  int average = 2 * histogram_total_/num ;
  histogram_max_num_ = 0;
  tmp[0] = angle_histogram_[0] + angle_histogram_[1] - average;
  if ( tmp[0] > histogram_max_num_ ) {
    histogram_max_num_ = tmp[0];
  }
  tmp[num -1] = angle_histogram_[num-2] + angle_histogram_[num -1] - average;
  if ( tmp[num -1] > histogram_max_num_ ) {
    histogram_max_num_ = tmp[num -1];
  }

  for ( int i = 1 ; i < num -1 ; i ++ ) {
    tmp[i] = angle_histogram_[i] + angle_histogram_[i - 1] + 
             angle_histogram_[i + 1] - average;
    if (tmp[i] > histogram_max_num_ ) {
      histogram_max_num_ = tmp[i];
    }
  }
  return 0;
}
int BOFWgc::Wgc(int &max ) {
  int ret = 0 ;
  //ret = AverageHistogram();
  if (ret != 0 ) {
    return -1;
  }
  max = histogram_max_num_;
  return 0;
}
int BOFWgc::TotalDiffAngle() const {
  return histogram_total_; 
}

