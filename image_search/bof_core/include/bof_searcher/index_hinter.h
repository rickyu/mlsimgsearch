#ifndef IMAGE_SEARCH_BOF_SEARCHER_INDEX_HINTER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_INDEX_HINTER_H_

#include <string>
#include <vector>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/thread.hpp>

class HinterItem {
 public:
  HinterItem() {
    start_pos_ = 0;
    end_pos_ = 0;
    bucket_size_ = 0;
  }

 private:
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar,
                                         const unsigned int version) {
    ar & start_pos_;
    ar & end_pos_;
    ar & bucket_size_;
  }

 public:
  int64_t start_pos_;
  int64_t end_pos_;
  int64_t bucket_size_;
};

class IndexHinter {
 public:
  IndexHinter(int max_bucket_num);
  ~IndexHinter();

  int SetHint(int bucket_id, int64_t bucket_size, int64_t start_pos, int64_t end_pos);
  int GetHint(int bucket_id, HinterItem &item);

 private:
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar,
                                         const unsigned int version) {
    ar & max_bucket_num_;
    ar & items_;
  }
  IndexHinter() {}

 private:
  int max_bucket_num_;
  std::vector<HinterItem> items_;
  boost::mutex io_mutex_;
};

#endif
