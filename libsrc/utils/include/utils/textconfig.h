#ifndef __TEXTCONFIG_H__
#define __TEXTCONFIG_H__

#include "configure.h"
//#include "leakdetect.h"

/// text config
class CTextConfig :
    public CConfigureBase
{
private:
    /// file
    std::string m_file;

public:
    /// con
    CTextConfig(const std::string& filename);
    /// decon
    virtual ~CTextConfig(void);
    /// save
    virtual bool Save(const std::string& content);

protected:
    /// real init
    bool RealInit(bool inited);
    /// parse line
    void ParseLine(const std::string& line);
    /// parse file
    virtual bool ParseFile(const std::string& name);
};

#endif // __TEXTCONFIG_H__
