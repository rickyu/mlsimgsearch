#include <bof_feature_analyzer/hessian_affine_feature_detector.h>

HessianFeatureDetector::HessianFeatureDetector() {
  detector_ = NULL;
}

HessianFeatureDetector::~HessianFeatureDetector() {
  if (detector_) {
    delete detector_;
    detector_ = NULL;
  }
}

ImageFeatureDetector* HessianFeatureDetector::Clone() {
  return new HessianFeatureDetector;
}

std::string HessianFeatureDetector::GetName() const {
  return "HESSIAN";
}
void HessianFeatureDetector::GetDescriptor(cv::Mat &desc) {
  desc = descriptor_;
}

int HessianFeatureDetector::Detect(const cv::Mat &img, 
                                   std::vector<cv::KeyPoint> &points ) {

  HessianAffineParams par;
  PyramidParams p;
  p.threshold = par.threshold;
  AffineShapeParams ap;
  ap.maxIterations = par.max_iter;
  ap.patchSize = par.patch_size;
  ap.mrSize = par.desc_factor;
  SIFTDescriptorParams sp;
  sp.patchSize = par.patch_size;

  cv::Mat image(img.rows, img.cols, CV_32FC1, cv::Scalar(0));
  float *out_p = image.ptr<float>(0);
  unsigned char *in  = const_cast<unsigned char *> (img.ptr<unsigned char>(0));
  for (size_t i = img.rows*img.cols ; i > 0; i--) {
    *out_p = (float(in[0]) + in[1] + in[2])/3.0f;
    out_p ++;
    in+=3;
  }

  
  detector_ = new AffineHessianDetector(image,p,ap,sp);
  detector_->detectPyramidKeypoints(image);
  descriptor_.create(detector_->keys_.size() , 128 ,CV_32FC1);
  float sum = 0.0f;
  for ( int i = 0 ; i < descriptor_.rows ; i++) {
    float *p = descriptor_.ptr<float>(i);
    sum = 0.0f;
    for ( int j = 0 ; j < 128 ; j ++ ) {
      p[j] = (float) detector_->keys_[i].desc[j];
      sum += p[j];
    }
    /*for ( int j = 0 ; j < 128 ; j ++ ) {
      p[j] = sqrt(p[j]/sum);
    }*/
  }

  return 0;
}
