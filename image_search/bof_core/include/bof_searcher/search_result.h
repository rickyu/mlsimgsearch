#ifndef IMAGE_SEARCH_BOF_SEARCHER_SEARCH_RESULT_H_
#define IMAGE_SEARCH_BOF_SEARCHER_SEARCH_RESULT_H_

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>

class SearchResultItem {
 public:
  SearchResultItem();
  SearchResultItem(int64_t pic_id, float sim) {
    pic_id_ = pic_id;
    sim_ = sim;
  }
  
 public:
  int64_t pic_id_;
  float sim_;
};

class SearchResult {
 public:
  SearchResult();

  void AddItem(int pic_id, float sim);

 public:
  std::vector<SearchResultItem> items_;
};

#endif
