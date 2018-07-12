#include <bof_feature_analyzer/bof_quantizer.h>

BOFQuantizer::BOFQuantizer() {
}

BOFQuantizer::~BOFQuantizer() {
}

void BOFQuantizer::SetPath(const std::string &median_path,
                           const std::string &p_path, 
                           const std::string &word_path) {
  median_path_ = median_path;
  project_p_path_ = p_path;
  word_weight_path_ = word_path;
}

void BOFQuantizer::SetVocabulary(const std::string &voc_path,
                                 const std::string &node_name) {
  vocabulary_file_path_ = voc_path;
  vocabulary_node_name_ = node_name;
}
