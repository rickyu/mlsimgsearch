#ifndef IMAGE_SEARCH_SEARCHD_PARALLEL_REQUEST_H_
#define IMAGE_SEARCH_SEARCHD_PARALLEL_REQUEST_H_

typedef int (*parallel_request_callback_fn)(void *context, int index);

class ParallelRequester {
 public:
  ParallelRequester(int requests_num);
  ~ParallelRequester();

  void SetRequestCallbackFn(parallel_request_callback_fn fn, void *context);
  int Run();

 private:
  parallel_request_callback_fn request_fn_;
  void *request_context_;
  int requests_num_;
};


#endif
