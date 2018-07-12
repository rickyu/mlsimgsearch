#ifndef IMAGE_PROCESS_IMAGE_RECOGNISE_H_
#define IMAGE_PROCESS_IMAGE_RECOGNISE_H_

#include <image/constant.h>
#include <opencv2/opencv.hpp>
#include <vector>

class ImageRecognise {
 public:
  ImageRecognise();

  const image_result_t FaceDetect(const cv::Mat &img,
                                  const std::string &cascade_file,
                                  std::vector<cv::Rect> &rects);
                                  
  
 private:
  const image_result_t TryToDetectFace(const cv::Mat& img,
                                       cv::CascadeClassifier& cascade,
                                       std::vector<cv::Rect>& faces);
 private:
};

#endif
