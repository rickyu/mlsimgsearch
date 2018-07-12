#include "exec_threads.h"

extern CLogger& g_logger;


const size_t PART_NUM_MAX_SIZE = 30;

//把img_qdata分成step份
int ExecThreads::BeginMerge(int step,
                            BOFQuantizeResultMap img_qdata,
                            PartMergeVector &pmv) {
  int count_num = 0;
  int len = img_qdata.size() / step;
  if (len == 0) {
    len = 1;
  }
  BOFQuantizeResultMap::iterator it = img_qdata.begin();  
  while (count_num < (step - 1)) {
    PartMerge pm; 
    for (int i = 0; i < len; i++) {
      pm.img_qdata.insert(std::pair<int,BOFQuantizeResult>(it->first, it->second));
      LOGGER_INFO(g_logger, "bucket = %d", it->first);
      it++;
    }
    count_num ++;
    LOGGER_INFO(g_logger, "part num = %d, img_qdata size = %ld", count_num, pm.img_qdata.size());
    pmv.push_back(pm);
  }
  PartMerge pm_end; 
  while(it != img_qdata.end()) {
    pm_end.img_qdata.insert(std::pair<int,BOFQuantizeResult>(it->first, it->second));
    it++;
  }
  pmv.push_back(pm_end);
  return 0;
}
//执行合并操作，每个分割后的数据块都对应一个线程去执行
int ExecThreads::ExecMerge(int max_bucket_count,
                           int He_Threshold,
                           PartMergeVector &pmv,
                           boost::promise<SearchResultSimilar> *promise_result_similar) {
  int ret = 0;
  int size = pmv.size();
  if (size == 0) {
    LOGGER_ERROR(g_logger, "exec_merge error PartMergeVector size is 0");
    return -1;
  }
  LOGGER_INFO(g_logger, "part_merge_vector size = %d", size);
  SearchResultSimilar result_similar;

  boost::array<UniqueFutureSearch, PART_NUM_MAX_SIZE> aufs;
  boost::array<PromiseSearch, PART_NUM_MAX_SIZE> aps;
  boost::array<UniqueFutureSearch, PART_NUM_MAX_SIZE>::iterator it_cur = aufs.begin();
  for(int i = 0; i < size; ++i) {
    pmv[i].searcher.SetParam(He_Threshold);
    ret = pmv[i].searcher.SetQueryData(&pmv[i].img_qdata);
    if (0 != ret) {
      LOGGER_ERROR(g_logger, "Searcher::SetQueryData failed. (%d)", ret);
      return -5;
    }
    /* 
    WordInfo wi;
    ret = pmv[i].searcher.Filter(&wi);
    if (0 != ret) {
      LOGGER_ERROR(g_logger, "Searcher::Filter failed. (%d)", ret);
      return -7;
    }
    */
    aufs[i] = aps[i].get_future();
    it_cur++;
    boost::thread(boost::bind(&Searcher::MergeByThread,
                              &pmv[i].searcher,
                              &pmv[i].img_qdata,
                              &aps[i]));
  }
  wait_for_all(aufs.begin(), it_cur);
  BOOST_FOREACH(UniqueFutureSearch& ufs, aufs) {
    if (ufs.is_ready() && ufs.has_value()) {
      SearchResultSimilar tmp_result;
      tmp_result = ufs.get(); 
      LOGGER_INFO(g_logger, "act=GetMergeResult tmp_result size = %ld", tmp_result.size());
      while (!tmp_result.empty()) {
        result_similar.push_back(tmp_result.front());
        tmp_result.pop_front();
      }
    }
  }
  promise_result_similar->set_value(result_similar);
  LOGGER_INFO(g_logger, "act=Merge END");
  return 0;
}
