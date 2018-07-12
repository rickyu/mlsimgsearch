#ifndef IMAGE_SEARCH_VOCABULARY_CREATOR_VOCABULARY_CREATOR_H_
#define IMAGE_SEARCH_VOCABULARY_CREATOR_VOCABULARY_CREATOR_H_

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <vector>
#include <string>

class VocabularyCreator {
 public:
  VocabularyCreator();
  ~VocabularyCreator();

  int ExtractDescriptors(const std::string &save_file);
  int SaveDescriptors(const std::string &file_path);
  int Cluster();
  int SaveVocabulary();

 private:
  int ListFiles();
  
 private:
  cv::Mat *descriptors_;
  cv::Mat vocabulary_;
  std::vector<std::string> files_;
};

#endif
