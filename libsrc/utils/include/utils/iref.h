#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_IREF_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_DB_IREF_H_
#include "atomic.h"

class IRef
{
    public:
        IRef();
        virtual ~IRef();

        /** 
         * \brief AddRef
         *    add ref count by 1
         * 
         * \return the value after operation
         */
        int32_t AddRef();
        /** 
         * \brief Release
         *   decrease ref count by 1
         * 
         * \return the value after operation
         */
        int32_t Release();

    private:
        int32_t m_ref;
};

#endif

