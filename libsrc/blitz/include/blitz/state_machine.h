#ifndef __BLITZ_STATEMACHINE_H_
#define __BLITZ_STATEMACHINE_H_ 1
#include <unordered_map>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <cassert>
#include "utils/atomic.h"
#include "callback.h"
#include "utils/logger.h"
#include "utils/time_util.h"


namespace blitz {
// 思路: 每个StateMachine是一个执行的流程,每次具体执行的数据保存在TData中.
// 系统中实现有不同类型的状态机，一个状态机的执行结果(输出)可以作为另一个状态机的输入.
// 一个状态机的执行可以包含几个子状态机.
// 一般流程：
// StateMachine machine1;
// StateMachine machine2;
// machine1.Start(input, callback, context); 
//
// int callback(int res, void* output, void* context) {
//   machine2.St
// }
//

struct Event {
  int type;
  void* data;
};


template<typename TInput, typename TData, typename TOutput>
class StateMachine {
 public:
  typedef StateMachine<TInput, TData, TOutput> MyType;
  // @return > 0 表示迁移的状态. <0表示出错. =0表示待在原状态不变.
  typedef int (TData::*FnStateAction)(int context_id, StateMachine* machine);
  struct State {
    int id;
    FnStateAction action;
  };
  // @return > 0 表示迁移的状态. <0表示出错. =0表示呆在原状态不变.
  typedef int (TData::*FnTransitionAction)( Event* event, int context_id,
                                            StateMachine* machine);
  struct Transition {
    int state;
    int event_type;
    FnTransitionAction action;
  };

  // 一个状态处理结束的回调函数.
  typedef int (*FnOnContextEnd)(int res, const TOutput*  output,
                                void* user_params);
  StateMachine(const char* name) : name_(name) {
    start_state_ = -1;
    end_state_ = -1;
    next_context_id_ = 0;
  }
  // 初始化状态机.
  // 状态ID必须大于0.
  // @param states: [IN] 状态数组.注意，状态id必须>=0。
  // @param state_count: [IN] 状态数组个数.
  // @param transitions [IN] 事件迁移数组.
  // @param transition_count [IN] 事件迁移数组元素个数.
  // @param start_state [IN] 起始状态id.
  // @param end_state [IN] 结束状态id.
  // @return  - 0 成果
  //          - 其他表示失败.
  int Init(const State* states,
           int state_count, 
           const Transition* transitions,
           int transition_count, 
           int start_state,
           int end_state);
  int StartOneContext(const TInput& input,
             FnOnContextEnd on_context_end, 
             void* params);
  // 处理事件.
  // @return  0 -  调用了对应Context的回调函数.
  //          1 - context_id对应的上下文不存在，回调函数没有被调用.
  //          2 - 对应的事件处理函数不存在.
  //          其他 - 出错，回调函数没有被调用.
  int ProcessEvent(int context_id, Event* event);
  const State* GetState(int id) const;
  int GetStateCount() const { return states_.size(); }
  const Transition* GetTransition(int state, int event) const;
  int GetTransitionCount() const { return transitions_.size(); }
  int GetContextCount() const { return contexts_.size(); }
 private:
  struct InternalContext {
    TData data; // 保存上下文的数据.
    int id; // 状态机上下文ID, 用于唯一标识一个状态机上下文.
    int cur_state; // 该状态机上下文当前状态.
    int prev_state; // 状态机上下文前一个状态.
    FnOnContextEnd fn_on_context_end; // 一个状态机跑完后调用这个回调函数.
    void* user_param; // 回调函数参数.
    boost::recursive_mutex mutex; // 保证状态机上下文在单线程环境中执行. 
    StateMachine* machine; // 执行该上下文的状态机.
  };
 private:
  StateMachine(const StateMachine&);
  StateMachine& operator= (const StateMachine&);
  /**
   * 当一个状态机结束时调用.
   */
  void OnContextEnd(InternalContext* context);
  // 状态切换.
  // @return - 0 , 成功，上下文正常运行.
  //         - 1 ,  成果，上下文处理完成.
  //         - <0, 出错.
  int InternalGotoState(InternalContext* context, int state);
  boost::shared_ptr<InternalContext> FindContext(int32_t id);
 private:
  typedef std::unordered_map<int, State> TStateMap;
  typedef std::unordered_map<int, Transition> TEventMap;
  typedef std::unordered_map<int, TEventMap> TTransitionMap;
  typedef std::unordered_map<int32_t, boost::shared_ptr<InternalContext> > TContextMap;

  TStateMap states_;
  TTransitionMap transitions_;
  int start_state_;
  int end_state_;
  int32_t next_context_id_; // 下一个context id.
  TContextMap contexts_;
  boost::recursive_mutex contexts_lock_;
  std::string name_; // 状态机名字.
};


template<typename TInput, typename TData, typename TOutput>
int StateMachine<TInput, TData, TOutput>::Init(
           const State* states, int state_count, 
           const Transition* transitions, int transition_count, 
           int start_state, int end_state) {
  assert(start_state > 0);
  assert(end_state > 0);
  // 必须有至少一个状态.
  assert(state_count > 0 && states != NULL);
  
  if (!states_.empty()) {
    // 已经Init过了.
    return 1;
  }
  
  std::pair<typename TStateMap::iterator, bool> insert_state_ret;
  int ret = 0;
  start_state_ = start_state;
  end_state_ = end_state;
   //插入状态.
  for (int i = 0; i < state_count; ++i) {
    if (states[i].id <= 0) {
      // 状态id必须大于0.
      return -1;
    }
    insert_state_ret = states_.insert(std::make_pair(states[i].id, states[i]));
    if (!insert_state_ret.second) {
        ret = -2;
        break;
    }
  }
  if (ret != 0) { 
    states_.clear();
    return ret; 
  }
  if (!GetState(end_state)) { return -3; } 
  //插入迁移.
  for(int i = 0; i < transition_count; ++i) {
    if (!GetState(transitions[i].state)) {
      ret = -3;
      break;
    }
    typename TTransitionMap::iterator iter_transition
                                     = transitions_.find(transitions[i].state);
    if (iter_transition == transitions_.end()) {
      std::pair<typename TTransitionMap::iterator, bool> insert_transition_ret = 
          transitions_.insert(std::make_pair(transitions[i].state, TEventMap()));
        iter_transition = insert_transition_ret.first;
    }
    std::pair<typename TEventMap::iterator, bool> insert_event_ret = 
      iter_transition->second.insert(
                        std::make_pair(transitions[i].event_type, transitions[i]));
    if (!insert_event_ret.second) {
        ret = -3;
        break;
    }
  }
  if (ret != 0) {
    transitions_.clear();
    states_.clear();
  }
  return ret; 
}


template<typename TInput, typename TData, typename TOutput>
int StateMachine<TInput, TData, TOutput>::StartOneContext(const TInput& input,
                                FnOnContextEnd on_context_end, 
                                void* user_param) {
  //注意，无论如何都要调用回调函数.调用回调函数前一定要释放锁.
  boost::shared_ptr<InternalContext> context(new InternalContext());
  if (!context) {
    if (on_context_end) { on_context_end(-1, NULL, user_param); }
    return -1;
  }
  context->data.SetInput(input);
  context->id = InterlockedIncrement(&next_context_id_);
  context->cur_state = start_state_;
  context->prev_state = 0;
  context->fn_on_context_end = on_context_end;
  context->user_param = user_param;
  context->machine = this;
  {
    contexts_lock_.lock();
    std::pair<typename TContextMap::iterator, bool> insert_ret = 
                       contexts_.insert(std::make_pair(context->id, context));
    contexts_lock_.unlock();
    if (!insert_ret.second) {
      if (on_context_end) { on_context_end(-2, NULL, user_param); }
      return -2;
    }
  }
  {
    //加锁，让每个上下文处于单线程环境
    context->mutex.lock();
    int ret = InternalGotoState(context.get(), start_state_);
    if (ret != 0) {
      // 一个上下文处理完了.
      TOutput output;
      context->data.GetOutput(&output);
      context->mutex.unlock();
      // context->fn_on_context_end 和 user_param这两个成员变量不会变化，不需要锁保护.
      context->fn_on_context_end(ret == 1 ? 0 : ret, &output, context->user_param);
      // 删除上下文.
      OnContextEnd(context.get());
    } else {
      context->mutex.unlock();
    }
  }
  return 0;
}

template<typename TInput, typename TData, typename TOutput>
void StateMachine<TInput, TData, TOutput>::OnContextEnd(InternalContext* context) {
  boost::recursive_mutex::scoped_lock lock(contexts_lock_);
  contexts_.erase(context->id);
}

template<typename TInput, typename TData, typename TOutput>
const typename StateMachine<TInput, TData, TOutput>::State* 
StateMachine<TInput, TData, TOutput>::GetState(int state) const {
  typename TStateMap::const_iterator iter = states_.find(state);
  if(iter == states_.end()) {
      return NULL;
  }
  return &iter->second;
}
template<typename TInput, typename TData, typename TOutput>
const typename StateMachine<TInput, TData, TOutput>::Transition* 
StateMachine<TInput, TData, TOutput>::GetTransition(
                                    int state, int event) const {
  typename TTransitionMap::const_iterator iter = transitions_.find(state);
  if(iter != transitions_.end()) {
      typename TEventMap::const_iterator iter_event = iter->second.find(event);
      if(iter_event != iter->second.end()) {
          return &(iter_event->second);
      }
  }
  return NULL;
}


template<typename TInput, typename TData, typename TOutput>
boost::shared_ptr<typename StateMachine<TInput, TData, TOutput>::InternalContext> 
StateMachine<TInput, TData, TOutput>::FindContext(int32_t id) {
  boost::recursive_mutex::scoped_lock lock( contexts_lock_);
  typename TContextMap::iterator iter = contexts_.find(id);
  if(iter == contexts_.end()) {
      return boost::shared_ptr<InternalContext>();
  }
  return iter->second;
}

template<typename TInput, typename TData, typename TOutput>
int StateMachine<TInput, TData, TOutput>::ProcessEvent(int context_id, 
                                                       Event* event) {
  static CLogger& g_log_framework = CLogger::GetInstance("framework");
  uint64_t time1,time2,time3,time4,time5;
  time1 = TimeUtil::GetTickMs();
  boost::shared_ptr<InternalContext> context = FindContext(context_id);
  if (!context.get()) { 
    return 1; 
  }
  context->mutex.lock();
  const Transition* transition = GetTransition(context->cur_state, event->type);
  if(!transition) {
    context->mutex.unlock();
    return 2; 
  }
  // 会影响data内部数据.
  int next_state = (context->data.*(transition->action))(event, context->id, this);
  time2 = TimeUtil::GetTickMs();
  if (next_state > 0) {
    int ret = InternalGotoState(context.get(), next_state);
    if (ret != 0) {
      // 上下文结束.
      TOutput output;
      context->data.GetOutput(&output);
      context->mutex.unlock();
      time3 = TimeUtil::GetTickMs();
      // context->fn_on_context_end 和 user_param这两个成员变量不会变化，不需要锁保护.
      context->fn_on_context_end(ret == 1 ? 0 : ret, &output, context->user_param);
      // 删除上下文.
      OnContextEnd(context.get());
      time4 = TimeUtil::GetTickMs();
    } else {
      context->mutex.unlock();
    }
  } else if (next_state == 0) {
    // 保持状态不变.
    context->mutex.unlock();
  } else {
    // < 0 , 出错.
    // 上下文结束.
    TOutput output;
    context->data.GetOutput(&output);
      time5 = TimeUtil::GetTickMs();
    context->mutex.unlock();
    // context->fn_on_context_end 和 user_param这两个成员变量不会变化，不需要锁保护.
    context->fn_on_context_end(next_state, &output, context->user_param);
    // 删除上下文.
    OnContextEnd(context.get());
  }
  LOGGER_DEBUG(g_log_framework, "ProcessEvent End, cost_time1 = %d, cost_time2 = %d, cost_time3 = %d, cost_time4 = %d, cost_time5 = %d",
        time2-time1, time3-time2, time4-time3, TimeUtil::GetTickMs() - time4, TimeUtil::GetTickMs() - time5);
  return 0;
}


template<typename TInput, typename TData, typename TOutput>
int StateMachine<TInput, TData, TOutput>::InternalGotoState(InternalContext* context,
                                                            int state_id) {
  assert(context);
  assert(state_id >= 0);
  const State* state = GetState(state_id);
  assert(state);
  if (!state) { 
    return -1;
  }
  context->prev_state = context->cur_state;
  context->cur_state = state_id;
  int next_state = 0; 
  if (state->action) {
    // 处理进入状态的Action.
     next_state = (context->data.*(state->action))(context->id, this);
  }
  if (state_id == end_state_) {
    //进入了结束状态,说明整个状态机处理完成, 不再关心next_state.
    return 1;
  }
  if (next_state > 0) {
    return InternalGotoState(context, next_state);
  } else if (next_state == 0) { 
    // 状态不发生变化.
    return 0; 
  } else {
    //  出错了.
    return next_state;
  }
}

} // namespace blitz.
#endif // __BLITZ_STATEMACHINE_H_ 
