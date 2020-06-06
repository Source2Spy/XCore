#ifndef _XCORE_LOCKLES_POOL_H
#define _XCORE_LOCKLES_POOL_H
#pragma once

namespace xcore::lockless::pool
{
    namespace details
    {
        //------------------------------------------------------------------------------
        // Description:
        //      general - An allocator object is require and it will handle how to allocate memory
        //------------------------------------------------------------------------------
        template< typename T, template<typename,typename> class T_ALLOCATOR, typename T_COUNTER = std::size_t >
        class mpmc_bounded_general
        {
        public:     using                                                   value_type      = T;
        protected:  struct alignas( xcore::target::getCacheLineSize() )     real_node; 
        public:     using                                                   counter         = T_COUNTER;
        public:     using                                                   allocator       = T_ALLOCATOR< real_node, counter >;
        public:

            constexpr               mpmc_bounded_general    ( void ) noexcept { clear(); }
    
            inline void clear ( void ) noexcept  
            {
                if( m_Allocator.size() )
                {
                    const counter Count = m_Allocator.size() - 1;

                    for( counter i = 0; i<Count; i++ )
                    {
                        m_Allocator[i].m_Next.store( &m_Allocator[i + 1], std::memory_order_relaxed );
                    }
        
                    m_Allocator[Count].m_Next.store( nullptr, std::memory_order_relaxed );
                    m_Head.store( &m_Allocator[0] );
                    // SanityCheck();
                }
            }
    
            inline value_type* pop ( void ) noexcept  
            {
                // SanityCheck();
                real_node* pLocalReality = m_Head.load( std::memory_order_relaxed );
                while( pLocalReality )
                {
                    if( m_Head.compare_exchange_weak( pLocalReality, pLocalReality->m_Next ) )
                    {
                        // SanityCheck();
                        // SanityCheck( pLocalReality );
                        return &pLocalReality->m_Entry;
                    }
                }
                return nullptr;
            }
    
            inline void push ( value_type& userEntry ) noexcept  
            {
                // SanityCheck();

                real_node& Node            = reinterpret_cast<real_node&>( (reinterpret_cast<std::byte*>(&userEntry)[-real_node_offset_v]) );
                real_node* pLocalReality   = m_Head.load( std::memory_order_relaxed );
        
                // SanityCheck( &Node );

                do
                {
                    Node.m_Next.store( pLocalReality, std::memory_order_relaxed );
                } while( false == m_Head.compare_exchange_weak( pLocalReality, &Node ) );

                // SanityCheck();
            }
    
            constexpr   bool                    Belongs         ( const void* pPtr )    const   noexcept { return m_Allocator.Belongs(pPtr);        }

            inline void SanityCheck( void )
            {
                xassert_block_basic()
                {
                    // Walk the link list to make sure everything is ok
                    real_node* pLocalReality = m_Head.load( std::memory_order_relaxed );
                    while( pLocalReality )
                    {
                        SanityCheck( pLocalReality );
                        pLocalReality = pLocalReality->m_Next;
                    }
                }
            }

        protected:
    
            struct alignas( xcore::target::getCacheLineSize() ) real_node 
            {
                std::atomic<real_node*>     m_Next      {};
                value_type                  m_Entry     {};
            };
    
            static constexpr std::int32_t real_node_offset_v = offsetof( real_node, m_Entry );

        protected:

            inline void SanityCheck( real_node* pLocalReality )
            {
                xassert_block_basic()
                {
                    const counter Count = m_Allocator.getCount() - 1;

                    xassert( pLocalReality >= &m_Allocator[0] );
                    xassert( pLocalReality <= &m_Allocator[Count] );
                    if( false )
                    {
                        int i=0;
                        for( ; i<=Count; i++ )
                        {
                            if( pLocalReality == &m_Allocator[i] ) break;
                        }
                        xassert(i<=Count);
                    }
                }
            }

        protected:
    
            alignas( xcore::target::getCacheLineSize() ) std::atomic<real_node*>    m_Head         { nullptr };
            allocator                                                               m_Allocator    {};
        };

        //------------------------------------------------------------------------------
        // Description:
        //      jitc    - ( Just In Time Construction/Destruction )
        //                  Means that when an entry is pop then we will constructed and push destruct it
        // Algorithm:
        //      Note the algorithm reuses the entry memory as a pointer to the next node.
        //------------------------------------------------------------------------------
        template< typename T, template<typename,typename> class T_ALLOCATOR, typename T_COUNTER = std::size_t >
        class mpmc_bounded_jitc_general 
        {
        public:

            using value_type  = T;
            using counter     = T_COUNTER;
            using allocator   = T_ALLOCATOR< T, counter >;

        public:

            constexpr mpmc_bounded_jitc_general( void ) noexcept { clear(); }
    
            ~mpmc_bounded_jitc_general( void ) noexcept
            {
                DestructAllocatedNodes();
            }

            inline void DestructAllocatedNodes( void ) noexcept
            {
                // Check
                const counter Count = m_Allocator.size();
                if( Count == 0 ) return;

                unique_span<bool>   Checks;

                // Alloc and clear the memory to mark what it has been freed so far
                auto Error = Checks.New( Count, false );
                xassert(Error == false);

                // Mark all nodes that have been free so far
                for(next* pNext = m_Head.load(std::memory_order_relaxed)
                     ; pNext
                     ; pNext = pNext->m_Next.load(std::memory_order_relaxed) )
                {
                    const auto Index = static_cast<int>( reinterpret_cast<value_type*>(pNext) - reinterpret_cast<value_type*>( &m_Allocator[0] ) );
            
                    // make sure there is not strange situations
                    xassert( Checks[ Index ] == false );

                    // mark the node as free
                    Checks[ Index ] = true;
                }

                // Destruct all nodes that have not been freed by the user
                // These nodes could be consider a leak....
                for( int i = 0; i < Count; i++ )
                    if( Checks[i] == false ) std::destroy_at(std::addressof(m_Allocator[i]));
            }


            inline void clear ( void ) noexcept
            {
                if( m_Allocator.size() ) 
                {
                    const counter Count = m_Allocator.size() - 1;
        
                    for( counter i = 0; i<Count; i++ )
                    {
                        reinterpret_cast<next*>(&m_Allocator[i])->m_Next.store( reinterpret_cast<next*>(&m_Allocator[i + 1]), std::memory_order_relaxed );
                    }
        
                    reinterpret_cast<next*>(&m_Allocator[Count])->m_Next.store( nullptr, std::memory_order_relaxed );
                    m_Head.store( reinterpret_cast<next*>(&m_Allocator[0]), std::memory_order_relaxed );
                }
            }

            inline value_type* popDontConstruct( void ) noexcept 
            {
                next* pLocalReality = m_Head.load( std::memory_order_relaxed );
                while( pLocalReality )
                {
                    if( m_Head.compare_exchange_weak( pLocalReality, pLocalReality->m_Next.load(std::memory_order_relaxed), std::memory_order_acquire, std::memory_order_relaxed ) )
                    {
                        value_type* pEntry = reinterpret_cast<value_type*>(pLocalReality);
                        return pEntry;
                    }
                }
                return nullptr;
            }
    
            template<typename ...T_ARG>
            inline value_type* pop ( T_ARG&&...Args ) noexcept 
            {
                next* pLocalReality = m_Head.load( std::memory_order_relaxed );
                while( pLocalReality )
                {
                    if( m_Head.compare_exchange_weak( pLocalReality, pLocalReality->m_Next.load(std::memory_order_relaxed), std::memory_order_acquire, std::memory_order_relaxed ) )
                    {
                        value_type* pEntry = reinterpret_cast<value_type*>(pLocalReality);
                        new(pEntry) value_type{ std::forward<T_ARG>(Args)... };
                        return pEntry;
                    }
                }
 
                return nullptr;
            }
    
            inline void pushDontDestruct ( value_type& Node ) noexcept 
            {
                next* pLocalReality   = m_Head.load( std::memory_order_relaxed );
                do
                {
                    reinterpret_cast<next*>(&Node)->m_Next.store( pLocalReality, std::memory_order_relaxed );
                } while( !m_Head.compare_exchange_weak( pLocalReality, reinterpret_cast<next*>(&Node), std::memory_order_release, std::memory_order_relaxed ) );
            }
    
            inline void push ( value_type& Node ) noexcept 
            {
                std::destroy_at(std::addressof(Node));
                pushDontDestruct( Node );
            }

            inline      value_type&             get             ( counter Index )                   noexcept { return m_Allocator[Index].m_Entry;       }
            inline      allocator&              getAllocator    ( void )                            noexcept { return m_Allocator;                      }
            constexpr   const allocator&        getAllocator    ( void )                    const   noexcept { return m_Allocator;                      }
            constexpr   bool                    Belongs         ( const void* pPtr )        const   noexcept { return m_Allocator.Belongs(pPtr);        }
            constexpr   int                     getIndexByEntry ( const value_type& Entry ) const   noexcept { return m_Allocator.getIndexByEntry( Entry); }
            template< typename T >
            constexpr   const value_type&       getEntryByIndex ( const T Index )           const   noexcept { return m_Allocator[Index]; }
            template< typename T >
            inline      value_type&             getEntryByIndex ( const T Index )                   noexcept { return m_Allocator[Index]; }

        protected:

            struct next;
            using  head  = std::atomic<next*>;
            struct next { head m_Next; };
     
            static_assert( alignof(value_type) >= alignof(next), "Please make sure that your entry has the alignment of an std::atomic<void*>" );
            static_assert( sizeof(value_type)  >= sizeof(next),  "Please make sure that the entry is byte size is larger than an atomic pointer" );
    
        protected:
    
            alignas( xcore::target::getCacheLineSize() )  head          m_Head         { nullptr };
            allocator                                                   m_Allocator    {};
        };
    }

    //------------------------------------------------------------------------------
    // Description:
    //      dynamic - The pool will be allocated in the heap, must Init to create the pool size
    //------------------------------------------------------------------------------
    template< typename T, typename T_COUNTER = std::size_t >
    class mpmc_bounded_dynamic : public details::mpmc_bounded_general< T, xcore::allocator::unique_span, T_COUNTER >
    {
    public:

        using parent        = details::mpmc_bounded_general< T, xcore::allocator::unique_span, T_COUNTER >;
        using self          = mpmc_bounded_dynamic;
        using value_type    = T;
    
    public:

        constexpr               mpmc_bounded_dynamic    ( void )                noexcept    = default;
        inline      xcore::err  Init                    ( std::size_t Count )   noexcept    { xcore::err Error; if( parent::m_Allocator.New( Count ).isError(Error) == false ) parent::clear(); return Error;  }
        inline      void        destroy                 ( void )                noexcept    { parent::m_Allocator.destroy();                        }
    };

    //------------------------------------------------------------------------------
    template< typename T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
    using mpmc_bounded = details::mpmc_bounded_general< T, xcore::types::container_static_to_dynamic< xcore::array, T_MAX_SIZE >::type, T_COUNTER >;

    //------------------------------------------------------------------------------
    // Description:
    //      dynamic - The pool will be allocated in the heap, must Init to create the pool size
    //      jitc    - ( Just In Time Construction/Destruction )
    //                  Means that when an entry is pop then we will constructed and push destruct it
    //------------------------------------------------------------------------------
    template< typename T, typename T_COUNTER >
    class mpmc_bounded_dynamic_jitc : public details::mpmc_bounded_jitc_general< T, xcore::allocator::raw::unique_span, T_COUNTER >
    {
    public:

        using parent        = details::mpmc_bounded_jitc_general< T, xcore::allocator::raw::unique_span, T_COUNTER >;
        using self          = mpmc_bounded_dynamic_jitc;
        using value_type    = T;
    
    public:

        constexpr               mpmc_bounded_dynamic_jitc       ( void )                noexcept   = default;
        inline      void        Init                            ( std::size_t Count )   noexcept   { parent::m_Allocator.Alloc( Count ); parent::Clear();              }
        inline      void        Kill                            ( void )                noexcept   { parent::DestructAllocatedNodes();   parent::m_Allocator.Free();   }
    };

    //------------------------------------------------------------------------------
    template< typename T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
    using mpmc_bounded_jitc = details::mpmc_bounded_jitc_general< T, xcore::types::container_static_to_dynamic< xcore::array, T_MAX_SIZE >::template type, T_COUNTER >;
}
#endif