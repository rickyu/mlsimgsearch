#ifndef __BLITZ_ARRAY_LIST_H_
#define __BLITZ_ARRAY_LIST_H_ 1

#include <ostream>
namespace blitz {
// 用数组实现的list,双向非循环链表.
template <typename T>
class ArrayList {
 public:
  ArrayList() {
    array_ = NULL;
    list_head_.prev = list_head_.next = 0;
    count_ = 0; 
    max_count_ = 0;
  }
  ~ArrayList() {
    if (array_) {
      delete [] array_;
      array_ = NULL;
    }
    count_ = 0;
    max_count_ = 0;
    list_head_.prev = list_head_.next = 0;
  }
  int Init(ssize_t count) {
    if (array_) {
      return 1;
    }
    max_count_ = count;
    array_ = new Item[max_count_ + 1];
    if ( !array_ ) {
      return -1;
    }
    Clear();
    return 0;
  }
  // 问题：不会释放T所包含的资源.
  void Clear() {
    count_  = 0;
    list_head_.next = 0;
    list_head_.prev = 0;
    // 串联空闲链表.
    for (ssize_t i = 0; i < max_count_+1; ++i) {
      array_[i].prev = -1;
      array_[i].next = i+1;
    }
    array_[max_count_].next = 0;
  }
  ssize_t get_count() const {return count_; }
  ssize_t get_max_count() const { return max_count_; }
  ssize_t PushBack(const T& data) {
    assert(array_!=NULL);
    size_t pos = AllocFromFreeList();
    if (pos == 0) {
      return 0;
    }
    array_[pos].next = 0;
    array_[pos].prev = list_head_.prev;
    array_[pos].data = data;
    if (list_head_.prev) {
      // 非空链表.
      array_[list_head_.prev].next = pos;
    } else {
      list_head_.next = pos;
    }
    list_head_.prev = pos;
    return pos;
  }
  ssize_t PushFront(const T& data) {
    ssize_t pos = AllocFromFreeList();
    if (pos == 0) {
      return 0;
    }
    array_[pos].next = list_head_.next;
    array_[pos].prev = 0;
    array_[pos].data = data;
    if (list_head_.next ) {
      // 非空链表.
      array_[list_head_.next].prev = pos;
    } else {
      list_head_.prev = pos;
    }
    list_head_.next = pos;
    return pos;
  }
  ssize_t InsertAfter(ssize_t pos, const T& data) {
    if (pos == 0) {
      // 在头部插入.
      return PushFront(data);
    }
    if (IsFree(pos)) {
      return -2;
    }
    ssize_t new_pos = AllocFromFreeList();
    if (new_pos <= 0) {
      return new_pos;
    }
    array_[new_pos].next = array_[pos].next;
    array_[new_pos].prev = pos;
    array_[new_pos].data = data;
    ssize_t next = array_[pos].next;
    if (next != 0) {
      array_[next].prev = new_pos;
    } else {
      // 在尾部插入.
      list_head_.prev = new_pos;
    }
    array_[pos].next = new_pos;
    return new_pos;
  }
  // 注意T代表的资源不会释放.
  void Remove(ssize_t pos) {
    if (pos <= 0) {
      return;
    }
    size_t prev = array_[pos].prev;
    size_t next = array_[pos].next;
    // array_[pos].data = T();
    if (prev == 0) {
      list_head_.next = next;
    } else {
      array_[prev].next = next;
    }

    if (next == 0) {
      list_head_.prev = prev;
    } else {
      array_[next].prev = prev;
    }
    ReturnToFreeList(pos);
  }
  ssize_t First() { return list_head_.next; }
  ssize_t Last() { return list_head_.prev; }
  ssize_t Next(ssize_t pos) const {
    if (IsFree(pos)) {
      return 0;
    }
    return array_[pos].next;
  }
  ssize_t Prev(ssize_t pos) const {
    if (IsFree(pos)) {
      return -1;
    }
    return array_[pos].prev;
  }
  T* Get(ssize_t pos)  {
    if (IsFree(pos)) {
      return NULL;
    }
    return &array_[pos].data;
  }
  void Print(std::ostream& os) {
    os << '[' << list_head_.prev << ',' << list_head_.next << ']';
    ssize_t iter = First();
    for (; iter!=0; iter=Next(iter)) {
      os << '[' << array_[iter].prev << ',' << array_[iter].next << ']';
    }
  }

 private:
  ssize_t AllocFromFreeList() {
    size_t pos = array_[0].next;
    if (pos == 0) {
      return 0;
    }
    array_[0].next = array_[pos].next;
    ++count_;
    return pos;
  }
  void ReturnToFreeList(ssize_t pos) {
    array_[pos].prev = -1;
    array_[pos].next = array_[0].next;
    array_[0].next = pos;
    --count_;
  }
  bool IsFree(ssize_t pos) const {
    assert(pos >= 0 && pos <= max_count_);
    return array_[pos].prev == -1;
  }
 private:
  ArrayList(const ArrayList&);
  ArrayList& operator=(const ArrayList&);
  static const ssize_t FREE_LIST_HEAD = 0;
  // 第0个存放空闲链表头,第1个存放链表头
  struct Item {
    T data;
    ssize_t prev; // =0表示在空闲链表中.
    ssize_t next;
  };
  Item* array_;
  Item list_head_;
  ssize_t count_;
  ssize_t max_count_;
};
} // namespace blitz.
#endif // __BLITZ_ARRAY_LIST_H_
