// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_REF_OBJECT_H_
#define __BLITZ_REF_OBJECT_H_ 1
#include <assert.h>
#include <utils/atomic.h>
#include <stdio.h>
namespace blitz {
class RefCountImpl {
 public:
  int32_t AddRef() { return InterlockedIncrement(&ref_count_); }
  int32_t Release() {
   int32_t ref =  InterlockedDecrement(&ref_count_);
   if (ref == 0) { delete this; }
   return ref;
  }
  int32_t GetRefCount() const { return ref_count_; }
 protected:
  RefCountImpl() { ref_count_ = 1; }
  RefCountImpl(const RefCountImpl&);
  virtual ~RefCountImpl() {}
  int32_t ref_count_;
};


// 对带有引用计数的对象进行保护，方便使用.
// 类型T必须带有AddRef和Release两个成员函数.
template<typename T>
class RefCountPtr {
 public:
  RefCountPtr() { ptr_ = NULL; }
  explicit RefCountPtr(T* obj) { 
    ptr_ = obj; 
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  RefCountPtr(const RefCountPtr& rhs) {
    ptr_ = rhs.ptr_;
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  RefCountPtr& operator=(const RefCountPtr& rhs) {
    if (this != &rhs) {
      Release();
      ptr_ = rhs.ptr_;
      if (ptr_) {
        ptr_->AddRef();
      }
    }
    return *this;
  }
  ~RefCountPtr() { 
    if (ptr_) {
      ptr_->Release();
      ptr_ = NULL;
    }
  }
  RefCountPtr& operator=(T* ptr) {
    Release();
    ptr_ = ptr;
    if (ptr_) {
      ptr_->AddRef();
    }
    return *this;
  }
  T* get_pointer() { return ptr_; }
  // 不影响引用计数.
  T* Detach() { 
    T* v = ptr_;
    ptr_ = NULL;
    return v;
  }
  // 不影响引用计数。
  void Attach(T* object) {
    Release();
    ptr_ = object;
  }
  void Release() {
    if (ptr_) {
      ptr_->Release();
      ptr_ = NULL;
    }
  }
  operator bool () { return ptr_ != NULL; }
  T* operator-> () { return ptr_; }
  T** operator& () { assert(ptr_ == NULL); return &ptr_ ; }
 private:
  T* ptr_;
};

// 侵入式引用计数实现类.
// 使用方法: typedef RefCountImpl<OutMsg> RefCountOutMsg;
// typedef RefCountPtr<RefCountOutMsg> RefCountOutMsgPtr;
// 要求，T必须有默认构造函数.
// 怎么创建呢?
template<typename T>
class RefCountObject : public T , public RefCountImpl {
 public:
  typedef RefCountObject<T> TMyself;
  typedef RefCountPtr<TMyself> TPointer;
  static void CreateInstance(TMyself** out) {
    assert(out);
    *out = new TMyself();
  }
  static  TPointer  CreateInstance2() {
    TMyself *object =  new TMyself();
    TPointer ptr;
    ptr.Attach(object);
    return ptr;
  }
  protected:
   RefCountObject() { }
   ~RefCountObject() {}
};

} // namespace blitz.
#endif // __BLITZ_REF_OBJECT_H_ 
