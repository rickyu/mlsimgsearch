#include "common_search.h"
#include <common/http_client.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_feature_analyzer/knn_quantizer.h>
#include <bof_feature_analyzer/idf_weighter.h>
#include <bof_searcher/ransac_ranker.h>
#include "parallel_request.h"
#include "exec_threads.h"
#include <SpatialRankerMsg.h>
#include <ExecMergeMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TBufferTransports.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

extern CLogger& g_logger;

int CommonSearch::searcher_ref_count_ = 0;

CommonSearch::CommonSearch()
  : detector_(NULL)
  , extractor_(NULL)
  , quantizer_(NULL)
  , cur_mat_(NULL) 
  , request_image_data_(NULL)
  , need_extractor_(true)
  , spatial_request_parts_(10)
  , max_response_result_count_(100000) {
  searcher_ = new Searcher;
  ++searcher_ref_count_;
}

CommonSearch::~CommonSearch() {
  --searcher_ref_count_;
  if (0 == searcher_ref_count_) {
    FeatureDetectorSelector::GetInstance()->ReleaseObject(detector_);
    FeatureDescriptorExtractorSelector::GetInstance()->ReleaseObject(extractor_);
    BOFQuantizerSelector::GetInstance()->ReleaseObject(quantizer_);
  }

  if (searcher_) {
    delete searcher_;
    searcher_ = NULL;
  }
  if (request_image_data_){
    delete [] request_image_data_;
    request_image_data_ = NULL;
  }
}

SearchServiceType CommonSearch::get_type() const {
  return ISS_COMMON_SEARCH;
}

SearchServiceBase* CommonSearch::Clone() {
  CommonSearch *instance = new CommonSearch();
  if (!weighter_ || !detector_ || !extractor_ || !quantizer_) {
    Init();
  }
  instance->detector_ = detector_;
  instance->extractor_ = extractor_;
  instance->quantizer_ = quantizer_;
  instance->need_extractor_ = need_extractor_;
  instance->weighter_ = weighter_;
  instance->he_threshold_ = he_threshold_;
  instance->spatial_ranker_name_ = spatial_ranker_name_;
  instance->image_service_server_ = image_service_server_;
  instance->spatial_ranker_servers_ = spatial_ranker_servers_;
  instance->spatial_request_parts_ = spatial_request_parts_;
  instance->max_response_result_count_ = max_response_result_count_;
  //for merge index
  instance->exec_merge_request_num_ = exec_merge_request_num_;
  instance->servers_to_datas_ = servers_to_datas_;
  instance->exec_merge_conf_ = exec_merge_conf_;
  instance->result_similar_ = result_similar_;
  return instance;
}

int CommonSearch::Init() {
  std::string detector_name = Configure::GetInstance()->GetString("detector_name");
  if("HESSIAN" == detector_name || "DSIFT" == detector_name ||"CSIFT" == detector_name ) {
    need_extractor_ = false;
  }else {
  }
  detector_ = FeatureDetectorSelector::GetInstance()->GenerateObject(detector_name);
  if (NULL == detector_) {
    LOGGER_ERROR(g_logger, "Generate detector failed. (%s)", detector_name.c_str());
    return -1;
  }
  std::string extractor_name = Configure::GetInstance()->GetString("extractor_name");
  extractor_ = FeatureDescriptorExtractorSelector::GetInstance()->GenerateObject(extractor_name);
  if (NULL == extractor_) {
    LOGGER_ERROR(g_logger, "Generate extractor failed. (%s)", extractor_name.c_str());
    return -2;
  }

  std::string quantizer_name = Configure::GetInstance()->GetString("quantizer_name");
  quantizer_ = BOFQuantizerSelector::GetInstance()->GenerateObject(quantizer_name);
  if (NULL == quantizer_) {
    LOGGER_ERROR(g_logger, "Generate quantizer selector failed. (%s)", quantizer_name.c_str());
    return -3;
  }

  //he and tf-idf
  std::string project_p_path = Configure::GetInstance()->GetString("project_p");
  std::string median_path = Configure::GetInstance()->GetString("median");
  std::string word_weight_path = Configure::GetInstance()->GetString("word_weight");
  quantizer_->SetPath(median_path,project_p_path,word_weight_path);
  std::string weighter_name = Configure::GetInstance()->GetString("weighter_name");
  he_threshold_ = Configure::GetInstance()->GetNumber<int>("he_threshold");
  weighter_ = BOFWeighterSelector::GetInstance()->GenerateObject(weighter_name);
  if (NULL == weighter_) {
    LOGGER_ERROR(g_logger, "Generate weighter failed. (%s)", weighter_name.c_str());
    return -4;
  }
  weighter_->SetConf(word_weight_path);

  std::string vocabulary_path = Configure::GetInstance()->GetString("vocpath");
  std::string voc_node_name = Configure::GetInstance()->GetString("voc_node_name");
  quantizer_->SetVocabulary(vocabulary_path, voc_node_name);
  int ret = quantizer_->Init();
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Quantizer init failed. (%d)", ret);
    return -5;
  }
  weighter_->SetWordsNum(quantizer_->GetVocabularySize());
  ret = weighter_->Init();
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Weighter init failed. (%d)", ret);
    return -6;
  }

  spatial_ranker_name_ = Configure::GetInstance()->GetString("spatial_ranker_name");
  image_service_server_ = Configure::GetInstance()->GetString("image_service_server");
  spatial_request_parts_ =
    Configure::GetInstance()->GetNumber<int>("spatial_request_parts", 10);
  max_response_result_count_ =
    Configure::GetInstance()->GetNumber<int>("max_response_result_count", 100000);

  std::vector<std::string> spatial_ranker_servers =
    Configure::GetInstance()->GetStrings("spatial_ranker_servers");
  for (size_t i = 0; i < spatial_ranker_servers.size(); ++i) {
    std::vector<std::string> parts;
    boost::algorithm::split(parts, spatial_ranker_servers[i], boost::algorithm::is_any_of(":"));
    IPAddress addr;
    addr.ip_.assign(parts[0]);
    addr.port_ = boost::lexical_cast<unsigned int>(parts[1]);
    spatial_ranker_servers_.push_back(addr);
  }
  ret = InitExecMergeConf();
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Init exec merge conf failed. (%d)", ret);
    return -7;
  }
  LOGGER_INFO(g_logger, "CommonSearch init success!");

  return 0;
}

void CommonSearch::Clear() {
  if (cur_mat_) {
    cvReleaseMat(&cur_mat_);
    cur_mat_ = NULL;
  }
   
  if (request_image_data_){
    delete [] request_image_data_;
    request_image_data_ = NULL;
  }
  
}

static bool _cmp_search_result_item(const SearchResultItem &item1,
                                    const SearchResultItem &item2) {
  return item1.sim_ > item2.sim_;
}

int CommonSearch::Process(const ImageSearchRequest &request, std::string &result) {
  if (!weighter_ || !detector_ || !extractor_ || !quantizer_) {
    LOGGER_ERROR(g_logger, "Process Param failed " );
    return -1;
  }

  char *data = NULL;
  size_t data_len = 0;
  int ret = 0;
  HttpClient http_client;
  http_client.Init();
  if (CR_SUCCESS != http_client.SendRequest(HTTP_GET,
                                            request.key.c_str(),
                                            //&request_image_data_,
                                            //&request_image_data_len_)) {
                                            &data,
                                            &data_len)) {
    LOGGER_ERROR(g_logger, "Send HTTP request failed. (%s)", request.key.c_str());
    return -1;
  }
  timeval tv1, tv2;
  gettimeofday(&tv1, NULL);
  std::auto_ptr<char> apdata(data);//智能指针
  CvMat mat_tmp = cvMat(1, data_len, CV_32FC1, data);
  cur_mat_ = cvDecodeImageM(&mat_tmp);
  cv::Mat img(cur_mat_);

  BOFQuantizeResultMap img_qdata;
  ret = detector_->Detect(img, points_);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Detect feature points failed. (%d)", ret);
    return -2;
  }
  LOGGER_INFO(g_logger, "Detect %ld feature points.", points_.size());
  if (need_extractor_) {
    ret = extractor_->Compute(img, points_, descriptors_);
  } else {
    detector_->GetDescriptor(descriptors_);
    ret = 0;
  }
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Extractor feature descriptors failed. (%d)", ret);
    return -3;
  }
  LOGGER_INFO(g_logger, "Extract %d feature features.", descriptors_.rows);

  ret = quantizer_->Compute(descriptors_, points_ , img_qdata, QUANTIZER_FLAG_HE);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Quantizer failed. (%d)", ret);
    return -4;
  }
  LOGGER_INFO(g_logger, "Compute %ld words.", img_qdata.size());

  float sum = 0;
  BOFQuantizeResultMap::iterator it = img_qdata.begin();
  std::string quantizer_info("");
  while (it != img_qdata.end()) {
    weighter_->Weighting(it->first, it->second.q_, &descriptors_.rows);
    sum += it->second.q_;
    ++it;
  }
  it = img_qdata.begin();
  while (it != img_qdata.end()) {
    quantizer_info += "(" + boost::lexical_cast<std::string>(it->first) + ":"
      + boost::lexical_cast<std::string>(it->second.q_) + ")";
    //it->second.q_ = sqrt( it->second.q_ / sum );
    it ++;
  }
  LOGGER_INFO(g_logger, "Compute image quantizer: %s sum = %f he_threshold = %d", 
              quantizer_info.c_str(),sum,he_threshold_);
  /*parallel merge begin*/
  ret = DispatchData(img_qdata);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "DispatchData failed ret = %d", ret);
    return -5;
  }
  if (0 == servers_to_datas_.size()) {
    LOGGER_ERROR(g_logger, "DispatchData failed servers_to_datas_ size is 0");
    return -5;
  }
  ParallelRequester pr_merge(servers_to_datas_.size());
  pr_merge.SetRequestCallbackFn(ExecMergeCallback, this);
  pr_merge.Run();
  Searcher searcher;
  searcher.SetParam(he_threshold_);
  searcher.SetResultSimilar(result_similar_);
  searcher.SetQueryPow(query_pow_);
  std::vector<int64_t> result_pics;
  /*parallel merge end*/

  /*************************orig merge begin************************/
  /*   
  Searcher searcher;
  searcher.SetParam(he_threshold_);
  ret = searcher.SetQueryData(&img_qdata);
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
  */ 
  /*************************orig merge end************************/

  RankInfo ri;
  ret = searcher.GetRankInfo(ri);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Searcher::GetRankInfo failed. (%d)", ret);
    return -9;
  }
  ret = searcher.Rank(ri, result_pics);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Searcher::Rank failed. (%d)", ret);
    return -10;
  }

  ret = searcher.GetResult(search_result_);
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "Searcher::GetResult failed. (%d)", ret);
    return -11;
  }

  /************************** ransanc *****************************/
  //LOGGER_INFO(g_logger, "Begin spatial ranker");
  //BOFSpatialRanker *spatial_ranker = BOFSpatialRankerSelector::GetInstance()->GenerateObject(spatial_ranker_name_);
  //if (NULL == spatial_ranker) {
  //  LOGGER_ERROR(g_logger, "BOFSpatialRankerSelector generate object failed, name=%s", spatial_ranker_name_.c_str());
  //  return -12;
  //}
  //if ("RANSAC" == spatial_ranker_name_) {
  //  RansacRankerConf conf;
  //  conf.detector_ = detector_;
  //  conf.extractor_ = extractor_;
  //  conf.image_service_server_ = image_service_server_;
  //  conf.min_inliers_number_ = Configure::GetInstance()->GetNumber<int>("min_inliers_number");
  //  conf.max_rank_pic_number_ = Configure::GetInstance()->GetNumber<int>("max_rank_pic_number");
  //  conf.rank_ignore_same_result_ = true;
  //  spatial_ranker->SetConf(&conf);
  //}
  //ret = spatial_ranker->Rank(points_, descriptors_, search_result_);
  //if (0 != ret) {
  //  BOFSpatialRankerSelector::GetInstance()->ReleaseObject(spatial_ranker);
  //  LOGGER_ERROR(g_logger, "spatial rank failed, ret=%d", ret);
  //  return -13;
  //}
  //BOFSpatialRankerSelector::GetInstance()->ReleaseObject(spatial_ranker);
  //LOGGER_INFO(g_logger, "End spatial ranker");

  // SearchResult sr_new;
  // std::vector<SearchResultItem>::iterator it_last = search_result_.items_.end();
  // if ((int)search_result_.items_.size() > max_response_result_count_) {
  //   it_last = search_result_.items_.begin() + max_response_result_count_;
  // }
  // sr_new.items_.insert(sr_new.items_.begin(), search_result_.items_.begin(), it_last);
  // search_result_ = sr_new;
  // LOGGER_INFO(g_logger, "Begin spatial ranker");
  // spatial_request_num_ = 0;
  // bool exact_division = search_result_.items_.size() % spatial_request_parts_ == 0 ? true : false;
  // spatial_request_num_ = search_result_.items_.size() / spatial_request_parts_;
  // if (0 == spatial_request_num_) {
  //   spatial_request_num_ = 1;
  // }
  // if (!exact_division) {
  //   ++spatial_request_num_;
  // }
  // ParallelRequester pr(spatial_request_num_);
  // pr.SetRequestCallbackFn(SpatialRankerCallback, this);
  // pr.Run();
  // sort(search_result_.items_.begin(), search_result_.items_.end(), _cmp_search_result_item);
  // LOGGER_INFO(g_logger, "End spatial ranker");
  /***************************ransacnc ****************************/

  ResultFormat(search_result_, result);
  //清理merge后的结果
  ClearMergeData();
  gettimeofday(&tv2, NULL);
  unsigned long ts = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1000;

  LOGGER_LOG(g_logger, "Searcher(%s) success. items_count=%ld, time_consumed=%ld, result=%s",
             request.key.c_str(), search_result_.items_.size(), ts, result.c_str());

  return 0;
}

void CommonSearch::ResultFormat(const SearchResult &sr, std::string &result) {
  result.clear();
  for (size_t i = 0; i < sr.items_.size(); ++i) {
    if (0.f == sr.items_[i].sim_) {
      break;
    }
    result += boost::lexical_cast<std::string>(sr.items_[i].pic_id_)
      + ":" + boost::lexical_cast<std::string>(sr.items_[i].sim_) + ";";
  }
  if (result[result.length()-1] == ';') {
    result.erase(result.length()-1);
  }
}

int CommonSearch::ClearMergeData() {
  query_pow_ = 0.0;
  result_similar_.clear();
  return 0;
}

int CommonSearch::SpatialRankerCallback(void *context, int thread_index) {
  CommonSearch *pcontext = static_cast<CommonSearch*>(context);
  return pcontext->SpatialRanker(thread_index);
}

int CommonSearch::ExecMergeCallback(void *context, int thread_index) {
  CommonSearch *pcontext = static_cast<CommonSearch*>(context);
  return pcontext->ExecMerge(thread_index);
}

//计算总的img_qdata应该分到哪些server取数据
int CommonSearch::DispatchData(const BOFQuantizeResultMap &img_qdata) {
  boost::mutex::scoped_lock lock(io_mutex_);
  LOGGER_INFO(g_logger, "act=DispatchData img_qdata sum size = %ld", img_qdata.size());
  if (img_qdata.size() == 0) {
    LOGGER_ERROR(g_logger, "act=DispatchData,err=img_qdata size is 0");
    return -1;
  }
  servers_to_datas_.clear();
  std::vector<MergeConf>::iterator it_conf = exec_merge_conf_.begin();
  ServerToData server_to_data;
  std::map<int, BOFQuantizeResult>::const_iterator it = img_qdata.begin(); 
  while (it != img_qdata.end()) {
    int word_id = it->first;
    if (word_id >= it_conf->begin_id_ && word_id <= it_conf->end_id_) {
      if (server_to_data.servers_.size() == 0) {
        server_to_data.servers_ = it_conf->servers_;
      }
      server_to_data.img_qdata_.insert(std::pair<int,BOFQuantizeResult>(it->first, it->second));
      ++it;
      //如果img_qdata最大的值都比第一个区域小
      if (it == img_qdata.end()) {
        LOGGER_INFO(g_logger, "act=DispatchData server=%s, img_qdata part size=%ld", 
                    server_to_data.servers_[0].ip_.c_str(), server_to_data.img_qdata_.size());
        servers_to_datas_.push_back(server_to_data);
        server_to_data.servers_.clear();
        server_to_data.img_qdata_.clear();
      }
    } else {
      //超出一个区域时保存上一个区域数据,并且server_to_data不空的时候保存数据
      if (server_to_data.img_qdata_.size() > 0) {
        LOGGER_INFO(g_logger, "act=DispatchData server=%s, img_qdata part size=%ld",
                    server_to_data.servers_[0].ip_.c_str(), server_to_data.img_qdata_.size());
        servers_to_datas_.push_back(server_to_data);
        server_to_data.servers_.clear();
        server_to_data.img_qdata_.clear();
      }
      ++it_conf;
      if (it_conf == exec_merge_conf_.end()) {
        LOGGER_ERROR(g_logger, "act=DispatchData exec_merge_conf max end_id < img_qdata max word_id");
        break;
      }
    }
  }
  return 0;
}

int CommonSearch::ExecMerge(int thread_index) {
  using namespace ::apache::thrift::protocol;
  using namespace ::apache::thrift::transport;
  using namespace ::apache::thrift;
  
  //先使用一个server测试
  std::vector<IPAddress> exec_merge_servers = servers_to_datas_[thread_index].servers_;
  BOFQuantizeResultMap img_qdata = servers_to_datas_[thread_index].img_qdata_;
  boost::shared_ptr<TTransport> socket(new TSocket(exec_merge_servers[0].ip_,
                                                   exec_merge_servers[0].port_));
  boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  ExecMergeMsgClient client(protocol);

  ExecMergeRequest request;
  ExecMergeResponse response;

  BOFQuantizeResultMap::const_iterator it = img_qdata.begin();
  while (it != img_qdata.end()) {
    TBOFQuantizeResult tbqr;
    tbqr.q = it->second.q_;
    tbqr.word_info = it->second.word_info_;
    request.img_qdata.insert(std::pair<int64_t, TBOFQuantizeResult>(it->first, tbqr));
    ++it;
  }

  transport->open();
  client.Merge(response, request); 
  if (response.ret_num == 0) {
    //boost::mutex::scoped_lock lock(io_mutex_);
    query_pow_ += response.query_pow;
    SearchResultSimilar thread_result;
    //反序列化
    std::istringstream iss(response.result);
    boost::archive::binary_iarchive iar(iss);
    for (int i = 0; i < response.result_len; i++) {
      MergeSimilarInfo msif;
      iar >> msif;
      thread_result.push_back(msif);
    } 
    /*
    LOGGER_INFO(g_logger, "act=ExecMerge response result size = %ld, query_pow_ = %f,response query_pow=%f",
                response.result.size(), query_pow_, response.query_pow);
    for (unsigned int i = 0; i < response.result.size(); i++) {
      MergeSimilarInfo msif; 
      TMergeSimilarInfo tmsif;
      msif.sim_ = response.result[i].sim;
      msif.pow_ = response.result[i].pow;
      msif.pic_id_ = response.result[i].picid;
      thread_result.push_back(msif);
    }
    */
    int ret = MergeThreadsResult(&thread_result); 
    if (0 != ret) {
      LOGGER_ERROR(g_logger, "act=MergeThreadsResult failed thread_result size = %ld",
                   thread_result.size());
      return -1;
    }
  } else {
    LOGGER_LOG(g_logger, "act=ExecMerge err:response ret=%d",
               response.ret_num);
    return -1;
  }
  transport->close();
  return 0;
}

int CommonSearch::SpatialRanker(int thread_index) {
  using namespace ::apache::thrift::protocol;
  using namespace ::apache::thrift::transport;
  using namespace ::apache::thrift;

  SpatialRankerRequest request;
  SpatialRankerResponse response;
  boost::shared_ptr<TTransport> socket(new TSocket(spatial_ranker_servers_[0].ip_,
                                                   spatial_ranker_servers_[0].port_));
  boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  SpatialRankerMsgClient client(protocol);

  // constuct request
  cv::FileStorage fs_points("1.yml",
                            cv::FileStorage::WRITE|cv::FileStorage::MEMORY|cv::FileStorage::FORMAT_YAML);
  fs_points << "points" << points_;
  request.keypoints = fs_points.releaseAndGetString();
  cv::FileStorage fs_desc("1.yml",
                          cv::FileStorage::WRITE|cv::FileStorage::MEMORY|cv::FileStorage::FORMAT_YAML);
  fs_desc << "descriptors" << descriptors_;
  request.descriptors = fs_desc.releaseAndGetString();
  for (int i = thread_index * spatial_request_num_;
       i < (thread_index+1) * spatial_request_num_ && i < (int)search_result_.items_.size();
       ++i) {
    request.pics.push_back(search_result_.items_[i].pic_id_);
  }
  bool spatial_ranker_failed = false;
  try {
    transport->open();
    client.Rank(response, request);
  }
  catch (...) {
    spatial_ranker_failed = true;
    LOGGER_ERROR(g_logger, "catch a spatial ranker exception! index=%d", thread_index);
    //return -1;
  }
  int start_index = thread_index*spatial_request_num_;
  int stop_index = (thread_index+1)*spatial_request_num_;
  for (int response_index = 0, result_index = start_index;
       result_index < stop_index && result_index < (int)search_result_.items_.size();
       ++result_index) {
    if (!spatial_ranker_failed && response.result.size() > 0
        && search_result_.items_[result_index].pic_id_ == response.result[response_index].picid) {
      float before_sim = search_result_.items_[result_index].sim_;
      search_result_.items_[result_index].sim_ *= response.result[response_index].sim;
      LOGGER_INFO(g_logger, "before_sim=%f, spatial_sim=%f, after_sim=%f",
		  before_sim, response.result[response_index].sim, search_result_.items_[result_index].sim_);
      ++response_index;
    } else {
      float before_sim = search_result_.items_[result_index].sim_;
      search_result_.items_[result_index].sim_ = 0;
      LOGGER_INFO(g_logger, "before_sim=%f, after_sim=0.0", before_sim);
    }
  }
  transport->close();

  return 0;
}

//exec_merge_conf:begin_id=0;end_id=1000;servers=127.0.0.1:7890,127.0.0.1:7980
int CommonSearch::InitExecMergeConf() {
  std::vector<std::string> exec_merge_conf =
      Configure::GetInstance()->GetStrings("exec_merge_conf");
  for (size_t i = 0; i < exec_merge_conf.size(); ++i) {
    MergeConf merge_conf;
    std::vector<std::string> parts;
    boost::algorithm::split(parts, exec_merge_conf[i], boost::algorithm::is_any_of(";"));
    for (size_t j = 0; j < parts.size(); j ++) {
      std::vector<std::string> kv;
      std::string key, value;
      boost::algorithm::split(kv, parts[j], boost::algorithm::is_any_of("="));
      if (2 != kv.size()) {
        throw 1;
      }
      key = kv[0];
      value = kv[1];
      boost::algorithm::trim(key);
      boost::algorithm::trim(value);
      if ("begin_id" == key) {
        merge_conf.begin_id_ = boost::lexical_cast<unsigned int>(value);
      }
      else if ("end_id" == key) {
        merge_conf.end_id_ = boost::lexical_cast<unsigned int>(value);
      } 
      else if ("servers" == key) {
        std::vector<std::string> servers;
        boost::algorithm::split(servers, value, boost::algorithm::is_any_of(","));
        for (size_t si = 0; si < servers.size(); si++) {
          std::vector<std::string> server_info;
          boost::algorithm::split(server_info, servers[si], boost::algorithm::is_any_of(":"));
          IPAddress addr;
          addr.ip_.assign(server_info[0]);
          addr.port_ = boost::lexical_cast<unsigned int>(server_info[1]);
          merge_conf.servers_.push_back(addr);
        }
      } 
    }
    exec_merge_conf_.push_back(merge_conf);
  }
  LOGGER_INFO(g_logger, "InitExecMergeConf success!");
  return 0;
}

//把每个线程的结果合并到result_similar_中去
int CommonSearch::MergeThreadsResult(const SearchResultSimilar *thread_result) {
  //涉及到写共享对象result_similar_，需要加锁
  boost::mutex::scoped_lock lock(io_mutex_);
  if (thread_result->size() == 0) {
    LOGGER_ERROR(g_logger, "act=MergeThreadsResult,thread_result is empty");
    return -1;
  }
  if (result_similar_.size() == 0) {
    result_similar_.assign(thread_result->begin(), thread_result->end()); 
    return 0;
  } 
  std::list<MergeSimilarInfo>::iterator it_curr = result_similar_.begin();
  std::list<MergeSimilarInfo>::const_iterator it_thread = thread_result->begin();
  while (it_thread != thread_result->end()) {
    if (it_curr->pic_id_ < it_thread->pic_id_) {
      ++it_curr;
      if (it_curr == result_similar_.end()) {
        result_similar_.push_back(*it_thread); 
        ++it_thread;
      }
    } 
    else if (it_curr->pic_id_ == it_thread->pic_id_) {
      it_curr->sim_ += it_thread->sim_;        
      it_curr->pow_ += it_thread->pow_;
      ++it_thread;
      ++it_curr;
    } else {
      result_similar_.insert(it_curr, *it_thread);
      ++it_thread;
    }
  }
  LOGGER_INFO(g_logger, "act=MergeThreadsResult,result_similar_ size = %ld", result_similar_.size());
  return 0;
}
