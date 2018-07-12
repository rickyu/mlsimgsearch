#include <common/configure.h>

Configure* Configure::instance_ = NULL;

Configure::Configure()
  : config_(NULL) {
}

Configure::~Configure() {
  if (config_) {
    delete config_;
    config_ = NULL;
  }
}

int Configure::Init(const std::string &conf_file) {
  config_ = new CTextConfig(conf_file);
  if (NULL == config_) {
    return -1;
  }
  if (!config_->Init()) {
    return -2;
  }
  return 0;
}

void Configure::Release() {
  if (instance_) {
    delete instance_;
    instance_ = NULL;
  }
}

Configure* Configure::GetInstance() {
  if (NULL == instance_) {
    instance_ = new Configure();
  }
  return instance_;
}

void Configure::GetString(const std::string &key, std::string &value) {
  try {
    value = config_->GetString(key);
  }
  catch (...) {
  }
}

void Configure::GetStrings(const std::string &key, std::vector<std::string> &values) {
  try {
    values = config_->GetStrings(key);
  }
  catch (...) {
  }
}

std::string Configure::GetString(const std::string &key) {
  std::string value("");
  try {
    value = config_->GetString(key);
  }
  catch (...) {
  }
  return value;
}

std::vector<std::string> Configure::GetStrings(const std::string &key) {
  std::vector<std::string> values;
  values.clear();

  try {
    values = config_->GetStrings(key);
  }
  catch (...) {
  }
  return values;
}
