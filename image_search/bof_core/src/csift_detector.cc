#include <bof_feature_analyzer/csift_detector.h> 
CsiftFeatureDetector::CsiftFeatureDetector() {
}

CsiftFeatureDetector::~CsiftFeatureDetector() {
}

ImageFeatureDetector* CsiftFeatureDetector::Clone() {
  return new CsiftFeatureDetector;
}

std::string CsiftFeatureDetector::GetName() const {
  return "CSIFT";
}
void CsiftFeatureDetector::GetDescriptor(cv::Mat &desc) {
  desc = descriptor_;
}

int CsiftFeatureDetector::Detect(const cv::Mat &img, 
                                   std::vector<cv::KeyPoint> &points ) {
  float t = 1.4;
  cv::Mat mat_r,mat_g,mat_b;
  SplitMat(img ,mat_r,mat_g,mat_b);
  cv::Mat h(img.rows,img.cols,CV_8UC1);
  ColorInvariant(mat_r,mat_g,mat_b,t,h);

  cv::SiftFeatureDetector sift_detect;
  sift_detect.detect(h,points);
  cv::SiftDescriptorExtractor extractor;
  extractor.compute(h, points, descriptor_);
  return 0;
}
int CsiftFeatureDetector::getGaussianKernel(int size,float t,
                                            cv::Mat &kernel_mat,int x,int y) {
  cv::Mat kernel(size,size,CV_32FC1);
  if (size % 2 != 1 ) {
    return -1;
  }
  float sum = 0;
  float tmp = 0.0f;
  for ( int i = 0; i < size; i++ ) {
    for ( int j = 0 ; j < size ; j++) {
      int n = i - size/2;
      int m = j - size/2;
      if ( 1 == x ) {
        tmp = -n * exp ( -(n*n +m*m)/(2*t*t) );
      }else if (1 == y ) {
        tmp = -m * exp ( -(n*n +m*m)/(2*t*t) );
      } else if (0 == x && 0 == y ) {
        tmp = exp ( -(n*n +m*m)/(2*t*t) );
      }
      kernel.at<float>(i,j) = tmp; 
      sum += tmp;
    }
  }
  for ( int i = 0; i < size; i++ ) {
    for ( int j = 0 ; j < size ; j++) {
      kernel_mat.at<float>(i,j) = kernel.at<float>(i,j)/sum;
    }
  }
  return 0;
}

void CsiftFeatureDetector::GaussianDBlur(const cv::Mat &img, cv::Mat &src,
                                         int size, float t,int x ,int y) {
  cv::Mat kernel(size,size,CV_32FC1);
  src.create(img.rows,img.cols,CV_32FC1);
  getGaussianKernel( size ,t, kernel,x,y);
  cv::filter2D(img,src,src.depth(),kernel);
}
void CsiftFeatureDetector::SplitMat(const cv::Mat &img , cv::Mat &out1,  
                                    cv::Mat &out2, cv::Mat &out3) {
  int r,g,b;
  float e1,e2,e3;
  for ( int i = 0; i < img.rows ; i++ ) {
    const cv::Vec3b *p = img.ptr<cv::Vec3b>(i);
    float *out_p1 = out1.ptr<float>(i);
    float *out_p2 = out2.ptr<float>(i);
    float *out_p3 = out3.ptr<float>(i);
    for ( int j = 0 ; j < img.cols ; j ++ ) {
      b = (int)p[j][0];
      g = (int)p[j][1];
      r = (int)p[j][2];
      e1 = 0.06 * r + 0.63 * g + 0.27 * b;
      e2 = 0.3 * r + 0.04 * g - 0.35 * b;
      e3 = 0.34 * r - 0.6 * g + 0.17 * b;
      out_p1[j] = e1 ;
      out_p2[j] = e2 ;
      out_p3[j] = e3 ;
    }
  }
}
void CsiftFeatureDetector::ColorInvariant(const cv::Mat &e, const cv::Mat &el, 
                                          const cv::Mat &ell,float t,cv::Mat &h) {
  cv::Mat ex,elx,ellx;
  cv::Mat ey,ely,elly;
  cv::Mat eg,elg,ellg;
  int size = 7;
  GaussianDBlur(el,elx,size,t,1,0);
  GaussianDBlur(ell,ellx,size,t,1,0);

  GaussianDBlur(el,ely,size,t,0,1);
  GaussianDBlur(ell,elly,size,t,0,1);

  GaussianDBlur(el,elg,size,t,0,0);
  GaussianDBlur(ell,ellg,size,t,0,0);

  float hx,hy;
  for ( int i = 0 ; i < h.rows; i ++ ) {
    uchar * p = h.ptr<uchar>(i);
    for ( int j = 0 ; j < h.cols; j ++ ) {
      if (el.at<float>(i,j) > 0 || ell.at<float>(i,j)) {

        hx = (ell.at<float>(i,j) * elx.at<float>(i,j) - 
              el.at<float>(i,j) * ellx.at<float>(i,j)) /
              (el.at<float>(i,j) * el.at<float>(i,j) + 
              ell.at<float>(i,j) * ell.at<float>(i,j));
        hy = (ell.at<float>(i,j) * ely.at<float>(i,j) - 
              el.at<float>(i,j) * elly.at<float>(i,j)) /
              (el.at<float>(i,j) * el.at<float>(i,j) + 
               ell.at<float>(i,j) * ell.at<float>(i,j));
        p[j] = 255 * sqrt( hx*hx + hy*hy);
      }else {
        p[j] = 0;
      }
      if ( p[j] > 255 ) {
        p[j] = 255;
      }
    }
  }
}
