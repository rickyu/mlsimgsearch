#pragma once

namespace blitz {
class Framework;
struct RunContext {
  int thread_number;
  pthread_t thread_id;
  Framework* framework;
};

class Runnable {
  public:
    virtual ~Runnable() {};
    virtual int Run(const RunContext& run_context) = 0;
};


struct TaskArg {
  union {
    uint64_t u64;
    void* ptr;
  };
};
struct TaskArgs {
  TaskArg arg1;
  TaskArg arg2;
  TaskArg arg3;
  TaskArg arg4;
};
typedef void (*FnTaskCallback)(const RunContext& run_context,
                               const TaskArgs& task_args);
typedef void (*FnTaskDelete)(const TaskArgs& task_args);
//
// fn_free只有在task超时，fn_callback不被回调的时候会被framework调用.正常
// 情况下应该由fn_callback释放内存.
class Task {
 public:
  Task() {
    fn_callback = NULL;
    fn_delete = NULL;
    args.arg1.ptr = NULL;
    args.arg2.ptr = NULL;
    args.arg3.ptr = NULL;
    args.arg4.ptr = NULL;
    expire_time = 0;
  }
 public:
  FnTaskCallback fn_callback;
  FnTaskDelete fn_delete; // 如果超时,或者退出时task没有执行,将调用该回调函数.
  TaskArgs args;
  uint64_t expire_time;
};
class TaskUtil {
 public:
  static void ZeroTask(Task* task) {
    task->fn_callback = NULL;
    task->fn_delete = NULL;
    task->expire_time = 0;
    task->args.arg1.ptr = NULL;
    task->args.arg2.ptr = NULL;
    task->args.arg3.ptr = NULL;
    task->args.arg4.ptr = NULL;
  }
};
} // namespace blitz;
