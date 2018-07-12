// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ2_BUFFER_WRITER_H_
#define __BLITZ2_BUFFER_WRITER_H_ 1
#include <blitz2/stdafx.h>
namespace blitz2 {
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
  }
  ~BufferWriter() {
    Free();
  }
 public:
  int Extend( int len );
  int Reserve( int len );
  int Free(); 
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
  void* ReserveBuffer( int length );
  void AddDataLen( int length ) {
    buffer_.length() += length;
  }

  int length() const {
    return buffer_.length();
  }
  int capacity() const {
    return buffer_.capacity();
  }
  const DataBuffer& data() const { return buffer_; }
  DataBuffer& data() { return buffer_; }
  void DetachData() {
    buffer_.Detach();
  }


 private:
  BufferWriter(const BufferWriter&);
  BufferWriter& operator=(const BufferWriter&);
 private:
  DataBuffer buffer_;
};
// inline function of BufferWriter.
//
inline int BufferWriter::Free() {
  buffer_.Clear();
  return 0;
}

inline int BufferWriter::WriteByte(int8_t b) {
  int ret = Extend(1);
  if( ret != 0) {
    return ret;
  }
  buffer_.mem()[ buffer_.length() ] = b;
  ++buffer_.length();
  return 1;
}
inline int BufferWriter::WriteI16(int16_t i16) {
  int ret =  Extend(2);
  if (ret != 0) { return ret; }
  *reinterpret_cast<int16_t*>(buffer_.mem()+buffer_.length()) = ntohs(i16);
  buffer_.length() += 2;
  return 2;
}
inline int BufferWriter::WriteI32(int32_t val) {
  int ret =  Extend(sizeof(val));
  if (ret != 0) { return ret; }
  *reinterpret_cast<int32_t*>(buffer_.mem()+buffer_.length()) = ntohl(val);
  buffer_.length() += sizeof( val );
  return sizeof(val);
}
inline int BufferWriter::WriteI64( int64_t val ) {
  int ret =  Extend( sizeof(val) );
  if (ret != 0) { return ret; }
  uint8_t* val_bytes = reinterpret_cast<uint8_t*>(&val);
  uint8_t* dest_ptr = buffer_.mem() + buffer_.length();
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
  buffer_.length() += sizeof(val);
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
  memcpy(buffer_.mem()+buffer_.length(), data, length);
  buffer_.length() += length;
  return length+4;
}
inline int BufferWriter::WriteData(const void* data, int length) {
  int ret = Extend(length);
  if (ret != 0) {
    return ret;
  }
  memcpy(buffer_.mem()+buffer_.length(), data, length);
  buffer_.length() += length;
  return length;
}
inline void* BufferWriter::ReserveBuffer(int length) {
  if ( ( Extend(length) ) != 0) {
    return NULL;
  }
  return buffer_.mem() + buffer_.length();
}


} // namespace blitz2.
#endif // __BLITZ2_BUFFER_WRITER_H_

