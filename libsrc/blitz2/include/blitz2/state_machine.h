#ifndef __BLITZ2_STATEMACHINE_H_
#define __BLITZ2_STATEMACHINE_H_ 1
#include <unordered_map>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <cassert>


namespace blitz2 {
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

/**
 * State Event base class.
 */ 
class Event {
 public:
  Event(int type)  {
    type_ = type;
  }
  int type() const { return type_; }
  virtual ~Event() {}
 protected:
  int type_;
} ;
/**
 * the return value of FnStateAction and FnTransitionAction, [0, 255].
 */
typedef uint8_t action_result_t;
/**
 * the state id that will goto, also in [0, 255].
 */
typedef uint8_t state_id_t;
static const state_id_t  kInvalidStateId = 0;
class ActionArcs {
 public:
  struct OneArc {
    OneArc(action_result_t result, state_id_t state) {
      this->result = result;
      this->state = state;
    }
    bool operator<(const OneArc& rhs) const {
      return result<rhs.result;
    }
    action_result_t result;
    state_id_t state;
  };
  ActionArcs& operator()(action_result_t result, state_id_t state) {
    arcs_.push_back(OneArc(result, state));
    return *this;
  }
  // convert to indexed by result.
  int CheckAndConvertToVec(std::vector<state_id_t>* state_ids) const;
 private:
  std::vector<OneArc> arcs_;
  mutable std::string last_error_;
};




template<typename Context>
class StateMachine {
 public:
  struct ContextDeleter {
    void operator()(Context* context) {
      delete context;
    }
  };
  typedef StateMachine<Context> TMyself;
  class State {
   public:
    // @return result of action, then statemachine will call State::GetNexState
    // to get next state..
    typedef action_result_t (Context::*FnStateAction)(uint32_t context_id, StateMachine<Context>* machine);
    State(state_id_t _id, FnStateAction _action,
        const ActionArcs& arcs) {
      id_ = _id;
      action_ = _action;
      arcs.CheckAndConvertToVec(&action_arcs_);
    }
    state_id_t GetNexState(action_result_t result) const {
      return action_arcs_[result];
    }
    state_id_t id() const { return id_; }
    FnStateAction action() const { return action_;}
   protected:
    state_id_t id_; //< use uint8_t to limit the value between 0 and 255.
    FnStateAction action_;
    std::vector<state_id_t> action_arcs_;
  };
  class Transition {
   public:
    typedef action_result_t (Context::*FnTransitionAction)(Event* event,
        uint32_t context_id, StateMachine<Context>* machine);
  
    Transition(state_id_t _state, int _event_type, FnTransitionAction _action, 
        const ActionArcs& arcs)
    :state_(_state), event_type_(_event_type), action_(_action) {
      arcs.CheckAndConvertToVec(&action_arcs_);
    }
    state_id_t GetNexState(action_result_t action_result) const {
      return action_arcs_[action_result];
    }
    state_id_t state() const { return state_;}
    int event_type() const { return event_type_;}
    FnTransitionAction action() const { return action_;}
   private:
    state_id_t state_;
    int event_type_;
    FnTransitionAction action_;
    std::vector<state_id_t> action_arcs_;
  };
  
  // 一个上下文处理结束的回调函数.
  typedef std::function<void(Context*)> FnOnContextEnd; 
  StateMachine(const char* name) : name_(name) {
    start_state_ = kInvalidStateId;
    end_state_ = kInvalidStateId;
    next_context_id_ = 0;
  }
  virtual ~StateMachine() {
    DeleteAllContext();
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
           state_id_t start_state,
           state_id_t end_state);
  int SetStates(const State* states, size_t count,
               state_id_t start_state, state_id_t end_state);
  int SetTransitions(const Transition* transitions, size_t count);
  int StartOneContext(Context* context, const FnOnContextEnd& callback);
  // 处理事件.
  // @return  0 -  调用了对应Context的回调函数.
  //          1 - context_id对应的上下文不存在，回调函数没有被调用.
  //          2 - 对应的事件处理函数不存在.
  //          其他 - 出错，回调函数没有被调用.
  int ProcessEvent(uint32_t context_id, Event* event);
  void DeleteAllContext();
  const State* GeState(state_id_t id) const;
  size_t GetStateCount() const { return states_.size(); }
  const Transition* GeTransition(state_id_t state, int event) const;
  size_t GeTransitionCount() const { return transitions_.size(); }
  size_t GetContextCount() const { return contexts_.size(); }
 private:
  struct InternalContext {
    Context* context; // 保存上下文的数据.
    uint32_t id; // 状态机上下文ID, 用于唯一标识一个状态机上下文.
    state_id_t cur_state; // 该状态机上下文当前状态.
    state_id_t prev_state; // 状态机上下文前一个状态.
    FnOnContextEnd fn_on_context_end; // 一个状态机跑完后调用这个回调函数.
    void* user_param; // 回调函数参数.
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
  int InternalGotoState(InternalContext* context, state_id_t state);
  InternalContext* FindContext(uint32_t context_id);
 private:
  typedef std::unordered_map<state_id_t, State> StateMap;
  typedef std::unordered_map<state_id_t, Transition> TEventMap;
  typedef std::unordered_map<state_id_t, TEventMap> TransitionMap;
  typedef std::unordered_map<uint32_t, InternalContext* > TContextMap;

  StateMap states_;
  TransitionMap transitions_;
  state_id_t start_state_;
  state_id_t end_state_;
  uint32_t next_context_id_; // 下一个context id.
  TContextMap contexts_;
  std::string name_; // 状态机名字.
};

template<typename Context>
int StateMachine<Context>::SetStates(
           const State* states, size_t state_count, 
           state_id_t start_state, state_id_t end_state) {
  assert(start_state > 0);
  assert(end_state > 0);
  // 必须有至少一个状态.
  assert(state_count > 0 && states != NULL);
  
  if (!states_.empty()) {
    // 已经SetStates过了.
    return 1;
  }
  std::pair<typename StateMap::iterator, bool> insert_state_ret;
  int ret = 0;
  start_state_ = start_state;
  end_state_ = end_state;
   //插入状态.
  for (size_t i = 0; i < state_count; ++i) {
    insert_state_ret = states_.insert(std::make_pair(states[i].id(), states[i]));
    if (!insert_state_ret.second) {
        ret = -2;
        break;
    }
  }
  if (ret != 0) { 
    states_.clear();
    return ret; 
  }
  if (!GeState(end_state)) { return -3; } 
  return 0;
}
template<typename Context>
int StateMachine<Context>::SetTransitions( const Transition* transitions,
                                           size_t transition_count ) {
  using namespace std;
  int ret = 0;
  //插入迁移.
  for(size_t i = 0; i < transition_count; ++i) {
    if (!GeState(transitions[i].state())) {
      ret = -3;
      break;
    }
    typename TransitionMap::iterator iter_transition
                                   = transitions_.find(transitions[i].state());
    if (iter_transition == transitions_.end()) {
      pair<typename TransitionMap::iterator, bool> insert_transition_ret = 
          transitions_.insert(make_pair(transitions[i].state(), TEventMap()));
        iter_transition = insert_transition_ret.first;
    }
    std::pair<typename TEventMap::iterator, bool> insert_event_ret = 
      iter_transition->second.insert(
                        make_pair(transitions[i].event_type(), transitions[i]));
    if (!insert_event_ret.second) {
        ret = -3;
        break;
    }
  }
  if (ret != 0) {
    transitions_.clear();
  }
  return ret; 
}


template<typename Context>
int StateMachine<Context>::Init(
           const State* states, int state_count, 
           const Transition* transitions, int transition_count, 
           state_id_t start_state, state_id_t end_state) {
  int rc = SetStates(states, state_count, start_state, end_state);
  rc = rc != 0 ? rc : SetTransitions(transitions, transition_count);
  if (rc != 0) {
    transitions_.clear();
    states_.clear();
  }
  return rc;
}


template<typename Context>
int StateMachine<Context>::StartOneContext(Context* context,
                                const FnOnContextEnd& on_context_end) {
  //注意，无论如何都要调用回调函数.调用回调函数前一定要释放锁.
  InternalContext* mycontext = new InternalContext();
  if (!mycontext) {
    on_context_end(context);
    return -1;
  }
  mycontext->context = context;
  mycontext->id = ++next_context_id_;
  mycontext->cur_state = start_state_;
  mycontext->prev_state = kInvalidStateId;
  mycontext->fn_on_context_end = on_context_end;
  mycontext->machine = this;
  std::pair<typename TContextMap::iterator, bool> insert_ret = 
                     contexts_.insert(std::make_pair(mycontext->id, mycontext));
  if (!insert_ret.second) {
    on_context_end(mycontext->context); 
    delete context;
    return -2;
  }
  int ret = InternalGotoState(mycontext, start_state_);
  if (ret != 0) {
    // 一个上下文处理完了.
    mycontext->fn_on_context_end(mycontext->context);
    // 删除上下文.
    OnContextEnd(mycontext);
  } 
  return 0;
}

template<typename Context>
void StateMachine<Context>::OnContextEnd(InternalContext* context) {
  contexts_.erase(context->id);
  delete context;
}

template<typename Context>
const typename StateMachine<Context>::State* 
StateMachine<Context>::GeState(state_id_t state) const {
  typename StateMap::const_iterator iter = states_.find(state);
  if(iter == states_.end()) {
      return NULL;
  }
  return &iter->second;
}
template<typename Context>
const typename StateMachine<Context>::Transition* 
StateMachine<Context>::GeTransition(state_id_t state, int event) const {
  typename TransitionMap::const_iterator iter = transitions_.find(state);
  if(iter != transitions_.end()) {
      typename TEventMap::const_iterator iter_event = iter->second.find(event);
      if(iter_event != iter->second.end()) {
          return &(iter_event->second);
      }
  }
  return NULL;
}


template<typename Context>
typename StateMachine<Context>::InternalContext* 
StateMachine<Context>::FindContext(uint32_t context_id) {
  typename TContextMap::iterator iter = contexts_.find(context_id);
  if(iter == contexts_.end()) {
      return  NULL;
  }
  return iter->second;
}

template<typename Context>
int StateMachine<Context>::ProcessEvent(uint32_t context_id, Event* event) {
  InternalContext* context = FindContext(context_id);
  if (!context) { 
    return 1; 
  }
  const Transition* transition = GeTransition(context->cur_state, event->type());
  if(!transition) {
    return 2; 
  }
  // 会影响data内部数据.
  int result = ((*(context->context)).*(transition->action()))(event, context->id, this);
  int next_state = transition->GetNexState(result);
  if (next_state > 0) {
    int ret = InternalGotoState(context, next_state);
    if (ret != 0) {
      // 上下文结束.
      // context->fn_on_context_end 和 user_param这两个成员变量不会变化，不需要锁保护.
      context->fn_on_context_end(context->context);
      // 删除上下文.
      OnContextEnd(context);
    }
  } else if (next_state == 0) {
    // 保持状态不变.
  } else {
    // < 0 , 出错.
    // 上下文结束.
    // context->fn_on_context_end 和 user_param这两个成员变量不会变化，不需要锁保护.
    context->fn_on_context_end(context->context);
    // 删除上下文.
    OnContextEnd(context);
  }
  return 0;
}
template<typename Context>
void StateMachine<Context>::DeleteAllContext() {
  auto end = contexts_.end();
  for(auto it = contexts_.begin(); it != end; ++it) {
    InternalContext* context = it->second;
    context->fn_on_context_end(context->context);
    OnContextEnd(context);
  }
  contexts_.clear();
}


template<typename Context>
int StateMachine<Context>::InternalGotoState(InternalContext* context,
    state_id_t state_id) {
  const State* state = GeState(state_id);
  if (!state) { 
    return -1;
  }
  context->prev_state = context->cur_state;
  context->cur_state = state_id;
  typename State::FnStateAction action = state->action();
  if (state_id == end_state_) {
    //进入了结束状态,说明整个状态机处理完成, 不再关心next_state.
    if(action) {
       ((*context->context).*(action))(context->id, this);
    }
    return 1;
  }
  state_id_t next_state = kInvalidStateId; 
  action_result_t result = 0;
  if (action) {
    // 处理进入状态的Action.
     result = ((*(context->context)).*(action))(context->id, this);
     next_state = state->GetNexState(result);
  }
  if (next_state != kInvalidStateId) {
    return InternalGotoState(context, next_state);
  } else  { 
    // 状态不发生变化.
    return 0; 
  } 
}

} // namespace blitz2.
#endif // __BLITZ2_STATEMACHINE_H_ 
