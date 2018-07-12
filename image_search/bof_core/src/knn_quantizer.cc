#include <bof_feature_analyzer/knn_quantizer.h>
#include <bof_feature_analyzer/hamming_embedding_descriptor.h>
#include <boost/lexical_cast.hpp>
#include <fstream>

/**
 * 关于此处为什么要用这种恶心的方式来实现：
 *     1. cv::flann::Index 这个东东在同一个进程中被new多次会有问题，
 *                         那么会造成不同进程对同样的矩阵量化结果不一样
 *     2. 之所以不把tree_flann_index_申明成KnnQuantizer的静态成员，谁用谁知道
 */
class FlannIndexInstance {
public:
  FlannIndexInstance() {
    vocabulary_.release();
    tree_flann_index_ = NULL;
  }

  static FlannIndexInstance* GetInstance() {
    if (NULL == instance_) {
      instance_ = new FlannIndexInstance;
    }
    return instance_;
  }

  bool IsInited() {
    return tree_flann_index_ ? true : false;
  }

  int Init(const std::string &vocabulary_file_path, const std::string &vocabulary_node_name) {
    cv::FileStorage fs(vocabulary_file_path, cv::FileStorage::READ);
    fs[vocabulary_node_name.c_str()] >> vocabulary_;
    fs.release();
    if (CV_32F != vocabulary_.type()) {
      cv::Mat mat;
      vocabulary_.convertTo(mat, CV_32F);
      vocabulary_ = mat;
    }

    if (0 == vocabulary_.rows) {
      return -1;
    }
  
    tree_flann_index_ = new cv::flann::Index(vocabulary_, 
                                             cv::flann::KDTreeIndexParams());
    if (NULL == tree_flann_index_) {
      return -2;
    }
    return 0;
  }

  cv::flann::Index* GetFlannIndex() {
    return tree_flann_index_;
  }

  int GetVocabularySize() const {
    return vocabulary_.rows;
  }
  
private:
  static FlannIndexInstance* instance_;
  cv::flann::Index* tree_flann_index_;
  cv::Mat vocabulary_;
};

FlannIndexInstance* FlannIndexInstance::instance_ = NULL;

KnnQuantizer::KnnQuantizer()
  : nndr_(0.6f) {
}

KnnQuantizer::~KnnQuantizer() {
}

int KnnQuantizer::Init() {
  if (!FlannIndexInstance::GetInstance()->IsInited()) {
    FlannIndexInstance::GetInstance()->Init(vocabulary_file_path_, vocabulary_node_name_);
  }
  tree_flann_index_ = FlannIndexInstance::GetInstance()->GetFlannIndex();

  //read matrix P
  if (project_p_path_.length()) {
    cv::FileStorage fr1(project_p_path_, cv::FileStorage::READ);
    fr1["Hamming_Project_P"] >> p_mat_;
    fr1.release();
  }

  //read median value matrix
  if(median_path_.length()) {
    cv::FileStorage fr2(median_path_, cv::FileStorage::READ);
    fr2["Hamming_Embedding"] >> median_mat_;
    fr2.release();
  }
  return 0;
}

int KnnQuantizer::GetVocabularySize() const {
  return FlannIndexInstance::GetInstance()->GetVocabularySize();
}

std::string KnnQuantizer::GetName() const {
  return "KNN";
}

BOFQuantizer* KnnQuantizer::Clone() {
  BOFQuantizer *quantizer = new KnnQuantizer();
  return quantizer;
}

int KnnQuantizer::Compute(const cv::Mat &line) {
  cv::Mat src;
  if (CV_32F != line.type()) {
    line.convertTo(src, CV_32F);
  }
  else {
    src = line;
  }
  
  int k = 2;
  cv::Mat results_mat(src.rows, k, CV_32FC1);
  cv::Mat dists_mat(src.rows, k, CV_32FC1);

  tree_flann_index_->knnSearch(src, results_mat, dists_mat, 
                               k, cv::flann::SearchParams());
  
  if (dists_mat.at<float>(0,0) <= nndr_ * dists_mat.at<float>(0,1)) {
    int word_id = results_mat.at<int>(0, 0);
    return word_id;
  }
  return -1;
}

int KnnQuantizer::Compute(const cv::Mat &descriptors,
                          const std::vector<cv::KeyPoint> &detect,
                          BOFQuantizeResultMap &result,
                          QuantizerFlag flag /* = QUANTIZER_FLAG_HE */) {
  cv::Mat src;
  if (CV_32F != descriptors.type()) {
    descriptors.convertTo(src, CV_32F);
  }
  else {
    src = descriptors;
  }
  
  result.clear();
  
  int k = 2;
  cv::Mat results_mat(src.rows, k, CV_32FC1);
  cv::Mat dists_mat(src.rows, k, CV_32FC1);
  tree_flann_index_->knnSearch(src, results_mat, dists_mat, 
                               k, cv::flann::SearchParams());
  std::string he_str="";
  HammingEmbedding he;

  bool bool_wgc = false;
  if ( detect.size() ) {
    bool_wgc = true;
  }

  std::string angle_str = "";
  int angle = -1;
  for (int i = 0; i < src.rows; i++) {
    if (dists_mat.at<float>(i,0) <= nndr_ * dists_mat.at<float>(i,1)) {
      int word_id = results_mat.at<int>(i, 0);
      result[word_id].q_ += 1.f;

      switch (flag) {
      case QUANTIZER_FLAG_HE:
        {
          float *desc_p = src.ptr<float>(i);
          float *mat_word = median_mat_.ptr<float>(word_id);
          if (bool_wgc) {
            angle = detect[i].angle;
            angle_str = angle /100 + '0';
            angle_str += ( angle % 100 ) / 10 + '0';
            angle_str += (angle % 10 ) + '0';
          }else {
            angle_str = "";
          }
          he.HammingCoder(desc_p, p_mat_, mat_word, he_str);
          result[word_id].word_info_[0] += he_str;
          result[word_id].word_info_[1] += angle_str;
        }
        break;
      default:
        break;
      }
    }
  }
  return 0;
}
