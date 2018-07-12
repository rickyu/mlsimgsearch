// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <string>
#include "gtest/gtest.h"
#include <blitz2/state_machine.h>


using namespace blitz2;
template <typename T>
struct StateMachineCallbackParam {
  T* machine;
  int32_t context_id;
};
class TestStateMachine {
 public:
  class TData;
  typedef StateMachine<TData> MyStateMachine;
  class TData {
   public:
    action_result_t State0Action(uint32_t context_id, MyStateMachine* ) {
      a = 0;
      b = a + context_id;
      return 1;
    }
  action_result_t State1Action(uint32_t context_id, MyStateMachine* ) {
    a += 5;
    b = a + context_id;
    return 2;
  }
  int State0OnEvent1(Event* event, int /* context_id */,
                     MyStateMachine* ) {
    if (event->type() != 1) { return -1; }
  }

    int a;
    int b;
  };

  TestStateMachine():state_machine_("test") {
  }
  int Init() {
    MyStateMachine::State states[] = {
      MyStateMachine::State(1, &TData::State0Action, ActionArcs()(1,1)),
      MyStateMachine::State(2, &TData::State1Action, ActionArcs()(2,2)),
      MyStateMachine::State(3, NULL, ActionArcs())
    };
    return state_machine_.Init(states, 3, NULL, 0, 1, 3);
  }
  MyStateMachine& GetStateMachine() { return state_machine_; }
  MyStateMachine state_machine_;
};

TEST(StateMachine, Init) {
  TestStateMachine state_machine;
  int ret = state_machine.Init();
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(state_machine.GetStateMachine().GetStateCount(), 3u);
}

TEST(StateMachine, StartOneContext) {
  TestStateMachine state_machine;
  int ret = state_machine.Init();
  EXPECT_EQ(ret, 0);
  // state_machine.GetStateMachine().StartOneContext(input, NULL, NULL);
}


