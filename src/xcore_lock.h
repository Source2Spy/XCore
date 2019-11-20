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
    // semaphore lock non-reentrant (Gives priority to writers)
    //------------------------------------------------------------------------------------------
    struct semaphore
    {
        struct thelock
        {
            std::uint16_t           m_nLocks:10             // max(1024) number of locks currently in action. Readers can lock many times, writers only ones
                                ,   m_nWaitingWritters:5    // max(32)   number of waiting to lock writers
                                ,   m_isLockWriter:1;       // What type of lock is in action? Reading or writing?
        };

        mutable std::atomic<thelock> m_Lock{thelock{0u,0u,0u}};

        // Writer lock
        inline void lock( void ) noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );

            do 
            { 
                xassert( p.m_nWaitingWritters < ((1u<<5)-2u) ); 
                if( p.m_nLocks )
                {
                    if ( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks, p.m_nWaitingWritters+1u, p.m_isLockWriter } ) ) break;
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ 1u, p.m_nWaitingWritters+1u, 1u } ) ) break;

            } while( true );

            if( p.m_nLocks ) do
            {
                if( p.m_nLocks ) 
                {

                    // Spin...
                    p = m_Lock.load( std::memory_order_relaxed );
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks+1u, p.m_nWaitingWritters, 1u } ) ) break;

            } while(true);
        }

        // Writer unlock
        inline void unlock( void ) noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            {
                xassert( p.m_isLockWriter );
                xassert( p.m_nWaitingWritters > 0 );
                xassert( p.m_nLocks == 1u );
            } while( false == m_Lock.compare_exchange_weak( p, thelock{ 0u, p.m_nWaitingWritters-1u, 0u } ) );
        }

        // Reader lock
        inline void lock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            {
                if( p.m_nWaitingWritters ) 
                {
                    // Spin...
                    p = m_Lock.load( std::memory_order_relaxed );
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks+1u, 0u, 0u } ) ) break;

            } while(true);
        }

        // Reader unlock
        inline void unlock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            {
                xassert( p.m_isLockWriter == false );
                xassert( p.m_nLocks > 0u );
            } while( false == m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks-1u, p.m_nWaitingWritters, 0u } ) );
        }

        inline xcore::err ExclusiveWriteLock( void ) noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            {
                if( p.m_nLocks || p.m_nWaitingWritters ) return xerr_failure_s( "Fail to exclusively lock for writing. There are other locks pending in xcore::semaphore");
            } while( false == m_Lock.compare_exchange_weak( p, thelock{ 1u, 1u, 1u } ) );

            return {};
        }

        inline xcore::err ExclusiveReadLock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            {
                if( p.m_nWaitingWritters ) return xerr_failure_s( "Fail to exclusively lock for reading. There is a writer pending using the xcore::semaphore");
            } while( false == m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks+1u, 0u, 0u } ) );

            return {};
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