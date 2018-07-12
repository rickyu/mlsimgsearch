#ifndef IMAGE_SEARCH_WEAK_GEOMETRY_CONSISTENCY_H_
#define IMAGE_SEARCH_WEAK_GEOMETRY_CONSISTENCY_H_

#include <string>

class BOFWgc {
 public:
  BOFWgc();
  ~BOFWgc();

  int Init(int angle_bin);
  int Wgc(int &max);
  int TotalDiffAngle() const;
  int BuildAngleHistogram( const int angle );
  int BuildAngleHistogram( const std::string & str1,
                           const std::string & str2);
 private:
  int AverageHistogram();

  int angle_bin_;
  int angle_histogram_[18];
  int histogram_max_num_;
  int histogram_total_;
};
#endif
