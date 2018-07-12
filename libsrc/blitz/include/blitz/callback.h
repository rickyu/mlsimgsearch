// Copyright 2012 meilishuo Inc.
// Author: Yu Zuo
#ifndef __BLITZ__CALLBACK_H_
#define __BLITZ__CALLBACK_H_ 1

namespace blitz {
// 32字节的Callback用户上下文.
struct CallbackContext {
  char small_obj[32];
};

// 测试必须是pod type类型.
template <typename T>
class MustPodType {
  union {
      T type;
  };
};
template <bool b> 
class CompileError {
  struct __A {
    __A(){}
  };
  union {
    __A a;
  };
};
template <> 
class CompileError<false> {
};
// 获得两个type的size大小比较.
template <typename T1, typename T2>
struct SizeCompareValue {
  enum {value = sizeof(T1) > sizeof(T2)};
};
template <typename T1, typename T2>
struct SizeMustLess {
  enum {value = sizeof(T1) > sizeof(T2)};
  CompileError<value>  e;
};

// 上下文转化到自定义结构体的辅助函数.
// 使用方法 ：
//   struct MyContext {
//     int a;
//     int b;
//   };
//   RpcCallback callback;
//   MyContext* context = CallbackContextTo<MyContext>(callback.context_args);
//   context->a = 1;
//   context->b = 2;
//   ***************注意确保sizeof(MyContext) <= sizeof(CallbackContext);
template <typename T>
const T* CallbackContextTo(const CallbackContext& callback_context) {
  MustPodType<T>();
  SizeMustLess<T, CallbackContext>();
  return reinterpret_cast<const T*>(&callback_context.small_obj);
}

template <typename T>
T* CallbackContextTo(CallbackContext& callback_context) {
  MustPodType<T>(); // T必须是POD，否则编译错误.
  SizeMustLess<T, CallbackContext>(); // sizeof(T)必须小于sizeof(CallbackContext).
  return reinterpret_cast<T*>(&callback_context.small_obj);
}
}

#endif // __BLITZ__CALLBACK_H_ 
