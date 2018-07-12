#ifndef __configure_base_H_20100127_1402__
#define __configure_base_H_20100127_1402__

#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>


//#include "protocol/imtype.h"

/// 不要改变这个枚举的值，要在别的地方作为下标使用!!!
enum 
{ 
	  CONFIG_SUCCESS 		= 0
	, CONFIG_NO_ATTRIBUTE 	= 1
	, CONFIG_WRONG_TYPE 	= 2
};

/// 遍历
typedef int32_t (*CONFIG_ITER)(const std::string& key, const std::string& value);

/// ConfigureBase
class CConfigureBase
{
protected:
    /// item
    typedef struct
    {
        /// name
        std::string name;
        /// value
        std::string value;
    }Item;

protected:
    /// typedef __gnu_cxx::hash_multimap<std::string, Item> hash_multi_map_t;
    typedef std::multimap<std::string, Item> hash_multi_map_t;
    hash_multi_map_t m_values;
    bool m_init;
    bool m_change;

public:
    CConfigureBase(void);
    virtual ~CConfigureBase(void);
    /// init
    bool Init(void);
    /// reload
    bool Reload(void);
    /// save
    bool Save(void);
    /// exist
    virtual bool Exist(const std::string& name);
    /// exlcusive
    virtual bool Exclusive(const std::string& name);
    /// remove
    virtual void Remove(const std::string& name);
    /// get string
    virtual std::string GetString(const std::string& name);
    /// get strings
    virtual std::vector<std::string> GetStrings(const std::string& name);
    /// get string
    virtual bool SetString(const std::string& name, const std::string& value);
    void iterate(CONFIG_ITER proc);

protected:
    virtual bool RealInit(bool inited) = 0;
    virtual bool Save(const std::string& content) = 0;

    std::string Rebuild(void);
};

/// configure
template<typename C>
class CConfigure
{
private:
    C* m_instance;

public:
    CConfigure(C* conf)
    : m_instance(conf)
    {
        if(m_instance)
        {
            m_instance->Init();
        }
    }
    virtual ~CConfigure(void)
    {
        if(m_instance)
        {
            delete m_instance;
        }
    }
    bool Init(void)
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->Init();
        }
        return ret;
    }
    bool Reload(void)
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->Reload();
        }
        return ret;
    }
    bool Save(void)
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->save();
        }
        return ret;
    }
    bool Exist(const std::string& name) const
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->Exist(name);
        }
        return ret;
    }
    bool Exclusive(const std::string& name) const
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->exclusive(name);
        }
        return ret;
    }
    void Remove(const std::string& name)
    {
        if(m_instance)
        {
            m_instance->Remove(name);
        }
    }
    std::string GetString(const std::string& name) const
    {
        std::string ret;
        if(m_instance)
        {
            ret = m_instance->GetString(name);
        }
        return ret;
    }
    std::vector<std::string> GetStrings(const std::string& name) const
    {
        std::vector<std::string> ret;
        if(m_instance)
        {
            ret = m_instance->GetStrings(name);
        }
        return ret;
    }
    bool SetString(const std::string& name, const std::string& value)
    {
        bool ret = false;
        if(m_instance)
        {
            ret = m_instance->SetString(name, value);
        }
        return ret;
    }
    template <typename T>
        T GetValue(const std::string& name) const
    {
        T ret;
        std::istringstream ins(GetString(name));
        ins >> ret;
        return ret;
    }
    template <typename T>
        T GetValue(const std::string& name, const T& defRet) const
    {
        if(Exist(name))
        {
            return GetValue<T>(name);
        }
        else
        {
            return defRet;
        }
    }
    template <typename T>
        std::vector<T> GetValues(const std::string& name) const
    {
        std::vector<T> ret;
        std::vector<std::string> use = GetStrings(name);
        for(size_t i = 0; i < use.size(); i++)
        {
            T value;
            std::istringstream ins(use[i]);
            ins >> value;
            ret.push_back(value);
        }
        return ret;
    }
    template <typename T>
        bool SetValue(const std::string& name, T value)
    {
        std::stringstream ss;
        ss << value;
        return SetString(name, ss.str());
    }
    void iterate(CONFIG_ITER proc) const
    {
        m_instance->iterate(proc);
    }

	//doubhor
	/*-------------------------------------
	返回值:
	enum 
	{ 
		  CONFIG_SUCCESS 		= 0
		, CONFIG_NO_ATTRIBUTE 	= 1
		, CONFIG_WRONG_TYPE 	= 2
	};
	-------------------------------------*/
	template <typename T>
	int QueryValue(const std::string& name, T* outValue, const T& defRet) const
	{
		// first set the default vlaue.
		*outValue = defRet;
		
        if(!Exist(name))
        {
			return CONFIG_NO_ATTRIBUTE;
        }

		// method of chenming
		//std::istringstream sstream(getString(name));

		// method of TinyXML
		std::stringstream sstream(GetString(name));
		sstream >> *outValue;

		if (!sstream.fail())
		{
			return CONFIG_SUCCESS;
		}
		return CONFIG_WRONG_TYPE;
	}
	
};

#endif //  __configure_base_H__
