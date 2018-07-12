#include <bof_feature_analyzer/hamming_embedding_descriptor.h>

static inline int count_one(unsigned long x) {
  x -= (x >> 1) & 0x5555555555555555;             
  x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333); 
  x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;        
  x += x >> 8;  
  x += x >> 16;  
  x += x >> 32;  
  return x & 0x7f;
}

HammingEmbedding::HammingEmbedding() {
  he_length_ = 64;
  he_threshold_ = 64;
}
HammingEmbedding::~HammingEmbedding() {

}
void HammingEmbedding::SetHammingParam(int he_length,int he_threshold) {
  he_length_ = he_length;
  he_threshold_ = he_threshold;
}
int HammingEmbedding::HammingCoder(const float * one_descriptor_p,cv::Mat & out_p,
                                   float * median_mat_p,std::string &hamming_str) {
  float *he_v = new float[he_length_];
  float *p;
  float sum = 0.0;
  for (size_t i = 0 ; i < he_length_ ; i++ ) {
    p = out_p.ptr<float>(i);
    sum = 0;
    for (int j = 0 ; j < out_p.cols ; j++) {
      sum += p[j] * one_descriptor_p[j];
    }
    he_v[i] = sum ;
  }

  hamming_str = "";
  int h_16 = 0 ;
  size_t count = 0;
  size_t rat[4] = {8,4,2,1};
  for (size_t j = 0 ; j < he_length_ ; j++) {
    if (median_mat_p[j] > he_v[j]) {
      h_16 += rat[count];
    }

    count ++;
    if (4 == count) {
      count = 0;
      if (h_16 >= 10) {
        hamming_str += 'A' + h_16 - 10;
      }else {
        hamming_str += '0' + h_16;
      }
      h_16 = 0;
    }
  }
  delete [] he_v;
  return 0;
}
int HammingEmbedding::HeDistance(int &similar_num,
                                 const unsigned long *he1,
                                 const int he_length1,
                                 const unsigned long *he2,
                                 const int he_length2 ) {
  if (he_length1 <= 0|| he_length2 <= 0 || 
      NULL == he1 || NULL == he2) {
    return -1;
  }
  unsigned long return_value = 0;
  size_t distance = 128;// > 64
  similar_num = 0;
  for (int i = 0; i < he_length1; i ++ ) {
    for ( int j = 0; j < he_length2; j ++ ) {
      return_value = he1[i] ^ he2[j]; 
      distance = count_one(return_value);
      if ( distance <= he_threshold_ ) {
        similar_num ++;
      }
    }
  }
  return 0;
}
int HammingEmbedding::HeWeight(const std::string &he,float &weight) {
  if (he.length() != he_length_ ) {
    return -1;
  }
  weight = -1.0;
  return 0;
}
int HammingEmbedding::HeString2Int(const std::string &he,
                                   unsigned long *&he64_int,int &length) {
  if (NULL != he64_int) {
    return -1;
  }
  int he_len = he.length();
  length = he_len / 16;
  he64_int = new unsigned long[length];
  if (NULL == he64_int ) {
    return -1;
  }
  unsigned long off_value[16] = {1};
  for (size_t i = 1; i < 16; i ++ ) {
    off_value[i] = off_value[i-1] << 4;
  }
  for (int j = 0; j < length; j ++ ) {
    he64_int[j] = 0;
  }
  for (int j = 0; j < length; j ++ ) {
    int start = j * 16;
    for (int l = 0; l < 16; l ++ ) {
      if (he[l + start] <= '9') {
        he64_int[j] += (he[l + start] - '0') * off_value[l];
      }else {
        he64_int[j] += (he[l + start] - 'A' + 10) * off_value[l];
      }
    }
  }
  return 0;
}
