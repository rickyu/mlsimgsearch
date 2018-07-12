#include <bof_searcher/ransac_ranker.h>
#include <sys/time.h>
#include <common/http_client.h>
#include <utils/json_parser.h>
#include <boost/lexical_cast.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <beansdb_client/beansdb_clusters.h>


RansacRanker::RansacRanker()
  : estimate_times_(200)
  , inlier_threshold_(3)
  , min_inliers_number_(8)
  , max_rank_pic_number_(100)
  , rank_ignore_same_result_(false)
  , image_service_get_if_("/record/get?id=") {
}

RansacRanker::~RansacRanker() {
  result_spatial_info_.clear();
  ransac_results_.clear();
  image_service_get_if_.clear();
}

std::string RansacRanker::GetName() const {
  return "RANSAC";
}

BOFSpatialRanker* RansacRanker::Clone() {
  return new RansacRanker();
}

int RansacRanker::Rank(const std::vector<cv::KeyPoint> &key_points,
                       const cv::Mat &descriptors,
                       SearchResult &result,
                       const void *context /* = NULL */) {
  (void)(context);

  querier_points_ = const_cast<std::vector<cv::KeyPoint>*>(&key_points);
  querier_descriptors_ = const_cast<cv::Mat*>(&descriptors);

  LOGGER_INFO(logger_, "begin RansacRanker::Rank result items=%ld", result.items_.size());
  CutHandleRedult(result);
  int ret = BeginRank(result);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "BeginRank failed, ret=%d", ret);
    return -2;
  }
  ret = Check(result);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Check result failed, ret=%d", ret);
    return -3;
  }
  ret = Ranking(result);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "Ranking failed, ret=%d", ret);
    return -4;
  }
  ret = EndRank(result);
  if (0 != ret) {
    LOGGER_ERROR(logger_, "EndRank failed, ret=%d", ret);
    return -5;
  }
  
  return ret;
}

int RansacRanker::BeginRank(const SearchResult &result) {
  ransac_results_.clear();
  for (size_t i = 0; i < result.items_.size(); ++i) {
    RansacSpatialInfo sinfo;
    int ret = GetResultLocalSpatialInfo(result.items_[i], sinfo);
    if (0 != ret) {
      LOGGER_ERROR(logger_, "GetResultLocalSpatialInfo failed, ret=%d", ret);
      ransac_results_.push_back(0);
      continue;
    }
    ComputeInliers(&sinfo);
    LOGGER_INFO(logger_, "search result =%ld", result.items_[i].pic_id_);
  }
  return 0;
}

// 如果内点小于查询图像特征点的1/10则认为两张图片不相似
int RansacRanker::Check(SearchResult &result) {
  int ransac_result_index = 0;
  std::vector<SearchResultItem>::iterator it = result.items_.begin();
  while (it != result.items_.end()) {
    int cur_inliers_number = ransac_results_[ransac_result_index++];
    if (cur_inliers_number < min_inliers_number_) {
      LOGGER_LOG(logger_,
                 "pic_id=%ld ranker_failed, cur_inliers_number=%d, min_inliers_number=%d",
                 it->pic_id_, cur_inliers_number, min_inliers_number_);
      it->sim_ = 0;
    }
    else {
      LOGGER_LOG(logger_,
                 "pic_id=%ld, cur_inliers_number=%d, querier_points number=%ld, cur_sim=%f",
                 it->pic_id_, cur_inliers_number, querier_points_->size(), it->sim_);
      it->sim_ *= (float)cur_inliers_number / (float)querier_points_->size();
    }
    ++it;
  }
  return 0;
}

static bool cmp_search_result_item(const SearchResultItem &item1,
                                    const SearchResultItem &item2) {
  return item1.sim_ > item2.sim_;
}

int RansacRanker::Ranking(SearchResult &result) {
  if (should_rank_) {
    sort(result.items_.begin(), result.items_.end(), cmp_search_result_item);
  }
  return 0;
}

int RansacRanker::EndRank(SearchResult &result) {
  result.items_.insert(result.items_.begin(),
                       same_result_before_rerank_.items_.begin(),
                       same_result_before_rerank_.items_.end());
                
  return 0;
}

int RansacRanker::CutHandleRedult(SearchResult &result) {
  same_result_before_rerank_.items_.clear();
  SearchResult result_new;
  // 得到相同图片
  if (!rank_ignore_same_result_) {
    std::vector<SearchResultItem>::iterator it = result.items_.begin();
    while (it != result.items_.end()) {
      if (it->sim_ >= 0.98f) {
        same_result_before_rerank_.items_.push_back(*it);
        result.items_.erase(it);
        it = result.items_.begin();
        LOGGER_INFO(logger_, "Got an same one, picid=%ld", it->pic_id_);
      }
      else {
        break;
      }
    }
  }
  if (result.items_.size() <= (size_t)max_rank_pic_number_) {
    return 0;
  }
  result_new.items_.insert(result_new.items_.begin(),
                           result.items_.begin(),
                           result.items_.begin() + max_rank_pic_number_);
  result = result_new;
  return 0;
}

int RansacRanker::GetResultLocalSpatialInfo(const SearchResultItem &item,
                                            RansacSpatialInfo &sinfo) {
  HttpClient re_client;
  re_client.Init();
  std::string url = image_service_server_ + image_service_get_if_
                    + boost::lexical_cast<std::string>(item.pic_id_);
  //LOGGER_INFO(logger_, "begin get item, url=%s", url.c_str());
  char *qpic_info=NULL;
  size_t qpic_info_len = 0;
  int ret = re_client.SendRequest(HTTP_GET,
                                  url.c_str(),
                                  &qpic_info, &qpic_info_len,
                                  NULL, 0,
                                  3000, 4000);
  if (CR_SUCCESS != ret) {
    if(qpic_info) {
      delete qpic_info;
      qpic_info = NULL;
    }
    LOGGER_ERROR(logger_, "Send HTTP request failed, url=%s", url.c_str());
    return -1;
  }
  //LOGGER_INFO(logger_, "Send HTTP request success, result: %.*s", (int)qpic_info_len, qpic_info);
  JsonParser jp;
  ret = jp.LoadMem(qpic_info, qpic_info_len);
  if (qpic_info) {
    free(qpic_info);
    qpic_info =NULL;
  }
  if (CR_SUCCESS == ret) {
    std::string return_str;
    if (CR_SUCCESS == jp.GetString("/data[0]/n_pic_file",return_str)) {
      url = image_service_server_ + "/" + return_str;
      char *imgdata = NULL;
      size_t imgdata_len = 0;
      ret = re_client.SendRequest(HTTP_GET,
                                  url.c_str(),
                                  &imgdata, &imgdata_len,
                                  NULL, 0,
                                  3000, 4000);
      //ret = BeansdbClusters::GetInstance()->Get(return_str, imgdata, imgdata_len);
      if (CR_SUCCESS != ret) {
        if (imgdata) {
          free(imgdata);
          imgdata = NULL;
        }
        LOGGER_ERROR(logger_, "Download image failed, url=%s", url.c_str());
        return -2;
      }
      //LOGGER_INFO(logger_, "Download picture success");
      CvMat mat_tmp = cvMat(1, imgdata_len, CV_32FC1, imgdata);
      CvMat *cur_mat = cvDecodeImageM(&mat_tmp);
      cv::Mat img(cur_mat);
      if (imgdata) {
        free(imgdata);
        imgdata = NULL;
      }

      std::vector<cv::KeyPoint> points;
      cv::Mat descriptors;
      ret = detector_->Detect(img, sinfo.points_);
      if (0 != ret) {
        cvReleaseMat(&cur_mat);
        LOGGER_ERROR(logger_, "Detect feature points failed, ret=%d", ret);
        return -3;
      }
      ret = extractor_->Compute(img, sinfo.points_, sinfo.descriptors_);
      if (0 != ret) {
        cvReleaseMat(&cur_mat);
        LOGGER_ERROR(logger_, "Compute feature descriptor failed, ret=%d", ret);
        return -4;
      }
      cvReleaseMat(&cur_mat);
    }
    else {
      LOGGER_ERROR(logger_, "Get /data[0]/n_pic_file failed!");
    }
  }
  return 0;
}

int RansacRanker::GetRandomNumbers(const int max_value,
                                   const int num,
                                   std::vector<int> &numbers) const {
  numbers.clear();
  if (max_value <= 0 || num <= 0) {
    return -1;
  }
  int got_num = 0;
  while (got_num < num) {
    timeval t;
    gettimeofday(&t, NULL);
    srand((unsigned)t.tv_usec);
    int rnum = rand() % max_value;
    bool exist = false;
    for (size_t i = 0; i < numbers.size(); ++i) {
      if (numbers[i] == rnum) {
        exist = true;
      }
    }
    if (!exist) {
      numbers.push_back(rnum);
      ++got_num;
    }
  }
  return 0;
}

int RansacRanker::ComputeInliers(const RansacSpatialInfo *spatial_info) {
  // 特征点匹配
  std::vector<cv::DMatch> matches;
  cv::BruteForceMatcher< cv::L2<float> > matcher;
  matcher.match(*querier_descriptors_, spatial_info->descriptors_, matches);
  std::vector<cv::Point2f> pmat1, pmat2;
  for (size_t i = 0; i < matches.size(); ++i) {
    pmat1.push_back((*querier_points_)[matches[i].queryIdx].pt);
    pmat2.push_back(spatial_info->points_[matches[i].trainIdx].pt);
  }
  // RANSAC模型匹配
  int inliers_number = 0;
  cv::Mat best_affine;
  for (int i = 0; i < estimate_times_; ++i) {
    int cur_inliers_number = 0;
    std::vector<int> random_nums;
    GetRandomNumbers(matches.size(), 3, random_nums);
    cv::Point2f ps1[3], ps2[3];
    for (size_t i = 0; i < random_nums.size(); ++i) {
      ps1[i] = pmat1[random_nums[i]];
      ps2[i] = pmat2[random_nums[i]];
    }
    cv::Mat mat_affine( 2, 3, CV_32FC1 );
    mat_affine = cv::getAffineTransform(ps1, ps2);

    cv::Mat A = mat_affine.rowRange(0, 2).colRange(0, 2);
    cv::Mat tmp;
    A.convertTo(tmp, CV_32F);
    A = tmp;
    cv::Mat B = mat_affine.rowRange(0, 2).colRange(2, 3);
    B.convertTo(tmp, CV_32F);
    B = tmp;
    for (size_t i = 0; i < matches.size(); ++i) {
      cv::Mat mat_query(2, 1, CV_32F),
        mat_transformed = cv::Mat::zeros(mat_query.rows,
                                         mat_query.cols,
                                         mat_query.type());
      mat_query.at<float>(0, 0) = pmat1[i].x;
      mat_query.at<float>(1, 0) = pmat1[i].y;
      mat_transformed = A * mat_query + B;
      float dist = sqrt(pow(pmat2[i].x-mat_transformed.at<float>(0, 0), 2.f)
                        + pow(pmat2[i].y-mat_transformed.at<float>(1, 0), 2.f));
      if (dist <= inlier_threshold_) {
        ++cur_inliers_number;
      }
    }
    if (cur_inliers_number > inliers_number) {
      inliers_number = cur_inliers_number;
      best_affine = mat_affine;
    }
  }

  ransac_results_.push_back(inliers_number);

  return 0;
}

void RansacRanker::SetConf(const void *pconf) {
  RansacRankerConf *conf = static_cast<RansacRankerConf*>(const_cast<void*>(pconf));
  min_inliers_number_ = conf->min_inliers_number_;
  max_rank_pic_number_ = conf->max_rank_pic_number_;
  detector_ = conf->detector_;
  extractor_ = conf->extractor_;
  image_service_server_ = conf->image_service_server_;
  rank_ignore_same_result_ = conf->rank_ignore_same_result_;
  should_rank_ = conf->should_rank_;
}
