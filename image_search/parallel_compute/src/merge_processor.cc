#include "merge_processor.h"
#include <common/http_client.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_feature_analyzer/knn_quantizer.h>
#include <bof_feature_analyzer/idf_weighter.h>
#include <bof_searcher/searcher.h>
#include <common/configure.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <sstream>
#include <iostream>
#include <string>

extern CLogger& g_logger;

MergeProcessor::MergeProcessor() {}

MergeProcessor::~MergeProcessor() {}

int MergeProcessor::Init() {
  partition_num_ = Configure::GetInstance()->GetNumber<int>("partition_num");  
  he_threshold_ = Configure::GetInstance()->GetNumber<int>("he_threshold"); 
  begin_word_id_ = Configure::GetInstance()->GetNumber<int>("begin_word_id");
  end_word_id_ = Configure::GetInstance()->GetNumber<int>("end_word_id");
  return 0;
}
//把img_qdata分成step份
int MergeProcessor::PartitionData() {
  int count_num = 0;
  int len = img_qdata_.size() / partition_num_;
  if (len == 0) {
    len = 1;
  }
  BOFQuantizeResultMap::iterator it = img_qdata_.begin();  
  while (count_num < (partition_num_ - 1)) {
    PartMerge pm; 
    for (int i = 0; i < len; i++) {
      pm.img_qdata.insert(std::pair<int,BOFQuantizeResult>(it->first, it->second));
      LOGGER_INFO(g_logger, "bucket = %d", it->first);
      it++;
    }
    count_num ++;
    LOGGER_INFO(g_logger, "part num = %d, img_qdata size = %ld", count_num, pm.img_qdata.size());
    pmv_.push_back(pm);
  }
  PartMerge pm_end; 
  while(it != img_qdata_.end()) {
    pm_end.img_qdata.insert(std::pair<int,BOFQuantizeResult>(it->first, it->second));
    it++;
  }
  pmv_.push_back(pm_end);
  return 0;
}

//执行合并操作，每个分割后的数据块都对应一个线程去执行
int MergeProcessor::ExecMerge(boost::promise<SearchResultSimilar> *promise_result_similar) {
  int ret = 0;
  int size = pmv_.size();
  if (size == 0) {
    LOGGER_ERROR(g_logger, "exec_merge error PartMergeVector size is 0");
    return -1;
  }
  LOGGER_INFO(g_logger, "part_merge_vector size = %d", size);

  boost::array<UniqueFutureSearch, PART_NUM_MAX_SIZE> aufs;
  boost::array<PromiseSearch, PART_NUM_MAX_SIZE> aps;
  boost::array<UniqueFutureSearch, PART_NUM_MAX_SIZE>::iterator it_cur = aufs.begin();
  for (int i = 0; i < size; ++i) {
    pmv_[i].searcher.SetParam(he_threshold_);
    pmv_[i].searcher.SetBeginBucketId(begin_word_id_);
    ret = pmv_[i].searcher.SetQueryData(&pmv_[i].img_qdata);
    if (0 != ret) {
      LOGGER_ERROR(g_logger, "Searcher::SetQueryData failed. (%d)", ret);
      return -5;
    }
    /* 
    WordInfo wi;
    ret = pmv_[i].searcher.Filter(&wi);
    if (0 != ret) {
      LOGGER_ERROR(g_logger, "Searcher::Filter failed. (%d)", ret);
      return -7;
    }
    */
    aufs[i] = aps[i].get_future();
    it_cur++;
    boost::thread(boost::bind(&Searcher::MergeByThread,
                              &pmv_[i].searcher,
                              &pmv_[i].img_qdata,
                              &aps[i]));
  }
  wait_for_all(aufs.begin(), it_cur);
  BOOST_FOREACH(UniqueFutureSearch& ufs, aufs) {
    if (ufs.is_ready() && ufs.has_value()) {
      SearchResultSimilar tmp_result;
      tmp_result = ufs.get(); 
      LOGGER_INFO(g_logger, "act=GetMergeResult tmp_result size = %ld", tmp_result.size());
      while (!tmp_result.empty()) {
        result_similar_->push_back(tmp_result.front());
        tmp_result.pop_front();
      }
    }
  }
  promise_result_similar->set_value(*result_similar_);
  LOGGER_INFO(g_logger, "act=Merge END");
  return 0;
}

int MergeProcessor::ParseRequest(const ExecMergeRequest& request) {
  std::map<int64_t, TBOFQuantizeResult>::const_iterator it = request.img_qdata.begin();
  while (it != request.img_qdata.end()) {
    BOFQuantizeResult bof_quantize_result;
    bof_quantize_result.q_ = it->second.q;
    bof_quantize_result.word_info_ = it->second.word_info;
    img_qdata_.insert(std::pair<int, BOFQuantizeResult>(it->first, bof_quantize_result)) ;
    it++;
  }
  //log info
  std::string quantizer_info("");
  BOFQuantizeResultMap::iterator itq = img_qdata_.begin();
  while (itq != img_qdata_.end()) {
   quantizer_info += "(" + boost::lexical_cast<std::string>(itq->first) + ":"
      + boost::lexical_cast<std::string>(itq->second.q_) + ")";
    itq ++; 
  }
  LOGGER_INFO(g_logger, "Compute image quantizer: %s", 
              quantizer_info.c_str());
  LOGGER_INFO(g_logger, "real img_qdata_ size = %ld", img_qdata_.size());
  return 0;
}

int MergeProcessor::ConstructResult(ExecMergeResponse& response) {
  LOGGER_INFO(g_logger, "act=ConstructResult real result_similar_ size = %ld", result_similar_->size());
  response.result_len = 0;
  // 序列化对象
  std::ostringstream oss;
  boost::archive::binary_oarchive oar(oss);
  while (!result_similar_->empty()) {
    MergeSimilarInfo msif = result_similar_->front();
    oar << msif;
    response.result_len += 1;
    result_similar_->pop_front();
  }
  response.result = oss.str();
  /*
  while (!result_similar_->empty()) {
    TMergeSimilarInfo tmsif; 
    MergeSimilarInfo msif = result_similar_->front();
    tmsif.sim = msif.sim_;
    tmsif.pow = msif.pow_;
    tmsif.picid = msif.pic_id_;
    response.result.push_back(tmsif);
    result_similar_->pop_front();
  }
  */
  if (response.result.size() > 0) {
    response.ret_num = 0;
    response.query_pow = query_pow_;
    LOGGER_INFO(g_logger, "act=ConstructResult response result.size = %ld,result_len = %d", 
                response.result.size(),response.result_len);
  } else {
    response.ret_num = -1;
    LOGGER_ERROR(g_logger, "act=ConstructResult response ret_num = -1");
  }
  return 0;
}

int MergeProcessor::Merge(ExecMergeResponse& response,
                          const ExecMergeRequest& request) {
  if (request.img_qdata.size() == 0) {
    LOGGER_ERROR(g_logger, "img_qdata size is empty");
    return -1;
  }  
  LOGGER_INFO(g_logger, "recive img_qdata size = %ld", request.img_qdata.size());

  /*orig merge method begin*/
  ParseRequest(request); 
  Searcher searcher;
  searcher.SetParam(he_threshold_);
  int ret = searcher.SetQueryData(&img_qdata_);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Searcher::SetQueryData failed. (%d)", ret);
    return -5;
  }
  std::vector<int64_t> result_pics;
  int max_bucket_count = Configure::GetInstance()->GetNumber<int>("max_merge_bucket_num");
  ret = searcher.Merge(max_bucket_count, result_pics);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Searcher::Merge failed. (%d)", ret);
    return -8;
  }
  searcher.GetResultSimilar(result_similar_);
  searcher.GetQueryPow(query_pow_);
  LOGGER_INFO(g_logger, "result_similar size (%ld),he_threshold_ = %d, query_pow_ = %f", result_similar_->size(), he_threshold_, query_pow_);
  /*orig merge method end*/

  /*use multi threads begin*/
  //ParseRequest(request); 
  //PartitionData();

  //UniqueFutureSearch  ufs;
  //PromiseSearch ps;
  //ufs = ps.get_future();
  //boost::thread(boost::bind(&MergeProcessor::ExecMerge,
  //                          this,
  //                          &ps));
  //ufs.wait();
  //if (ufs.is_ready() && ufs.has_value()) {
  //  result_similar_ = ufs.get();
  //}
  //LOGGER_INFO(g_logger, "result_similar size (%ld)", result_similar_.size());
  /*use multi threads end*/
  ConstructResult(response);
  return 0;
}

