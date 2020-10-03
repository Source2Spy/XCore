#ifndef _XCORE_LOCK_H
#define _XCORE_LOCK_H
#pragma once

namespace xcore::lock
{
    namespace details
    {
        template< std::size_t T_SIZE_BYTES >
        using                                   byte_size_uint_t        = std::tuple_element_t< T_SIZE_BYTES - 1, std::tuple<std::uint8_t, std::uint16_t, std::uint32_t, std::uint32_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>>;
        using                                   atomic_flag_base_type   = byte_size_uint_t<sizeof(std::atomic_flag)>;
        constexpr static atomic_flag_base_type  flag_value_v            = 0;
    }

    //------------------------------------------------------------------------------
    // Description:
    //      A spin lock that is thread aware so recursion will work
    //      Example: xcore::lock::spin_reentrance
    //------------------------------------------------------------------------------
    struct spin_reentrance
    {
        xforceinline void lock(void) noexcept
        {
            const auto Id = _getID();

            if (_TryLock(Id) == false)
            {
                XCORE_PERF_ZONE_SCOPED_NC("Re-entrance spin Lock is Spinning and waiting", tracy::Color::ColorType::Red);
                while (_TryLock(Id) == false);
            }
        }

        xforceinline bool trylock    (void) noexcept
        {
            const auto Id = _getID();
            return _TryLock(Id);
        }

        xforceinline void unlock( void ) noexcept
        {
            xassume( m_EnteringCount > 0 );
            --m_EnteringCount;
            if( m_EnteringCount == 0 )
            {
                m_FullThreadID = std::thread::id();
                m_State.clear(std::memory_order_release);
            }
        }

        xforceinline bool isLockedDebug( void ) const noexcept { return reinterpret_cast<std::atomic<details::atomic_flag_base_type>&>(const_cast<std::atomic_flag&>(m_State)).load(std::memory_order_relaxed) != details::flag_value_v; }

        xforceinline static const std::thread::id _getID( void ) noexcept
        {
            return std::this_thread::get_id();
        }
        

        xforceinline bool _TryLock( const std::thread::id Id ) noexcept
        {
            // Are we dealing with the same thread?
            if( m_FullThreadID == Id )
            {
                xassume( Id != std::thread::id{} );
                xassume( m_EnteringCount > 0 );
                m_EnteringCount++;
                return true;
            }

            // try to lock it officially
            if( !m_State.test_and_set( std::memory_order_acquire ) )
            {
                xassume( Id              != std::thread::id{} );
                xassume( m_FullThreadID  == std::thread::id{} );
                xassume( m_EnteringCount == 0 );
                m_FullThreadID = Id;
                m_EnteringCount++;
                return true;
            }

            return false;
        }
        
        std::thread::id  m_FullThreadID     {};
        std::uint32_t    m_EnteringCount    {0};
        std::atomic_flag m_State            { ATOMIC_FLAG_INIT };
    };

    //------------------------------------------------------------------------------
    // Description:
    //      A spin lock that is NOT thread aware so recursion won't work
    //      Example: xcore::lock::spin
    //------------------------------------------------------------------------------
    struct spin
    {
        xforceinline void lock( void ) noexcept
        {
            const auto Id = _getID();
            if( _TryLock(Id) == false )
            {
                XCORE_PERF_ZONE_SCOPED_NC("Lock is Spinning and waiting", tracy::Color::ColorType::Red);
                while( _TryLock(Id) == false );
            }
        }

        xforceinline bool trylock( void ) noexcept
        {
            const auto Id = _getID();
            return _TryLock(Id);
        }

        xforceinline bool isLockedDebug(void) const noexcept { return reinterpret_cast<std::atomic<details::atomic_flag_base_type>&>(const_cast<std::atomic_flag&>(m_State)).load(std::memory_order_relaxed) != details::flag_value_v; }

        xforceinline void unlock     ( void ) noexcept
        {
            xassert( m_Debug_Counter == 1 );
            XCORE_CMD_DEBUG( m_Debug_Counter-- );
            XCORE_CMD_DEBUG( m_Debug_FullThreadID = std::thread::id{} );
            m_State.clear(std::memory_order_release);
        }

    #if _XCORE_DEBUG
        xforceinline static const std::thread::id   _getID( void ) noexcept  { return std::this_thread::get_id(); }
    #else
        xforceinline static constexpr const int     _getID( void ) noexcept  { return 0; }
    #endif
        
    #if _XCORE_DEBUG
        xforceinline bool _TryLock ( const std::thread::id Id ) noexcept
    #else
        xforceinline bool _TryLock ( int ) noexcept
    #endif
        {
            if( !m_State.test_and_set(std::memory_order_acquire) )
            {
                XCORE_CMD_DEBUG( m_Debug_Counter++ );
                xassert( m_Debug_Counter == 1 );
                XCORE_CMD_DEBUG( m_Debug_FullThreadID = Id );
                return true;
            }
        #if _XCORE_DEBUG
            else
            {
                //const std::thread::id Id = std::this_thread::get_id();
                //x_assert( Id == std::this_thread::get_id() );
                
                // if this assert hits you may want to change to the reentrance version
                // since you did try to relock again with the same thread.
                xassert( m_Debug_FullThreadID != Id );
            }
        #endif
            return false;
        }

    #if _XCORE_DEBUG
        int              m_Debug_Counter        {0};
        std::thread::id  m_Debug_FullThreadID   {};
    #endif
        std::atomic_flag m_State                { ATOMIC_FLAG_INIT };
    };

    /*
    //------------------------------------------------------------------------------------------
    // Spin lock non-re-entrant
    //------------------------------------------------------------------------------------------
    class spin  
    {
    public:

        inline void lock( void ) noexcept
        {
            if(m_qFlag.test_and_set(std::memory_order_acquire))
            {
                XCORE_PERF_ZONE_SCOPED_NC( "Lock is Spinning and waiting", tracy::Color::ColorType::Red );
                while(m_qFlag.test_and_set(std::memory_order_acquire));
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
    */


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

        inline bool isLockedDebug(void) const noexcept { return m_Lock.load(std::memory_order_relaxed).m_nLocks; }

        // Writer lock
        inline void lock( void ) noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );
            do 
            { 
                xassert( p.m_nWaitingWritters < ((1u<<5)-2u) ); 
                if( p.m_nLocks )
                {
                    if ( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks, p.m_nWaitingWritters+1u, p.m_isLockWriter } ) ) 
                    {
                        XCORE_PERF_ZONE_SCOPED_NC( "Waiting to access for writing", tracy::Color::ColorType::Red );
                        do
                        {
                            if( p.m_nLocks ) 
                            {
                                // Spin...
                                p = m_Lock.load( std::memory_order_relaxed );
                            }
                            else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks+1u, p.m_nWaitingWritters, 1u } ) ) goto LOCKED;

                        } while(true);
                    }
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ 1u, p.m_nWaitingWritters+1u, 1u } ) ) goto LOCKED;

            } while( true );
        
        LOCKED:;
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
                    XCORE_PERF_ZONE_SCOPED_NC( "Waiting to access for reading", tracy::Color::ColorType::Red )
                    do 
                    {
                        // Spin...
                        p = m_Lock.load( std::memory_order_relaxed );
                    } while (p.m_nWaitingWritters);
                }
                else if( m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks+1u, 0u, 0u } ) ) break;

            } while(true);
        }

        /*
        // Transition From Reader Lock to Writer Lock. Note you need to unlock like a writer now!
        inline void TransitionFromReadToWriteLock( void ) const noexcept
        {
            auto p = m_Lock.load( std::memory_order_relaxed );

            // We must have a lock already!
            assert( p.m_isLockWriter == false );
            assert( p.m_nLocks >= 1 );

            // Let the lock know that we are trying to transition
            while( false == m_Lock.compare_exchange_weak( p, thelock{ p.m_nLocks, p.m_nWaitingWritters+1u, p.m_nLocks == 1 ? true : false } ) );

            p = m_Lock.load( std::memory_order_relaxed );
            if( p.m_isLockWriter ) return;

            while( p.m_nLocks > 1 )
            {
                // Spin and wait!
                p = m_Lock.load( std::memory_order_relaxed );
            }
            
            m_Lock.store( thelock{ 1u, p.m_nWaitingWritters-1u, true }, std::memory_order_release );
        }
        */

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