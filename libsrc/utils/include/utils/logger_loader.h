
#ifndef __LOGGER_LOADER_H_20100127_1318__
#define __LOGGER_LOADER_H_20100127_1318__

#include "logappender.h"
#include "logger.h"

/// inner use
std::vector<std::string> _logger_config_split(const std::string& src, const std::string& det, 
    unsigned int count = 0);
/// inner use
std::vector<std::string> _logger_config_split(const std::string& src, char det, 
    unsigned int count = 0);

/// logger loader
int InitLogger(const std::string& path,const std::string& conf);
int InitLoggers(const std::string& conf);
int ReinitLoggers(const std::string& conf);

#endif

