// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ_REF_OBJECT_H_
#define __BLITZ_REF_OBJECT_H_ 1
#include <assert.h>
#include <stdio.h>
namespace blitz2 {
class RefCountImpl {
 public:
  int32_t AddRef() { return ++ref_count_; }
  int32_t Release() {
   int32_t ref =  --ref_count_;
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
// 参考CComPtr.
template<typename T>
class SharedPtr {
 public:
  SharedPtr() { ptr_ = NULL; }
  explicit SharedPtr(T* obj) { 
    ptr_ = obj; 
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  SharedPtr(const SharedPtr& rhs) {
    ptr_ = rhs.ptr_;
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  SharedPtr& operator=(const SharedPtr& rhs) {
    if (this != &rhs) {
      Release();
      ptr_ = rhs.ptr_;
      if (ptr_) {
        ptr_->AddRef();
      }
    }
    return *this;
  }
  ~SharedPtr() { 
    if (ptr_) {
      ptr_->Release();
      ptr_ = NULL;
    }
  }
  SharedPtr& operator=(T* ptr) {
    Release();
    ptr_ = ptr;
    if (ptr_) {
      ptr_->AddRef();
    }
    return *this;
  }
  T* get_pointer() { return ptr_; }
  const T* get_pointer() const { return ptr_;}
  operator T*() { return ptr_; }
  operator const T*() const { return ptr_; }
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
  const T* operator->() const { return ptr_; }
  T** operator& () { assert(ptr_ == NULL); return &ptr_ ; }
 private:
  T* ptr_;
};

// 侵入式引用计数实现类.
// 使用方法: typedef RefCountObject<OutMsg> SharedOutMsg;
// typedef RefCountPtr<RefCountOutMsg> SharedOutMsgPtr;
// 要求，T必须有默认构造函数.
// 和Init函数
template<typename T>
class SharedObject : public T , public RefCountImpl {
 public:
  typedef SharedObject<T> TSharedObject;
  typedef SharedPtr<TSharedObject> TSharedObjectPtr;
  template<typename... InitArgs>
  static  TSharedObjectPtr  CreateInstance(InitArgs... args) {
    TSharedObject *object =  new TSharedObject();
    object->Init(args...);
    TSharedObjectPtr ptr;
    ptr.Attach(object);
    return ptr;
  }
  private:
   SharedObject() {}
   ~SharedObject() {}
};

} // namespace blitz.
#endif // __BLITZ_REF_OBJECT_H_ 
