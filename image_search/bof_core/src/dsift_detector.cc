#include <bof_feature_analyzer/dsift_detector.h>

DsiftFeatureDetector::DsiftFeatureDetector() {
}

DsiftFeatureDetector::~DsiftFeatureDetector() {
}

ImageFeatureDetector* DsiftFeatureDetector::Clone() {
  return new DsiftFeatureDetector;
}

std::string DsiftFeatureDetector::GetName() const {
  return "DSIFT";
}
void DsiftFeatureDetector::GetDescriptor(cv::Mat &desc) {
  desc = descriptor_;
}

int DsiftFeatureDetector::Detect(const cv::Mat &img, 
                                   std::vector<cv::KeyPoint> &points ) {

  VlDsiftFilter *vl;
  float *im = new float [img.rows * img.cols];
  uchar *p = const_cast<uchar *> (img.ptr<uchar>(0));
  int m,n;
  for ( int i = 0; i < img.rows*img.cols ; i++) {
    m = i % img.cols;
    n = i / img.cols;
    im[img.cols * n + m] = (float)(p[0] + p[1] + p[2])/3.0f; 
    p +=3;
  }

  vl = vl_dsift_new(img.rows,img.cols);
  vl_dsift_process(vl,im);

  int num = vl_dsift_get_keypoint_num (vl);
  int size = vl_dsift_get_descriptor_size (vl);

  const float * out_desc =  vl_dsift_get_descriptors(vl);
  
  descriptor_.create(num,size,CV_32FC1);
  for ( int i = 0 ;i < num ; i ++) {
    for ( int j = 0 ;j < size ; j++ ) {
      descriptor_.at<float>(i,j) = out_desc[size * i + j];
    }
  }
  vl_dsift_delete (vl);
  delete [] im;
  return 0;
}
