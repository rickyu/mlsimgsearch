#ifndef IMAGE_PROCESS_IMAGE_OPENCV_PROCESS_H_
#define IMAGE_PROCESS_IMAGE_OPENCV_PROCESS_H_


#include <image/image_base.h>
#include <opencv2/opencv.hpp>


class OpencvProcess : public ImageProcess {
 public:
  OpencvProcess();
  OpencvProcess(const IplImage *image);
  ~OpencvProcess();

 public:
  const image_result_t Init(const char *context);
  
  const ImageProcess* Clone() const;

  const image_result_t LoadFile(const char * path);

  const image_result_t LoadMem(const void *content,
                               unsigned int len);

  const image_result_t SaveFile(const char *path);

  const image_result_t GetContent(void **img_content,
                                  unsigned int *len,
                                  char *ext,
                                  int jpgQuality = 90,
                                  bool keepAnimation = false);

  const image_result_t Resize(unsigned int width, unsigned int height);

  const image_result_t Crop(unsigned int dx,
                            unsigned int dy,
                            unsigned int width,
                            unsigned int height);
  const image_result_t CutPicture(std::vector<ImageProcess *> &img_vec);
  const image_result_t AutoCut();

  const image_result_t Scale(unsigned int width,
                             unsigned int height,
                             bool upscale);

  
  const image_result_t ChangeHSB(float hue,
                                 float saturation,
                                 float bright);

  const image_result_t SharpEx(float amount,
                               float radius,
                               float threshold);
  const image_result_t ChannelSharpEx(int channel,
                                      float amount,
                                      float radius,
                                      float threshold);

  const image_result_t Contrast(float amount);

  const image_result_t AddWaterMark(const ImageProcess *img,
                                    unsigned int x,
                                    unsigned int y,
                                    bool transparancy);

  const image_result_t ComposePics(const ImageProcess *img,
                                   unsigned int x,
                                   unsigned int y);

  const image_result_t StretchPic(unsigned int width,
                                  unsigned int height); 

  const image_result_t ImageFilter(const image_curve imgCurve,void *context);

  const image_result_t Type(const int imagetype );

  const image_result_t Colorize(unsigned int opacity,
                                const int red,
                                const int green,
                                const int blue) ;

  const image_result_t Blur(const double radius =0.0,
                            const double sigma  =1.0);

  const image_result_t SetImageFormat(const char *format);

  const image_result_t SetQuality(unsigned int quality);

  const bool IsLoaded() const;

  const bool IsMatte() const;

  const int GetColorSpace() const;

  const unsigned int GetHeight() const;

  const unsigned int GetWidth() const;

  const std::string GetExtension() const;

  const image_result_t SetImage(const void * img);

  void* GetImage() const;

 private:
  const image_result_t CalcScaleSize(float owidth,
                                     float oheight,
                                     float width,
                                     float height,
                                     unsigned int &width_out,
                                     unsigned int &height_out,
                                     bool upscale);
  const int Sharp(const cv::Mat &src, cv::Mat &sharpened,
		  float amount, float radius, float threshold);

 private:
  IplImage *cv_image_;
};

#endif
