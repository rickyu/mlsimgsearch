#pragma once

// #include "service.h"
#include <unordered_map>
#include <vector>
#include <list>
#include <blitz2/configure.h>

namespace blitz2
{
template<typename T>
class SelectNode {
 public:
  SelectNode() {
    id_ = 0;
    mark_ = 100;
    weight_ = 1;
    hash_ = 0;
  }
  SelectNode(int id, const T& data, int weight) 
      :data_(data){
    id_ = id;
    mark_ = 100;
    weight_ = weight;
    hash_ = 0;
  }
  int id() const { return id_;}
  void set_id(int id) { id_ = id; }
  int weight() const { return weight_; }
  void set_weight(int weight) { weight_ = weight;}
  int mark() const { return mark_; }
  void set_mark(int mark) { mark_ = mark; }
  uint32_t hash() const { return hash_; }
  void set_hash(uint32_t hash) { hash_ = hash; }
  void set_data(const T& data) { data_ = data; }
  const T& data() const { return data_; }
 private:
  int id_;
  int mark_;
  int weight_;
  uint32_t hash_;
  T data_;
};

template<typename T>
struct OneSelectResult {
  int id;
  T item;
  int mark;
};
/**
 * all selector must have the method.
 * int AddItem(const TItem& item, int weight, const Configure& conf);
 * int Select(const TKey& key, TOneResult* result);
 * int SelectById(int id, TOneResult* result);
 * int UpdateMark(int id, int mark_delta);
 * int UpdateAllMark(int mark_delta);
 *
 */
template<typename Key, typename Item>
class RandomSelector {
 public:
  typedef Key TKey;
  typedef Item TItem;
  typedef SelectNode<Item> TSelectNode;
  typedef OneSelectResult<Item> TOneResult;
  RandomSelector() {
    curnode_used_count_ = 0;
    select_idx_ = 0;
  }
  ~RandomSelector() {
    Uninit();
  }

  int AddItem(const TItem& item, int weight,
      const Configure& conf);
  int Select(const TKey& key, OneSelectResult<Item>* result);
  int SelectById(int id, OneSelectResult<Item>* result);
  int UpdateMark(int id, int mark_delta);
  void UpdateAllMark(int mark_delta);
  bool IsValidId(int id) const {
    return (id>=0 && id<static_cast<int>(nodes_.size()));
  }
  void Uninit();
 private:
  static void Node2SelectResult(OneSelectResult<Item>* result, 
      const SelectNode<Item>& node) {
    result->id = node.id();
    result->item = node.data();
    result->mark = node.mark();
  }
private:
  typedef std::vector<TSelectNode> TVector;
  std::vector<TSelectNode> nodes_;
  uint32_t curnode_used_count_;
  int select_idx_;
};

template<typename Key, typename Item>
int RandomSelector<Key,Item>::AddItem(const TItem& item, int weight,
    const Configure& conf) {
  (void)(conf);
  int id = static_cast<int>(nodes_.size());
   
  nodes_.push_back(TSelectNode(id, item, weight));
  return id;
}
template<typename Key, typename Item>
int RandomSelector<Key, Item>::Select(const TKey& key,
                                      OneSelectResult<Item>* result) {
  (void)(key);
  int size = nodes_.size();
  if(size==0) {
    return -1;
  }
  TSelectNode* nodes = &nodes_[0];
  if(nodes[select_idx_].mark() > 0 
      &&nodes[select_idx_].weight() > (int)curnode_used_count_) {
      Node2SelectResult(result, nodes[select_idx_]);
      ++curnode_used_count_;
      return 0;
  }
  curnode_used_count_ = 0;
  
  for(int idx = (select_idx_+1) % size; idx!=select_idx_; idx=(idx+1)%size) {
    if(nodes[idx].mark() > 0) {
      Node2SelectResult(result, nodes[idx]);
      ++curnode_used_count_;
      select_idx_ = idx;
      return 0;
    }
  }
  // anyway , we'll select one node.
  select_idx_ = (++select_idx_) % size;
  Node2SelectResult(result, nodes[select_idx_]);
  ++curnode_used_count_;
  return 0;
}
template<typename Key, typename Item>
int RandomSelector<Key, Item>::SelectById(int id,
                                      OneSelectResult<Item>* result) {
  if (!IsValidId(id)) {
    return -1;
  }
  Node2SelectResult(result, nodes_[id]);
  return 0;
}

template<typename Key, typename Item>
int RandomSelector<Key,Item>::UpdateMark(int id, int mark_delta)  {
  if(id<0 || id>=nodes_.size()) {
    return -1;
  }
  int new_mark = nodes_[id].mark() + mark_delta;
  if (new_mark < -100) {
    new_mark = -100;
  }
  if (new_mark > 100) {
    new_mark = 100;
  }
  nodes_[id].set_mark(new_mark);
  return 0;
}
template<typename Key, typename Item>
void RandomSelector<Key,Item>::UpdateAllMark(int mark_delta) {
  for(int i=0; i<nodes_.size();++i) {
    UpdateMark(i, mark_delta);
  }
}
template<typename Key, typename Item>
void RandomSelector<Key,Item>::Uninit() {
  nodes_.clear();
  curnode_used_count_ = 0;
  select_idx_ = 0;
}


template<typename Key, typename Item>
class RangeSelector {
 public:
  typedef Key TKey;
  typedef Item TItem;
  struct OneResult {
    uint32_t id;
    TItem item;
  };
  typedef OneResult TOneResult;
  class BatchResult {
   public:
    class A {
     std::vector<TKey> keys;
     TOneResult selected_item;
    };
    std::vector<TKey> keys_not_selected;
    std::vector<A> keys_selected;
  };
  typedef BatchResult TBatchResult;
  RangeSelector() {
  }
  int AddItem(const TItem& item, int weight,
      const Configure& conf);
  int Select(const TKey& key, TOneResult* result);
  int Select(const TKey& key, TOneResult* result, int count);
  int SelectBatch(const TKey* keys, size_t keys_count, TBatchResult* result);
  int UpdateMark(int id, int mark_delta);
  int UpdateAllMark(int mark_delta);
 private:
};

} // namespace blitz2
