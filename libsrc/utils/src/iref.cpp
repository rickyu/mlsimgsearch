#include "iref.h"

using namespace std;

IRef::IRef()
    : m_ref( 0 )
{
}

IRef::~IRef()
{
}

int32_t IRef::AddRef()
{
    return HXAtomicIncRetINT32( &m_ref );
}

int32_t IRef::Release()
{
    int32_t ret = HXAtomicDecRetINT32( &m_ref );
    if( 0 == ret )
    {
        delete this;
    }
    return ret;
}

