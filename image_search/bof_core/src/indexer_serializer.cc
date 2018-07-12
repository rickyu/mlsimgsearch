#include <bof_searcher/indexer_serializer.h>
#include <string.h>


static inline char* memcpy_data(char *dest, const char *src, int n) {
  char *cur = dest;
  memcpy(cur, src, n);
  cur = cur + n;
  return cur;
}

static inline char* dump_data(char *dest, char *src, int n) {
  memcpy(dest, src, n);
  return src + n;
}

static inline char* memcpy_data(char *dest, int data) {
  return memcpy_data(dest, (char*)&data, sizeof(data));
}

static inline char* memcpy_data(char *dest, float data) {
  return memcpy_data(dest, (char*)&data, sizeof(data));
}

static inline char* memcpy_data(char *dest, int64_t data) {
  return memcpy_data(dest, (char*)&data, sizeof(data));
}

static inline char* memcpy_data(char *dest, char data) {
  return memcpy_data(dest, (char*)&data, sizeof(data));
}

static inline char* memcpy_data(char *dest, short data) {
  return memcpy_data(dest, (char*)&data, sizeof(data));
}

static inline char* dump_data(int &data, char *dest) {
  return dump_data((char*)&data, dest, sizeof(data));
}

static inline char* dump_data(float &data, char *dest) {
  return dump_data((char*)&data, dest, sizeof(data));
}

static inline char* dump_data(int64_t &data, char *dest) {
  return dump_data((char*)&data, dest, sizeof(data));
}

static inline char* dump_data(short &data, char *dest) {
  return dump_data((char*)&data, dest, sizeof(data));
}


IndexItem::IndexItem() {
  pic_id_ = 0;
  weight_ = 0.0f;
  length_ = 0;
  he_ = NULL;
  angle_ = NULL;
}

IndexItem::~IndexItem() {
  if (he_) {
    delete [] he_;
    he_ = NULL;
  }
  if (angle_) {
    delete [] angle_;
    angle_ = NULL;
  }
}

int IndexItem::GetSize() const {
  int size = 0;
  size += sizeof(pic_id_);
  size += sizeof(weight_);
  size += sizeof(length_);
  size += sizeof(unsigned long) * length_;
  size += sizeof(unsigned int) * length_;
  /*size += sizeof(short); // len of he_
  size += strlen(he_) + 1; // +1 for \0
  size += sizeof(short); // len of he_
  size += strlen(angle_) + 1; // +1 for \0
  */
  return size;
}

char* IndexItem::DumpData(char *buf) {
  char *cur = buf;
  //short he_len = (short)strlen(he_) + 1;
  //short angle_len = (short)strlen(angle_) + 1;
  short he_len = sizeof(unsigned long) * length_;
  short angle_len = sizeof(unsigned int) * length_;
  cur = memcpy_data(cur, pic_id_);
  cur = memcpy_data(cur, weight_);
  cur = memcpy_data(cur, length_);
  cur = memcpy_data(cur,(char*)he_, he_len);
  cur = memcpy_data(cur,(char*)angle_, angle_len);
  return cur;
}

char* IndexItem::ReinterpretData(const char *data, int size) {
  char *cur = const_cast<char*>(data);
  cur = dump_data(pic_id_, cur);
  cur = dump_data(weight_, cur);
  cur = dump_data(length_, cur);
  he_ = (unsigned long *)(cur);
  cur += sizeof(unsigned long) * length_;
  angle_ = (unsigned int *)(cur);
  cur += sizeof(unsigned int) * length_;
  /*short he_len = 0, angle_len = 0;
  he_ = cur;
  cur += he_len;
  cur = dump_data(angle_len, cur);
  angle_ = cur;
  cur += angle_len;
  */
  return cur;
}

void IndexItem::Clear() {
  he_ = NULL;
  angle_ = NULL;
}

WordIndex::WordIndex() {
  word_id_ = 0;
}

WordIndex::~WordIndex() {
  for (size_t i = 0; i < items_.size(); ++i) {
    if (items_[i]) {
      delete items_[i];
    }
  }
  items_.clear();
}

void WordIndex::AddItem(const IndexItem *item) {
  items_.push_back(const_cast<IndexItem*>(item));
}

void WordIndex::ClearItems() {
  for (size_t i = 0; i < items_.size(); ++i) {
    if (items_[i]) {
      items_[i]->Clear();
      delete items_[i];
      items_[i] = NULL;
    }
  }
  items_.clear();
}

IndexerSerializer::IndexerSerializer() {
}

IndexerSerializer::~IndexerSerializer() {
}

int IndexerSerializer::Serialize(const WordIndex &index, char *&data) {
  int word_index_size = CalcWordIndexSize(index);
  data = new char[word_index_size];
  if (NULL == data) {
    return -1;
  }
  char *cur = data;
  int items_size = (int)index.items_.size();
  cur = memcpy_data(cur, index.word_id_);
  cur = memcpy_data(cur, items_size);
  for (int i = 0; i < items_size; ++i) {
    cur = index.items_[i]->DumpData(cur);
  }
  
  return word_index_size;
}

int IndexerSerializer::Unserialize(const char *data, int size, WordIndex &index) {
  if (NULL == data) {
    return -1;
  }
  char *cur = const_cast<char*>(data);
  cur = dump_data(index.word_id_, cur);
  int items_size = 0;
  cur = dump_data(items_size, cur);
  index.items_.clear();
  for (int i = 0; i < items_size; ++i) {
    IndexItem *cur_item = new IndexItem;
    cur = cur_item->ReinterpretData(cur, (int)(cur-data));
    index.items_.push_back(cur_item);
  }

  return 0;
}

int IndexerSerializer::CalcWordIndexSize(const WordIndex &index) {
  int size = 0;
  size += sizeof(index.word_id_);
  int items_size = (int)index.items_.size();
  size += sizeof(items_size);
  for (int i = 0; i < items_size; ++i) {
    size += index.items_[i]->GetSize();
  }
  return size;
}
