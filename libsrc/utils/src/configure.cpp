#include "configure.h"
#include "string_op.h"


CConfigureBase::CConfigureBase(void)
: m_init(false)
, m_change(false)
{
}

CConfigureBase::~CConfigureBase(void)
{
}

bool CConfigureBase::Init(void)
{
    if(!m_init)
    {
        m_values.clear();
        m_init = RealInit(m_init);
        m_change = false;
    }
    return m_init;
}

bool CConfigureBase::Reload(void)
{
    m_init = false;
    m_init = Init();
    if(m_init)
    {
        m_change = false;
    }
    return m_init;
}

bool CConfigureBase::Save(void)
{
    if(m_change)
    {
        m_change = !Save(Rebuild());
    }
    return !m_change;
}

bool CConfigureBase::Exist(const std::string& name)
{
    bool ret = false;
    if(Init())
    {
        hash_multi_map_t::const_iterator use = m_values.find(_configure_upper(name));
        if(use != m_values.end())
        {
            ret = true;
        }
    }
    return ret;
}

bool CConfigureBase::Exclusive(const std::string& name)
{
    bool ret = false;
    int count = 0;
    if(Init())
    {
        hash_multi_map_t::const_iterator use;
        for(use = m_values.lower_bound(_configure_upper(name))
            ; use != m_values.upper_bound(_configure_upper(name))
            ; use++, count++)
        {
        }
    }
    ret = (count == 1);
    return ret;
}

void CConfigureBase::Remove(const std::string& name)
{
    std::string iname = _configure_upper(name);
    hash_multi_map_t::iterator use = m_values.find(iname);
    while(use != m_values.end())
    {
        m_values.erase(use);
        use = m_values.find(iname);
    }
}

std::string CConfigureBase::GetString(const std::string& name)
{
    std::stringstream ret;
    if(Init())
    {
        hash_multi_map_t::const_iterator use = m_values.find(_configure_upper(name));
        if(use != m_values.end())
        {
            ret << use->second.value;
        }
    }
    return ret.str();
}

std::vector<std::string> CConfigureBase::GetStrings(const std::string& name)
{
    std::vector<std::string> ret;
    if(Init())
    {
        hash_multi_map_t::const_iterator use;
        for(use = m_values.lower_bound(_configure_upper(name))
            ; use != m_values.upper_bound(_configure_upper(name))
            ; use++)
        {
            ret.push_back(use->second.value);
        }
    }
    return ret;
}

bool CConfigureBase::SetString(const std::string& name, const std::string& value)
{
    bool ret = false;
    bool bExist = false;
    std::string iname = _configure_upper(name);
    std::string ivalue = _configure_trim(value, " \r\n\t");
    if(iname.length() > 0 && ivalue.length() > 0)
    {
        hash_multi_map_t::const_iterator use;
        for(use = m_values.lower_bound(iname)
            ; use != m_values.upper_bound(iname)
            ; use++)
        {
            if(use->second.value.compare(ivalue) == 0)
            {
                bExist = true;
                break;
            }
        }
        if(!bExist)
        {
            Item item;
            item.name = name;
            item.value = ivalue;
            m_values.insert(make_pair(iname, item));
            m_change = true;
        }
        ret = true;
    }
    return ret;
}

void CConfigureBase::iterate(CONFIG_ITER proc)
{
    if(Init())
    {
        if(proc)
        {
            hash_multi_map_t::const_iterator iter;
            for(iter = m_values.begin(); iter != m_values.end(); iter++)
            {
                proc(iter->first, iter->second.value);
            }
        }
    }
}

std::string CConfigureBase::Rebuild(void)
{
    std::stringstream ss;
    hash_multi_map_t::const_iterator iter = m_values.begin();
    while(iter != m_values.end())
    {
        ss << iter->second.name << " = " << iter->second.value << std::endl;
        iter++;
    }
    return ss.str();
}
