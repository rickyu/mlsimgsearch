#ifndef IMAGE_SEARCH_BOF_SEARCHER_INDEXER_SERIALIZER_H_
#define IMAGE_SEARCH_BOF_SEARCHER_INDEXER_SERIALIZER_H_

#include <vector>
#include <string>
#include <utils/logger.h>
#include <utils/logger_loader.h>

class IndexItem {
 public:
  IndexItem();
  ~IndexItem();

  int GetSize() const;
  char* DumpData(char *buf);
  char* ReinterpretData(const char *data, int size);
  /**
   *@brief 只将he_和angle_置空，并不释放他们的内存
   */
  void Clear();
  
 public:
  int64_t pic_id_;
  float weight_;
  short length_;
  unsigned long *he_;
  unsigned int *angle_;
};

class WordIndex {
 public:
  WordIndex();
  ~WordIndex();

  void AddItem(const IndexItem* item);
  /**
   * @brief 这个函数只释放IndexItem*内存，而不会将IndexItem*的he_和angle_释放
   */
  void ClearItems();

 public:
  int word_id_;
  std::vector<IndexItem*> items_;
};

class IndexerSerializer {
 public:
  IndexerSerializer();
  ~IndexerSerializer();

  int Serialize(const WordIndex &index, char *&data);
  int Unserialize(const char *data, int size, WordIndex &index);

 private:
  int CalcWordIndexSize(const WordIndex &index);
};

#endif
