#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_HAMMING_EMBEDDING_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_HAMMING_EMBEDDING_H_

#include<opencv2/opencv.hpp>

class HammingEmbedding {
 public:
   HammingEmbedding();
   ~HammingEmbedding();
   void SetHammingParam(int he_length,int he_threshold);
   int HammingCoder(const float * one_descriptor_p , cv::Mat & out_p,
                    float * median_mat_p , std::string &hamming_str);
   int HeDistance(int &similar_num,const unsigned long *he1,const int he_length1,
                  const unsigned long *he2, const int he_length2 );
   int HeWeight(const std::string & he, float &weight);
   int HeString2Int(const std::string &he,unsigned long *&he64_int,int &length);
 private:
   size_t he_length_;
   size_t he_threshold_;
};
#endif
