#ifndef _XCORE_LOCK_H
#define _XCORE_LOCK_H
#pragma once

namespace xcore::lock
{
    //------------------------------------------------------------------------------------------
    // Spin lock non-reentrant
    //------------------------------------------------------------------------------------------
    class spin  
    {
    public:

        inline void lock( void ) noexcept
        {
            while(m_qFlag.test_and_set(std::memory_order_acquire))
            {
                // spin!!
            }
            XCORE_CMD_ASSERT( xassert(m_bLocked==false); m_bLocked=true; )
        }
 
        inline void unlock( void ) noexcept
        {
            XCORE_CMD_ASSERT( xassert(m_bLocked); m_bLocked=false; )
            m_qFlag.clear(std::memory_order_release);
        }

#if _XCORE_ASSERTS
        constexpr bool isLockedDebug( void ) const noexcept
        {
            return m_bLocked;
        }
#endif

    protected:

        std::atomic_flag m_qFlag = ATOMIC_FLAG_INIT;
        XCORE_CMD_ASSERT( bool m_bLocked = false; )
    };

    //------------------------------------------------------------------------------------------
    // scope the lock
    //------------------------------------------------------------------------------------------
    template< typename T >
    class scope 
    {
    public:

        constexpr scope( T& Lock ) noexcept : m_Lock(Lock) 
        { 
            m_Lock.lock();  // lock the mutex in the constructor
        }

        inline ~scope( void ) 
        { 
            m_Lock.unlock(); // unlock the mutex in the constructor
        }

    protected:
         T& m_Lock;
    };

    //------------------------------------------------------------------------------------------
    // locked object
    //------------------------------------------------------------------------------------------
    template< typename T_CLASS, typename T_LOCK >
    class object : public T_LOCK
    {
    public:
    
                                        template<typename ...T_ARG>
        constexpr                       object              ( T_ARG&&...Args )          noexcept : m_Class{ std::forward<T_ARG>(Args)... } {}
        inline          T_CLASS&        getClass            ( void )                    noexcept { xassert( T_LOCK::isLockedDebug() ); return m_Class; }
        constexpr const T_CLASS&        getClass            ( void )             const  noexcept { xassert( T_LOCK::isLockedDebug() ); return m_Class; }

    protected:

        T_CLASS m_Class;
    };

}

#endif