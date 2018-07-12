// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include "gtest/gtest.h"
#include <blitz2/buffer.h>

using namespace blitz2;
/*
TEST(BufferWriter, Init) {
//  BufferWriter w;
//  EXPECT_EQ(w.get_data_length(), 0);
//  EXPECT_EQ(w.get_buffer_count(), 0);
//  EXPECT_TRUE(w.get_buffers() == NULL);
//  int ret = w.Init(1024);
//  EXPECT_EQ(ret, 0);
//  EXPECT_EQ(w.get_buffer_count(), 1);
//  EXPECT_TRUE(w.get_buffers() != NULL);
//  EXPECT_GT(w.get_buffer_capacity(), 0);
//  ret = w.Init(1024);
//  EXPECT_EQ(ret, BufferWriter::kAlreadyInited);
//  const DataBuffer* buffers = w.get_buffers();
//  EXPECT_TRUE(buffers != NULL);
//  for (int i = 0; i < w.get_buffer_count(); ++i) {
//    EXPECT_TRUE(buffers[i].ptr != NULL);
//    EXPECT_GT(buffers[i].length , 0);
//    EXPECT_EQ(buffers[i].data_length, 0);
//    EXPECT_EQ(buffers[i].read_pos, 0);
//  }
//  for (int i = w.get_buffer_count(); i < w.get_buffer_capacity(); ++i) {
//    EXPECT_TRUE(buffers[i].ptr == NULL);
//    EXPECT_EQ(buffers[i].length, 0);
//    EXPECT_EQ(buffers[i].data_length, 0);
//    EXPECT_EQ(buffers[i].read_pos, 0);
//  }
}

TEST(BufferWriter, WriteByte) {
 // BufferWriter w;
 // int ret = w.Init(1024);
 // EXPECT_EQ(ret, 0);
 // ret = w.WriteByte(34);
 // EXPECT_EQ(ret, 1);
 // EXPECT_EQ(w.get_data_length(), 1);
 // const DataBuffer* buffers = w.get_buffers();
 // EXPECT_TRUE(buffers != NULL);
 // EXPECT_EQ(buffers[0].data_length, 1);
 // EXPECT_EQ(buffers[0].ptr[0], 34);

 // for (int8_t i = 0; i < 127; ++i) {
 //   ret = w.WriteByte(i);
 //   EXPECT_EQ(ret, 1);
 //   EXPECT_EQ(w.get_data_length(), static_cast<int>(i)+2);
 //   EXPECT_EQ(buffers[0].data_length, static_cast<int>(i)+2);
 //   EXPECT_EQ(buffers[0].ptr[i+1], i);
 // }
}
TEST(BufferWriter, WriteByte2) {
  //BufferWriter w;
  //int ret = w.Init(1024);
  //EXPECT_EQ(ret, 0);
  //for (int i = 0; i < 4096; ++i) {
  //  ret = w.WriteByte(34);
  //  EXPECT_EQ(ret, 1);
  //  EXPECT_EQ(w.get_data_length(), i+1);
  //}
  //const DataBuffer* buffers = w.get_buffers();
  //EXPECT_TRUE(buffers != NULL);

  //for (int8_t i = 0; i < 127; ++i) {
  //  ret = w.WriteByte(i);
  //  EXPECT_EQ(ret, 1);
  //  EXPECT_EQ(w.get_data_length(), static_cast<int>(i)+4096+1);
  //  EXPECT_EQ(w.get_buffer_count(), 2);
  //  EXPECT_EQ(buffers[1].data_length, static_cast<int>(i)+1);
  //  EXPECT_EQ(buffers[1].ptr[i], i);
  //}
}

TEST(BufferWriter, WriteI16) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  ret = w.WriteI16(0x4134);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(w.get_data_length(), 2);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 2);
  EXPECT_EQ(buffers[0].ptr[0], 0x41);
  EXPECT_EQ(buffers[0].ptr[1], 0x34);

  ret = w.WriteI16(0x71FE);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(w.get_data_length(), 4);
  EXPECT_EQ(buffers[0].data_length, 4);
  EXPECT_EQ(buffers[0].ptr[2], 0x71);
  EXPECT_EQ(buffers[0].ptr[3], 0xFE);
}
TEST(BufferWriter, WriteI32) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  ret = w.WriteI32(0x4134ABCD);
  EXPECT_EQ(ret, 4);
  EXPECT_EQ(w.get_data_length(), 4);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 4);
  EXPECT_EQ(buffers[0].ptr[0], 0x41);
  EXPECT_EQ(buffers[0].ptr[1], 0x34);
  EXPECT_EQ(buffers[0].ptr[2], 0xAB);
  EXPECT_EQ(buffers[0].ptr[3], 0xCD);
}

TEST(BufferWriter, WriteI64) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  ret = w.WriteI64(0x123456789ABCDEF0L);
  EXPECT_EQ(ret, 8);
  EXPECT_EQ(w.get_data_length(), 8);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 8);
  EXPECT_EQ(buffers[0].ptr[0], 0x12);
  EXPECT_EQ(buffers[0].ptr[1], 0x34);
  EXPECT_EQ(buffers[0].ptr[2], 0x56);
  EXPECT_EQ(buffers[0].ptr[3], 0x78);
  EXPECT_EQ(buffers[0].ptr[4], 0x9A);
  EXPECT_EQ(buffers[0].ptr[5], 0xBC);
  EXPECT_EQ(buffers[0].ptr[6], 0xDE);
  EXPECT_EQ(buffers[0].ptr[7], 0xF0);
}
TEST(BufferWriter, WriteString) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  std::string str("1234567890");
  ret = w.WriteString(str);
  EXPECT_EQ(ret, 14);
  EXPECT_EQ(w.get_data_length(), 14);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 14);
  int32_t length = htonl(10);
  EXPECT_EQ(memcmp(buffers[0].ptr, reinterpret_cast<uint8_t*>(&length), 4), 0);
  EXPECT_EQ(memcmp(buffers[0].ptr+4, "1234567890", 10), 0);
}
TEST(BufferWriter, WriteBinary) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  std::string str("1234567890");
  ret = w.WriteBinary(reinterpret_cast<const uint8_t*>(str.data()), str.size());
  EXPECT_EQ(ret, 14);
  EXPECT_EQ(w.get_data_length(), 14);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 14);
  int32_t length = htonl(10);
  EXPECT_EQ(memcmp(buffers[0].ptr, reinterpret_cast<uint8_t*>(&length), 4), 0);
  EXPECT_EQ(memcmp(buffers[0].ptr+4, "1234567890", 10), 0);
}

TEST(BufferWriter, WriteData) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7'};
  ret = w.WriteData(data, sizeof(data));
  EXPECT_EQ(ret, 7);
  EXPECT_EQ(w.get_data_length(), 7);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 7);
  EXPECT_EQ(memcmp(buffers[0].ptr, data, 7), 0);
}

TEST(BufferWriter, PrintInt) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  ret = w.PrintInt(345, '\n');
  EXPECT_EQ(ret, 4);
  EXPECT_EQ(w.get_data_length(), 4);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, 4);
  EXPECT_EQ(memcmp(buffers[0].ptr, "345\n", 4), 0);
  ret = w.PrintInt(-45678, '\0');
  EXPECT_EQ(ret, 7);
  EXPECT_EQ(w.get_data_length(), 11);
  EXPECT_EQ(memcmp(buffers[0].ptr+4, "-45678\0", 7), 0);
}

TEST(BufferWriter, UseNakedPtr) {
  BufferWriter w;
  int ret = w.Init(1024);
  EXPECT_EQ(ret, 0);
  uint8_t* ptr = NULL;
  int length = 10;
  ret = w.UseNakedPtr(&ptr, length);
  memcpy(ptr, "9876543210", 10);
  EXPECT_EQ(ret, length);
  EXPECT_EQ(w.get_data_length(), length);
  const DataBuffer* buffers = w.get_buffers();
  EXPECT_TRUE(buffers != NULL);
  EXPECT_EQ(buffers[0].data_length, length);
  EXPECT_EQ(memcmp(buffers[0].ptr, "9876543210", length), 0);
}

TEST(BufferReader, constructor) {
  DataBuffer data;
  DataBufferUtil::Zero(&data);
  data.data_length = 10;
  BufferReader r(&data, 1);
  EXPECT_EQ(r.get_data_length(), 10);
  EXPECT_EQ(r.get_data_left_length(), 10);
  EXPECT_EQ(r.get_buffer_count(), 1);
  EXPECT_TRUE(r.get_buffers() == &data);
}
TEST(BufferReader, ReadByte) {
  DataBuffer data[2];
  DataBufferUtil::Zero(data);
  DataBufferUtil::Zero(data+1);
  uint8_t real_data[] = {1, 2, 3};
  data[0].data_length = 1;
  data[0].ptr = real_data;
  data[1].data_length = 2;
  data[1].ptr = real_data + 1;
  BufferReader r(data, 2);
  EXPECT_EQ(r.get_data_length(), 3);
  EXPECT_EQ(r.get_data_left_length(), 3);
  EXPECT_EQ(r.get_buffer_count(), 2);
  EXPECT_TRUE(r.get_buffers() == data);
  int8_t byte = 0;
  int ret = 0;
  ret = r.ReadByte(&byte);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(byte, 1);
  EXPECT_EQ(r.get_data_length(), 3);
  EXPECT_EQ(r.get_data_left_length(), 2);
  ret = r.ReadByte(&byte);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(byte, 2);
  EXPECT_EQ(r.get_data_length(), 3);
  EXPECT_EQ(r.get_data_left_length(), 1);
  ret = r.ReadByte(&byte);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(byte, 3);
  EXPECT_EQ(r.get_data_length(), 3);
  EXPECT_EQ(r.get_data_left_length(), 0);
  ret = r.ReadByte(&byte);
  EXPECT_EQ(ret, BufferReader::kNotEnoughData);
  EXPECT_EQ(r.get_data_length(), 3);
  EXPECT_EQ(r.get_data_left_length(), 0);
}
TEST(BufferReader, ReadI16) {
  DataBuffer data[2];
  DataBufferUtil::Zero(data);
  DataBufferUtil::Zero(data+1);
  uint8_t real_data[] = {0x0A, 0x0B, 0x1A, 0x1B, 0xC0};
  int data_length = sizeof(real_data);
  data[0].data_length = 3;
  data[0].ptr = real_data;
  data[1].data_length = 2;
  data[1].ptr = real_data + 3;
  BufferReader r(data, 2);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length);
  EXPECT_EQ(r.get_buffer_count(), 2);
  EXPECT_TRUE(r.get_buffers() == data);
  int16_t val = 0;
  int ret = 0;
  ret = r.ReadI16(&val);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(val, 0x0A0B);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-2);
  ret = r.ReadI16(&val);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(val, 0x1A1B);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-4);
  ret = r.ReadI16(&val);
  EXPECT_EQ(ret, BufferReader::kNotEnoughData);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-4);
}

TEST(BufferReader, ReadI32) {
  DataBuffer data[2];
  DataBufferUtil::Zero(data);
  DataBufferUtil::Zero(data+1);
  uint8_t real_data[] = {0x0A, 0x0B, 0x1A, 0x1B, 0x70, 0x71, 0x72, 0x73, 0x60};
  int data_length = sizeof(real_data);
  data[0].data_length = 5;
  data[0].ptr = real_data;
  data[1].data_length = data_length - data[0].data_length;
  data[1].ptr = real_data + data[0].data_length;
  BufferReader r(data, 2);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length);
  EXPECT_EQ(r.get_buffer_count(), 2);
  EXPECT_TRUE(r.get_buffers() == data);
  int32_t val = 0;
  int ret = 0;
  ret = r.ReadI32(&val);
  EXPECT_EQ(ret, 4);
  EXPECT_EQ(val, 0x0A0B1A1B);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-4);
  ret = r.ReadI32(&val);
  EXPECT_EQ(ret, 4);
  EXPECT_EQ(val, 0x70717273);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-8);
  ret = r.ReadI32(&val);
  EXPECT_EQ(ret, BufferReader::kNotEnoughData);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-8);
}
TEST(BufferReader, ReadI64) {
  DataBuffer data[2];
  DataBufferUtil::Zero(data);
  DataBufferUtil::Zero(data+1);
  uint8_t real_data[] = {0x0A, 0x0B, 0x1A, 0x1B, 0x70, 0x71, 0x72, 0x73, 0x60};
  int data_length = sizeof(real_data);
  data[0].data_length = 5;
  data[0].ptr = real_data;
  data[1].data_length = data_length - data[0].data_length;
  data[1].ptr = real_data + data[0].data_length;
  BufferReader r(data, 2);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length);
  EXPECT_EQ(r.get_buffer_count(), 2);
  EXPECT_TRUE(r.get_buffers() == data);
  int64_t val = 0;
  int ret = 0;
  ret = r.ReadI64(&val);
  EXPECT_EQ(ret, 8);
  EXPECT_EQ(val, 0x0A0B1A1B70717273LL);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-8);
  ret = r.ReadI64(&val);
  EXPECT_EQ(ret, BufferReader::kNotEnoughData);
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-8);
}

TEST(BufferReader, ReadString) {
  DataBuffer data[2];
  DataBufferUtil::Zero(data);
  DataBufferUtil::Zero(data+1);
  uint8_t real_data[] = {0x00, 0x00, 0x00, 0x0A, '1', '2', '3', '4', '5', '6',
                        '7', '8', '9', '0', 0x00, 0x00, 0x00, 0x05, '1'};
  int data_length = sizeof(real_data);
  data[0].data_length = 5;
  data[0].ptr = real_data;
  data[1].data_length = data_length - data[0].data_length;
  data[1].ptr = real_data + data[0].data_length;
  BufferReader r(data, 2);
  std::string val;
  int ret = 0;
  ret = r.ReadString(&val);
  EXPECT_EQ(ret, 14);
  EXPECT_EQ(val, "1234567890");
  EXPECT_EQ(r.get_data_length(), data_length);
  EXPECT_EQ(r.get_data_left_length(), data_length-14);
  ret = r.ReadString(&val);
  EXPECT_EQ(ret, BufferReader::kNotEnoughData);
  EXPECT_EQ(r.get_data_left_length(), data_length-14);
}
*/

