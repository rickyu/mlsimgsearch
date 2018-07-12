#ifndef __BLITZ2_BUFFER_H_
#define __BLITZ2_BUFFER_H_ 1
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <functional>
#include <blitz2/ref_object.h>

namespace blitz2 {

/**
 * class for memory buffer.
 */
class Buffer {
 public:
  typedef std::function<void(uint8_t*)> TDestroyFunc;
  Buffer() {
    mem_ = NULL;
    capacity_ = 0;
  }
  virtual ~Buffer() { Clear();}
  bool Alloc(size_t capacity) ;
  void Attach(uint8_t* mem, size_t capacity, const TDestroyFunc& destroy);
  void Detach();
  virtual void Clear();
  uint8_t* mem() { return mem_;}
  const uint8_t* mem() const {return mem_;}
  size_t capacity() const { return capacity_;}
  const TDestroyFunc& destroy_functor() const { return destroy_func_;}
  static void DestroyDeleteImpl(uint8_t* mem) {
    delete [] mem;
  }
  static void DestroyFreeImpl(uint8_t* mem) {
    free(mem);
  }
  static void DestroyNothingImpl(uint8_t* ) {}

 protected:
  // for SharedObject.
  int Init() {
    return 0;
  }
 protected:
  uint8_t* mem_;
  size_t capacity_;
  TDestroyFunc destroy_func_;
  static TDestroyFunc s_default_destroy_func_;
};

typedef SharedObject<Buffer> SharedBuffer;
class DataBuffer : public Buffer {
 public:
  DataBuffer() {
    offset_ = length_ = 0;
  }
  virtual void Clear() {
    Buffer::Clear();
    offset_ = length_ = 0;
  }
  size_t offset() const {return offset_;}
  size_t& offset() {return offset_;}
  size_t length() const { return length_;}
  size_t& length()  { return length_;}
 protected:
  size_t offset_;
  size_t length_;
};

typedef SharedObject<DataBuffer> SharedDataBuffer;
inline  bool Buffer::Alloc(size_t capacity) {
  if(capacity_  >= capacity) {
    return true;
  }
  Clear();
  mem_ = new (std::nothrow) uint8_t[capacity];
  if(!mem_) {
    return false;
  }
  destroy_func_ = s_default_destroy_func_;
  capacity_ = capacity;
  return true;
}

inline void Buffer::Attach(uint8_t* mem, size_t capacity,
    const TDestroyFunc& destroy) {
  Clear();
  mem_ = mem;
  capacity_ = capacity;
  destroy_func_ = destroy;
}
inline void Buffer::Detach()  {
  mem_ = NULL;
  capacity_ = 0;
}
inline void Buffer::Clear() {
  if(mem_) {
    destroy_func_(mem_);
    mem_ = NULL;
    capacity_ = 0;
  }
}

} // namespace blitz2
#endif // __BLITZ2_BUFFER_H_
