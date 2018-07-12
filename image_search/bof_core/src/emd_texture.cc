#include <bof_feature_analyzer/emd_texture.h>

static const int S = 4;
static const int K = 6;

float EmdTexture::CalcImageMean(const IplImage *img) {
  cv::Mat mat(img);
  float sum = 0.f;
  for (int row = 0; row < mat.rows; ++row) {
    for (int col = 0; col < mat.cols; ++col) {
      sum += mat.at<float>(row, col);
    }
  }
  return sum / (mat.rows * mat.cols);
}

float EmdTexture::CalcImageVariance(const IplImage *img, float mean) {
  cv::Mat mat(img);
  float sum = 0.f;
  for (int row = 0; row < mat.rows; ++row) {
    for (int col = 0; col < mat.cols; ++col) {
      sum += pow(mat.at<float>(row, col), 2.f);
    }
  }
  float var = sum / mat.rows / mat.cols;
  var = var - (mean * mean);
  return sqrt(var);
}

int EmdTexture::CalcGaborFeatureVector(const IplImage *src, cv::Mat &mat) {
  double Sigma = 2*PI;
  double F = sqrt(2.0);

  mat.create(K*S, 2, CV_32F);
  for (int i = 1; i <= S; ++i) {
    for (int j = 1; j <= K; ++j) {
      GaborFilter *gabor = new GaborFilter;
      gabor->Init(PI*i/S, j, Sigma, F);

      IplImage *img = cvCreateImage(cvSize(src->width,src->height), IPL_DEPTH_32F, 1);
      gabor->conv_img(const_cast<IplImage*>(src), img, CV_GABOR_MAG);
      float mean = CalcImageMean(img);
      float var = CalcImageVariance(img, mean);
      mat.at<float>(i*j, 0) = mean;
      mat.at<float>(i*j, 1) = var;
      cvReleaseImage(&img);
      delete gabor;
    }
  }
  return 0;
}

float EmdTexture::CalcSimilar(const IplImage *src1, const IplImage *src2) {
  cv::Mat mat1, mat2;
  if (0 != CalcGaborFeatureVector(src1, mat1)) {
    return -1;
  }
  if (0 != CalcGaborFeatureVector(src2, mat2)) {
    return -2;
  }
  cv::Mat mat11, mat22;
  normalize(mat1, mat11);
  normalize(mat2, mat22);
  float dist = EMD(mat11, mat22, CV_DIST_C);

  return 1 - dist;
}

