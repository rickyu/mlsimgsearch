//#include <memory.h>
//#include <setjmp.h>
//
//@author shaotinglv
//@data 2012-11-20
#include "common/jpeg_code.h"

CLogger& JpegCode::logger_ = CLogger::GetInstance("JpegCode");

static const unsigned int std_luminance_quant_tbl[DCTSIZE2] = { 
  16,  11,  10,  16,  24,  40,  51,  61, 
  12,  12,  14,  19,  26,  58,  60,  55, 
  14,  13,  16,  24,  40,  57,  69,  56, 
  14,  17,  22,  29,  51,  87,  80,  62, 
  18,  22,  37,  56,  68, 109, 103,  77, 
  24,  35,  55,  64,  81, 104, 113,  92, 
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99  
};  
static void init_mem_destination (j_compress_ptr cinfo) {
      /* no work necessary here */
}
static boolean empty_mem_output_buffer (j_compress_ptr cinfo) { 
  size_t nextsize;
  JOCTET * nextbuffer;
  jpeg_code_mem_dest_ptr dest = (jpeg_code_mem_dest_ptr) cinfo->dest;

  /* Try to allocate new buffer with double size */
  nextsize = dest->bufsize * 2;
  nextbuffer = (JOCTET *) malloc(nextsize);

  if (NULL == nextbuffer) {
    return FALSE;
  }

  memcpy(nextbuffer, dest->buffer, dest->bufsize);
  if (dest->newbuffer != NULL) {
     free(dest->newbuffer);
  }

  dest->newbuffer = nextbuffer;

  dest->pub.next_output_byte = nextbuffer + dest->bufsize;
  dest->pub.free_in_buffer = dest->bufsize;

  dest->buffer = nextbuffer;
  dest->bufsize = nextsize;
  return TRUE;
}

static void term_mem_destination (j_compress_ptr cinfo) {
  jpeg_code_mem_dest_ptr dest = (jpeg_code_mem_dest_ptr) cinfo->dest;
  *dest->outbuffer = dest->buffer;
  *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}
static void init_source (j_decompress_ptr cinfo) {
   // no work necessary here 
}
static boolean fill_mem_input_buffer (j_decompress_ptr cinfo) { 
  static const JOCTET jpeg_codebuffer[4] = {
         (JOCTET) 0xFF, (JOCTET) JPEG_EOI, 0, 0
  };

  cinfo->src->next_input_byte = jpeg_codebuffer;
  cinfo->src->bytes_in_buffer = 2;
  return TRUE;

}
static void term_source (j_decompress_ptr cinfo) { 
   // no work necessary here 
}
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes) { 
  struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;
  if (num_bytes > 0) {
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

void static MyErrorExit(j_common_ptr cinfo) {
  jpeg_code_error_ptr jpeg_codeerr = (jpeg_code_error_ptr) cinfo->err;
  longjmp(jpeg_codeerr->setjmp_buffer, 1);
} 

JpegCode::JpegCode() {
  components_ = 3;
  color_space_=JCS_RGB;
  image_compress_data_len_ = -1;
  image_pixels_data_ = NULL;
  image_compress_data_ = NULL;
}
JpegCode::~JpegCode() {
  if (image_pixels_data_) {
    delete [] image_pixels_data_;
    image_pixels_data_ = NULL;
  }
  if (image_compress_data_) {
    free(image_compress_data_);
    image_compress_data_ = NULL;
  }
  Clear();
}
int JpegCode::Clear() {
  
  //jpeg_destroy_decompress(&d_info_);
  //jpeg_destroy_compress(&c_info_);
  return CR_SUCCESS;
}

int JpegCode::JpegMemSrc(j_decompress_ptr dinfo,
                         void * inbuffer,size_t size) {

  struct jpeg_source_mgr* src;
  if (NULL == inbuffer || 0 == size) {
    LOGGER_ERROR(logger_, "act=Jpeg_mem_src failed, param error ");
    return CR_JPEG_CODE_PARAM_ERROR;
  }
  if (NULL == dinfo->src) {
    dinfo->src = (struct jpeg_source_mgr *)
    (*dinfo->mem->alloc_small) ((j_common_ptr) dinfo, JPOOL_PERMANENT,
                                sizeof(struct jpeg_source_mgr));
  } 
  src =(struct jpeg_source_mgr*) dinfo->src;
  src->init_source =init_source;
  src->fill_input_buffer = fill_mem_input_buffer;
  src->skip_input_data = skip_input_data;
  src->resync_to_restart = jpeg_resync_to_restart;
  src->term_source = term_source;
  src->bytes_in_buffer = (size_t) size;
  src->next_input_byte = (JOCTET *) inbuffer;
  return CR_SUCCESS;
}
int JpegCode::JpegMemDest(j_compress_ptr cinfo,
                         unsigned char ** outbuffer,size_t *len) {
  jpeg_code_mem_dest_ptr dest;
  if (outbuffer == NULL || len == NULL) /* sanity check */ {
    LOGGER_ERROR(logger_, "act=Jpeg_mem_dest failed, param error ");
    return CR_JPEG_CODE_PARAM_ERROR;
  }
  if (cinfo->dest == NULL) {    /* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                sizeof(jpeg_code_mem_destination_mgr));
  }
  dest = (jpeg_code_mem_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_mem_destination;
  dest->pub.empty_output_buffer = empty_mem_output_buffer;
  dest->pub.term_destination = term_mem_destination;
  dest->outbuffer = outbuffer;
  dest->outsize = len;
  dest->newbuffer = NULL;
  if (*outbuffer == NULL || *len == 0) {
    dest->newbuffer = *outbuffer = (unsigned char *) malloc(OUTPUT_BUF_SIZE);
    if (dest->newbuffer == NULL) {
      LOGGER_ERROR(logger_, "act=Jpeg_mem_dest failed, out of memory ");
      return CR_JPEG_CODE_OUT_OF_MEM;
    }
    *len = OUTPUT_BUF_SIZE;
  }
  dest->pub.next_output_byte = dest->buffer = *outbuffer;
  dest->pub.free_in_buffer = dest->bufsize = *len;
  return CR_SUCCESS;
}

int JpegCode::JpegDecompress(const char * image_file_data,size_t input_size) {
  
  if (NULL ==image_file_data) {
    LOGGER_ERROR(logger_, "act=jpeg_decompress failed, param error ");
    return CR_JPEG_CODE_PARAM_ERROR;
  }
  struct jpeg_code_error_mgr jerr;
  int row_stride;

  d_info_.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = MyErrorExit;
  if (setjmp(jerr.setjmp_buffer)) {
    LOGGER_ERROR(logger_, "act=jpeg_decompress failed, unkown error ");
    return CR_JPEG_CODE_PARAM_ERROR;
  }

  jpeg_create_decompress(&d_info_);
  
  JpegMemSrc(&d_info_,(void *)image_file_data,input_size);

  jpeg_read_header(&d_info_, TRUE);
  image_width_ = d_info_.image_width;
  image_height_ = d_info_.image_height;

  jpeg_start_decompress(&d_info_);
  row_stride = d_info_.output_width * d_info_.output_components;
  components_ = d_info_.output_components;
  JSAMPROW row_pointer[1];
  if (NULL != image_pixels_data_) {
    delete []image_pixels_data_;
  }
  image_pixels_data_ = new unsigned char[d_info_.image_width
                      *d_info_.image_height*d_info_.num_components];
  if (NULL == image_pixels_data_) {
    LOGGER_ERROR(logger_, "act=jpeg_decompress failed, out of memory ");
    return CR_JPEG_CODE_OUT_OF_MEM;
  }

  while (d_info_.output_scanline < d_info_.output_height) {
    row_pointer[0] = &image_pixels_data_[d_info_.output_scanline
                     * d_info_.image_width*d_info_.num_components];
    jpeg_read_scanlines(&d_info_, row_pointer, 1);
  }

  color_space_ = d_info_.out_color_space;
  jpeg_finish_decompress(&d_info_);
  jpeg_destroy_decompress(&d_info_);
  return CR_SUCCESS;

}
int JpegCode::JpegCompress(unsigned char * image_data,int width,int height,
                           int widthstep,int quality) {
  if (width <= 0 || height <= 0 || NULL == image_data) {
    LOGGER_ERROR(logger_, "act=jpeg_compress failed, param error"); 
    return CR_JPEG_CODE_PARAM_ERROR;
  }   
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];    
  int row_stride;
  c_info_.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&c_info_);
  JpegMemDest(&c_info_,(unsigned char **)&image_compress_data_,&image_compress_data_len_);

  components_ = widthstep/(sizeof(unsigned char)*width);
  if (1 == components_) {
    color_space_ = JCS_GRAYSCALE; 
  }else{
    color_space_ = JCS_RGB;
  }
  c_info_.image_height = height;
  c_info_.image_width = width;
  c_info_.input_components = components_;
  c_info_.in_color_space = color_space_;
  jpeg_set_defaults(&c_info_);
  jpeg_set_quality(&c_info_, quality, true);

  SetParameters();
  c_info_.optimize_coding = TRUE;

  jpeg_start_compress(&c_info_, TRUE);
  row_stride = c_info_.image_width*c_info_.input_components;

  while (c_info_.next_scanline < c_info_.image_height) {
    row_pointer[0] = & image_data[c_info_.next_scanline * row_stride];
    jpeg_write_scanlines(&c_info_, row_pointer, 1);
  }
  jpeg_finish_compress(&c_info_);
  jpeg_destroy_compress(&c_info_);
  return CR_SUCCESS;
}
int JpegCode::JpegCompress(int quality) {
  if (NULL == image_pixels_data_) {
    LOGGER_ERROR(logger_, "act=jpeg_compress failed, Need pixels data");
    return CR_JPEG_CODE_PARAM_ERROR;
  }
  int ret =JpegCompress(image_pixels_data_,image_width_,image_height_,
                        image_width_*sizeof(unsigned char)*components_,quality);
  if (CR_SUCCESS != ret) {
    LOGGER_ERROR(logger_, "act=jpeg_compress failed,Compress");
    return CR_JPEG_CODE_COMPRESS_FAILED;
  }
  return CR_SUCCESS;
}

size_t JpegCode::GetSize() const {
  return image_compress_data_len_;
}
char * JpegCode::GetImage() const {
  return image_compress_data_;
}
int JpegCode::SetParameters() {
  if (JCS_RGB == c_info_.in_color_space) {
    jpeg_add_quant_table(&c_info_, 0, std_luminance_quant_tbl,22, true);
    c_info_.comp_info[0].component_id = 1;
    c_info_.comp_info[0].h_samp_factor = 1;
    c_info_.comp_info[0].v_samp_factor = 1;
    c_info_.comp_info[0].quant_tbl_no = 0;
    c_info_.comp_info[0].dc_tbl_no = 0;
    c_info_.comp_info[0].ac_tbl_no = 0;
  }
  return CR_SUCCESS;
}
