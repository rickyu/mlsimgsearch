#include "image/opencv_process.h"


OpencvProcess::OpencvProcess()
  : cv_image_(NULL) {
}

OpencvProcess::OpencvProcess(const IplImage *image)
  : cv_image_(const_cast<IplImage*>(image)) {
}

OpencvProcess::~OpencvProcess() {
  if (cv_image_) {
    cvReleaseImage(&cv_image_);
  }
}

const ImageProcess* OpencvProcess::Clone() const {
  IplImage *new_img = cvCloneImage(cv_image_);
  if (NULL == new_img) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::Clone failed");
    return NULL;
  }
  OpencvProcess *cv_proc = new OpencvProcess(new_img);
  if (NULL == cv_proc) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::Clone failed");
    cvReleaseImage(&new_img);
    return NULL;
  }
  return cv_proc;
}

const image_result_t OpencvProcess::Init(const char *context) {
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::LoadFile(const char *path) {
  cv_image_ = cvLoadImage(path);
  if (NULL == cv_image_) {
    LOGGER_ERROR(logger_, "act=OpencvProcess:LoadFile failed, path=%s", path);
    return IMGR_ERROR_FILE_OPEN;
  }
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::LoadMem(const void *content,
                                            unsigned int len) {
  CvMat mat = cvMat(1, len, CV_32FC1, const_cast<void*>(content));
  cv_image_ = cvDecodeImage(&mat, CV_LOAD_IMAGE_COLOR);
  if (NULL == cv_image_) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::LoadMem failed");
    return IMGR_IMAGE_PARSE_FAILED;
  }
  return IMGR_SUCCESS;
}


const image_result_t OpencvProcess::SaveFile(const char *path) {
  int save_ret = cvSaveImage(path, cv_image_);
  if (save_ret) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::LoadFile failed, path=%s", path);
    return IMGR_IMAGE_IO_FAILED;
  }
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::GetContent(void **img_content,  //图片数据
                                               unsigned int *len,  //图片数据长度
                                               char *ext,  
                                               int jpgQuality,  //图片质量默认90
                                               bool keepAnimation /*= false*/) {
  if (jpgQuality <= 0 && jpgQuality > 100)
    jpgQuality = 90;
  int params[2] = {CV_IMWRITE_JPEG_QUALITY, 90};
  CvMat *mat = cvEncodeImage(".jpg", cv_image_, params);
  if (NULL == mat) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::GetContent failed");
    return IMGR_IMAGE_ENCODE_FAILED;
  }
  cvGetRawData(mat, (uchar**)img_content, (int*)len);
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::Resize(unsigned int width,
                                           unsigned int height) {
  CvSize dst_size;
  dst_size.width = width;
  dst_size.height = height;
  IplImage *dst = cvCreateImage(dst_size, cv_image_->depth, cv_image_->nChannels);
  if (NULL == dst) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::Resize create image failed");
    return IMGR_ERROR_OUTOFMEM;
  }
  cvResize(cv_image_, dst, CV_INTER_CUBIC/*CV_INTER_AREA*/);
  cvReleaseImage(&cv_image_);
  cv_image_ = dst;
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::Crop(unsigned int dx,
                                         unsigned int dy,
                                         unsigned int width,
                                         unsigned int height) {
  CvRect rect = {dx, dy, width, height};
  cvSetImageROI(cv_image_, rect);
  CvSize dst_size = {width, height};
  IplImage *dst = cvCreateImage(dst_size, cv_image_->depth, cv_image_->nChannels);
  if (NULL == dst) {
    LOGGER_ERROR(logger_, "act=OpencvProcess::Resize create image failed");
    return IMGR_ERROR_OUTOFMEM;
  }
  cvCopy(cv_image_, dst, 0);
  cvReleaseImage(&cv_image_);
  cv_image_ = dst;
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::CutPicture(std::vector<ImageProcess *> &img_vec)
{
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::AutoCut() {
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::Scale(unsigned int width,
                                          unsigned int height,
                                          bool upscale) {
  unsigned int height_out = 0;
  unsigned int width_out = 0;
  CalcScaleSize(static_cast<float>(GetWidth()),
                static_cast<float>(GetHeight()),
                static_cast<float>(width),
                static_cast<float>(height),
                width_out, height_out, upscale);
  return Resize(width_out, height_out);
}

const image_result_t OpencvProcess::ChangeHSB(float hue,
                                              float saturation,
                                              float bright) {
  return IMGR_SUCCESS;
}

const int OpencvProcess::Sharp(const cv::Mat &src, cv::Mat &sharpened,
			       float amount, float radius, float threshold) {
  cv::Mat blurred;
  amount /= 100;
  threshold /= 256;
  GaussianBlur(src, blurred, cv::Size(), radius, radius);
  cv::Mat lowContrastMask = abs(src - blurred) < threshold;
  sharpened = src*(1+amount) + blurred*(-amount);
  src.copyTo(sharpened, lowContrastMask);
  return 0;
}

const image_result_t OpencvProcess::SharpEx(float amount,
                                            float radius,
                                            float threshold) {
  cv::Mat sharpened;
  IplImage sharp_dst;
  IplImage *dst, *dst_rgb;
  IplImage *dst1;
  IplImage *dst2;
  IplImage *dst3;
  IplImage *lab;

  dst1 = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 1);
  dst2 = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 1);
  dst3 = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 1);
  sharpened = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 1);
  lab = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 3);
  dst = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 3);
  dst_rgb = cvCreateImage(cvSize(cv_image_->width, cv_image_->height), IPL_DEPTH_8U, 3);
  
  cvCvtColor(cv_image_, lab, CV_RGB2Lab);
  cvSplit(lab, dst1, dst2, dst3, NULL);
  Sharp(dst1, sharpened, amount, radius, threshold);
  sharp_dst = sharpened;
  cvMerge(&sharp_dst, dst2, dst3, 0, dst);
  cvCvtColor(dst, dst_rgb, CV_Lab2RGB);
  cvReleaseImage(&cv_image_);
  cvReleaseImage(&dst);
  cvReleaseImage(&dst1);
  cvReleaseImage(&dst2);
  cvReleaseImage(&dst3);
  cv_image_ = dst_rgb;

  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::ChannelSharpEx(int channel,
                                                   float amount,
                                                   float radius,
                                                   float threshold) {
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::ImageFilter(const image_curve imgCurve,void *context)
{
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::Type(const int imagetype ){
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::Colorize(unsigned int opacity,
                                              const int red,
                                              const int green,
                                              const int blue){
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::Blur(const double radius,
                                          const double sigma){
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::SetImageFormat(const char *format) {
  return IMGR_SUCCESS;
}
const image_result_t OpencvProcess::Contrast(float amount) {
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::AddWaterMark(const ImageProcess *img,
                                                 unsigned int x,
                                                 unsigned int y,
                                                 bool transparancy) {
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::ComposePics(const ImageProcess *img,
                                                unsigned int x,
                                                unsigned int y) {
  CvRect rect = cvRect(x, y, img->GetWidth(), img->GetHeight());
  cvSetImageROI(cv_image_, rect);
  cvCopy(static_cast<IplImage*>(img->GetImage()), cv_image_);
  cvResetImageROI(cv_image_);
  return IMGR_SUCCESS;
}

const image_result_t OpencvProcess::StretchPic(unsigned int width,
                                               unsigned int height) {
  return IMGR_SUCCESS;
}

const bool OpencvProcess::IsMatte() const {
  return false;
}
const bool OpencvProcess::IsLoaded() const {
  return cv_image_ ? true : false;
}

const unsigned int OpencvProcess::GetWidth() const {
  return cv_image_->width;
}

const unsigned int OpencvProcess::GetHeight() const {
  return cv_image_->height;
}
const int OpencvProcess::GetColorSpace() const {
  return -1;
}


const std::string OpencvProcess::GetExtension() const {
  return "";
}

const image_result_t OpencvProcess::SetQuality(unsigned int quality){
  return IMGR_ERROR_PROCESS;
}

const image_result_t OpencvProcess::SetImage(const void * img) {
  return IMGR_ERROR_PROCESS;
}
void *OpencvProcess::GetImage() const {
  return cv_image_;
}

const image_result_t OpencvProcess::CalcScaleSize(float owidth,
                                                  float oheight,
                                                  float width,
                                                  float height,
                                                  unsigned int &width_out,
                                                  unsigned int &height_out,
                                                  bool upscale) {
  if (!owidth || !oheight) {
    return IMGR_ERROR_PROCESS;
  }
  float aspect = oheight / owidth;
  if (upscale) {
    width = width ? width : height / aspect;
    height = height ? height : width / aspect;
  } else {
    width = width ? width : 9999999;
    height = height ? height : 9999999;
    if (round(width) >= owidth && round(height) >= oheight) {
      width_out = (unsigned int)owidth;
      height_out = (unsigned int)oheight;
      return IMGR_SUCCESS;
    }
  }
  if (aspect < height / width) {
    height = width * aspect; 
  } else {
    width = height / aspect;
  }
  width_out = (unsigned int)width;
  height_out = (unsigned int)height;
  return IMGR_SUCCESS;
}
