/**
 * @brief:
 * @version:
 * @author:
 */

#ifndef IMAGE_PROCESS_IMAGE_BASE_H_
#define IMAGE_PROCESS_IMAGE_BASE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <utils/logger.h>
#include <image/constant.h>

#define IMAGE_PROCESS_NAME "imageproc"
typedef enum {
  IMG_EXT_NONE,
  IMG_EXT_TOLOWER,
  IMG_EXT_TOUPPER
} image_ext_type_t;

typedef void (*image_curve)(int &red_value,int &green_value,int &blue_value,void * context);
/**
 * Image interface
 */
class ImageProcess {
public:
  ImageProcess();
  virtual ~ImageProcess();

public:
  ////////////////////// I/O ///////////////////
  /**
  * @brief Init class
  * @param 
  * @return 
  */
  virtual const image_result_t Init(const char *context) = 0;
  /**
   * @brief
   * @return
   */
  virtual const ImageProcess* Clone() const = 0;
  /**
   * @brief Load image from file.
   * @param path [in]
   * @return 
   */
  virtual const image_result_t LoadFile(const char *path) = 0;
  /**
   * @brief Load image from memory.
   * @param content [in]
   * @param len [in]
   * @return 
   */
  virtual const image_result_t LoadMem(const void *content,
                                       unsigned int len) = 0; 
  /**
   * @brief Save image.
   * @param path [in]
   * @return 
   */
  virtual const image_result_t SaveFile(const char *path) = 0;
  /**
   * @brief Get content.
   * @param img_content [out]
   * @param len [out]
   * @return 
   */
  virtual const image_result_t GetContent(void **img_content,
                                          unsigned int *len,
                                          char *ext,
                                          int jpgQuality = 90,
                                          bool keepAnimation = false) = 0;

  /**
   * @brief if have matte channel
   * @return 
   */

  virtual const bool IsMatte() const = 0;

  /**
   * @brief
   * @return
   */
  virtual const bool IsLoaded() const = 0;

  //////////////////////// Property ////////////////////////
  /**
   * @brief 
   * @param ext [out]
   */
  virtual const std::string GetExtension() const = 0;
  /**
   * @brief
   * @return
   */
  virtual const int GetColorSpace() const = 0;
  /**
   *
   */
  virtual const unsigned int GetWidth() const = 0;
  /**
   * @brief
   * @return
   */
  virtual const unsigned int GetHeight() const = 0;

  ////////////////// Operations /////////////////////
  /**
   * @brief
   * @param width [in]
   * @param height [in]
   * @return
   */
  virtual const image_result_t Resize(unsigned int width,
                                      unsigned int height) = 0;
  /**
   * @brief
   * @param dx [in]
   * @param dy [in]
   * @param width [in]
   * @param height [in]
   * @return
   */
  virtual const image_result_t Crop(unsigned int dx,
                                    unsigned int dy,
                                    unsigned int width,
                                    unsigned int height) = 0;
  /**
   * @brief
   * @param width [in]
   * @param height [in]
   * @param upscale [in]
   * @return
   */
  virtual const image_result_t Scale(unsigned int owidht,
                                     unsigned int oheight,
                                     bool upscale) = 0;
  /**
   * @brief
   * @param hue [in]
   * @param saturation [in]
   * @param bright [in]
   * @return
   */
  virtual const image_result_t ChangeHSB(float hue,
                                         float saturation,
                                         float bright) = 0;

  /*
   * @brief
   * @param img_vec  vector ImageProcess
   *
   */
  virtual const image_result_t CutPicture(std::vector<ImageProcess *> &img_vec) = 0;
  /**
   *@brief
   *@auto cut image
   */
  virtual const image_result_t AutoCut() = 0;
  /**
   * @brief
   * @param amount [in]
   * @param radius [in]
   * @param threshold [in]
   * @return
   */
  virtual const image_result_t SharpEx(float amount,
                                       float radius,
                                       float threshold) = 0;
  virtual const image_result_t ChannelSharpEx(int channel,
					      float amount,
					      float radius,
					      float threshold) = 0;

  /**
   * @brief
   * @param sharpen [in]
   * @return
   */
  virtual const image_result_t Contrast(float amount) = 0;
  /**
   * @brief
   * @param img [in]
   * @param x [in]
   * @param y [in]
   * @param transparancy [in]
   * @return
   */
  virtual const image_result_t AddWaterMark(const ImageProcess *img,
                                            unsigned int x,
                                            unsigned int y,
                                            bool transparancy) = 0;
  /**
   * @brief
   * @param img [in]
   * @param x [in]
   * @param y [in]
   */
  virtual const image_result_t ComposePics(const ImageProcess *img,
                                           unsigned int x,
                                           unsigned int y) = 0;

   /** 
   * @brief
   * @param set img
   * */

  virtual const image_result_t SetImage(const void * img) = 0;

  /**
  *
  *set quality
  **/
  virtual const image_result_t SetQuality(unsigned int quality) =0;

   /** 
   * @brief
   * @param img
   * */

  virtual void *GetImage() const = 0;
  /**
   * @brief
   * @param width [in]
   * @param height [in]
   * */
  virtual const image_result_t StretchPic(unsigned int width,
                                          unsigned int height) = 0;
  /**
   * 
   * 
   * */
  virtual const image_result_t ImageFilter(const image_curve imgCurve,void *context) =0;
  /*
   *
   *
   * */
  virtual const image_result_t Type(const int imagetype ) =0;
  /*
   *
   *
   * */
  virtual const image_result_t Colorize(unsigned int opacity,
                                        const int red,
                                        const int green,
                                        const int blue) =0;
  /*
   *
   *
   * */
  virtual const image_result_t Blur(const double radius =0.0,
                                    const double sigma  =1.0) =0;

  virtual const image_result_t SetImageFormat(const char *format) =0;
protected:
  static CLogger& logger_;
};


#endif
