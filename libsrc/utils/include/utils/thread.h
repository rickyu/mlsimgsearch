/*
 * @file thread.h
 * @brief wrapper class of pthread
 * @date 2008-8-18
 * ATTENTION: This implementation of thread
 * has caused lots of bugs, and is strongly
 * not recommended to use in new modules
 */

#ifndef __OMNI_THREAD_H__
#define __OMNI_THREAD_H__


#include<pthread.h>
#include <stdint.h>
#include<string>

#define OMNI_THREAD_LOG "omni_thread_log"

class CThread
{
    protected:
        volatile int64_t m_bThreadRun;
        bool m_bIsJoinable;
        pthread_t m_ThreadID;
        std::string m_loggerName;

        /// To avoid instantiate object on stack
    protected:
        CThread( bool joinable = true );
        /**
         * @brief virtual destructor, Thread can be inherited
         */
        virtual ~CThread();

    public:
        /**
         * @brief Instantiate thread object on heap
         * @return Pointer to the object, NULL is no memory
         */
        static CThread* CreateInstance( bool joinable = true );

        /**
         * @brief Destroy the object
         */
        void Destroy();

        /**
         * @brief Start entrance of the thread, can be rewrite
         */
        virtual bool Start();

        /**
         * @brief Stop entrance of the thread, can be rewrite
         * @param[in] timeout : Thread should be stopped when timeout reached
         * @return true when success, false otherwise
         */
        virtual bool Stop( int timeout = 0 );

        /**
         * @brief See if the thread is running
         */
        bool IsRunning();

         /**
         * @brief Pause the thread, current pthread doesn't support this method
         * @return true when success, false otherwise
         */
        bool Pause();

        /**
         * @brief Resume the paused thread, current pthread doesn't support this
         * method
         * @return true when success, false otherwise
         */
        bool Resume();

        /**
         * @brief Wait the thread to exit
         * @param[in] timeout : no use right now
         * @return true when success, false otherwise
         */
        bool Wait( int timeout = 0 );

        /**
         * @brief Get the thread ID as said
         * @return m_ThreadID when m_bThreadRun is true, 0 otherwise
         */
        uint32_t GetThreadID(); 

        /**
         * @brief Exception handle method
         */
        virtual int32_t OnException();

        /**
         * @brief Pre handle method when thread starts
         */
        virtual int32_t OnThreadStart();

        /**
         * @brief Post handle method when thread stops
         */
        virtual int32_t OnThreadStop();

        /**
         * @brief The actual stuff need this thread to run
         * @return true when need run again, false when the task is finished
         */
        virtual bool Run();
        
        /**
         * @brief Set the user-defined log name
         */
        void SetLogger( const char* logger );
    protected:
        /**
         * @brief The thread entrance provide to pthread
         * @param[in] arg : arguments for thread running
         */
        static void* ThreadRoutine( void* arg );
};
#endif
