// Copyright 2012 meilishuo Inc.
// Author: 余佐
#include <string>
#include "gtest/gtest.h"
#include "blitz/state_machine.h"


template <typename T>
struct StateMachineCallbackParam {
  T* machine;
  int32_t context_id;
};
class TestStateMachine {
 public:
  class TInput {
   public:
    int a;
  };
  class TOutput {
   public:
    int b;
  };

  class TData;
  typedef blitz::StateMachine<TInput, TData, TOutput> MyStateMachine;
  class TData {
   public:
    void SetInput(const TInput& input) {
      a = input.a;
    }
    void GetOutput(TOutput* output) {
      output->b = b;
    }
    int State0Action(int context_id, MyStateMachine* ) {
      a = 0;
      b = a + context_id;
      return 1;
    }
  int State1Action(int context_id, MyStateMachine* ) {
    a += 5;
    b = a + context_id;
    return 2;
  }
  int State0OnEvent1(blitz::Event* event, int /* context_id */,
                     MyStateMachine* ) {
    if (event->type != 1) { return -1; }
  }

    int a;
    int b;
  };

  TestStateMachine():state_machine_("test") {
  }
  int Init() {
    MyStateMachine::State states[] = {
      {1, &TData::State0Action},
      {2, &TData::State1Action},
      {3, NULL}
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
  EXPECT_EQ(state_machine.GetStateMachine().GetStateCount(), 3);
}

TEST(StateMachine, StartOneContext) {
  TestStateMachine state_machine;
  int ret = state_machine.Init();
  EXPECT_EQ(ret, 0);
  TestStateMachine::TInput input;
  input.a = 10;
  // state_machine.GetStateMachine().StartOneContext(input, NULL, NULL);
}


