#pragma once
namespace blitz {
struct BytesMsg {
  public:
    BytesMsg(uint8_t* _data,uint32_t _length){
      this->data = _data;
      this->length = _length;
    }
    uint8_t* data;
    uint32_t length;
};
typedef int (*GetBodyLenFunc)(uint8_t*);
class FixHeadLenMsgReader: public MsgReader {
  public:
    FixHeadLenMsgReader(int fd,uint32_t head_len,GetBodyLenFunc get_body_len_func)
      :head_len_(head_len){
      get_body_len_func_ = get_body_len_func;
      data_len_ = 0;
      buffer_ = NULL;
      buffer_len_ = 0;
      msg_len_ = 0;
      fd_ = fd;
    }
    int ReadMsg(void** msg){
      using namespace std;
      int result;
      uint32_t got = 0;
      
      if(buffer_ == NULL){
        buffer_ = new uint8_t[1024];
        buffer_len_ = 1024;
      }
      
      if(data_len_ < head_len_){
        result = RecvHelper(fd_,buffer_ + data_len_, head_len_ - data_len_,&got);
        data_len_ += got;
        if(result != 0){
          return result;
        }
        int body_len = get_body_len_func_(buffer_);
        if(body_len < 0){
          return -1;
        }
        msg_len_ = body_len + head_len_;
        if(msg_len_ > buffer_len_){
          uint8_t* new_buffer = new uint8_t[msg_len_];
          buffer_len_ = msg_len_;
          memcpy(new_buffer,buffer_,head_len_);
          delete [] buffer_;
          buffer_  = new_buffer;
        }
      }
      result = RecvHelper(fd_,buffer_ + data_len_, msg_len_ - data_len_ ,&got);
      data_len_ += got;
      if(result == 0){
        *msg = new BytesMsg(buffer_,data_len_);;
        buffer_ = NULL;
        data_len_ = 0;
      }
      return result;
    }
  private:
    int fd_;
    const uint32_t head_len_;
    GetBodyLenFunc get_body_len_func_;
    uint32_t data_len_;
    uint8_t *buffer_;
    uint32_t buffer_len_;
    uint32_t msg_len_;
  
};
#pragma pack(push)
#pragma pack(1)
struct MyMsgHead {
  uint32_t length;
  uint32_t magic;
};
#pragma pack(pop)
class MyMsgReaderFactory:public MsgReaderFactory{
  public:
    static int MyGetBodyLen(uint8_t* body){
      MyMsgHead* head = reinterpret_cast<MyMsgHead*>(body);
      if(head->magic != 0x00112233){
        return -1;
      }
      return (int)head->length;
      
    }
    MsgReader* CreateReader(int fd){
      return new FixHeadLenMsgReader(fd,8,MyGetBodyLen);
    }
    void RevokeReader(MsgReader* reader){
      delete reader;
    }
};
} // namespace blitz.
