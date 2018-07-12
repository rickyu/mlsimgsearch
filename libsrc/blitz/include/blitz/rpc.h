#pragma once
#include "msg.h"
#include "callback.h"
namespace blitz {
typedef void (*FnRpcCallback)(int result,const InMsg* response, 
                              void* user_data);

struct RpcCallback {
  FnRpcCallback fn_action;
  void* user_data;
};

// 调用回调的辅助函数
inline void CallRpc(const RpcCallback& callback, int result,
                    const InMsg* response) {
  callback.fn_action(result, response, callback.user_data);
}

} // namespace blitz.

