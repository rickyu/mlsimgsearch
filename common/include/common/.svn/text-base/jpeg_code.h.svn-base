#ifndef JPEG_CODE_INCLUDE_H
#define JPEG_CODE_INCLUDE_H

#include <memory.h>
#include <setjmp.h>
#include <utils/logger.h>
#include <common/constant.h>
extern "C"{
#include <jpeglib.h>
}

#define INPUT_BUF_SIZE  4096
#define OUTPUT_BUF_SIZE  4096 
struct jpeg_code_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};
typedef struct jpeg_code_error_mgr * jpeg_code_error_ptr;

typedef struct {//for memory read image_data
  struct jpeg_destination_mgr pub; /* public fields */
  unsigned char ** outbuffer;   /* target buffer */
  unsigned long * outsize;
  unsigned char * newbuffer;    /* newly allocated buffer */
  JOCTET * buffer;              /* start of buffer */
  size_t bufsize;
} jpeg_code_mem_destination_mgr;

typedef jpeg_code_mem_destination_mgr * jpeg_code_mem_dest_ptr;

class JpegCode {
 public:
  JpegCode();
  ~JpegCode();

  int JpegDecompress(const char * image_file_data,size_t input_size);
  int JpegCompress(unsigned char * image_data,int width,int height,int widthstep,int quality);
  int JpegCompress(int quality);

  int SetParameters();//set parameter

  char * GetImage() const;

  size_t GetSize() const;//compression length;

 private:
  int JpegMemSrc(j_decompress_ptr dinfo,
                 void * inbuffer, size_t insize);
  int JpegMemDest(j_compress_ptr cinfo,
                  unsigned char ** outbuffer, size_t *len);
  int Clear();

  struct jpeg_decompress_struct d_info_;
  struct jpeg_compress_struct c_info_;
  int image_width_;
  int image_height_;
  int components_;
  unsigned char * image_pixels_data_;//image_width_ x image_heigh_
  char * image_compress_data_;
  size_t image_compress_data_len_;
  J_COLOR_SPACE color_space_;
  static CLogger& logger_;
};
#endif
