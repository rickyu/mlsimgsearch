#include <image/image_recognise.h>

ImageRecognise::ImageRecognise() {
}

const image_result_t ImageRecognise::FaceDetect(const cv::Mat &img,
                                                const std::string &cascade_file,
                                                std::vector<cv::Rect> &rects) {
  cv::CascadeClassifier cascade;
  
  if (!cascade.load(cascade_file)) {
    return IMGR_IMAGE_RECOG_LOAD_CASCADE_FAILED;
  }
  return TryToDetectFace(img, cascade, rects);
}

const image_result_t ImageRecognise::TryToDetectFace(const cv::Mat& img,
                                                     cv::CascadeClassifier& cascade,
                                                     std::vector<cv::Rect>& faces) {
  cv::Mat gray, smallImg( img.rows, img.cols, CV_8UC1 );

  cvtColor( img, gray, CV_BGR2GRAY );
  resize( gray, smallImg, smallImg.size(), 0, 0, CV_INTER_LINEAR );
  equalizeHist( smallImg, smallImg );

  cascade.detectMultiScale( smallImg, faces,
                            1.1, 2, 0
                            //|CV_HAAR_FIND_BIGGEST_OBJECT
                            //|CV_HAAR_DO_ROUGH_SEARCH
                            |CV_HAAR_SCALE_IMAGE,
                            cv::Size(30, 30) );
  return IMGR_SUCCESS;
}
