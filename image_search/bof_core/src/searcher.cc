#include <bof_searcher/searcher.h>
#include <bof_searcher/indexer.h>
#include <bof_feature_analyzer/hamming_embedding_descriptor.h>
#include <boost/lexical_cast.hpp>
#include <sys/time.h>

CLogger& Searcher::logger_ = CLogger::GetInstance("searcher");

Searcher::Searcher() {
  begin_bucket_id_ = 0;
}

Searcher::~Searcher() {
}
int Searcher::SetParam(int he_t) {
  he_threshold_ = he_t;
  return 0;
}

int Searcher::SetBeginBucketId(int begin_bucket_id) {
  begin_bucket_id_ = begin_bucket_id;
  return 0;
}

int Searcher::SetQueryData(const BOFQuantizeResultMap *query_data) {
  query_data_ = const_cast<BOFQuantizeResultMap*>(query_data);
  return 0;
}

int Searcher::Merge(int max_bucket_count, std::vector<int64_t> &result) {
  LOGGER_INFO(logger_, "act=Merge query word number %ld : ", query_data_->size());
  result.clear();
  result_similar_.clear();
  query_pow_ = 0;
  BOFQuantizeResultMap::iterator it = query_data_->begin();
  while (it != query_data_->end()) {
    timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    if (0 != MergeBucketResult(it->first)) {
      LOGGER_ERROR(logger_, "act=Merge MergeBucketResult:Load Bucket failed word  = %d  ", it->first);
      //return -1;
    }
    gettimeofday(&tv2, NULL);
    unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
    LOGGER_INFO(logger_, "act=Merge bucket %d MergeBucketResult timed used: %ld", it->first, ts);
    ++it;
  }
  return result_similar_.size() > 0 ? 0 : -2;
}

/*
 * 之前bucket表示的是其在向量中的位置；
 * 分块后开始位置变成了begin_bucket_id_
 * 因此bucket在向量中的偏移开始位置设置为begin_bucket_id_
 */
int Searcher::MergeBucketResult(int bucket) {
  bucket = bucket - begin_bucket_id_;
  SearchResultSimilar::iterator it_result_cur = result_similar_.begin();
  timeval tv1, tv2, tv3, tv4, tv5, tv6;
  gettimeofday(&tv1, NULL);
  WordIndex word_index;
  char *word_data = NULL;
  /*
  //优先从内存加载，如果失败从磁盘加载
  int ret = BOFIndex::GetInstance()->GetWord(bucket, word_index);
  if (0 != ret) {
    //LOGGER_DEBUG(logger_, "GetWord from mem failed , word_id = %d", bucket);
    //重新从磁盘加载
    if (0 != BOFIndex::GetInstance()->LoadWord(bucket, word_index, word_data)) {
      if (word_data) {
        delete []word_data;
        word_data = NULL;
      }
      return -1;
    }
  }
  */
  //直接从磁盘load data
  if (0 != BOFIndex::GetInstance()->LoadWord(bucket, word_index, word_data)) {
    if (word_data) {
      delete []word_data;
      word_data = NULL;
    }
    return -1;
  }
  gettimeofday(&tv2, NULL);
  unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
  LOGGER_INFO(logger_, "act=MergeBucketResult LoadWord:%d,timed used:%ld", bucket, ts);

  HammingEmbedding he;
  int h_l = 64;
  he.SetHammingParam(h_l, he_threshold_);
  float query_weight = (*query_data_)[bucket].q_;
  std::vector<IndexItem*> &items = word_index.items_;
  std::string query_he = (*query_data_)[bucket].word_info_[0];
  std::string query_angle = (*query_data_)[bucket].word_info_[1];
  unsigned long *query_he64_int = NULL;
  int query_he64_int_len = 0;
  he.HeString2Int(query_he,query_he64_int,query_he64_int_len);
  LOGGER_INFO(logger_, "bucket %d query_he =%s",bucket,query_he.c_str());
  
  unsigned long *pic_info = NULL; 
  int he_similar_num = 0;
  query_pow_ += pow(query_weight, 2);

  unsigned int *pic_angle = NULL;
  short len = 0;
  float weight = 0;
  size_t item_index = 0;
 
  sum_he_time_ = 0;
  sum_angle_time_ = 0;
  LOGGER_LOG(logger_, "act=MergeBucketResult compute begin items size=%ld", items.size());
  while (item_index < items.size()) {
    if (items[item_index]->pic_id_ == it_result_cur->pic_id_) {
      //****************hamming embedding**************
      gettimeofday(&tv3, NULL);
      he_similar_num = 0;
      pic_info = items[item_index]->he_;
      pic_angle = items[item_index]->angle_;
      len = items[item_index]->length_;
      he.HeDistance(he_similar_num,pic_info,len, query_he64_int,query_he64_int_len);
      gettimeofday(&tv4, NULL);
      unsigned long he_time = ((tv4.tv_sec-tv3.tv_sec)*1000000+(tv4.tv_usec-tv3.tv_usec));
      sum_he_time_ += he_time;
      if (0 == he_similar_num) {
        weight = 0;
      } else {
        gettimeofday(&tv5, NULL);
        weight = items[item_index]->weight_;
        //it_result_cur->wgc_angle_.BuildAngleHistogram(query_angle,pic_angle);
        it_result_cur->sim_ += weight * query_weight;
        it_result_cur->pow_ += pow(items[item_index]->weight_, 2); // lost some word
        gettimeofday(&tv6, NULL);
        unsigned long angle_time = ((tv6.tv_sec-tv5.tv_sec)*1000000+(tv6.tv_usec-tv5.tv_usec));
        sum_angle_time_ += angle_time;
      }
      ++it_result_cur;
      ++item_index;
    }
    else if (items[item_index]->pic_id_ < it_result_cur->pic_id_
             || result_similar_.end() == it_result_cur) {
      MergeSimilarInfo sinfo;
      sinfo.pic_id_ = items[item_index]->pic_id_;

      //****************hamming embedding**************
      gettimeofday(&tv3, NULL);
      he_similar_num = 0;
      pic_info = items[item_index]->he_;
      pic_angle = items[item_index]->angle_;
      len = items[item_index]->length_;
      he.HeDistance(he_similar_num,pic_info,len, query_he64_int,query_he64_int_len);
      gettimeofday(&tv4, NULL);
      unsigned long he_time = ((tv4.tv_sec-tv3.tv_sec)*1000000+(tv4.tv_usec-tv3.tv_usec));
      sum_he_time_ += he_time;
      
      if (0 == he_similar_num ) {
        weight = 0;
      } else {
        gettimeofday(&tv5, NULL);
        weight = items[item_index]->weight_; sinfo.wgc_angle_.Init(20);//angle histogram bin
        //sinfo.wgc_angle_.BuildAngleHistogram(query_angle,pic_angle);
        sinfo.sim_ += weight * query_weight;
        sinfo.pow_ += pow(items[item_index]->weight_, 2);
        if (sinfo.pow_ > 0.f) {
          result_similar_.insert(it_result_cur, sinfo);
        }
        gettimeofday(&tv6, NULL);
        unsigned long angle_time = ((tv6.tv_sec-tv5.tv_sec)*1000000+(tv6.tv_usec-tv5.tv_usec));
        sum_angle_time_ += angle_time;
      }
      ++item_index;
    } else {
      ++it_result_cur;
    }
  }
  LOGGER_LOG(logger_, "act=MergeBucketResult exited while HeDistance time used:%ld,BuildAngleHistogram time used:%ld", sum_he_time_, sum_angle_time_);
  word_index.ClearItems();
  if (query_he64_int) {
    delete [] query_he64_int;
    query_he64_int = NULL;
  }
  if (word_data) {
    delete []word_data;
    word_data = NULL;
  }
  return 0;
}

int Searcher::GetRankInfo(RankInfo &rank_info) {
  return 0;
}

static bool _cmp_search_result_item(const SearchResultItem &item1,
                                    const SearchResultItem &item2) {
  return item1.sim_ > item2.sim_;
}

int Searcher::Rank(const RankInfo &rank_info, std::vector<int64_t> &result) {
  LOGGER_INFO(logger_, "act=Rank merge result size=%ld query_pow = %f",
              result_similar_.size(),query_pow_);
  // 用余弦内积表示其相似度
  SearchResultSimilar::iterator it_sim = result_similar_.begin();
  result_.items_.clear();
  while (it_sim != result_similar_.end()) {
    int max_his_angle = 0;
    it_sim->wgc_angle_.Wgc(max_his_angle);
    SearchResultItem item;
    item.pic_id_ = it_sim->pic_id_;
    if(it_sim->pow_ > 0 && query_pow_ > 0 ) {
      item.sim_ = it_sim->sim_ / sqrt(it_sim->pow_ * query_pow_);
      //LOGGER_INFO(logger_, "it_sim->pic_id_ = %ld,it_sim->sim_ = %f,it_sim->pow_ = %f,query_pow_ = %f,item.sim_ = %f", 
      //            it_sim->pic_id_, it_sim->sim_, it_sim->pow_, query_pow_, item.sim_);
      //item.sim_ = it_sim->sim_;
    }
    //weak geometry consistency
    //item.sim_ *= max_his_angle / query_data_->size(); 
    // 相似度太低就算了吧，性能伤不起
    if (item.sim_ > 0.1) {
      result_.items_.push_back(item);
    }
    ++it_sim;
  }

  sort(result_.items_.begin(), result_.items_.end(), _cmp_search_result_item);

  return 0;
}

int Searcher::GetResult(SearchResult &result) {
  result = result_;
  return 0;
}
void Searcher::MergeByThread(const BOFQuantizeResultMap *query_data, 
                             boost::promise<SearchResultSimilar> *promise_result) {
  LOGGER_INFO(logger_, "query word number %ld : ", query_data->size());
  result_similar_.clear();
  query_pow_ = 0;
  BOFQuantizeResultMap::const_iterator it = query_data->begin();
  while (it != query_data->end()) {
    timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    if (0 != MergeBucketResult(it->first)) {
      LOGGER_ERROR(logger_, "Load Bucket failed word  = %d  ", it->first);
      //return -1;
    }
    gettimeofday(&tv2, NULL);
    unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;
    LOGGER_INFO(logger_, "Merge bucket %d timed used: %ld", it->first, ts);
    ++it;
  }
  promise_result->set_value(result_similar_);
}
//通过线程来写数据
int Searcher::SetResultSimilar(const SearchResultSimilar &result_similar) {
  result_similar_ = result_similar; 
  //LOGGER_INFO(logger_, "act = SetResultSimilar after set result_similar size = %ld", result_similar_.size());
  return 0;
}

int Searcher::GetResultSimilar(SearchResultSimilar *&result_similar) {
  //LOGGER_INFO(logger_, "act = GetResultSimilar result_similar size = %ld", result_similar_.size());
  result_similar = &result_similar_;
  return 0;
}

int Searcher::SetQueryPow(const float query_pow) {
  query_pow_ = query_pow;
  return 0;
}

int Searcher::GetQueryPow(float &query_pow) {
  query_pow = query_pow_;
  return 0;
}
