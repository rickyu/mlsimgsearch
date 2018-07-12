#ifndef IMAGE_SEARCH_BOF_SEARCHER_INDEXER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_INDEXER_H_

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <bof_searcher/index_hinter.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <boost/thread.hpp>
#include <bof_searcher/indexer_serializer.h>

class BOFIndexItem {
 public:
  BOFIndexItem();
  ~BOFIndexItem();

  void Set(const uint32_t pic_id,
           const BOFQuantizeResult &bof_info);
  void SetPicID(const uint32_t pic_id);
  void SetBofResult(const BOFQuantizeResult &bof_info);

 private:
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar,
                                         const unsigned int version) {
    ar & pic_id_;
    ar & bof_info_;
  }
  
 public:
  uint32_t pic_id_;
  BOFQuantizeResult bof_info_;
};

class BOFIndexBucket {
 public:
  BOFIndexBucket(const int bucket_id);
  BOFIndexBucket();
  ~BOFIndexBucket();

  const int AddItem(const BOFIndexItem &item);
  const int AddItem(const uint32_t pic_id,
                    const BOFQuantizeResult &bof_info);

  const int GetBucketID() const;
  void GetItems(std::vector<BOFIndexItem> *&items) const;
  const size_t GetItemsCount() const;

  int ConvertB2W(WordIndex &word);

 private:
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar,
                                         const unsigned int version) {

    ar & bucket_id_;
    ar & items_;
  }
  
 private:
  int bucket_id_;
  std::vector<BOFIndexItem> items_;
};

typedef std::map<int, BOFIndexBucket> BOFIndexList;

class MemWord {
 public:
  MemWord() {
  }
  MemWord(WordIndex word_index, char* word_data):word_index_(word_index), word_data_(word_data){}
  void GetWordIndex(WordIndex &word_index) {
    word_index = word_index_;
  }
  void ReleaseWord() {
    if (word_data_) {
      delete[] word_data_;
      word_data_ = NULL;
    }
    word_index_.ClearItems();
  }
 private:
  WordIndex word_index_;
  char *word_data_;
};

typedef std::vector<MemWord> MemWordsManager;

class BOFIndex {
 public:
  BOFIndex();
  virtual ~BOFIndex();

  static BOFIndex* GetInstance();
  void Finalize();
  
  const int Init(const int max_bucket_num,
                 const std::string &hint_file,
                 const std::string &data_file);
  const int LoadWord(const int word_id, WordIndex &word_index, char *&data);

  const int SetBeginWordId(const int begin_word_id);
  const int LoadFileData(const int begin_id, const int end_id); 
  const int ReleaseFileData();
  const int GetWord(const int word_id, WordIndex &word_index);
 private:
  IndexHinter *hinter_;
  std::fstream fs_data_;
  std::vector<BOFIndexBucket*> buckets_;
  MemWordsManager mem_words_manager_;
  int begin_word_id_;
  static BOFIndex* instance_;
  static CLogger &logger_;
  boost::mutex io_mutex_;
};

#endif
