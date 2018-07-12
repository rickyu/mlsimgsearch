#include <bof_searcher/index_hinter.h>


IndexHinter::IndexHinter(int max_bucket_num)
  : max_bucket_num_(max_bucket_num) {
  items_.clear();
  items_.resize(max_bucket_num_);
}

IndexHinter::~IndexHinter() {
}

int IndexHinter::SetHint(int bucket_id, int64_t bucket_size,
                         int64_t start_pos, int64_t end_pos) {
  if (bucket_id >= (int)items_.size()) {
    return -1;
  }
  HinterItem item;
  item.bucket_size_ = bucket_size;
  item.start_pos_ = start_pos;
  item.end_pos_ = end_pos;
  items_[bucket_id] = item;
  return 0;
}

int IndexHinter::GetHint(int bucket_id, HinterItem &item) {
  if (bucket_id >= (int)items_.size()) {
    return -1;
  }
  item = items_[bucket_id];
  return 0;
}
