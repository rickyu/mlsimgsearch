#ifndef IMAGE_PROCESS_IMAGE_CONSTANT_H_
#define IMAGE_PROCESS_IMAGE_CONSTANT_H_


typedef enum {
  IMGR_SUCCESS,  /** 0 */
  IMGR_ERROR_FILE_OPEN,  /*open fails*/
  IMGR_ERROR_CORRUPT_IMAGE,  /*image file is corrupt*/
  IMGR_ERROR_RESOURCE_LIMIT,  /*a system resource has been exhausted */
  IMGR_ERROR_PROCESS,  /*process failed*/
  IMGR_ERROR_OUTOFMEM,
  IMGR_IMAGE_SCALE_FAILED,
  IMGR_IMAGE_CROP_FAILED,
  IMGR_IMAGE_PARSE_FAILED,
  IMGR_IMAGE_IO_FAILED,
  IMGR_IMAGE_ENCODE_FAILED,
  IMGR_IMAGE_EXCEPTION,

  IMGR_IMAGE_RECOG_BASE = 50,
  IMGR_IMAGE_RECOG_LOAD_CASCADE_FAILED,
  IMGR_MAX = 200
} image_result_t; 


#endif
