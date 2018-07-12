#include <bof_searcher/indexer.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/time.h>

CLogger& BOFIndex::logger_ = CLogger::GetInstance("searcher");

/////////////////////////// class BOFIndexItem ////////////////////////////////
BOFIndexItem::BOFIndexItem() {
  pic_id_ = 0;
}

BOFIndexItem::~BOFIndexItem() {
}

void BOFIndexItem::Set(const uint32_t pic_id,
                       const BOFQuantizeResult &bof_info) {
  SetPicID(pic_id);
  SetBofResult(bof_info);
}

void BOFIndexItem::SetPicID(const uint32_t pic_id) {
  pic_id_ = pic_id;
}

void BOFIndexItem::SetBofResult(const BOFQuantizeResult &bof_info) {
  bof_info_ = bof_info;
}

///////////////////////// class BOFIndexBucket ////////////////////////////////
BOFIndexBucket::BOFIndexBucket(const int bucket_id)
  : bucket_id_(bucket_id) {
  items_.clear();
}

BOFIndexBucket::BOFIndexBucket() {
  bucket_id_ = 0;
  items_.clear();
}

BOFIndexBucket::~BOFIndexBucket() {
}

const int BOFIndexBucket::AddItem(const BOFIndexItem &item) {
  items_.push_back(item);
  return 0;
}

const int BOFIndexBucket::AddItem(const uint32_t pic_id,
                                  const BOFQuantizeResult &bof_info ) {
  BOFIndexItem item;
  item.Set(pic_id, bof_info);
  return AddItem(item);
}

const int BOFIndexBucket::GetBucketID() const {
  return bucket_id_;
}

void BOFIndexBucket::GetItems(std::vector<BOFIndexItem> *&items) const {
  items = const_cast<std::vector<BOFIndexItem>*>(&items_);
}

const size_t BOFIndexBucket::GetItemsCount() const {
  return items_.size();
}

int BOFIndexBucket::ConvertB2W(WordIndex &word) {
  word.word_id_ = GetBucketID();
  unsigned long off_value[16] = {1};
  for (size_t i = 1; i < 16; i ++ ) {
    off_value[i] = off_value[i-1] << 4;
  }
  for (size_t i = 0; i < items_.size(); ++i) {
    BOFIndexItem &bitem = items_[i];
    IndexItem *item = new IndexItem;
    item->pic_id_ = bitem.pic_id_;
    item->weight_ = bitem.bof_info_.q_;
    int he_len = (int)bitem.bof_info_.word_info_[0].length();

    //he
    item->length_ = he_len / 16;
    item->he_ = new unsigned long[item->length_];
    for (int j = 0; j < item->length_; j ++ ) {
      item->he_[j] = 0;
    }
    for (int j = 0; j < item->length_; j ++ ) {
      int start = j * 16;
      for (int l = 0; l < 16; l ++ ) {
        if (bitem.bof_info_.word_info_[0][l + start] <= '9') {
          item->he_[j] += (bitem.bof_info_.word_info_[0][l + start] - '0') *
                          off_value[l];
        }else {
          item->he_[j] += (bitem.bof_info_.word_info_[0][l + start] - 'A' + 10) *
                          off_value[l];
        }
      }
    }

    //angle
    item->angle_ = new unsigned int[item->length_];
    for (int i = 0; i < item->length_; i++ ) {
      item->angle_[i] = 0;
    }
    for (int j = 0; j < item->length_; j ++ ) {
      item->angle_[j] += (bitem.bof_info_.word_info_[1][3*j + 0] - '0') * 100;
      item->angle_[j] += (bitem.bof_info_.word_info_[1][3*j + 1] - '0') * 10;
      item->angle_[j] += (bitem.bof_info_.word_info_[1][3*j + 2] - '0') * 1;
    }
    word.items_.push_back(item);
   /* int he_len = (int)bitem.bof_info_.word_info_[0].length() + 1;
    item->he_ = new char[he_len];
    strcpy(item->he_, bitem.bof_info_.word_info_[0].c_str());
    int angle_len = (int)bitem.bof_info_.word_info_[1].length() + 1;
    item->angle_ = new char[angle_len];
    strcpy(item->angle_, bitem.bof_info_.word_info_[1].c_str());
    word.items_.push_back(item);
    */
  }
  return 0;
}


//////////////////////////// class BOFIndex ///////////////////////////////////
BOFIndex* BOFIndex::instance_ = NULL;
BOFIndex::BOFIndex() {
  begin_word_id_ = 0;
}

BOFIndex::~BOFIndex() {
  if (hinter_) {
    delete hinter_;
    hinter_ = NULL;
  }
}

BOFIndex* BOFIndex::GetInstance() {
  if (NULL == instance_) {
    instance_ = new BOFIndex();
  }
  return instance_;
}

const int BOFIndex::Init(const int max_bucket_num,
                         const std::string &hint_file,
                         const std::string &data_file) {
  // load index hint
  hinter_ = new IndexHinter(max_bucket_num);
  if (NULL == hinter_) {
    return -1;
  }
  std::fstream fs;
  fs.open(hint_file.c_str(), std::fstream::in | std::fstream::binary);
  if (!fs.is_open()) {
    return -2;
  }
  boost::archive::text_iarchive ia(fs);
  ia >> *hinter_;
  // load index data
  fs_data_.open(data_file.c_str(), std::fstream::in | std::fstream::binary);
  if (!fs_data_.is_open()) {
    return -3;
  }
  return 0;
}

const int BOFIndex::LoadWord(const int word_id, WordIndex &word_index, char *&data) {
  boost::mutex::scoped_lock lock(io_mutex_);
  HinterItem hint_item;
  data = NULL;
  if (0 != hinter_->GetHint(word_id, hint_item)) {
    LOGGER_ERROR(logger_, "Get bucket %d hint failed", word_id);
    return -1;
  }
  if (hint_item.start_pos_ == hint_item.end_pos_) {
    LOGGER_ERROR(logger_, "Bucket %d is empty, start_pos=%ld, end_pos=%ld",
                 word_id, hint_item.start_pos_, hint_item.end_pos_);
    return -2;
  }

  timeval tv1, tv2, tv3;
  gettimeofday(&tv1, NULL);
  int data_len = hint_item.end_pos_ - hint_item.start_pos_ + 1;
  data = new char[data_len];
  data[data_len-1] = '\0';
  fs_data_.seekg(hint_item.start_pos_);
  fs_data_.read(data, data_len-1);
  gettimeofday(&tv2, NULL);
  IndexerSerializer index_serializer;
  if (0 != index_serializer.Unserialize(data, data_len, word_index)) {
    return -3;
  }
  gettimeofday(&tv3, NULL);
  unsigned long ts1 = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
  unsigned long ts2 = ((tv3.tv_sec-tv2.tv_sec)*1000000+(tv3.tv_usec-tv2.tv_usec)) / 1000;
  LOGGER_INFO(logger_, "Get word %d timed consumed: %ld-%ld", word_id, ts1, ts2);
  return 0;
}

const int BOFIndex::SetBeginWordId(const int begin_word_id) {
  begin_word_id_ = begin_word_id;
  return 0;
}

const int BOFIndex::LoadFileData(const int begin_id, const int end_id) {
  int size = end_id - begin_id;
  if (size <= 0) {
    LOGGER_ERROR(logger_, "LoadFileData fail begin_id = %d, end_id = %d", begin_id, end_id);
    return -1;
  }
  int manager_begin_id  = begin_id - begin_word_id_;
  int manager_end_id = end_id - begin_word_id_;
  int current_id = begin_id;
  mem_words_manager_.resize(size);
  for (int i = manager_begin_id; i < manager_end_id; i++) {
    WordIndex word_index;
    char *word_data = NULL;
    if (0 != BOFIndex::GetInstance()->LoadWord(current_id, word_index, word_data)) {
      if (word_data) {
        delete []word_data;
        word_data = NULL;
      }
      LOGGER_ERROR(logger_, "LoadWord fail ");
      return -1;
    }
    MemWord mem_word(word_index, word_data);
    mem_words_manager_[i] = mem_word;
    LOGGER_INFO(logger_, "load file data word_id = %d", current_id);
    ++current_id;
  }
  return 0;
}

const int BOFIndex::ReleaseFileData() {
  std::vector<MemWord>::iterator it = mem_words_manager_.begin();
  while(it != mem_words_manager_.end()) {
    it->ReleaseWord();
    ++it;
  }
  return 0;
}

const int BOFIndex::GetWord(const int word_id, WordIndex &word_index) {
  int manager_word_id = word_id - begin_word_id_;
  if (mem_words_manager_.size() == 0) {
    LOGGER_INFO(logger_, "mem words manager size is 0");
    return -1;
  }
  mem_words_manager_[manager_word_id].GetWordIndex(word_index);
  if (word_index.word_id_ == word_id) {
    LOGGER_INFO(logger_, "GetWord success word_id=%d", word_id);
    return 0;
  } else {
    LOGGER_ERROR(logger_, "GetWord fail word_id=%d", word_id);
    return -1;
  }
}
