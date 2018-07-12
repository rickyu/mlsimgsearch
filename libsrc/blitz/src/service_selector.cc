#include <utils/crc32.h>
#include "utils/md5_new.h"
#include "blitz/service_selector.h"

namespace blitz {
uint32_t GetIntValueFirstN(const std::string key, int n) {
  std::string md5_str = md5(key); 
  int len = static_cast<int>(md5_str.length());
  if (len != 32) {
    return 0;
  }
  char* first_n = (char*)malloc(8*(n+3));
  if (NULL == first_n) {
    return 0;
  }
  first_n[0] = '0';
  first_n[1] = 'x';
  for (uint8_t i=0; i<n; i++) {
    first_n[i+2] = md5_str[i];
  }
  first_n[n+2] = '\0';
  uint32_t val= strtol(first_n, NULL, 16);
  free(first_n);
  first_n = NULL;
  return val;
}
////////////// RandomServiceSelector /////////////////
RandomServiceSelector::RandomServiceSelector()
{
  sids_ = NULL;
  sids_count_ = 0;
  sids_cursor_ = 0;
}

RandomServiceSelector::~RandomServiceSelector()
{
  if (sids_) {
    delete [] sids_;
    sids_ = NULL;
  }
}

int RandomServiceSelector::SelectService(uint32_t &sid, const void *key /*= NULL*/, size_t key_len /*= 0*/)
{
  AVOID_UNUSED_VAR_WARN(key);
  AVOID_UNUSED_VAR_WARN(key_len);

  boost::mutex::scoped_lock guard(mutex_);
  sid =  sids_[(sids_cursor_ ++) % sids_count_];
  return 0;
}

int RandomServiceSelector::Calculate(const ServiceStat* stats,size_t cnt)
{
  size_t total_cnt = 0;
  uint32_t* sids = new uint32_t[cnt * 10];
  for(size_t i = 0; i < cnt; ++ i){
    if(stats[i].fail_count > 5){
      continue;
    }
    int weight = stats[i].weight > 10 ? 10 : stats[i].weight;
    for(int i = 0; i < weight; ++ i){
      sids[total_cnt ++] = stats[i].sid;
    }
  }
  if(total_cnt == 0){
    for(size_t i = 0; i < cnt; ++ i){
      sids[total_cnt ++ ] = stats[i].sid;
    }
  }
  std::random_shuffle(sids,sids + total_cnt);
  {
    boost::mutex::scoped_lock guard(mutex_);
    uint32_t* tmp = sids_;
    sids_ = sids;
    sids_count_ = total_cnt;
    sids_cursor_ = 0;
    if(tmp){
      delete [] tmp;
    }
  }
  return 0;
}


////////// consistent hash selector: from libmemcached ///////////
static uint32_t _memcached_hash_crc32(const void *key, size_t key_len) {
  return ((hash_crc32((char*)key, key_len) >> 16) & 0x7fff);
}

ConsistentHashServiceSelector::ConsistentHashServiceSelector()
  : service_stats_(NULL)
  , stats_count_(0)
{
}

ConsistentHashServiceSelector::~ConsistentHashServiceSelector()
{
  if (service_stats_) {
    delete [] service_stats_;
    service_stats_ = NULL;
  }
}

int ConsistentHashServiceSelector::ServiceStatCmp(const void *s1, const void *s2)
{
  if (!s1 || !s2)
    return -1;

  CHServiceStat *stat1 = (CHServiceStat *)s1;
  CHServiceStat *stat2 = (CHServiceStat *)s2;
  if (stat1->hash_value == stat2->hash_value)
    return 0;
  else if (stat1->hash_value >stat2->hash_value)
    return 1;

  return -1;
}

int ConsistentHashServiceSelector::SelectService(uint32_t &sid,
						 const void *key /*=NULL*/,
						 size_t key_len /*= 0*/)
{
  if (!service_stats_ || !key || !key_len)
    return -1;

  //CHServiceStat *begin, *left, *middle, *right, *end;
  uint32_t begin, left, middle, right, end;
  uint32_t hash = _memcached_hash_crc32(key, key_len);

  begin = left = 0;
  end = right = stats_count_-1;

  while (left < right) {
    middle = left + (right - left) / 2;
    if (service_stats_[middle].hash_value < hash)
      left = middle + 1;
    else
      right = middle;
  }
  //	if (right == end)
  //		right = begin;

  sid = service_stats_[right].stat.sid;
  return 0;
}

int ConsistentHashServiceSelector::Calculate(const ServiceStat* stats, size_t cnt)
{
  if (!stats)
    return -1;

  CHServiceStat *ch_stats = new CHServiceStat[cnt];
  uint32_t valid_count = 0;
  // 淘汰无效的节点 (失败次数>5)
  for (size_t i = 0; i < cnt; i++) {
    if (stats[i].fail_count > 5)
      continue;

    ch_stats[i].stat = stats[i];
    ch_stats[i].hash_value = _memcached_hash_crc32((char*)stats[i].addr.c_str(), stats[i].addr.length());
    ++valid_count;
  }

  qsort(ch_stats, cnt, sizeof(CHServiceStat), ServiceStatCmp);

  {
    boost::mutex::scoped_lock guard(mutex_);
    CHServiceStat *tmp = service_stats_;
    stats_count_ = valid_count;
    service_stats_ = ch_stats;
    if (tmp)
      delete [] tmp;
  }
  return 0;
}
// ============= Selector =====================
int Selector::UpdateNodeMark(SelectNode* node, int mark_delta) {
  assert(node);
  int new_mark = node->get_mark() + mark_delta;
  if (new_mark < -100) {
    new_mark = -100;
  }
  if (new_mark > 100) {
    new_mark = 100;
  }
  node->set_mark(new_mark);
  return new_mark;
}
int RandomSelector::AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id) {
  AVOID_UNUSED_VAR_WARN(properties);
  AVOID_UNUSED_VAR_WARN(name);
  AVOID_UNUSED_VAR_WARN(weight);
  AVOID_UNUSED_VAR_WARN(addr);
  AVOID_UNUSED_VAR_WARN(virtual_count);
  SelectNode node(user_data, weight);
  int32_t id = ++next_id_;
  TList::iterator iter = avail_items_.insert(avail_items_.begin(), node);
  MapNode map_node;
  map_node.iter = iter;
  map_node.list = &avail_items_;
  services_[id] = map_node;
  *service_id = id;
  if (avail_iter_ == avail_items_.end()) {
    avail_iter_ = avail_items_.begin();
  }
  if (unavail_iter_ == unavail_items_.end()) {
    unavail_iter_ = unavail_items_.begin();
  }
  ++avail_items_count_;
  return 0;
}
int RandomSelector::RemoveItem(int32_t service_id) {
  TMap::iterator map_iter = services_.find(service_id);
  if (map_iter == services_.end()) {
    return 1;
  }
  if (map_iter->second.iter == avail_iter_ ) {
    ++avail_iter_;
    if (avail_iter_ == avail_items_.end()) {
      avail_iter_ = avail_items_.begin();
    }
    avail_used_count_ = 0;
  } else if (map_iter->second.iter == unavail_iter_) {
    ++unavail_iter_;
    if (unavail_iter_ == unavail_items_.end()) {
      unavail_iter_ = unavail_items_.begin();
    }
    unavail_used_count_ = 0;
  } 
  if (map_iter->second.list == &avail_items_) {
    --avail_items_count_;
  } else if (map_iter->second.list == &unavail_items_) {
    --unavail_items_count_;
  }
  map_iter->second.list->erase(map_iter->second.iter);
  return 0;
}
SelectNode* RandomSelector::GetItem(int32_t id) {
  TMap::iterator iter = services_.find(id);
  if (services_.end() == iter) {
    return NULL;
  }
  return &(*iter->second.iter);
}
int RandomSelector::Select(const SelectKey& key, SelectNode* result) {
  AVOID_UNUSED_VAR_WARN(key);
  if (avail_items_count_ > 0) {
    if (avail_used_count_ < avail_iter_->get_weight()) {
      ++avail_used_count_;
      *result = *avail_iter_;
      return 0;
    }
    ++ avail_iter_;
    avail_used_count_ = 0;
    if (avail_iter_ == avail_items_.end()) {
      avail_iter_ = avail_items_.begin();
    }
    *result = *avail_iter_;
    return 0;
  }
  if (unavail_items_count_ > 0) {
    if (unavail_used_count_ < unavail_iter_->get_weight()) {
      ++unavail_used_count_;
      *result = *unavail_iter_;
      return 0;
    }
    ++ unavail_iter_;
    unavail_used_count_ = 0;
    if (unavail_iter_ == unavail_items_.end()) {
      unavail_iter_ = unavail_items_.begin();
    }
    *result = *unavail_iter_;
    return 0;
  }
  return -1;
}
int RandomSelector::UpdateMark(int32_t service_id, int mark_delta)  {
  TMap::iterator map_iter = services_.find(service_id);
  if (map_iter == services_.end()) {
    return 1;
  }
  SelectNode* node = &(*map_iter->second.iter);
  if (!node) {
    return -1;
  }
  int new_mark = node->get_mark() + mark_delta;
  if (new_mark < -100) {
    new_mark = -100;
  }
  if (new_mark > 100) {
    new_mark = 100;
  }
  node->set_mark(new_mark);
  if (node->get_mark() <= 0 && map_iter->second.list == &avail_items_) {
    // 移到unavail.
    ++unavail_items_count_;
    --avail_items_count_;
    if (avail_iter_ == map_iter->second.iter) {
      ++avail_iter_;
    }
    unavail_items_.splice(unavail_items_.end(), avail_items_, map_iter->second.iter);
    if (unavail_iter_ == unavail_items_.end()) {
      unavail_iter_ = unavail_items_.begin();
    }
    if (avail_iter_ == avail_items_.end()) {
        avail_iter_ = avail_items_.begin();
    }
    map_iter->second.list = &unavail_items_;
  } else if (node->get_mark() > 0 && map_iter->second.list == &unavail_items_ ) {
    // 移到avail
    --unavail_items_count_;
    ++avail_items_count_;
    if (unavail_iter_ == map_iter->second.iter) {
      ++unavail_iter_;
    }
    avail_items_.splice(avail_items_.end(), unavail_items_, map_iter->second.iter);
    if (avail_iter_ == avail_items_.end()) {
      avail_iter_ = avail_items_.begin();
    }
    if (unavail_iter_ == unavail_items_.end()) {
        unavail_iter_ = unavail_items_.begin();
    }
    map_iter->second.list = &avail_items_;
  }
  return 0;
}
int RandomSelector::UpdateAllMark(int mark_delta) {
  TMap::iterator iter = services_.begin();
  TMap::iterator end = services_.end();
  for (; iter != end; ++iter) {
    UpdateMark(iter->first, mark_delta);
  }
  return 0;
}
// ========================ConsistHashSelector ========================

int ConsistHashSelector::AddItem(void* user_data, int weight,
      const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id) {
  AVOID_UNUSED_VAR_WARN(properties);
  AVOID_UNUSED_VAR_WARN(name);
  int32_t id = ++next_id_;
  /*std::pair<int64_t, bool> prop = properties.GetInt("consist-hash");
  if (!prop.second) {
    // 缺少属性值.
    return -1;
  }*/
  char virtual_str[64];
  std::string tmp = addr.substr(6,addr.length());
  for (int i = 0; i < virtual_count; i++) {
    virtual_str[64] = {'\0'};
    sprintf(virtual_str, "#%lu", static_cast<unsigned long>(i));
    std::string str_to_hash = tmp + std::string(virtual_str);
    uint32_t hash_val = GetIntValueFirstN(str_to_hash, 8);
    // 二分查找到要插入的位置.
    ListNode* list_node = new ListNode();
    list_node->hash = hash_val;
    std::pair<size_t, bool> search_result = BinarySearchVector(*list_node, 
        avail_items_, CompareNodeHash);
    if (search_result.second) {
      // 如果已经找到，说明hash重复, 不支持.
      return 1;
    }
  
    // 插入到map中.
    MapNode* map_node = new MapNode();
    map_node->hash = list_node->hash;
    map_node->status = 1;
    // map_node.node.set_id(id); 
    map_node->node.set_weight(weight);
    map_node->node.set_data(user_data);
    if (i == 0) {
      std::vector<MapNode> vec;
      vec.push_back(*map_node);
      std::pair<TMap::iterator, bool> ins_res = items_.insert(
          TMap::value_type(id, vec));
      if (!ins_res.second) {
        return -2;
      }
    } else {
      items_[id].push_back(*map_node);
    }
    // 插入到有序数组中.
    list_node->node_ptr = &(map_node->node);
    InsertToVector(avail_items_, search_result.first, *list_node);
    //avail_items_.push_back(*list_node);
  }
  *service_id = id;
  return 0;
}
int ConsistHashSelector::RemoveItem(int32_t service_id) {
  TMap::iterator map_iter = items_.find(service_id);
  if (map_iter == items_.end()) {
    // 不存在.
    return 1;
  }
  std::vector<MapNode>::iterator iter = map_iter->second.begin();
  for (; iter != map_iter->second.end(); iter ++) {
    std::vector<ListNode>* target_vec = NULL;
    if (iter->status == 1) {
      target_vec = &avail_items_;
    } else {
      target_vec = &error_items_;
    }
    ListNode list_node;
    list_node.hash = iter->hash;
    std::pair<size_t, bool> result = BinarySearchVector(list_node, *target_vec,
        CompareNodeHash);
    assert(result.second);
    RemoveFromVector(*target_vec, result.first);
  }
  items_.erase(map_iter);
  return 0;
}
SelectNode* ConsistHashSelector::GetItem(int32_t id) {
  TMap::iterator map_iter = items_.find(id);
  if (map_iter == items_.end()) {
    // 不存在.
    return NULL;
  }
  return &(map_iter->second[0].node);
}
int ConsistHashSelector::Select(const SelectKey& key, SelectNode* node) {
  ListNode list_node;
  list_node.hash = GetIntValueFirstN(key.main_key, 8);
  std::vector<ListNode>* target_vec = &avail_items_;
  if (!target_vec->empty()) {
    std::pair<size_t, bool> result = BinarySearchVector(list_node, *target_vec, 
        CompareNodeHash);
    *node = *((*target_vec)[result.first % target_vec->size()].node_ptr);
    return 0;
  }
  target_vec = &error_items_;
  if (!target_vec->empty()) {
    std::pair<size_t, bool> result = BinarySearchVector(list_node, *target_vec, 
        CompareNodeHash);
    *node = *((*target_vec)[result.first % target_vec->size()].node_ptr);
    return 0;
  }
  return 1;
}
int ConsistHashSelector::UpdateMark(int32_t service_id, int mark_delta) {
  AVOID_UNUSED_VAR_WARN(mark_delta);
  AVOID_UNUSED_VAR_WARN(service_id);
  /*TMap::iterator map_iter = items_.find(service_id);
  if (map_iter == items_.end()) {
    // 不存在.
    return 1;
  }
  std::vector<MapNode>::iterator iter = map_iter->second.begin();
  for (; iter != map_iter->second.end(); iter ++) {
    int new_mark = UpdateNodeMark(&iter->node, mark_delta);
    if (new_mark <= 0 && iter->status == 1) {
      // 原来是avail，需要移到error中.
      MoveNode(iter->hash, &avail_items_, &error_items_);
      iter->status = 2;
    } else if (new_mark > 0 && iter->status == 2) {
      // 原来在error中, 需要遇到avail中
      MoveNode(iter->hash, &error_items_, &avail_items_);
      iter->status = 1;
    }
  }*/
  return 0;
}
int ConsistHashSelector::UpdateAllMark(int mark_delta) {
 /* TMap::iterator iter = items_.begin();
  TMap::iterator end = items_.end();
  for (; iter!=end; ++iter) {
    UpdateMark(iter->first, mark_delta);
  }*/
  AVOID_UNUSED_VAR_WARN(mark_delta);
  return 0;
}
int ConsistHashSelector::MoveNode(uint32_t hash, std::vector<ListNode>* from,
    std::vector<ListNode>* to) {
  ListNode list_node;
  list_node.hash = hash;
  std::pair<size_t, bool> search_result = BinarySearchVector(list_node, *from,
      CompareNodeHash);
  if (!search_result.second) {
    // from中不存在.
    return 1;
  }
  
  std::pair<size_t, bool> search_result2 = BinarySearchVector(list_node, *to,
      CompareNodeHash);
  if (search_result2.second) {
    // to中存在，hash重复.
    return 2;
  }
  list_node.node_ptr = ((*from)[search_result.first]).node_ptr;
  RemoveFromVector(*from, search_result.first);
  InsertToVector(*to, search_result2.first, list_node);
  return 0;
}

int PatternSelector::AddItem(void* user_data, int weight, const PropertyMap& properties, const std::string& name, const std::string& addr, int virtual_count, int32_t* service_id) {
  AVOID_UNUSED_VAR_WARN(properties);
  AVOID_UNUSED_VAR_WARN(weight);
  AVOID_UNUSED_VAR_WARN(addr);
  AVOID_UNUSED_VAR_WARN(virtual_count);
  int32_t id = ++next_id_;
  MapNode* map_node = new MapNode();
  map_node->name = name;
  map_node->status = 1;
  map_node->node.set_weight(weight);
  map_node->node.set_data(user_data);
  ListNode* list_node = new ListNode();
  list_node->name = name;
  list_node->node_ptr = &(map_node->node);
  avail_items_.push_back(*list_node);
  *service_id = id;
  items_.insert(TMap::value_type(id,*map_node));
  return 0;
}

int PatternSelector::RemoveItem(int32_t service_id) {
  TMap::iterator map_iter = items_.find(service_id);
  if (map_iter == items_.end()) {
    // 不存在.
    return 1;
  }
  if (map_iter->second.status == 1) {
    std::vector<ListNode>::iterator iter = avail_items_.begin();
    while(iter!=avail_items_.end()) {
      if (iter->name == map_iter->second.name)
        avail_items_.erase(iter);
      else 
        iter ++;
    }
  } else {
    std::vector<ListNode>::iterator iter = error_items_.begin();
    while(iter != error_items_.end()) {
      if (iter->name == map_iter->second.name)
        error_items_.erase(iter);
      else 
        iter ++;
    }
  }
  items_.erase(map_iter);
  return 0;
}

SelectNode* PatternSelector::GetItem(int32_t id) {
  TMap::iterator map_iter = items_.find(id);
  if (map_iter == items_.end()) {
    // 不存在.
    return NULL;
  }
  return &(map_iter->second.node);
}

int PatternSelector::Select(const SelectKey& key, SelectNode* node) {
  std::string redis_key = key.main_key;
  size_t pos = redis_key.find(':');
  if (pos == std::string::npos) {
    pos = redis_key.length();
  }
  std::string pattern = redis_key.substr(0, pos);
  std::vector<ListNode>::iterator iter = avail_items_.begin();
  for (; iter != avail_items_.end(); ++ iter) {
    if (iter->name == pattern)
      break;
  }
  if (iter != avail_items_.end()) {
    *node = *(iter->node_ptr);
    return 0;
  } 
  iter = error_items_.begin();
  for (; iter != error_items_.end(); ++ iter) {
    if (iter->name == pattern)
      break;
  }
  if (iter != error_items_.end()) {
    *node = *(iter->node_ptr);
    return 0;
  }
  return 1;
}

int PatternSelector::UpdateMark(int32_t service_id, int mark_delta) {
  AVOID_UNUSED_VAR_WARN(service_id);
  AVOID_UNUSED_VAR_WARN(mark_delta);
  return 0;
  // 目前PatternSelector下可用服务数都为1,不写评分了
}

int PatternSelector::UpdateAllMark(int mark_delta) {
  AVOID_UNUSED_VAR_WARN(mark_delta);
  return 0;
}
    






} // namespace blitz.
