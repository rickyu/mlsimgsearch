#pragma once

// #include "service.h"
#include <unordered_map>
#include <list>
#include <boost/thread.hpp>
#include "property.h"
#include "array_list.h"
#include <utils/compile_aux.h>

namespace blitz {


uint32_t GetIntValueFirstN(const std::string key, int n);
  
struct ServiceStat {
  uint32_t sid;
  std::string addr;

  int32_t weight;//权重
  int32_t query_count;//调用次数
  int32_t success_count;//成功次数
  int32_t fail_count;//失败次数
  int32_t delay;//平均时延
  int32_t delay_max;//最大时延
};

class ServiceSelector {
public:
  virtual ~ServiceSelector() {}
  virtual int SelectService(uint32_t &sid, const void *key = NULL, size_t key_len = 0) = 0;
  virtual int Calculate(const ServiceStat* stats, size_t cnt) = 0;
};

class RandomServiceSelector : public ServiceSelector {
public:
  RandomServiceSelector();
  ~RandomServiceSelector();
  int SelectService(uint32_t &sid, const void *key = NULL, size_t key_len = 0);
  int Calculate(const ServiceStat* stats, size_t cnt);

private:
  uint32_t *sids_;
  size_t sids_count_;
  size_t sids_cursor_;
  boost::mutex mutex_;
};

class ConsistentHashServiceSelector : public ServiceSelector {
private:
  typedef struct _CHServiceStat {
    ServiceStat stat;
    uint32_t hash_value;
  } CHServiceStat;

public:
  ConsistentHashServiceSelector();
  ~ConsistentHashServiceSelector();

  int SelectService(uint32_t &sid, const void *key = NULL, size_t key_len = 0);
  int Calculate(const ServiceStat* stats, size_t cnt);

private:
  static int ServiceStatCmp(const void *s1, const void *s2);

private:
  CHServiceStat *service_stats_;
  uint32_t stats_count_;

  boost::mutex mutex_;
};

class SelectKey {
 public:
  const char* main_key;
  uint64_t sub_key;
};
class SelectNode {
 public:
  SelectNode() {
    mark_ = 100;
    weight_ = 1;
    hash_ = 0;
  }
  SelectNode(void* data, int weight) {
    mark_ = 100;
    weight_ = weight;
    hash_ = 0;
    data_ = data;
  }
  int get_weight() const { return weight_; }
  void set_weight(int weight) { weight_ = weight;}
  int get_mark() const { return mark_; }
  void set_mark(int mark) { mark_ = mark; }
  uint32_t get_hash() const { return hash_; }
  void set_hash(uint32_t hash) { hash_ = hash; }
  void set_data(void* data) { data_ = data; }
  void* get_data() { return data_; }
 private:
  int mark_;
  int weight_;
  uint32_t hash_;
  void* data_;
  int32_t service_id_;
};

/**
 * 思路: 给每个service算一个mark，范围为[-100, 100]. 失败一次减5分,
 * 每隔10秒加10分. 分数<=0的service不被选到.
 */
class Selector {
 public:
  virtual int AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id) = 0;
  virtual int RemoveItem(int32_t service_id) = 0;
  virtual SelectNode* GetItem(int32_t id) = 0;
  virtual int Select(const SelectKey& key, SelectNode* result) = 0;
  // 更新service的分数.
  virtual int UpdateMark(int32_t service_id, int mark_delta) = 0;
  // 给所有service加分.
  virtual int UpdateAllMark(int mark_delta) = 0;
  virtual ~Selector() {}
 protected:
  int UpdateNodeMark(SelectNode* node, int mark_delta);
};

// 随机选择可用的服务.
class RandomSelector : public Selector {
 public:
  RandomSelector() {
    next_id_ = 0;
    avail_items_count_ = 0;
    avail_iter_ = avail_items_.begin();
    avail_used_count_ = 0;
    unavail_items_count_ = 0;
    unavail_iter_ = unavail_items_.begin();
    unavail_used_count_ = 0;
  }
  virtual int AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id);
  virtual int RemoveItem(int32_t service_id);
  virtual SelectNode* GetItem(int32_t id);
  virtual int Select(const SelectKey& key, SelectNode* result);
  virtual int UpdateMark(int32_t service_id, int mark_delta);
  virtual int UpdateAllMark(int mark_delta);
 private:
  typedef std::list<SelectNode> TList;
  struct MapNode {
    TList::iterator iter;
    TList* list;
  };
  typedef std::unordered_map<int32_t, MapNode> TMap;
  int32_t next_id_;
  TMap services_;

  TList avail_items_;
  int32_t avail_items_count_;
  TList::iterator avail_iter_;
  int32_t avail_used_count_;
  
  TList unavail_items_;
  int32_t unavail_items_count_;
  TList::iterator unavail_iter_;
  int32_t unavail_used_count_;
};

template <typename T, typename Compare>
std::pair<size_t, bool>  BinarySearch(const T& v, const T* array, size_t count,
    Compare compare) {
  if (!array || count==0) {
    return std::pair<size_t, bool>(0, false);
  }
  size_t l = 0;
  size_t r = count;
  size_t mid = 0;
  int cmp = 0;
  while (l<r) {
    mid = (l+r)/2;
    cmp = compare(v, array[mid]);
    if (cmp == 0) {
      return std::pair<size_t, bool>(mid, true);
    } else if (cmp < 0) {
      r = mid;
    } else {
      l = mid+1;
    }
  } 
  return std::pair<size_t, bool>(cmp > 0 ? mid+1 : mid, false);
}
template <typename T, typename Compare>
inline std::pair<size_t, bool>  BinarySearchVector(const T& v, const std::vector<T>& array, 
    Compare compare) {
  if (array.empty()) {
    return std::pair<size_t, bool>(0, false);
  }
  return BinarySearch(v, &array[0], array.size(), compare);
}


// 一致性hash选择.
class ConsistHashSelector: public Selector {
 public:
  ConsistHashSelector() {
    next_id_ = 0;
  }
  virtual int AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id);
  virtual int RemoveItem(int32_t service_id);
  virtual SelectNode* GetItem(int32_t id);
  virtual int Select(const SelectKey& key, SelectNode* result);
  virtual int UpdateMark(int32_t service_id, int mark_delta);
  virtual int UpdateAllMark(int mark_delta);
 private:
  // 类型定义.
  struct MapNode {
    uint32_t hash;
    int status; // 1: avail, 2: error
    SelectNode node;
  };
  struct ListNode {
    uint32_t hash;
    SelectNode* node_ptr;
  };
  typedef std::unordered_map<int32_t, std::vector<MapNode> > TMap;
 private:
  void InsertToVector(std::vector<ListNode>& vec, size_t pos,
      const ListNode& val) {
    vec.insert(vec.begin()+pos, val);
  }
  void RemoveFromVector(std::vector<ListNode>& vec, size_t pos) {
    vec.erase(vec.begin()+pos);
  }
  int MoveNode(uint32_t hash, std::vector<ListNode>* from,
      std::vector<ListNode>* to);
  static int CompareNodeHash(const ListNode& a, const ListNode& b) {
    if (a.hash == b.hash) { return 0; }
    else if (a.hash < b.hash) { return -1;}
    else  { return 1; }
  }
 private:
  TMap items_; // 存储id到hash的映射.
  std::vector<ListNode> avail_items_; // 按hash值排序.
  std::vector<ListNode> error_items_; // 按hash值排序.
  int32_t next_id_;
};

//模式
class PatternSelector: public Selector {
 public:
  PatternSelector() {
    next_id_ = 0;
  }
  virtual int AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id);
  virtual int RemoveItem(int32_t service_id);
  virtual SelectNode* GetItem(int32_t id);
  virtual int Select(const SelectKey& key, SelectNode* result);
  virtual int UpdateMark(int32_t service_id, int mark_delta);
  virtual int UpdateAllMark(int mark_delta);
 private:
  // 类型定义.
  struct MapNode {
    std::string name;
    int status; // 1: avail, 2: error
    SelectNode node;
  };
  struct ListNode {
    std::string name;
    SelectNode* node_ptr;
  };
  typedef std::unordered_map<int32_t, MapNode > TMap;
 private:
  TMap items_; // 存储id到name的映射.
  std::vector<ListNode> avail_items_; 
  std::vector<ListNode> error_items_; 
  int32_t next_id_;
};
// 取模.
class ModSelector: public Selector {
};

class FixHashSelector: public Selector {
};
// 按号段. sid [s1, s2]
class SegRangeSelector: public Selector {
};
} // namespace blitz.
