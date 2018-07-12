#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_common/object_selector.h>
#include <common/configure.h>
#include <utils/logger.h>
#include <utils/logger_loader.h>
#include "vocabulary_creator.h"

CLogger& g_logger = CLogger::GetInstance("vcreator");


int main(int argc, char **argv, char **env) {
  if (argc < 2) {
    printf("Usage: descriptors_extractor conf_file\n");
    return -1;
  }
  Configure::GetInstance()->Init(argv[1]);

  std::string log_file = Configure::GetInstance()->GetString("log_file");
  if (0 != InitLoggers(log_file)) {
    printf("parse log configure failed!\n");
    return -1;
  }

  CreateObjectFactory();

  VocabularyCreator vcreator;
  int ret = vcreator.ExtractDescriptors(Configure::GetInstance()->GetString("descriptors_save_path"));
  if (0 != ret) {
    LOGGER_ERROR(g_logger, "VocabularyCreator::ExtractDescriptors failed, ret=%d", ret);
    return -2;
  }

  return 0;
}
