#include "blitz/buffer.h"

namespace blitz {
int BufferWriter::Init(int hint_length) {
  if (cur_buffer_) {
    return kAlreadyInited;
  }
  buffer_capacity_ = 2;
  buffers_ = reinterpret_cast<DataBuffer*>(malloc(sizeof(DataBuffer)*buffer_capacity_));
  if (!buffers_) {
    buffer_capacity_ = 0;
    return kOutOfMemory;
  }
  memset(buffers_, 0 , sizeof(DataBuffer)*buffer_capacity_);
  int ret = Alloc(buffers_, hint_length);
  if (ret != 0) { return ret; }
  cur_buffer_ = buffers_;
  buffer_count_ = 1;
  return 0;
}
int BufferWriter::Extend(int len) {
  assert(cur_buffer_ != NULL);
  if (cur_buffer_->length - cur_buffer_->data_length >= len) {
    return 0;
  }
  if (buffer_count_ == buffer_capacity_) {
    int new_capacity = buffer_capacity_ + 2;
    DataBuffer* new_buffers = reinterpret_cast<DataBuffer*>(
        malloc(sizeof(DataBuffer)*new_capacity));
    if (!new_buffers) {
      return kOutOfMemory;
    }
    memcpy(new_buffers, buffers_, sizeof(DataBuffer)*buffer_capacity_);
    memset(new_buffers + buffer_capacity_, 0,
           sizeof(DataBuffer)*(new_capacity-buffer_capacity_));
    buffer_capacity_ = new_capacity;
    free(buffers_);
    buffers_ = new_buffers;
  }
  if (cur_buffer_->data_length < 128) {
    // 重新分配内存并拷贝.
  }
  int ret = Alloc(buffers_+buffer_count_, len);
  if (ret != 0) {
    return ret;
  }
  cur_buffer_ = buffers_ + buffer_count_;
  ++buffer_count_;
  return 0;
}

// implementation of BufferReader.
int BufferReader::ScanfDigit(int64_t* integer) {
 *integer = 0;
 int length = 0;
 while (1) {
   if (current_buffer_->read_pos >= current_buffer_->data_length) {
      ++current_buffer_;
      if (current_buffer_ == buffers_ + buffer_count_) {
        break;
      }
      current_buffer_->read_pos = 0;
    } 
    uint8_t* ptr = current_buffer_->ptr+current_buffer_->read_pos;
    uint8_t* ptr_end = current_buffer_->ptr+current_buffer_->data_length;
    while (ptr < ptr_end) {
      if (*ptr >= '0' && *ptr <= '9') {
        *integer = (*integer)*10+(*ptr)-'0';
        ++ptr;
        ++length;
      } else {
        break;
      }
    }
    current_buffer_->read_pos += ptr_end - ptr;
    if (ptr < ptr_end) {
      break;
    }
  }
 return length;
}

int BufferReader::ReadString(std::string* str) {
  int32_t length = 0;
  int ret = ReadI32(&length);
  if (ret <= 0) { return ret; }
  if (data_left_length_ < length) {
    Back(ret);
    return kNotEnoughData;
  }
  str->reserve(length);
  StringReader string_reader;
  string_reader.set_str(str);
  return IterateRead(static_cast<int>(length), string_reader) + 4;
}


int BufferReader::UseNakedPtr(DataBuffer* data, int length) {
  DataBufferUtil::Zero(data);
  if (current_buffer_->data_length-current_buffer_->read_pos >= length) {
    data->ptr = current_buffer_->ptr+current_buffer_->read_pos;
    data->length = data->data_length = length;
    data->fn_free = NULL;
    current_buffer_->read_pos += length;
    return length;
  } else {
    data->ptr = reinterpret_cast<uint8_t*>(Alloc::Malloc(length));
    if (!data->ptr) {
      return -100;
    }
    int read_length =  IterateCopy(data->ptr, length);
    data->fn_free = Alloc::Free;
    data->free_context = NULL;
    data->length = data->data_length = read_length;
    return read_length;
  }
}
void BufferReader::Back(int length) {
  while (length > 0) {
    if (current_buffer_->read_pos >= length) {
      current_buffer_->read_pos -= length;
      data_left_length_ += length;
      length = 0;
      break;
    } else {
      length -= current_buffer_->read_pos;
      data_left_length_ += current_buffer_->read_pos;
      current_buffer_->read_pos = 0;
      --current_buffer_;
    }
  }
}

} // namespace blitz
