#include "textconfig.h"
#include "string_op.h"
#include <fstream>
#include <iostream>


CTextConfig::CTextConfig(const std::string& filename)
    : m_file(filename)
{
}

CTextConfig::~CTextConfig(void)
{
}

bool CTextConfig::Save(const std::string& content)
{
    bool ret = false;
    std::ofstream ofs(m_file.c_str());
    if(ofs)
    {
        ofs << content;
        ret = true;
    }
    return ret;
}

void CTextConfig::ParseLine(const std::string& line)
{
    std::string::const_iterator iter = line.begin();
    std::string name;
    std::string value;
    if((iter == line.end()) || (*iter == '#'))
    {
        return;
    }
    while((*iter != ':') && (iter != line.end()))
    {
        iter++;
    }
    if(iter != line.end())
    {
        name.assign(line.begin(), iter);
        value.assign(iter + 1, line.end());
        name = _configure_trim(name);
        value = _configure_trim(value);
    }
    SetString(name, value);
}

bool CTextConfig::ParseFile(const std::string& name)
{
    bool ret = false;
    std::ifstream ifs(name.c_str());
    if(ifs)
    {
        std::string line;
        while(getline(ifs, line))// && !ifs.eof())
        {
            ParseLine(line);
        }
        ret = true;
    }
    else
    {
    }
    return ret;
}

bool CTextConfig::RealInit(bool inited)
{
    bool ret = inited;
    if(!ret)
    {
        ret = ParseFile(m_file);
    }
    return ret;
}
