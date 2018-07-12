#ifndef UTILS_GEARMAN_WORKER_H_
#define UTILS_GEARMAN_WORKER_H_

#include <string>
#include <queue>
#include <libgearman/gearman.h>
#include "logger.h"
#include "locks.h"
#include "constant.h"

typedef void* (gearman_worker_callback_fn)(gearman_job_st *job,
                                           void *context,
                                           size_t *result_size,
                                           gearman_return_t *ret_ptr);

class GearmanWorker {
 public:
  GearmanWorker();
  ~GearmanWorker();

  /**
   * @brief GearmanWorker初始化，在此创建gearman_worker_st对象
   * @return 成功返回CR_SUCCESS
   */
  const int Init();
  /**
   * @brief 将gearman worker连接到一台job server服务器
   * @param addr [in] job server服务器IP地址
   * @param port [in] job server服务器端口
   */
  const int AddServer(const std::string &addr,
                      const uint32_t port);
  /**
   * @brief 将gearman worker连接到多台服务器
   * @param addr [in], 服务器地址，按以下格式：SERVER[:PORT][,SERVER[:PORT]]...
   */
  const int AddServers(const std::string &addr);
  const int SetGrabUnique();
  const int SetTimeoutReturn();
  /**
   * @brief Set socket I/O activity timeout for connections
   *        in a Gearman structure.
   * @param ms_timeout [in] Milliseconds to wait for I/O activity.
   *                        A negative value means an infinite timeout.
   */
  void SetIOTimeout(const int ms_timeout = -1);
  /**
   * @brief 给gearman worker注册服务函数
   * @param func_name [in] 服务函数名称
   * @param timeout [in] 等待任务时的超时时间
   */
  const int RegisterFunction(const std::string &func_name,
                             uint32_t timeout = 0);
  /**
   * @brief 从job server获取多个任务
   * @param num [in] 欲获取的任务数
   */
  const int GrabJobs(int num);
  const gearman_job_st* GrabJob();
  /**
   * @brief 得到当前已分配而未处理的任务数
   */
  const size_t GetJobQueueSize();
  /**
   * @brief 从任务队列中得到一个任务
   * @return 如果没有任务，返回NULL
   */
  const gearman_job_st* PopJob();
  /**
   * @brief 发送处理结果
   * @param job [in] 被处理的任务
   * @param succ [in] 任务是否处理成功
   * @param result [in] 处理结果
   * @param result_size [in] 处理结果大小
   * @return
   * @note 在处理完一个任务之后，调用此函数给job server发送处理结果
   *       后才能将此任务从job server的任务队列中删除
   */
  const int SendJobResponse(const gearman_job_st *job,
                            const bool succ,
                            const void *result = NULL,
                            const size_t result_size = 0);
  /**
   * @brief 释放一个任务所占用的资源
   */
  void FreeJob(gearman_job_st *&job);
  /**
   * @brief 获取任务的unique ID
   */
  void GetJobUniqueIDString(const gearman_job_st *job,
                            std::string &id);
  int GetJobUniqueIDInt(const gearman_job_st *job);
  int64_t GetJobUniqueIDInt64(const gearman_job_st *job);
  long GetJobUniqueIDLong(const gearman_job_st *job);
  /**
   * @brief 获取任务的worload
   */
  void GetJobWorkloadString(const gearman_job_st *job,
                            std::string &work_load);
  void GetJobWorkload(const gearman_job_st *job,
                      void *&wl,
                      size_t &wl_size);

  // 下面的函数通过回调函数来实现worker的功能
  const int AddFunction(const std::string &func_name,
                        const gearman_worker_callback_fn callback,
                        const int timeout,
                        void *context);
  void Quit();
  void Start();

 private:
  void PushJob(const gearman_job_st *job);

 private:
  gearman_worker_st *worker_;
  std::string function_name_;
  Mutex mutex_;
  std::queue<gearman_job_st*> jobs_;
  bool quit_;
  static CLogger& logger_;
};

#endif
