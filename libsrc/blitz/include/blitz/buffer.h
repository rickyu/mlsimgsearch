#ifndef __LIBSRC_BLITZ_BUFFER_H_
#define __LIBSRC_BLITZ_BUFFER_H_ 1
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string>

namespace blitz {

typedef void (*FnMemoryFree)(void* ptr, void* context);
struct DataBuffer {
  uint8_t* ptr;
  int length;
  int data_length;
  FnMemoryFree fn_free;
  void* free_context;
  int read_pos;

};
class DataBufferUtil {
 public:
  static void Zero(DataBuffer* buffer) {
    buffer->ptr = NULL;
    buffer->length = 0;
    buffer->data_length = 0;
    buffer->fn_free = NULL;
    buffer->free_context = NULL;
    buffer->read_pos = 0;
  }
  static void Free(DataBuffer* buffer) {
    if (buffer->fn_free && buffer->ptr) {
      (*buffer->fn_free)(buffer->ptr, buffer->free_context);
    }
    Zero(buffer);
  }
};
class Alloc {
  public:
    static void* Malloc(int length) {
      return ::malloc(length);
    }
    static void Free(void* ptr, void* ) {
      ::free(ptr);
    }
};
/**
 * BufferWriter w;
 * w.WriteXXX();
 * ... 
 * char* ptr;
 * int len = 1024*1024*1024;
 * w.UseNakedPtr(&ptr,len);
 * fread(fp,ptr,1024*1024*1024);
 * Buffer* data;
 * int count;
 * w.GetData(&data, &count);
 * 
 * BufferReader r(data, count);
 * int32_t i32;
 * r.ReadI32(&i32);
 * 有拷贝
 * char data[126];
 * r.ReadBinary(data,126);
 * 
 * char* str;
 * int len = 12;
 * 如果字符串被拆为两段怎么处理?
 * 1. 内部分配临时内存区域进行拷贝
 * ret = r.GetNaked(&str, &len);
 * r.ReportUsage(str,len);
 * 2. 外部循环读取
 * do {
 *  ret = r.GetNaked(&str,len);
 *  fwrite(fp, str, ret, 0);
 *  r.ReportUsage(str, len);
 * }
 * while (ret<len);
 *
 * 3. 提供回调函数.
 * r.ReadAll(len, callback);  
 *
 * 
 */
class BufferWriter {
 public:
  enum RetValue {
    kAlreadyInited = 1,
    kOutOfMemory = -100
  };
  BufferWriter() {
    ZeroMember();
  }
  ~BufferWriter() {
    Uninit();
  }
 public:
  int Init(int hint_length);
  int Uninit(); 
  int DetachBuffers();
  int WriteByte(int8_t b);
  int WriteI16(int16_t i16);
  int WriteI32(int32_t val); 
  int WriteI64(int64_t val);
  /**
   * 输出字符串到buffer中，格式length(4Byte) | data \0 |
   */
  int WriteString(const std::string& str); 
  /**
   * 写入binary数据，格式|length(int32_t)|data|
   */
  int WriteBinary(const uint8_t* data, int length);
  int WriteData(const void* data, int length);
  int PrintInt(int64_t v, char end);
  /**
   * 取裸指针直接write,比如snprintf,fread等.
   * 有个陷阱:使用完UseNakedPtr后，如果又调用其他写入函数，可能导致取出
   * 的指针无效，所以需要立即使用,调用其他函数后，不再使用该指针.
   * @param ptr: OUT 存放缓冲区指针.
   * @param length: IN 需要的长度.
   */
  int UseNakedPtr(uint8_t** ptr, int length);
  int get_data_length() const {
    return data_length_;
  }
  int get_buffer_capacity() const {
    return buffer_capacity_;
  }
  int get_buffer_count() const {
    return buffer_count_;
  }
  const DataBuffer* get_buffers() const {
    return buffers_;
  }

 private:
  BufferWriter(const BufferWriter&);
  BufferWriter& operator=(const BufferWriter&);
  int Extend(int len);
  int Alloc(DataBuffer* buffer, int length) {
    length = ((length-1)/4096+1)*4096;
    buffer->ptr = reinterpret_cast<uint8_t*>(Alloc::Malloc(length));
    if (buffer->ptr == NULL) {
      return -100;
    }
    buffer->fn_free = Alloc::Free;
    buffer->length = length;
    return 0;
  }
  void ZeroMember() {
    buffers_ = NULL;
    buffer_count_ = 0;
    buffer_capacity_ = 0;
    cur_buffer_ = NULL;
    data_length_ = 0;
  }
 private:
  DataBuffer* buffers_;
  int buffer_count_;
  int buffer_capacity_;
  DataBuffer* cur_buffer_;
  int data_length_;
};
class BufferReader {
 public:
  enum {
    kNotEnoughData = -5,
    kOutOfMemory = -100
  };
  BufferReader(DataBuffer* buffers, int count);
  int ReadByte(int8_t* b);
  int ReadI16(int16_t*  i16); 
  int ReadI32(int32_t* i32);
  int ReadI64(int64_t* i64);
  /**
   * read字符串.格式见BufferWriter::WriteString.
   * @return 成功返回正整数，表示消耗的数据量.<=0表示失败.
   */
  int ReadString(std::string* str);
  /**
   * read二进制.格式见BufferWriter::WriteBinary.
   * @return :int , >0 成功,表示消耗的数据量.<=0表示失败.
   */
  int ReadBinary(std::string* data);
  int ReadData(uint8_t* buffer, int length);
  int UseNakedPtr(DataBuffer* data, int length);
  /**
   * 读取字符串形式的i32,比如"123"=>123.
   * @return 返回读取的字节数, <=0 表示出错.
   */
  int ScanfDigit(int64_t* integer) ;
  // 对数据进行枚举读取.
  //
  // @param length: int, 要枚举的长度.
  // @param function: void function(const uint8_t* data, int length),会被多次调用.
  template <typename TFunc>
  int IterateRead(int length, TFunc function);
  const DataBuffer* get_buffers() const { return buffers_; }
  int get_buffer_count() const { return buffer_count_; }
  int get_data_length() const { return data_length_; }
  int get_data_left_length() const { return data_left_length_; }
 private:
  int IterateCopy(uint8_t* buffer, int length);
  int ReverseIterateCopy(uint8_t* buffer, int length);
  void AdvanceCurrentBuffer(int length);
  void Back(int length);
 private:
  DataBuffer* buffers_;
  int buffer_count_;
  DataBuffer* current_buffer_;
  int data_length_;
  int data_left_length_;

  class StringReader {
   public:
    StringReader() {
      str_ = NULL;
    }
    void set_str(std::string* str) {
      str_ = str;
    }
    void operator()(const uint8_t* data, int length) {
      str_->append(reinterpret_cast<const char*>(data), length);
    }
   private:
    std::string* str_;
  };

  class CopyReader {
   public:
    CopyReader() {
      buffer_ = NULL;
    }
    void set_buffer(uint8_t* buffer) {
      buffer_ = buffer;
    }
    void operator()(const uint8_t* data, int length) {
      memcpy(buffer_, data, length);
      buffer_ += length;
    }
   private:
    uint8_t* buffer_;
  };
  class ReverseCopyReader {
   public:
    ReverseCopyReader() {
      buffer_ = NULL;
    }
    void set_buffer(uint8_t* buffer, int length) {
      buffer_ = buffer + length;
    }
    void operator()(const uint8_t* data, int length) {
      for (int i = 0; i < length; ++i) {
        *--buffer_ = data[i];
      }
    }
   private:
    uint8_t* buffer_;
  };
};
// inline function of BufferWriter.
//
inline int BufferWriter::Uninit() {
  if (!buffers_) { return 0; }
  DataBuffer* buffer_cur = buffers_;
  DataBuffer* buffer_end = buffers_ + buffer_count_;
  while (buffer_cur != buffer_end) {
    (*buffer_cur->fn_free)(buffer_cur->ptr, buffer_cur->free_context);
    ++buffer_cur;
  }
  free(buffers_);
  ZeroMember();
  return 0;
}

inline int BufferWriter::DetachBuffers() {
  free(buffers_);
  ZeroMember();
  return 0;
}
inline int BufferWriter::WriteByte(int8_t b) {
  int ret = Extend(1);
  if( ret != 0) {
    return ret;
  }
  cur_buffer_->ptr[cur_buffer_->data_length] = b;
  ++cur_buffer_->data_length;
  ++data_length_;
  return 1;
}
inline int BufferWriter::WriteI16(int16_t i16) {
  int ret =  Extend(2);
  if (ret != 0) { return ret; }
  *reinterpret_cast<int16_t*>(cur_buffer_->ptr+cur_buffer_->data_length) = ntohs(i16);
  cur_buffer_->data_length+=2;
  data_length_+=2;
  return 2;
}
inline int BufferWriter::WriteI32(int32_t val) {
  int ret =  Extend(sizeof(val));
  if (ret != 0) { return ret; }
  *reinterpret_cast<int32_t*>(cur_buffer_->ptr+cur_buffer_->data_length) = ntohl(val);
  cur_buffer_->data_length += sizeof(val);
  data_length_ += sizeof(val);
  return sizeof(val);
}
inline int BufferWriter::WriteI64(int64_t val) {
  int ret =  Extend(sizeof(val));
  if (ret != 0) { return ret; }
  uint8_t* val_bytes = reinterpret_cast<uint8_t*>(&val);
  uint8_t* dest_ptr = cur_buffer_->ptr + cur_buffer_->data_length;
#if __BYTE_ORDER == __BIG_ENDIAN
  dest_ptr[0] = val_bytes[0];
  dest_ptr[1] = val_bytes[1];
  dest_ptr[2] = val_bytes[2];
  dest_ptr[3] = val_bytes[3];
  dest_ptr[4] = val_bytes[4];
  dest_ptr[5] = val_bytes[5];
  dest_ptr[6] = val_bytes[6];
  dest_ptr[7] = val_bytes[7];
#else
  dest_ptr[0] = val_bytes[7];
  dest_ptr[1] = val_bytes[6];
  dest_ptr[2] = val_bytes[5];
  dest_ptr[3] = val_bytes[4];
  dest_ptr[4] = val_bytes[3];
  dest_ptr[5] = val_bytes[2];
  dest_ptr[6] = val_bytes[1];
  dest_ptr[7] = val_bytes[0];
#endif
  cur_buffer_->data_length += sizeof(val);
  data_length_ += sizeof(val);
  return sizeof(val);
}
/**
 * 输出字符串到buffer中，格式length(4Byte) | data \0 |
 */
inline int BufferWriter::WriteString(const std::string& str) {
  int32_t length = str.size();
  int ret = Extend(length+4);
  if (ret != 0) { return ret; }
  WriteI32(length);
  WriteData(str.data(), length);
  return length+4;
}

inline int BufferWriter::PrintInt(int64_t v, char end) {
  char buf[32];
  char* buf_end = buf+32;
  buf[31] = end;
  int64_t val = abs(v);
  char* write_head = buf_end-2;
  do {
    *write_head-- = val%10 + '0';
    val /= 10;
  } while(val != 0);
  if (v < 0) {
    *write_head-- = '-';
  }
  return WriteData(write_head+1, buf_end-write_head-1);
}
/**
 * 写入binary数据，格式|length(int32_t)|data|
 */
inline int BufferWriter::WriteBinary(const uint8_t* data, int length) {
  int ret = Extend(length+4);
  if (ret != 0) {
    return ret;
  }
  WriteI32(length);
  memcpy(cur_buffer_->ptr+cur_buffer_->data_length, data, length);
  cur_buffer_->data_length += length;
  data_length_ += length;
  return length+4;
}
inline int BufferWriter::WriteData(const void* data, int length) {
  int ret = Extend(length);
  if (ret != 0) {
    return ret;
  }
  memcpy(cur_buffer_->ptr+cur_buffer_->data_length, data, length);
  cur_buffer_->data_length += length;
  data_length_ += length;
  return length;
}
inline int BufferWriter::UseNakedPtr(uint8_t** ptr, int length) {
  int ret = 0;
  if ( (ret=Extend(length) ) != 0) {
    return ret;
  }
  *ptr = cur_buffer_->ptr+cur_buffer_->data_length;
  cur_buffer_->data_length += length;
  data_length_ += length;
  return length;
}

// inline function of BufferReader.
inline BufferReader::BufferReader(DataBuffer* buffers, int count) {
  buffers_ = buffers;
  buffer_count_ = count;
  data_length_ = 0;
  for (int i = 0; i < count; ++i) {
    buffers_[i].read_pos = 0;
    data_length_ += buffers_[i].data_length;
  }
  data_left_length_ = data_length_;
  current_buffer_ = buffers_;
}
inline int BufferReader::ReadByte(int8_t* b) {
  if (current_buffer_->data_length-current_buffer_->read_pos >= 1) {
    *b = static_cast<int8_t>(
        *(current_buffer_->ptr+current_buffer_->read_pos));
    AdvanceCurrentBuffer(1);
    return 1;
  }
  return IterateCopy(reinterpret_cast<uint8_t*>(b), 1);
}
inline int BufferReader::ReadI16(int16_t*  i16) {
  if (current_buffer_->data_length - current_buffer_->read_pos >= 2) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(i16);
#if __BYTE_ORDER == __BIG_ENDIAN
    // NetworkOrder = BIG_ENDIAN
    // Big endian 0x1234 => 12 | 34
    bytes[0] = current_buffer_->ptr[current_buffer_->read_pos];
    bytes[1] = current_buffer_->ptr[current_buffer_->read_pos+1];
#else
    bytes[0] = current_buffer_->ptr[current_buffer_->read_pos+1];
    bytes[1] = current_buffer_->ptr[current_buffer_->read_pos];
#endif
    AdvanceCurrentBuffer(2);
    return 2;
  } else {
#if __BYTE_ORDER == __BIG_ENDIAN
    return IterateCopy(reinterpret_cast<uint8_t*>(i16), 2);
#else
    return ReverseIterateCopy(reinterpret_cast<uint8_t*>(i16), 2);
#endif
  }
}
inline int BufferReader::ReadI32(int32_t* i32) {
  static const int I32_LENGTH = 4;
  if (current_buffer_->data_length - current_buffer_->read_pos >= I32_LENGTH) {
#if __BYTE_ORDER == __BIG_ENDIAN
    // NetworkOrder = BIG_ENDIAN
    // Big endian 0x1234 => 12 | 34
    *i32 = reinterpret_cast<int32_t*>(current_buffer_->ptr
        +current_buffer_->read_pos);
#else
    uint8_t* bytes = reinterpret_cast<uint8_t*>(i32);
    bytes[0] = current_buffer_->ptr[current_buffer_->read_pos+3];
    bytes[1] = current_buffer_->ptr[current_buffer_->read_pos+2];
    bytes[2] = current_buffer_->ptr[current_buffer_->read_pos+1];
    bytes[3] = current_buffer_->ptr[current_buffer_->read_pos];
#endif
    AdvanceCurrentBuffer(I32_LENGTH);
    return I32_LENGTH;
  } else {
#if __BYTE_ORDER == __BIG_ENDIAN
    return IterateCopy(reinterpret_cast<uint8_t*>(i32), I32_LENGTH);
#else
    return ReverseIterateCopy(reinterpret_cast<uint8_t*>(i32), I32_LENGTH);
#endif
  }
}
inline int BufferReader::ReadI64(int64_t* i64) {
  static const int TYPE_LENGTH = 8;
  if (current_buffer_->data_length-current_buffer_->read_pos >= TYPE_LENGTH) {
#if __BYTE_ORDER == __BIG_ENDIAN
   // NetworkOrder = BIG_ENDIAN
   // Big endian 0x1234 => 12 | 34
   *i64 = *reinterpret_cast<int64_t*>(current_buffer_->ptr
       +current_buffer_->read_pos);
#else
   uint8_t* bytes = reinterpret_cast<uint8_t*>(i64);
   bytes[0] = current_buffer_->ptr[current_buffer_->read_pos+7];
   bytes[1] = current_buffer_->ptr[current_buffer_->read_pos+6];
   bytes[2] = current_buffer_->ptr[current_buffer_->read_pos+5];
   bytes[3] = current_buffer_->ptr[current_buffer_->read_pos+4];
   bytes[4] = current_buffer_->ptr[current_buffer_->read_pos+3];
   bytes[5] = current_buffer_->ptr[current_buffer_->read_pos+2];
   bytes[6] = current_buffer_->ptr[current_buffer_->read_pos+1];
   bytes[7] = current_buffer_->ptr[current_buffer_->read_pos];
#endif
   AdvanceCurrentBuffer(TYPE_LENGTH);
   return TYPE_LENGTH;
  } else {
#if __BYTE_ORDER == __BIG_ENDIAN
    return IterateCopy(reinterpret_cast<uint8_t*>(i64), TYPE_LENGTH);
#else
    return ReverseIterateCopy(reinterpret_cast<uint8_t*>(i64), TYPE_LENGTH);
#endif
  }
  return 8;
}
inline int BufferReader::ReadBinary(std::string* data) {
  return ReadString(data);
}
inline int BufferReader::ReadData(uint8_t* buffer, int length) {
  assert(length > 0);
  if (current_buffer_->data_length - current_buffer_->read_pos >= length) {
    memcpy(buffer, current_buffer_->ptr+current_buffer_->read_pos, length);
    AdvanceCurrentBuffer(length);
    return length;
  }
  return IterateCopy(buffer, length);
}
inline int BufferReader::IterateCopy(uint8_t* buffer, int length) {
  CopyReader copy_reader;
  copy_reader.set_buffer(buffer);
  return IterateRead(length, copy_reader);
}
inline int BufferReader::ReverseIterateCopy(uint8_t* buffer, int length) {
  ReverseCopyReader copy_reader;
  copy_reader.set_buffer(buffer, length);
  return IterateRead(length, copy_reader);
}

inline void BufferReader::AdvanceCurrentBuffer(int length) {
  current_buffer_->read_pos += length;
  data_left_length_ -= length;
}

template <typename TFunc>
int BufferReader::IterateRead(int length, TFunc function) {
  if (data_left_length_ < length) { return kNotEnoughData; }
  int left = length;
  while (left > 0) {
    if (current_buffer_->read_pos >= current_buffer_->data_length) {
      ++current_buffer_;
      if (current_buffer_ == buffers_ + buffer_count_) {
        break;
      }
      current_buffer_->read_pos = 0;
    } 
    int buffer_left = current_buffer_->data_length - current_buffer_->read_pos;
    int copy_length = left > buffer_left ? buffer_left : left;
    function(current_buffer_->ptr + current_buffer_->read_pos, copy_length);
    left -= copy_length;
    AdvanceCurrentBuffer(copy_length);
  }
  assert(left == 0);
  return length;
}
} // namespace blitz
#endif // __LIBSRC_BLITZ_BUFFER_H_
