#include <bof_feature_analyzer/gmm_quantizer.h>
#include <bof_feature_analyzer/hamming_embedding_descriptor.h>
#include <boost/lexical_cast.hpp>

int GMMQuantizer::Init() {
  CvFileStorage *fs = cvOpenFileStorage(vocabulary_file_path_.c_str(),
                                        0,
                                        CV_STORAGE_READ);
  if (NULL == fs) {
    return -1;
  }
  CvFileNode *node = cvGetFileNodeByName(fs, 0, vocabulary_node_name_.c_str());
  if (NULL == node) {
    return -2;
  }
  em_model_.read(fs, node);
  cvReleaseFileStorage(&fs);

  //read matrix P
  cv::FileStorage fr1(project_p_path_, cv::FileStorage::READ);
  fr1["Hamming_Project_P"] >> p_mat_;
  fr1.release();

  //read median value matrix
  cv::FileStorage fr2(median_path_, cv::FileStorage::READ);
  fr2["Hamming_Embedding"] >> median_mat_;
  fr2.release();

  return 0;
}

int GMMQuantizer::GetVocabularySize() const {
  return em_model_.getNClusters();
}

std::string GMMQuantizer::GetName() const {
  return "GMM";
}

BOFQuantizer* GMMQuantizer::Clone() {
  return new GMMQuantizer;
}

int GMMQuantizer::Compute(const cv::Mat &line) {
  int word_id = boost::lexical_cast<int>(em_model_.predict(line.row(0)));
  return word_id;
}

int GMMQuantizer::Compute(const cv::Mat &descriptors,
                          const std::vector<cv::KeyPoint> &detect,
                          BOFQuantizeResultMap &result,
                          QuantizerFlag flag /*= QUANTIZER_FLAG_HE*/) {
  HammingEmbedding he;
  std::string he_str;
  cv::Mat probs;
  for (int row = 0; row < descriptors.rows; ++row) {
    float m = em_model_.predict(descriptors.row(row), &probs);
    int word_id = boost::lexical_cast<int>(m);
    result[word_id].q_ += 1.f;

    switch (flag) {
    case QUANTIZER_FLAG_HE:
      {
        float *desc_p = const_cast<float*>(descriptors.ptr<float>(row));
        float *mat_word = median_mat_.ptr<float>(word_id);
        he.HammingCoder(desc_p, p_mat_, mat_word, he_str);
        result[word_id].word_info_[0] += he_str;
      }
      break;
    default:
      break;
    }
  }

  return 0;
}
