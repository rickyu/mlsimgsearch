/**
 * @file actoken.h
 * @author yuzuo
 * @date 2007/11/06 14:09:18
 * @version 1.0
 * @brief 异步网络处理相关的基础类和定义.
 *  
 */
#ifndef __BLITZ_TOKEN_MGR_H_
#define __BLITZ_TOKEN_MGR_H_ 1

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
namespace blitz {
/**
 * 32bit的token.高16位放index, 低16位放salt.
 */
class Token32{
 public:
  Token32() {
    token_ = 0;
  }
  explicit Token32(uint32_t token) {
    token_ = token;
  }
  bool IsValid() const { 
    return token_ != 0;
  }
  uint32_t GetToken()  const {
    return  token_;
  }
  int SetToken(uint32_t token) {
    token_ = token;
    return 0;
  }
  uint32_t GetIndex() const {
    return token_ >> 16;
  }
  void SetIndex(uint32_t index) {
    token_ &= 0x0000FFFF;
    token_ |= (index << 16);
  }
  uint32_t GetSalt() const {
    return token_ & 0xFFFF;
  }
  void SetSalt(uint32_t salt){
    token_ &= 0xFFFF0000;
    token_ |= (salt & 0xFFFF);
  }
  void NewSalt(){
    uint32_t salt = GetSalt();
    SetSalt(++salt);
  }
  bool operator==(const Token32& rhs) {
    return token_ == rhs.token_;
  } 
  bool operator!=(const Token32& rhs) {
    return token_ != rhs.token_;
  }
  bool operator<(const Token32& rhs) {
    return token_ < rhs.token_;
  }

  static uint32_t GetMaxCount() {return 65535;}
 private:
  uint32_t token_;
};

template<typename TToken,typename TData>
class TokenMgr {
  public:
    TokenMgr() {
    items_ = NULL;
    count_ = 0;
    free_count_ = 0;
    }
    ~TokenMgr() {
      Uninit();
    }
    int Init(uint32_t count);
    void Uninit();
    int get_free_count() const { return static_cast<int>(free_count_); }
    int Alloc(TToken& token);
    int SetData(const TToken& token,const TData& data);
    //int GetData(const TToken& token,TData& data);
    TData* GetData(const TToken& token);
    //int Dealloc(const TToken& t,TData& data);
    int Dealloc(const TToken& t);
    bool isValid() const {
      return items_ != NULL;
    }

  private:
    TokenMgr(const TokenMgr&);
    TokenMgr& operator=(const TokenMgr&);

  /** 将index放入freelist.
   * 前提：index必须是>1并且<m_max.
   *
   */ 
  void inFreelist(uint32_t index) {
    items_[index].next = items_[0].next;
  
    items_[0].next = index;
    ++ free_count_;
  }
  /** 从freelist中取出一个空槽,返回空槽的下标.
   * 前提:m_free >=1 .
   */
  uint32_t outFreelist()
  {
    uint32_t ret =  items_[0].next;
    items_[0].next = items_[ret].next;
    items_[ret].next = 0;
    -- free_count_;
    return ret;
  }
  private: 

  struct TokenItem {
    TToken token;
    TData data;
    uint32_t next;
  };
    TokenItem* items_;/**< token数组,第1到m_max-1为有效的存储,第0个元素作为freelist的头.*/
    uint32_t count_;/**< token数组个数.*/
    uint32_t free_count_;/**< 空闲的槽数目.*/
};

template<typename TToken,typename TData>
int TokenMgr<TToken,TData>::Init(uint32_t count) {
  if(isValid())
  {
    return 1;
  }
  if(0 == count)
  {
    return -1;
  }
  if(count > TToken::GetMaxCount()){
    return -1;
  }
  
  //分配内存
  items_ = new TokenItem[count + 1];
  if(!items_)
  {
    return -1;
  }
  //链成freelist并且初始化
  for(uint32_t i = 0; i <= count; ++ i) {
    items_[i].token.SetIndex(i);
    items_[i].token.NewSalt();
    items_[i].next = i + 1;
  }
  count_ = count;
  free_count_ = count;
  return 0;
}

template<typename TToken,typename TData>
void TokenMgr<TToken,TData>::Uninit() {
  if(items_){
    delete [] items_;
    items_ = NULL;
  }
}
template<typename TToken,typename TData>
int TokenMgr<TToken,TData>::Alloc(TToken& token) {
  if(!isValid()) {
    return -1;
  }
  if(free_count_ > 0) {
    uint32_t index = outFreelist();
    assert(index <= count_);
    token = items_[index].token;
    return 0;
  }
  else {
    return -1;
  }
}
template<typename TToken,typename TData>
int TokenMgr<TToken,TData>::SetData(const TToken& token,const TData& data) {
  if(!isValid()) {
    return -1;
  }
  uint32_t index = token.GetIndex();
  //检查token下标的有效性
  if(index > 0 && index <= count_) {
    if(items_[index].token.GetSalt() == token.GetSalt()) {
      items_[index].data = data;
      return 0;
    }
    else {
      //无效的flag
      return 2;
    }
  }
  else {
    //无效的index.
    return 1;
  }
}


template<typename TToken,typename TData>
TData* TokenMgr<TToken,TData>::GetData(const TToken& token) {
  
  if(!isValid()) {
    return NULL;
  }
  uint32_t index = token.GetIndex();
  //检查token下标的有效性
  if(index > 0 && index <= count_) {
    if(items_[index].token.GetSalt() == token.GetSalt()) {
      return &items_[index].data;
    }
    else {
      //无效的flag
      return NULL;
    }
  }
  else {
    //无效的index.
    return NULL;
  }

}

template<typename TToken,typename TData>
int TokenMgr<TToken,TData>::Dealloc(const TToken& token) {
  if(!isValid()) { return -1; }
  uint32_t index = token.GetIndex();
  //检查token下标的有效性
  if(index > 0 && index <= count_) {
    if(items_[index].token.GetSalt() == token.GetSalt()) {
      items_[index].data.~TData();
      //data = items_[index].data;
      items_[index].token.NewSalt();//拿着同样的token就取不到了.
      //将该token元素放入freelist
      inFreelist(index);
    }
    else {
      //无效的flag
      return 2;
    }
  } else {
    //无效的index.
    return 1;
  }
  return 0;
}
} // namespace blitz.

#endif
