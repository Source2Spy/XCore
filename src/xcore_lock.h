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
    // semaphore lock non-reentrant
    //------------------------------------------------------------------------------------------
    struct semaphore
    {
        struct thelock
        {
            mutable std::uint16_t        m_nReaders:15
                                       , m_nWritters:1;
        };

        mutable std::atomic<thelock> m_Lock{thelock{0u,0u}};

        // Writer lock
        inline void lock( void ) noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );

            do
            {
                if( p.m_nWritters || p.m_nReaders ) 
                {
                    // Spin...
                    p = m_Lock.load( std::memory_order_relaxed );
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nReaders, 1u } ) ) break;

            } while(true);
        }

        // Writer unlock
        inline void unlock( void ) noexcept
        {
            m_Lock.store( thelock{0,0}, std::memory_order_relaxed );
        }

        // Reader lock
        inline void lock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );

            do 
            {
                if( p.m_nWritters ) 
                {
                    // Spin...
                    p = m_Lock.load( std::memory_order_relaxed );
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nReaders+1u, 0u } ) ) break;

            } while(true);
        }

        // Reader unlock
        inline void unlock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            while( false == m_Lock.compare_exchange_weak( p, thelock{ p.m_nReaders-1u, 0u } ) )
            {
                // Spin...
            }
        }
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
        inline          T_CLASS&        get                 ( void )                    noexcept { xassert( T_LOCK::isLockedDebug() ); return m_Class; }
        constexpr const T_CLASS&        get                 ( void )             const  noexcept { xassert( T_LOCK::isLockedDebug() ); return m_Class; }

    protected:

        T_CLASS m_Class;
    };

}

#endif