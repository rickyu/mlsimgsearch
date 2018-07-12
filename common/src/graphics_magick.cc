#include "image/graphics_magick.h"
#include <string.h>
bool GraphicsMagick::global_inited_ = false;

static const image_result_t calc_scale_size(float owidth,
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
    width = width ? width : height / aspect;//源图大小
    height = height ? height : width / aspect;
  } else {
    width = width ? width : 9999999;
    height = height ? height : 9999999;
    if (round(width) >= owidth && round(height) >= oheight) {//放大不缩放
      width_out = (unsigned int)owidth;
      height_out = (unsigned int)oheight;
      return IMGR_SUCCESS;
    }
  }
  if (aspect < height / width) {
    height = width * aspect;//高度缩放 
  } else {
    width = height / aspect;
  }
  width_out = (unsigned int)width;
  height_out = (unsigned int)height;
  return IMGR_SUCCESS;
}

GraphicsMagick::GraphicsMagick() {
  image_obj_ = NULL;
  image_content_ = NULL;
}

GraphicsMagick::~GraphicsMagick() {
  if (image_obj_ != NULL) {
    delete image_obj_;
    image_obj_ = NULL;
  }
  if (image_content_ != NULL) {
    delete image_content_;
    image_content_ = NULL;
  }
}

const ImageProcess* GraphicsMagick::Clone() const {
  if (image_obj_ == NULL) {
    return NULL;
  } 
  GraphicsMagick *imgnew = new GraphicsMagick;
  try {
    imgnew->image_obj_ = new Magick::Image(*image_obj_);
    return imgnew;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=Clone err=Magick::Exception %s", e.what());
    return NULL; 
  }
}

const image_result_t GraphicsMagick::Init(const char *context) {
  if (!global_inited_) {
    global_inited_ = true;
    Magick::InitializeMagick(context);
    return IMGR_SUCCESS;
  }
  return IMGR_SUCCESS;
}

const image_result_t GraphicsMagick::LoadFile(const char *path) {
  image_result_t ret = IMGR_SUCCESS;
  if (!path || !strlen(path)) {
    LOGGER_ERROR(logger_, "LoadFile failed, empty path");
    return IMGR_ERROR_FILE_OPEN;
  }
  try {
    image_obj_ = new Magick::Image;
    ret = IMGR_ERROR_FILE_OPEN;
    //image_obj_->ping(path);
    image_obj_->read(path);
    ret = IMGR_SUCCESS;
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=LoadFile err=Magick::Exception %s,File_path=%s", e.what(), path);
    if (image_obj_) {
      delete image_obj_;
      image_obj_ = NULL;
    }
    ret = IMGR_ERROR_FILE_OPEN;
  }
  return ret;
}

const image_result_t GraphicsMagick::LoadMem(const void *content,
                                             unsigned int len) {
  if (content == NULL)
    return  IMGR_ERROR_RESOURCE_LIMIT;
  if (image_obj_) {
    delete image_obj_;
    image_obj_ = NULL;
  }
  if (image_content_) {
    delete image_content_;
    image_content_ = NULL;
  }
  image_content_ = NULL;
  image_result_t ret = IMGR_SUCCESS;
  try {
    ret = IMGR_ERROR_RESOURCE_LIMIT;
    image_content_ = new Magick::Blob(content,len);
    if (image_content_) {
      //image_obj_ = new Magick::Image(*image_content_);
      image_obj_ = new Magick::Image;
      if (image_obj_) {
        ret = IMGR_SUCCESS;
        image_obj_->read(*image_content_);
      } else {
        LOGGER_ERROR(logger_, "act=LoadMem err=new Magick::Image fail");
        ret = IMGR_ERROR_OUTOFMEM;
      }
    }
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=LoadMem err=Magick::Exception %s,len=%d",e.what(),len);
    if (image_obj_) {
      delete image_obj_;
      image_obj_ = NULL;
    }
    if (image_content_) {
      delete image_content_;
      image_content_ = NULL;
    }
    return IMGR_ERROR_OUTOFMEM;
  }
  return ret;
}


const image_result_t GraphicsMagick::SaveFile(const char *path) {
  image_result_t ret = IMGR_SUCCESS;
  try {
    ret = IMGR_ERROR_RESOURCE_LIMIT;
    MagickLib::OrientationType orientation_type = image_obj_->orientation();
    if (orientation_type != MagickLib::UndefinedOrientation && orientation_type != MagickLib::TopLeftOrientation) {
      image_obj_->orientation(MagickLib::TopLeftOrientation);
    }
    image_obj_->write(path);
    ret = IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=SaveFile err=Magick::Exception %s", e.what());
    return IMGR_ERROR_RESOURCE_LIMIT;
  }
  return ret;
}

const image_result_t GraphicsMagick::GetContent(void **img_content,  //图片数据
                                                unsigned int *len,  //图片数据长度
                                                char *ext,  
                                                int jpgQuality,  //图片质量默认90
                                                bool keepAnimation /*= false*/) {
  image_result_t ret = IMGR_ERROR_CORRUPT_IMAGE;
  if (!jpgQuality) {
    jpgQuality = 90;//默认值
  }
  try {
    //image_obj_->compressType(Magick::CompressionType::JPEGCompression);
    //char *ext_orig = const_cast<char *>(GetExtension());
    std::string ext_orig = GetExtension();
    MagickLib::OrientationType orientation_type = image_obj_->orientation();
    if (orientation_type != MagickLib::UndefinedOrientation && orientation_type != MagickLib::TopLeftOrientation) {
      image_obj_->magick("DPX");
      image_obj_->orientation(MagickLib::TopLeftOrientation);
      if (0 != ext_orig.length())
        image_obj_->magick(ext_orig);
    } 
    if (ext) {
      image_obj_->magick(ext);
    }
    image_obj_->quality(jpgQuality);
    if (!image_content_) {
      image_content_ = new Magick::Blob();
    }
    if (!keepAnimation) {
      image_obj_->write(image_content_);
      *img_content = const_cast<void *>(image_content_->data());
      *len = static_cast<unsigned int>(image_content_->length());
      ret = IMGR_SUCCESS;
    } else {
      //多张图片，gif
      image_obj_->magick("jpg");//set format jpg
      image_obj_->write(image_content_);
      *img_content = const_cast<void *>(image_content_->data());
      *len = static_cast<unsigned int>(image_content_->length());
      ret = IMGR_SUCCESS;
    }
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=GetContent err=Magick::Exception %s,len=%d,jpgQuality=%d,ext=%s", e.what(),*len,jpgQuality,ext);
    if (image_content_) {
      delete image_content_;
      image_content_ = NULL;
    }
    return IMGR_ERROR_OUTOFMEM;
  }
  return ret;
}

const image_result_t GraphicsMagick::SetImageFormat(const char *format) {
  if (NULL == format || "" == format) {
    return IMGR_ERROR_PROCESS;
  }
  image_obj_->magick(format);
  return IMGR_SUCCESS;
}

const image_result_t GraphicsMagick::Resize(unsigned int width,
                                            unsigned int height) {//已经知道宽高情况下，使用
  image_result_t ret = IMGR_ERROR_PROCESS;
  if (IsLoaded()) {
    try {
      //image_obj_->sample(Magick::Geometry(width,height));
      char param_scale[64];
      sprintf(param_scale,"%dx%d!",width,height);
      image_obj_->scale(Magick::Geometry(param_scale));
      if (0 == GetWidth() || 0 == GetHeight()) {
        LOGGER_ERROR(logger_, "Resize failed, width=%d, height=%d", GetWidth(), GetHeight());
      }
      else {
        ret = IMGR_SUCCESS;
      }
    }
    catch (Magick::Exception &e) {
      LOGGER_ERROR(logger_, "act=Resize err=Magick::Exception %s,width=%d,height=%d", e.what(),width,height); 
      return ret;
    }
  } 
  return ret;
}

const image_result_t GraphicsMagick::Crop(unsigned int dx,
                                          unsigned int dy,
                                          unsigned int width,
                                          unsigned int height) {// 已经确定参数
  image_result_t ret = IMGR_ERROR_PROCESS;
  try {
    //image_obj_->crop(Magick::Geometry(width, height, dx, dy));
    //image_obj_->transform(Magick::Geometry(width,height),Magick::Geometry(dx,dy));
    image_obj_->crop(Magick::Geometry(width,height,dx,dy));
    ret = IMGR_SUCCESS;
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=Crop err=Magick::Exception %s,dx=%d,dy=%d,"
                 "width=%d,height=%d", e.what(),dx,dy,width,height);
    return IMGR_ERROR_PROCESS;
  }
  return ret;
}

const image_result_t GraphicsMagick::Scale(unsigned int width,
                                           unsigned int height,
                                           bool upscale) {//自己的scale
  unsigned int height_out = 0;
  unsigned int width_out = 0;
  calc_scale_size(static_cast<float>(GetWidth()),//获得参数
                  static_cast<float>(GetHeight()),
                  static_cast<float>(width),
                  static_cast<float>(height),
                  width_out, height_out, upscale);
  return Resize(width_out, height_out);//直接resize
}

const image_result_t GraphicsMagick::ChangeHSB(float hue,
                                               float saturation,
                                               float bright) {
  try {
    image_obj_->modulate(bright*100, saturation*100, hue*100);
    return IMGR_SUCCESS;
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=ChangeHSB Magick::Exception %s,hue=%f,saturation=%f"
                 ",bright=%f", e.what(),hue,saturation,bright);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::CutPicture(std::vector<ImageProcess *> &img_vec) {
  return IMGR_SUCCESS;
}

const image_result_t GraphicsMagick::AutoCut() {
  return IMGR_SUCCESS;
}

const image_result_t GraphicsMagick::SharpEx(float amount,
                                             float radius,
                                             float threshold) {
  try {
    image_obj_->unsharpmask(radius,radius,amount/100.0,threshold/255.0);
    return IMGR_SUCCESS;
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=SharpEx err=Magick::Exception %s,amount=%f"
                 "radius=%f,threshold=%f", e.what(),amount,radius,threshold);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::ChannelSharpEx(int channel,
                                                    float amount,
                                                    float radius,
                                                    float threshold) {
  try {
    image_obj_->unsharpmaskChannel(static_cast<MagickLib::ChannelType>(channel),
                                   radius,
                                   radius,
                                   amount/100.0,
                                   threshold/255.0);
    return IMGR_SUCCESS;
  }
  catch (Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=SharpEx err=Magick::Exception %s,amount=%f"
                 "radius=%f,threshold=%f", e.what(),amount,radius,threshold);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::Contrast(float amount) {
  try {
    image_obj_->contrast(static_cast<unsigned int>(amount));
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=Contrast err=Magick::Exception %s,amount=%f", e.what(),amount);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::AddWaterMark(const ImageProcess *img,
                                                  unsigned int x,
                                                  unsigned int y,
                                                  bool transparancy) {
  try {
    Magick::Image *img_logo = static_cast<Magick::Image *>(img->GetImage());//取水印图片
    image_obj_->composite(*img_logo, x, y, MagickLib::AtopCompositeOp);
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=AddWater err=Magick::Exception %s,x=%d,y=%d", e.what(),x,y);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::ComposePics(const ImageProcess *img,
                                                 unsigned int x,
                                                 unsigned int y) {
  try {
    Magick::Image *img_one = static_cast<Magick::Image *>(img->GetImage());
    image_obj_->composite(*img_one, x, y, MagickLib::AtopCompositeOp);
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_,"act=ComposePics err=Magick::Exception %s,x=%d,y=%d",e.what(),x,y);
    return IMGR_ERROR_PROCESS;
  }
}

const image_result_t GraphicsMagick::StretchPic(unsigned int width,
                                                unsigned int height) {
  unsigned int width_orig = GetWidth();
  unsigned int height_orig = GetHeight();
  unsigned int new_height;
  unsigned int new_width;
  float ratio_orig = static_cast<float>(width_orig) / static_cast<float>(height_orig);
  if (static_cast<float>(width) / static_cast<float>(height) > ratio_orig) {//宽度不变，高度拉伸。目的：维持源图比例不变，再只要crop
    new_height = (unsigned int)(static_cast<float>(width) / ratio_orig);
    new_width = width;
  } else {//高度不变，宽度拉伸
    new_width = static_cast<unsigned int>(height * ratio_orig);
    new_height = height;
  } 
  unsigned int x_mid = static_cast<unsigned int>(new_width / 2);
  unsigned int y_mid = static_cast<unsigned int>(new_height / 2);
  image_result_t ret;
  ret = Resize(new_width, new_height);
  ret = Crop((x_mid - (width / 2)), (y_mid - (height / 2)), width, height);//对称截取
  return ret;
}
const image_result_t GraphicsMagick::ImageFilter(const image_curve imgCurve,void *context) {
  if (NULL == imgCurve) {
    return IMGR_ERROR_PROCESS;
  }
  int width = GetWidth();
  int height = GetHeight();
  int rgb[3];//存修改后的rgb值
  try {
    Magick::PixelPacket *pixel_cache = image_obj_->getPixels(0,0,width,height);
    Magick::PixelPacket *pixel;
    for (int i = 0;i < height; i++) {
      for (int j = 0;j < width; j++) {
        pixel = pixel_cache + i * width + j;
        rgb[0] = static_cast<unsigned int>(pixel->red);
        rgb[1] = static_cast<unsigned int>(pixel->green);
        rgb[2] = static_cast<unsigned int>(pixel->blue);
        imgCurve(rgb[0],rgb[1],rgb[2],context);//对通道数值进行变换
        *pixel = Magick::Color(rgb[0],rgb[1],rgb[2]);
      }
    }
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "act=Imgcurve err=Magick::Exception %s", e.what());
    return IMGR_ERROR_PROCESS;
  }
}
const image_result_t GraphicsMagick::Type(const int imagetype ) {
  try {
    image_obj_->type(static_cast<Magick::ImageType>(imagetype));
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e){
    LOGGER_ERROR(logger_, "act=Type err=Magick::Exception %s", e.what());
    return IMGR_ERROR_PROCESS;
  }
}
const image_result_t GraphicsMagick::Colorize(unsigned int opacity,
                                              const int red,
                                              const int green,
                                              const int blue) {
  try {
    Magick::Color color(red,green,blue);
    image_obj_->colorize(opacity,color);
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e){
    LOGGER_ERROR(logger_, "act=Colorize err=Magick::Exception %s", e.what());
    return IMGR_ERROR_PROCESS;
  }
}
const image_result_t GraphicsMagick::Blur(const double radius ,
                                          const double sigma ) {
  try {
    image_obj_->blur(radius,sigma);
    return IMGR_SUCCESS;
  }
  catch(Magick::Exception &e) {
    LOGGER_ERROR(logger_, "err=Blur Magick::Exception %s", e.what());
    return IMGR_ERROR_PROCESS;
  }
}
const bool GraphicsMagick::IsMatte() const {
  bool ismatte = false;
  if (IsLoaded()) {
    try {
      ismatte = image_obj_->matte();
    } 
    catch(Magick::Exception &e) {
      LOGGER_ERROR(logger_, "err=IsMatte Magick::Exception %s", e.what());
      return ismatte;
    }
  } 
  return ismatte;
}

const bool GraphicsMagick::IsLoaded() const {
  if (image_obj_ == NULL) {
    return false;
  } else {
    return true;
  }
}

const unsigned int GraphicsMagick::GetWidth() const {
  unsigned int columns = 0;
  if (IsLoaded()) {
    try {
      columns = image_obj_->columns();
    }
    catch(Magick::Exception &e) {
      LOGGER_ERROR(logger_, "err=GetWidth Magick::Exception %s", e.what());
      return columns;
    }
  } 
  return columns;
}

const unsigned int GraphicsMagick::GetHeight() const {
  unsigned int rows = 0;
  if (IsLoaded()) {
    try {
      rows = image_obj_->rows();
    }
    catch(Magick::Exception &e) {
      LOGGER_ERROR(logger_, "err=GetHeight Magick::Exception %s", e.what());
      return rows;
    }
  } 
  return rows;
}
const int GraphicsMagick::GetColorSpace() const {
  int type = 0;
  if (IsLoaded()) {
    try {
      type = image_obj_->type();
    }
    catch(Magick::Exception &e) {
      LOGGER_ERROR(logger_, "err=GetColorSpace Magick::Exception %s", e.what()); 
      return type;
    }
  } 
  return type;
}

const std::string GraphicsMagick::GetExtension() const {
  //char *ext = const_cast<char*>(image_obj_->magick().c_str());
  std::string magick = "";
  try {
    magick = image_obj_->magick();
  }
  catch(Magick::Exception &e){
    LOGGER_ERROR(logger_, "err=GetColorSpace Magick::Exception %s", e.what());
    return magick;
  }
  return magick;
}

const image_result_t GraphicsMagick::SetQuality(unsigned int quality) {
  image_result_t ret = IMGR_ERROR_PROCESS;
  if(IsLoaded()) {
    try {
      image_obj_->quality(quality);
      image_obj_->syncPixels();
      ret = IMGR_SUCCESS;
    }
    catch(Magick::Exception &e) {
      LOGGER_ERROR(logger_, "err=SetQuality Magick::Exception %s", e.what());
      return ret;
    }
  }
  return ret;
}

const image_result_t GraphicsMagick::SetImage(const void * img) {
  if(NULL == img) {
    return IMGR_ERROR_PROCESS; 
  }  
  if(image_obj_ != NULL) {
    delete image_obj_;
    image_obj_ = NULL;
  }
  if (image_content_ != NULL) {
    delete image_content_;
    image_content_ = NULL;
  }
  
  image_obj_ = static_cast<Magick::Image *>(const_cast<void *>(img));
  return IMGR_SUCCESS;
}
void *GraphicsMagick::GetImage() const {
  return image_obj_;
}
