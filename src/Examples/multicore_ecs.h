
namespace mecs
{
    //---------------------------------------------------------------------------------
    // TOOLS
    //---------------------------------------------------------------------------------
    namespace tools
    {
        //---------------------------------------------------------------------------------
        // TOOLS::map_fixed V2
        //---------------------------------------------------------------------------------
        template< typename T_KEY, typename T_VALUE, std::size_t T_MAXSIZE_V > 
        struct map_fixed
        {
            constexpr static auto max_size_v = T_MAXSIZE_V + (T_MAXSIZE_V/10); // Add an additional 10% of entries for collisions and such

            struct lock;

            struct entry
            {
                T_KEY               m_Key;
                T_VALUE             m_Value;       // TODO: These pointers below could be converted to indices to save memory
                entry*              m_pNext;       // Next for cases where collisions happen
                std::atomic<lock*>  m_pLock;       // Pointer back to its lock
            };
                
            struct lock
            {
                xcore::lock::spin   m_Lock;
                entry*              m_pEntry;
                entry               m_Entry;
            };

            map_fixed()
            {
                // Allocate the memory
                m_Map.Alloc(max_size_v).CheckError();

                // Initialize the memory as fast as possible
                xcore::scheduler::channel Channel( xconst_universal_str("map_fixed::constructor") );
                Channel.ForeachLog( m_Map, 8, 1000, [&]( xcore::span<lock> View )
                {
                    if constexpr ( std::is_trivially_constructible_v<T_VALUE> )
                    {
                        memset( View.data(), 0, View.size()*sizeof(lock) );
                    }
                    else
                    {
                        // Call constructors 
                        for( auto& E : View ) new(&E) lock{};
                    }
                });
                Channel.join();
            }

            void clear()
            {
                m_Map.clear();
            }

            template< typename T_CALLBACK >
            auto& alloc( T_KEY Key, T_CALLBACK&& Callback = [](entry&){} ) noexcept
            {
                assert(Key.isValid());

                auto            Index      = Key.m_Value % max_size_v;
                auto&           MapEntry   = m_Map[Index];
                entry*          pEntry     = &m_Map[Index].m_Entry;

                // Find/Alloc free memory
                do 
                {
                    auto  L = pEntry->m_pLock.load(std::memory_order_relaxed);
                    if( L )
                    {
                        Index   = (Index + 1)%max_size_v;
                        pEntry  = &m_Map[Index].m_Entry;
                        continue;
                    }
                    if( pEntry->m_pLock.compare_exchange_weak( L, &MapEntry ) ) break;

                } while (true);

                xcore::lock::scope Lk( MapEntry.m_Lock );

                // Do we have a collision? 
                // Make sure we have not collisions with any one
                xassert_block_basic()
                {
                    for( auto p = MapEntry.m_pEntry; p ; p = p->m_pNext )
                    {
                        xassert( Key.m_Value != p->m_Key.m_Value );
                    }
                }

                pEntry->m_Key     = Key;
                pEntry->m_pNext   = MapEntry.m_pEntry;
                MapEntry.m_pEntry = pEntry;

                // Setup the node
                Callback(*pEntry);           // should set ( pNode->m_Value = ... )

                return *pEntry;
            }

            template< typename T > xforceinline
            bool find( T_KEY Key, T&& CallBack = [](entry&){} ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope Lk( MapEntry.m_Lock );

                for( auto p = MapEntry.m_pEntry; p; p = p->m_pNext )
                {
                    if( p->m_Key == Key ) 
                    {
                        CallBack( *p );
                        return true;
                    }
                }

                return false;
            }

            template< typename T > xforceinline
            bool find( T_KEY Key, T&& CallBack = [](const entry&){} ) const noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope Lk( MapEntry.m_Lock );

                for( auto p = MapEntry.m_pEntry; p; p = p->m_pNext )
                {
                    if( p->m_Key == Key ) 
                    {
                        CallBack( *p );
                        return true;
                    }
                }

                return false;
            }

            template< typename T_CREATE_CALLBACK > xforceinline
            T_VALUE& getOrCreate( T_KEY Key, T_CREATE_CALLBACK&& CreateCallBack = []()->T_VALUE{} ) noexcept
            {
                T_VALUE* pV = nullptr;
                if( find(Key,[&](entry& E){ pV = &E.m_Value; }) )
                {
                }
                else alloc(Key,[&](entry& E){ E.m_Value = std::move(CreateCallBack()); pV = &E.m_Value; });
                return *pV;
            }

            template< typename T_CALLBACK > xforceinline
            bool getExlusive( T_KEY Key, T_CALLBACK&& CallBack = [](T_VALUE&){} ) noexcept
            {
                return find(Key,[&](entry& E){ CallBack(E.m_Value); });
            }

            xforceinline
            T_VALUE get( T_KEY Key ) noexcept
            {
                T_VALUE Val;
                bool b = find(Key,[&](entry& E){ Val = E.m_Value; });
                xassert(b);
                return Val;
            }

            xforceinline
            T_VALUE get( T_KEY Key ) const noexcept
            {
                T_VALUE Val;
                bool b = find(Key,[&](entry& E){ Val = E.m_Value; });
                xassert(b);
                return Val;
            }

            template< typename T >  xforceinline
            const bool free( T_KEY Key, T&& CallBack = [](entry&){} ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope Lk( MapEntry.m_Lock );

                xassert( MapEntry.m_pEntry != nullptr );
                for( auto p = &MapEntry.m_pEntry; *p; p = &(*p)->m_pNext )
                {
                    auto& E = **p;
                    if( E.m_Key == Key ) 
                    {
                        CallBack( E );
                        *p = E.m_pNext;
                        E.m_pLock.store( nullptr, std::memory_order_release );
                        return true;
                    }
                }

                return false;
            }

            xcore::unique_span<lock>    m_Map{};
        };

        //---------------------------------------------------------------------------------
        // TOOLS::PAGE
        //---------------------------------------------------------------------------------
        struct alignas(128) page
        {
            std::byte    m_RawData[1024*16];
        };

        //---------------------------------------------------------------------------------
        // TOOLS::PAGE MANAGER
        //---------------------------------------------------------------------------------
        struct page_mgr
        {
            struct next
            {
                next* m_pNext;
            };

            page_mgr( void ) noexcept
            {
                XCORE_PERF_PLOT_CONFIG("Total Pages", tracy::PlotFormatType::Number )
                XCORE_PERF_PLOT_CONFIG("Free Pages",  tracy::PlotFormatType::Number )
            }

            ~page_mgr( void ) noexcept
            {
                clear();
                XCORE_PERF_PLOT("Total Pages", static_cast<int64_t>(m_TotalPages.load(std::memory_order_relaxed)) )
                XCORE_PERF_PLOT("Free Pages",  static_cast<int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
            }

            void clear( void ) noexcept
            {
                std::uint32_t i=0;
                auto p =reinterpret_cast<next*>(m_pFreeHead.load(std::memory_order_relaxed));
                while( p )
                {
                    auto n = p->m_pNext;
                    delete p;
                    p = n;
                    i++;
                }
                m_pFreeHead.store(nullptr, std::memory_order_relaxed);
                m_TotalPages -= i;
                m_FreePages  -= i;
            }

            xforceinline
            page* AllocPage( void ) noexcept
            {
                next* pLocal = m_pFreeHead.load(std::memory_order_relaxed);
                do 
                {
                    if(pLocal == nullptr)
                    {
                        m_TotalPages++;
                        XCORE_PERF_PLOT("Total Pages", static_cast<int64_t>(m_TotalPages.load(std::memory_order_relaxed)) )
                        return new page;
                    }
                } while( m_pFreeHead.compare_exchange_weak( pLocal, pLocal->m_pNext) == false );

                m_FreePages--;
                XCORE_PERF_PLOT("Free Pages",  static_cast<int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
                return reinterpret_cast<page*>(pLocal);
            }

            xforceinline
            void FreePage(page* pPage) noexcept
            {
                next* p      = reinterpret_cast<next*>(pPage);
                p->m_pNext   = m_pFreeHead.load(std::memory_order_relaxed);
                while (m_pFreeHead.compare_exchange_weak(p->m_pNext, p) == false);
                m_FreePages++;
                XCORE_PERF_PLOT("Free Pages",  static_cast<std::int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
            }

            void PreallocatePages( int nPages ) noexcept
            {
                for( int i=0; i<nPages; ++i ) FreePage( new page );
                XCORE_PERF_PLOT("Total Pages", static_cast<int64_t>(m_TotalPages.load(std::memory_order_relaxed)) )
                XCORE_PERF_PLOT("Free Pages",  static_cast<int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
            }

            std::atomic<next*>          m_pFreeHead     { nullptr };
            std::atomic<std::uint32_t>  m_TotalPages    { 0 };
            std::atomic<std::uint32_t>  m_FreePages     { 0 };
        };

        //---------------------------------------------------------------------------------
        // TOOLS::PAGE PAGE MANAGER
        //---------------------------------------------------------------------------------
        /*
        template< std::size_t T_MAX_PAGES_V, std::size_t T_CUSTOM_PAGE_SIZE >
        struct page_page_mgr
        {
            union custom_page
            {
                custom_page*    m_pNext;
                std::byte       m_RawData[T_CUSTOM_PAGE_SIZE];
            };

            constexpr static std::size_t custom_pages_per_page_v = size(page) / sizeof(custom_page);

            struct page_info
            {
                page*          m_pPage      { nullptr };
                custom_page*   m_pFreeHead  { nullptr };
                std::uint16_t  m_nFree      { 0 };
            };

            page_mgr&                                   m_PageMgr;
            std::vector<page_info>                      m_lPages    {};
            xcore::lock::spin                           m_Lock      {};
            int                                         m_AllocIndex{-1}

            custom_page* AllocCustomPage( void ) noexcept
            {
                xcore::lock::scope Lk(m_Lock);

                if( m_AllocIndex < 0 )
                {
                    m_AllocIndex = 0;

                    auto pPage = m_PageMgr.AllocPage();
                    auto pCustomPage = reinterpret_cast<custom_page*>(pPage);
                    auto& Entry = m_lPages.push_front{ { pPage, pCustomPage+1, custom_pages_per_page_v-1 } };

                    // Link list all the entries
                    for( int i=1; i<(custom_pages_per_page_v-2); i++ )
                    {
                        pCustomPage[i].m_pNext = &pCustomPage[i+1];
                    }
                    pCustomPage[custom_pages_per_page_v-1].m_pNext = nullptr;

                    return pCustomPage;
                }
                else
                {
                    xassert(m_lPages[m_AllocIndex].m_nFree);
                    m_lPages[m_AllocIndex].m_nFree--;
                    auto pCustomPage = m_lPages[m_AllocIndex].m_pFreeHead;
                    m_lPages[m_AllocIndex].m_pFreeHead = pCustomPage->m_pNext;
                    if( m_lPages[m_AllocIndex].m_nFree == 0 ) m_AllocIndex--;
                    else
                    {
                        if( m_AllocIndex )
                        {
                            // Swap the priorities
                            if( m_lPages[m_AllocIndex].m_nFree > m_lPages[m_AllocIndex-1].m_nFree )
                            {
                                swap( m_lPages[m_AllocIndex], m_lPages[m_AllocIndex-1] );
                                m_AllocIndex--;
                            }
                        }
                    }
                    return pCustomPage;
                }
            }

            custom_page* FreeCustomPage( custom_page* pCustomPage ) noexcept
            {
                xcore::lock::scope Lk(m_Lock);

                
                for( int i=0; i<m_lPages.size(); ++i )
                {
                    if( static_cast<std::size_t>(pCustomPage) >= static_cast<std::size_t>(m_lPages[i].m_pPage) &&
                        static_cast<std::size_t>(pCustomPage) <  static_cast<std::size_t>(&m_lPages[i].m_pPage->m_RawData[size(custom_page)]) )
                    {
                        pCustomPage->m_pNext = m_lPages[i].m_pFreeHead;
                        m_lPages[i].m_pFreeHead; = pCustomPage;
                        m_lPages[i].m_nFree++;

                        if( i > 0 && m_lPages[i-1].m_nFree < m_lPages[i].m_nFree )
                        {
                            swap( m_lPages[m_AllocIndex], m_lPages[m_AllocIndex+1] );
                            if( m_AllocIndex == i ) 
                            {
                                m_AllocIndex--;
                                if( m_lPages[m_AllocIndex].m_nFree == custom_pages_per_page_v )
                            }
                        }
                    }
                }
            }

            xforceinline
            void FreePage(page* pPage) noexcept
            {
                next* p      = reinterpret_cast<next*>(pPage);
                p->m_pNext   = m_pFreeHead.load(std::memory_order_relaxed);
                while (m_pFreeHead.compare_exchange_weak(p->m_pNext, p) == false);
                m_FreePages++;
                XCORE_PERF_PLOT("Free Pages",  static_cast<std::int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
            }

            void PreallocatePages( int nPages ) noexcept
            {
                for( int i=0; i<nPages; ++i ) FreePage( new page );
                XCORE_PERF_PLOT("Total Pages", static_cast<int64_t>(m_TotalPages.load(std::memory_order_relaxed)) )
                XCORE_PERF_PLOT("Free Pages",  static_cast<int64_t>(m_FreePages.load(std::memory_order_relaxed)) )
            }
        };

        template< typename T, std::size_t T_MAX_PAGE_COUNT_V,typename T_PAGE_PAGE_MGR >
        struct vector_pool
        {
            using page_page_mgr_t = T_PAGE_PAGE_MGR;
            using page            = T_PAGE_PAGE_MGR::custom_page;

            constexpr static std::size_t  max_page_count_v   = T_MAX_PAGE_COUNT_V;
            constexpr static std::size_t  entries_per_page_v = sizeof(page)/sizeof(T);

            template< typename...T_ARGS >
            T* AllocEntry( T_ARGS&&...Args ) noexcept
            {
                auto Count = m_Count.load(std::memory_order_relaxed);
                do 
                {
                    if( Count.m_bLock ) 
                    {
                        while( Count.m_bLock ) 
                        {
                            // spin!!
                            Count = m_Count.load(std::memory_order_relaxed);
                        }
                    }

                    if( m_nPages * entries_per_page_v == Count.m_Count )
                    {
                        if( false == m_Count.compare_exchange_weak( Count, { Count.m_Count, 1} ) continue;
                        m_lPages[m_nPages++] = m_PageMgr.AllocPage();
                        m_Count.store( { Count.m_Count+1, 0}, std::memory_order_relaxed );
                    }
                    else
                    {
                        if( false == m_Count.compare_exchange_weak( Count, { Count.m_Count + 1, 0} ) continue;
                    }

                    break;

                } while (true);
                
                Count.m_Count++;
                const auto iPage  = Count.m_Count / entries_per_page_v;
                const auto iIndex = Count.m_Count % entries_per_page_v;

                return new( &m_lPages[iPage].m_RawData[iIndex] ) T{ std::forward<T_ARGS>(Args) };
            }

            void DeleteEntry( std::uint32_t Entry ) noexcept
            {
                auto Count = m_Count.load(std::memory_order_relaxed);
                do 
                {
                    if( Count.m_bLock ) 
                    {
                        while( Count.m_bLock ) 
                        {
                            // spin!!
                            Count = m_Count.load(std::memory_order_relaxed);
                        }
                    }

                    // always lock on destruction
                    if( false == m_Count.compare_exchange_weak( Count, { Count.m_Count, 1} ) continue;

                    const auto NewCount = Count.m_Count - 1;

                    if( m_nPages * entries_per_page_v > NewCount )
                    {
                        if constexpr ( false == std::is_trivially_destructible_v )
                            std::destroy_at( reinterpret_cast<T*>(&m_lPages[--m_nPages].m_RawData[0]) );
                        m_PageMgr.FreePage(m_lPages[--m_nPages]);
                    }
                    else
                    {
                        if constexpr ( false == std::is_trivially_destructible_v )
                            std::destroy_at( reinterpret_cast<T*>(&m_lPages[NewCount/entries_per_page_v].m_RawData[NewCount%entries_per_page_v]) );
                    }

                    m_Count.store( { NewCount, 0}, std::memory_order_relaxed );
                    break;

                } while (true);
            }

            void DeleteAllEntries( void ) noexcept
            {
                auto Count = m_Count.load(std::memory_order_relaxed);
                do 
                {
                    if( Count.m_bLock ) 
                    {
                        while( Count.m_bLock ) 
                        {
                            // spin!!
                            Count = m_Count.load(std::memory_order_relaxed);
                        }
                    }

                    // always lock on destruction
                    if( false == m_Count.compare_exchange_weak( Count, { Count.m_Count, 1} ) continue;

                    if constexpr ( false == std::is_trivially_destructible_v )
                    {
                        int         iPage = 0;
                        const auto  Count = Count.m_Count;
                        for( std::uint32_t I = entries_per_page_v; I<Count; I+=entries_per_page_v, ++iPage )
                        {
                            std::destroy( &reinterpret_cast<T*>(&m_lPages[iPage].m_RawData[0]),
                                          &reinterpret_cast<T*>(&m_lPages[iPage].m_RawData[entries_per_page_v] );
                        }
                        std::destroy( &reinterpret_cast<T*>(&m_lPages[iPage].m_RawData[0]),
                                      &reinterpret_cast<T*>(&m_lPages[iPage].m_RawData[Count%entries_per_page_v] );
                    }

                    m_Count.store( { 0, 0}, std::memory_order_relaxed );
                    break;

                } while (true);
            }

            struct count
            {
                std::uint32_t           m_Count:31,
                                        m_Lock:1;
            };

            std::uint16_t                           m_nPages        { 0 };
            std::array< page*, max_page_count_v >   m_lPages;
            std::atomic<count>                      m_Count         { 0,0 };
            T_PAGE_PAGE_MGR&                        m_PagePageMgr;
        };
        */

        //---------------------------------------------------------------------------------
        // TOOLS::POOL
        //---------------------------------------------------------------------------------
        template< typename T, std::size_t MAX_PAGES >
        struct pool;

        namespace details
        {
            template< typename T, std::size_t MAX_PAGES >
            struct pool_iterator
            {
                using pool_t = pool<T,MAX_PAGES>;

                pool_t*             m_pPool;
                std::uint32_t       m_nEntries;
                std::uint16_t       m_nPages;
                std::uint16_t       m_Index;
                pool_iterator   operator    ++  (void)                   noexcept;
                bool            operator    !=  (std::nullptr_t) const   noexcept;
                T&              operator    *   (void)                   noexcept;
            };
        }
        
        template< typename T, std::size_t MAX_PAGES >
        struct pool
        {
            constexpr static auto entries_per_page_v = sizeof(page)/sizeof(T);

            struct alignas(std::uint64_t) counts
            {
                std::uint32_t   m_nEntries;
                std::uint16_t   m_nPages;
                std::uint16_t   m_bLock;
            };

            constexpr pool( page_mgr& PageMgr ) noexcept : m_PageMgr{PageMgr}{}

            std::uint32_t                           size() const noexcept{ return m_Counts.load( std::memory_order_relaxed).m_nEntries; }
            details::pool_iterator<T,MAX_PAGES>     begin() noexcept{ counts Counts = m_Counts.load( std::memory_order_relaxed); return { this, Counts.m_nEntries, 0, 0 }; }
            std::nullptr_t                          end() const noexcept { return nullptr; }

            template< typename...T_ARGS >
            xforceinline T& append( T_ARGS&&... Args ) noexcept
            {
                auto Counts    = m_Counts.load( std::memory_order_relaxed);
                do 
                {
                    if( Counts.m_bLock )
                    {
                        Counts = m_Counts.load( std::memory_order_relaxed );
                    }
                    else if( Counts.m_nEntries < Counts.m_nPages*entries_per_page_v )
                    {
                        const counts NewCounts{ Counts.m_nEntries+1, Counts.m_nPages, 0 }; 
                        
                        if( m_Counts.compare_exchange_weak( Counts, NewCounts ) )
                            return *new( &m_lPages[Counts.m_nPages-1]->m_RawData[(Counts.m_nEntries%entries_per_page_v)*sizeof(T)] ) 
                                        T{std::forward<T_ARGS>(Args)...};
                    }
                    else if( Counts.m_nEntries == Counts.m_nPages*entries_per_page_v )
                    {
                        counts NewCounts{ Counts.m_nEntries+1, static_cast<std::uint16_t>(Counts.m_nPages+1), 1u }; 
                        if( m_Counts.compare_exchange_weak( Counts, NewCounts ) )
                        {
                            auto p = m_PageMgr.AllocPage();
                            m_lPages[Counts.m_nPages] = p;
                            auto& E = *new( &p->m_RawData[(Counts.m_nEntries%entries_per_page_v)*sizeof(T)] ) 
                                        T{std::forward<T_ARGS>(Args)...};

                            // Release the lock
                            NewCounts.m_bLock = 0;
                            m_Counts.store( NewCounts );

                            return E;
                        }
                    }

                    // Spin Lock...
                } while (true);

                xassert(false);
            }

            void clear( void ) noexcept
            {
                counts Counts    = m_Counts.load( std::memory_order_relaxed);
                do 
                {
                    if( Counts.m_bLock )
                    {
                        Counts = m_Counts.load( std::memory_order_relaxed );
                    }
                    else if( m_Counts.compare_exchange_weak( Counts, counts{ Counts.m_nEntries, Counts.m_nPages, 1 } ) )
                    {
                        if constexpr (false == std::is_trivially_destructible_v<T>)
                        {
                            for( std::uint32_t i=0; i<Counts.m_nEntries; ++i )
                            {
                                std::destroy_at
                                (
                                    reinterpret_cast<T*>(&m_lPages[i/entries_per_page_v]->m_RawData[i%entries_per_page_v])
                                );
                            }                        
                        }

                        for( int i=0; i<Counts.m_nPages; ++i )
                        {
                            m_PageMgr.FreePage(m_lPages[i]);
                        }

                        m_Counts.store( counts{ 0, 0, 0 } );
                        break;
                    }

                    // Spin Lock...
                } while (true);
            }

            std::atomic<counts>             m_Counts    {counts{0,0,0}};
            std::array<page*, MAX_PAGES>    m_lPages    {};
            page_mgr&                       m_PageMgr;
        };

        namespace details
        {
            template< typename T, std::size_t MAX_PAGES >
            xforceinline pool_iterator<T,MAX_PAGES> pool_iterator<T,MAX_PAGES>::operator++ (void) noexcept
            {
                m_nEntries--;
                m_Index++;
                if (m_Index == pool_t::entries_per_page_v)
                {
                    m_nPages++;
                    m_Index = 0;
                    _mm_prefetch((const char*)m_pPool->m_lPages[m_nPages], _MM_HINT_NTA);
                }
                return *this;
            }

            template< typename T, std::size_t MAX_PAGES >
            xforceinline bool pool_iterator<T,MAX_PAGES>::operator !=  (std::nullptr_t) const noexcept
            {
                return m_nEntries;
            }

            template< typename T, std::size_t MAX_PAGES >
            xforceinline T& pool_iterator<T,MAX_PAGES>::operator * (void) noexcept
            {
                return reinterpret_cast<T*>(m_pPool->m_lPages[m_nPages])[m_Index];
            }
        }

        //---------------------------------------------------------------------------------
        // TOOLS::COMMAND POOL
        //---------------------------------------------------------------------------------
        template< typename T_CMD_VARIANT >
        struct cmd_pool
        {
            struct cmd
            {
                T_CMD_VARIANT   m_Cmd;
                cmd*            m_pNext;
            };

            cmd_pool( page_mgr& PageMgr ) : m_PageMgr{PageMgr} 
            {
            }

            template< typename T, typename...T_ARGS >
            T& NewAlloc( T_ARGS...Args ) noexcept
            {
                auto    Position    = m_Position.load(std::memory_order_relaxed);
            START:
                
                // Check if we are locked... 
                while( Position < sizeof(page*) ) 
                {
                    // If one means we have not initialize... so we begin the race of whom will initialize it
                    if( Position == 1 )
                    {
                        // Try to win the right to do it
                        if( m_Position.compare_exchange_weak( Position, 2 ) )
                        {
                            m_pPageHead = m_PageMgr.AllocPage();
                            reinterpret_cast<page*&>(*m_pPageHead) = nullptr;
                            Position    = sizeof(page*);
                            m_Position.store( Position );
                            break;
                        }
                    }

                    // See if someone has unlock us
                    Position = m_Position.load(std::memory_order_relaxed);
                }

                auto* p = xcore::bits::Align( &m_pPageHead->m_RawData[Position], std::alignment_of_v<T> );
                if( (p + sizeof(T)) > &m_pPageHead->m_RawData[sizeof(page)] )
                {
                    // Lock the command pool
                    if( m_Position.compare_exchange_weak( Position, 0 ) == false ) goto START;

                    // Need new page
                    auto  pNewPage = m_PageMgr.AllocPage();
                    auto& pNewNext = reinterpret_cast<page*&>(pNewPage->m_RawData);
                    pNewNext    = m_pPageHead;
                    m_pPageHead = pNewPage;

                    // Allocate our entry
                    p                = xcore::bits::Align( &pNewPage->m_RawData[sizeof(page*)], std::alignment_of_v<T> );
                    auto NewPosition = xcore::types::static_cast_safe<std::uint32_t>( p - m_pPageHead->m_RawData ) + static_cast<std::uint32_t>(sizeof(T));

                    // Release the lock and set the new position
                    m_Position.store( NewPosition );
                }
                else
                {
                    auto NewPosition  = xcore::types::static_cast_safe<std::uint32_t>( p - m_pPageHead->m_RawData ) + static_cast<std::uint32_t>(sizeof(T));
                    if( m_Position.compare_exchange_weak( Position, NewPosition ) == false ) goto START;
                }

                return *new(p) T{ std::forward<T_ARGS>(Args)... };
            }

            template< typename T, typename...T_ARGS >
            T& NewCmd( T_ARGS...Args ) noexcept
            {
                auto& Cmd       = NewAlloc<cmd>();
                auto& Requested = Cmd.m_Cmd.emplace<T>( std::forward<T_ARGS>(Args)... );
                Cmd.m_pNext = m_pCmdHead.load( std::memory_order_relaxed );
                while( false == m_pCmdHead.compare_exchange_weak( Cmd.m_pNext, &Cmd ) );
                return Requested;
            }

            void clear( void )
            {
                // We keep one page and return the rest 
                if(m_pPageHead) for( auto p = reinterpret_cast<page**>(m_pPageHead->m_RawData); *p; p = &(*p) )
                {
                    m_PageMgr.FreePage(*p);
                }

                m_pCmdHead.store(nullptr,std::memory_order_relaxed);
                m_Position.store(sizeof(page*),std::memory_order_relaxed);
            }

            page_mgr&                           m_PageMgr;
            page*                               m_pPageHead     { nullptr };
            std::atomic<cmd*>                   m_pCmdHead      { nullptr };
            std::atomic<std::uint32_t>          m_Position      {1};                // 1 means that needs to be initialize
        };
    }

    //---------------------------------------------------------------------------------
    // SETTINGS
    //---------------------------------------------------------------------------------
    namespace settings
    {
        constexpr static std::size_t max_systems_v              = 256;
        constexpr static std::size_t max_components_in_entity_v = 64;
        constexpr static std::size_t max_tags_in_entity_v       = 64;
        constexpr static float       fixed_deltatime_size_v     = 16/1000.0f;

    }

    //---------------------------------------------------------------------------------
    // COMPONENT
    //---------------------------------------------------------------------------------
    namespace component
    {
        namespace group
        {
            struct instance;
            struct local_index  : xcore::units::type<local_index,   std::uint16_t> {using type::type;};
            struct page_index   : xcore::units::type<page_index,    std::uint16_t> {using type::type;};
            struct global_index : xcore::units::type<global_index,  std::uint32_t> {using type::type;};
        }
    }

    using type_guid = xcore::guid::unit<64, struct component_type_guid_tag >;

    namespace world { struct instance; }

    //---------------------------------------------------------------------------------
    // COMPONENT
    //---------------------------------------------------------------------------------
    namespace component
    {
        enum class write_type : std::uint8_t
        {
                READ_ONLY
            ,   LINEAR_WRITE
            ,   QUANTUM_WRITE
        };

        struct shared           {};
        struct quantum_mutable  {};
        struct double_buffer    {};
        struct tag              {};
        
        struct descriptor
        {
            using fn_construct  = void(void*) noexcept;
            using fn_destruct   = void(void*) noexcept;
            using fn_move       = void(void*dst,void*src) noexcept;

            const type_guid         m_Guid;
            fn_construct* const     m_fnConstruct;
            fn_construct* const     m_fnDestruct;
            fn_move*      const     m_fnMove;  
            //property::table&  m_PropertyTable;
            const std::uint16_t     m_Size;
            const std::uint16_t     m_Aligment;
            const bool              m_isDoubleBuffer;
            mutable int             m_BitNumber;
        };
    }

    //---------------------------------------------------------------------------------
    // ENTITY
    //---------------------------------------------------------------------------------
    namespace entity
    {
        struct tmp_ref
        {
            constexpr bool isValid(void) const noexcept { return m_PageIndex.m_Value == 0xffff; }

            template< typename T >
            T& getComponent(type_guid TypeGuid = T::type_guid_v) noexcept;

            template< typename T >
            constexpr const T& getComponent(type_guid TypeGuid = T::type_guid_v) const noexcept;

            void clear( void )
            {
                m_PageIndex.m_Value = 0xffff;
            }

            component::group::instance*        m_pGroupInstance;
            component::group::local_index      m_LocalIndex;
            component::group::page_index       m_PageIndex;
        };

        constexpr static std::size_t                                max_entity_count_v = xcore::bits::RoundToNextPowOfTwo(100000000ull)
                                                                    XCORE_CMD_ASSERT(  - xcore::bits::RoundToNextPowOfTwo(100000000ull)
                                                                                       + xcore::bits::RoundToNextPowOfTwo(  1000000ull));
        using guid      = xcore::guid::unit<64, struct ecs_entity_tag>;
        using map_t     = tools::map_fixed< guid, tmp_ref, max_entity_count_v >;

        struct instance
        {
            constexpr static type_guid type_guid_v{1ull};
            map_t::entry*    m_pInstance;

            inline bool     isZombie    ( void ) const noexcept{ return reinterpret_cast<std::size_t>(m_pInstance)&1; }
            tmp_ref         getTmpRef   ( void ) const noexcept{ return getPointer().m_Value; }
            map_t::entry&   getPointer  ( void ) const noexcept{ return *reinterpret_cast<map_t::entry*>(reinterpret_cast<std::size_t>(m_pInstance) & (~std::size_t(1))); }
        };
    }

    //---------------------------------------------------------------------------------
    // SYNC_POINT - A sync point establish a moment in time where systems can be run. You can
    //              also build complex graphs this way.
    //---------------------------------------------------------------------------------
    namespace sync_point
    {
        struct instance;

        struct events
        {
            using done  = xcore::types::make_unique< xcore::event::type<sync_point::instance&>, struct done_tag >;
            using start = xcore::types::make_unique< xcore::event::type<sync_point::instance&>, struct start_tag >;

            start m_Start;
            done  m_Done;
        };

        struct instance : xcore::scheduler::trigger<settings::max_systems_v>
        {
            events          m_Events;
            bool            m_bDisable = false;
            int             m_FreethCount;
            std::uint32_t   m_Tick{ 0 };

            void FreezCount() noexcept
            {
                m_FreethCount = m_NotificationCounter.load();
            }

            virtual void qt_onTriggered ( void ) noexcept override
            {
                m_NotificationCounter = m_FreethCount;
                m_Events.m_Done.NotifyAll(*this);
                m_Tick++;
                if(m_bDisable == false)
                {
                    m_Events.m_Start.NotifyAll( *this );
                    xcore::scheduler::trigger<settings::max_systems_v>::qt_onTriggered();
                }
            }

            ~instance()
            {
                m_NotificationCounter = 0;
            }

        };
    }

    //---------------------------------------------------------------------------------
    // GROUP
    //---------------------------------------------------------------------------------
    namespace component::group
    {
        using guid = xcore::guid::unit<64, struct ecs_group_tag>;

        //---------------------------------------------------------------------------------
        // GROUP::DETAILS
        //---------------------------------------------------------------------------------
        namespace details
        {
            //---------------------------------------------------------------------------------

            template< typename...T_COMPONENTS >
            constexpr auto MakeArrayTypeGuids( std::tuple<T_COMPONENTS...>* ) noexcept
            {
                return std::array{ T_COMPONENTS::type_guid_v ... };
            }  

            //---------------------------------------------------------------------------------

            template<>
            constexpr auto MakeArrayTypeGuids( std::tuple<>* ) noexcept
            {
                return std::span<type_guid>{ nullptr, std::size_t{0u} };
            }  

            //---------------------------------------------------------------------------------

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  true == std::is_base_of_v<mecs::component::double_buffer,         T_COMPONENT>
                , std::tuple<T_COMPONENT, T_COMPONENT>
            > FilterOutTagsExtendDoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                   false == std::is_base_of_v<mecs::component::double_buffer,       T_COMPONENT>
                && false == std::is_base_of_v<mecs::component::tag,                 T_COMPONENT>
                , std::tuple<T_COMPONENT>
            > FilterOutTagsExtendDoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                   false == std::is_base_of_v<mecs::component::double_buffer,       T_COMPONENT>
                && true  == std::is_base_of_v<mecs::component::tag,                 T_COMPONENT>
                , std::tuple<>
            > FilterOutTagsExtendDoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  true == std::is_base_of_v<mecs::component::tag,   T_COMPONENT>
                , std::tuple<T_COMPONENT>
            > FilterOutComponents();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  false == std::is_base_of_v<mecs::component::tag,  T_COMPONENT>
                , std::tuple<>
            > FilterOutComponents();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  false == std::is_base_of_v<mecs::component::tag,   T_COMPONENT>
                , std::tuple<T_COMPONENT>
            > FilterOutTags();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  true == std::is_base_of_v<mecs::component::tag,  T_COMPONENT>
                , std::tuple<>
            > FilterOutTags();

            //---------------------------------------------------------------------------------

            template< typename T_A, typename T_B >
            struct compare
            {
                constexpr static bool value = std::decay_t<T_A>::type_guid_v.m_Value
                                            < std::decay_t<T_B>::type_guid_v.m_Value;
            };

            //---------------------------------------------------------------------------------

            template< typename...T_COMPONENTS_AND_TAGS >
            struct guid_arrays_from_types
            {
                constexpr static auto component_guid_list_v = MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            compare, xcore::types::tuple_cat_t
                            < 
                                decltype(FilterOutTags<std::decay_t<T_COMPONENTS_AND_TAGS>>())...
                            >
                        >*
                    >(nullptr)
                );

                constexpr static auto tag_guid_list_v = MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            compare, xcore::types::tuple_cat_t
                            <
                                decltype(FilterOutComponents<std::decay_t<T_COMPONENTS_AND_TAGS>>())...
                            >
                        >*
                    >(nullptr)
                );
            };

            //---------------------------------------------------------------------------------

            template< typename...T_COMPONENTS_AND_TAGS >
            struct guid_arrays_from_types_with_duplication
            {
                constexpr static auto component_guid_list_v = component::group::details::MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            compare, xcore::types::tuple_cat_t
                            < 
                                decltype(FilterOutTagsExtendDoubleBuffer<T_COMPONENTS_AND_TAGS>())...
                            >
                        >*
                    >(nullptr)
                );

                constexpr static auto tag_guid_list_v = component::group::details::MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            compare, xcore::types::tuple_cat_t
                            <
                                decltype(FilterOutComponents<T_COMPONENTS_AND_TAGS>())...
                            >
                        >*
                    >(nullptr)
                );
            };
        }

        //---------------------------------------------------------------------------------
        // GROUP::ITERATOR
        //---------------------------------------------------------------------------------
        struct iterator : entity::tmp_ref
        {
            inline              iterator        operator++  (void)                      noexcept;
            inline constexpr    bool            operator!=  (std::nullptr_t)    const   noexcept;
            inline constexpr    entity::tmp_ref operator*   (void)              const   noexcept { return *this; }
            inline              entity::tmp_ref operator*   (void)                      noexcept { return *this; }
        };

        //---------------------------------------------------------------------------------
        // GROUP::INSTANCE::DETAIL
        //---------------------------------------------------------------------------------
        namespace details
        {
            struct offsets
            {
                std::uint16_t   m_Offset;       // Offset   - Where in the page is this component
                std::uint16_t   m_Size :14      // Size     - Size of the component
                              , m_T0   :1       // T0       - Which of the double buffer is T0 [0,1]
                              , m_HasT1:1;      // HasT1    - Does this component has a double buffer?
            };

            struct delete_entity
            {
                component::group::local_index      m_LocalIndex;
                component::group::page_index       m_PageIndex;
            };

            struct command_list
            {
                using commands = std::variant
                <
                    delete_entity
                >;


                inline void clear( void ) noexcept { m_lCmds.clear(); }

                command_list(tools::page_mgr& Mgr) : m_lCmds{Mgr} {}

                tools::cmd_pool<commands>   m_lCmds;
            };
        }

        //---------------------------------------------------------------------------------
        // GROUP::DEPENDENCY
        //---------------------------------------------------------------------------------
        template< auto& T_GROUP_GUID_V, typename...T_COMPONENTS >
        struct dependency
        {
            constexpr static type_guid  m_gGroup;
            constexpr static std::array m_ComponentsTypes{ T_COMPONENTS::type_guid_v };
        };

        //---------------------------------------------------------------------------------
        // GROUP::DESCRIPTOR
        //---------------------------------------------------------------------------------
        struct descriptor
        {
            constexpr descriptor( xcore::span<const component::descriptor*> lComponetDesc ) noexcept
                : m_Guid                    { [&]() constexpr { std::uint64_t K = entity::instance::type_guid_v.m_Value; for( const auto p:lComponetDesc) K += p->m_Guid.m_Value; return type_guid{K};      }() }
                , m_lComponentTypeGuids     { [&]() constexpr { std::array<type_guid,64>                    D{}; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i]->m_Guid;  return D; }() }
                , m_lComponentDescriptors   { [&]() constexpr { std::array<const component::descriptor*,64> D{}; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i];          return D; }() }
                , m_nComponents             { static_cast<std::uint16_t>(lComponetDesc.size()) }
                , m_EntriesPerPage          { [&]() constexpr
                                                {
                                                    const std::uint32_t     SizeOfOne           = [&]() constexpr { std::uint32_t S = 0; for (auto p : lComponetDesc) S += p->m_Size; return S; }();
                                                    const std::uint32_t     OriginalAddress     = std::alignment_of_v<tools::page>;
                                                    std::uint32_t           WastedToAligment    = 0;
                                                    std::uint32_t           p                   = OriginalAddress;

                                                    for( auto pE : lComponetDesc )
                                                    {
                                                        WastedToAligment += xcore::bits::Align(p, pE->m_Aligment) - p;
                                                        p += pE->m_Size;
                                                    }

                                                    return static_cast<std::uint16_t>((sizeof(tools::page) - WastedToAligment) / SizeOfOne);
                                                }()
                                            }
                , m_lComponentOffsets       { [&]() constexpr 
                                                {
                                                    std::array<details::offsets, 64>      lComponentOffsets{};
                                                    const std::uint32_t                   OriginalAddress     = std::alignment_of_v<tools::page>;
                                                    std::uint32_t                         p                   = OriginalAddress;

                                                    for (int i = 0; i < lComponetDesc.size(); i++)
                                                    {
                                                        auto& D     = lComponentOffsets[i];
                                                        auto& S     = *lComponetDesc[i];
                                                        D.m_HasT1   = S.m_isDoubleBuffer;
                                                        D.m_T0      = 0;
                                                        D.m_Size    = S.m_Size;
                                                        D.m_Offset  = xcore::bits::Align( p, S.m_Aligment ) - OriginalAddress;
                                                        p          += m_EntriesPerPage * D.m_Size;
                                                    }
                                                    assert((p - OriginalAddress) <= sizeof(tools::page));
                                                    return lComponentOffsets;
                                                }()
                                            }
                {}

            
            xforceinline entity::instance& getEntity( tools::page* Page, group::local_index LocalIndex ) const noexcept
            {
                xassert(LocalIndex.m_Value < m_EntriesPerPage );
                auto&       E       = m_lComponentOffsets[0];
                const auto  Index   = E.m_Offset + static_cast<std::uint32_t>(LocalIndex.m_Value) * E.m_Size;
                return *reinterpret_cast<entity::instance*>( &Page->m_RawData[Index] );
            }

            xforceinline void ConstructOnlyEntityComponent( tools::page* Page, group::local_index LocalIndex, entity::map_t::entry* pEntry ) const noexcept
            {
                // we don't construct the entity case here
                getEntity( Page, LocalIndex ).m_pInstance = pEntry;
            }

            xforceinline void ConstructOnlyComponent( tools::page* Page, group::local_index LocalIndex, int iComponent ) const noexcept
            {
                // we don't construct the entity case here
                xassert(iComponent!=0);
                auto&       E       = m_lComponentOffsets[iComponent];
                const auto  Index   = E.m_Offset + static_cast<std::uint32_t>(LocalIndex.m_Value) * E.m_Size;
                m_lComponentDescriptors[iComponent]->m_fnConstruct( &Page->m_RawData[Index] );
            }

            xforceinline void ConstructEntity( tools::page* Page, group::local_index LocalIndex, entity::map_t::entry* pEntry ) const noexcept
            {
                ConstructOnlyEntityComponent( Page, LocalIndex, pEntry );
                for( int i=1; i<m_nComponents; ++i )
                {
                    ConstructOnlyComponent(Page,LocalIndex,i);
                }
            }

            xforceinline void DestructEntity( tools::page* Page, std::uint32_t LocalIndex ) const noexcept
            {
                // Entity component does not need any destructor
                for( int i=1; i<m_nComponents; ++i )
                {
                    auto&       E       = m_lComponentOffsets[i];
                    const auto  Index   = E.m_Offset + LocalIndex * E.m_Size;
                    m_lComponentDescriptors[i]->m_fnDestruct( &Page->m_RawData[Index] );
                }
            }

            xforceinline constexpr const details::offsets* findMutableOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) const noexcept
            {
                for (int i = 0; i <m_nComponents; ++i)
                {
                    if (m_lComponentTypeGuids[i] >= TypeGuid)
                    {
                        if (m_lComponentTypeGuids[i] > TypeGuid) break;

                        const auto& Offsets = m_lComponentOffsets[i];
                        const auto  T1      = Offsets.m_HasT1 - Offsets.m_T0;
                        return &m_lComponentOffsets[i + T1];
                    }
                }
                return nullptr;
            }

            xforceinline constexpr const details::offsets* findReadOnlyOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) const noexcept
            {
                for (int i = 0; i <m_nComponents; ++i)
                {
                    if (m_lComponentTypeGuids[i] >= TypeGuid)
                    {
                        if (m_lComponentTypeGuids[i] > TypeGuid) break;
                        const auto& Offsets = m_lComponentOffsets[i];
                        return &m_lComponentOffsets[i + Offsets.m_T0];
                    }
                }
                return nullptr;
            }

            const type_guid                                                                         m_Guid;
            const std::uint16_t                                                                     m_EntriesPerPage;
            const std::uint16_t                                                                     m_nComponents;
            const std::array<type_guid,                     settings::max_components_in_entity_v>   m_lComponentTypeGuids;
            const std::array<const component::descriptor*,  settings::max_components_in_entity_v>   m_lComponentDescriptors;
            const std::array<details::offsets,              settings::max_components_in_entity_v>   m_lComponentOffsets;
        };

        //---------------------------------------------------------------------------------
        // GROUP::EVENTS
        //---------------------------------------------------------------------------------
        struct events
        {
            // onNewEntities...      (which new entities where created this frame)
            // onDeleteEntities...   (which entities got deleted this frame)
            // onUpdatedEntities...  (Some system may have change this entity (Had write access to it))
            // onMovedInEntities...  (Some new entity got moved in here, from another group)
            // onMovedOutEntities... (By Adding or Removing Componets)

            using created_entity    = xcore::types::make_unique< xcore::event::type<entity::tmp_ref>, struct new_entity_tag >;
            using deleted_entity    = xcore::types::make_unique< xcore::event::type<entity::tmp_ref>, struct delete_entity_tag >;
            using move_out_entity   = xcore::types::make_unique< xcore::event::type<entity::tmp_ref>, struct move_out_entity_tag >;
            using move_in_entity    = xcore::types::make_unique< xcore::event::type<entity::tmp_ref>, struct move_in_entity_tag >;
           // using updated_entity    = xcore::types::make_unique< xcore::event::type<entity::instance&>, struct updated_entity_tag >;
           // using updated_component = xcore::types::make_unique< xcore::event::type<entity::instance&>, struct updated_component_tag >;

            created_entity   m_CreatedEntity;
            deleted_entity   m_DeletedEntity;
            move_out_entity  m_MovedOutEntity;
            move_in_entity   m_MovedInEntity;
        };

        //---------------------------------------------------------------------------------
        // GROUP::INSTANCE
        // The contract for events sent to a group.
        //      All events sent to a group must respect the order in which the local thread
        //      Issue the command. However it can be intermix with other events sent from
        //      other threads. 
        //      Deletion of entities should happen at the memory barriers. 
        //          The Guid of an entity is clear at the end of the frame.
        //          It is the responsibility of a system to check for the validity of an entity.
        //
        // Key Commands for a group:
        //
        //  Add Entity - This appends an entity to the back of the entity list as a temporary storage.
        //               The official insertion will be done at the barrier. Here is where we will 
        //               Add the entity to the world.
        //  Del Entity - This is queued as a message and it wont execute until the memory barrier.
        //               Entities at the back of the list may be moved in the holes. 
        //               World will get updated base on the new location of the entity.
        //               The World will keep an active reference of the entity until the end of the frame.
        //               Then the world will remove the real entity. This allows temporary reference to 
        //               Entities to exists until the end of the frame. If the user needs a longer reference
        //               to an entity it should store the guid.
        //  Remove/Add Component - This will require that both groups will be locked at the same time in the memory
        //                         Barrier. One of the groups will be added in the other one will be erase.
        //                         The world will update its entity pointer at the memory barrier.
        //  getComponent - Will access the component for writing/reading. This can happen in a the system update.
        //                 However if this happens the group should be locked as read/write. Which means that if the
        //                 user gets a entity::tmp_ref it must write lock the group.
        // Other details:
        // System should specify the group which components are going to be reading and which are going to be written.
        // The memory barrier can only work when there is no reading or written systems.
        //---------------------------------------------------------------------------------
        struct instance
        {
            instance( group::guid Guid, tools::page_mgr& PageMgr, const descriptor& GroupDescriptor ) noexcept
                : m_Guid                { Guid }
                , m_PageMgr             { PageMgr }
                , m_GroupDescriptor     { GroupDescriptor }
                , m_DeleteList          { m_PageMgr }
                {}

            ~instance() noexcept
            {
                xcore::scheduler::channel Channel( xconst_universal_str("mecs::group::~instance") );
                const auto c    = m_nPages.load(std::memory_order_relaxed).m_Value;
                auto  nEntities = m_Count;
                for( std::uint16_t i=0; i<c; ++i )
                {
                    auto        pPage    = m_lPages[i];
                    const auto  n        = static_cast<std::uint16_t>(std::min( (int)m_GroupDescriptor.m_EntriesPerPage, (int)nEntities ));
                    Channel.SubmitJob( [pPage, n, this]
                    {
                        for( std::uint16_t i=0; i<n; ++i)
                        {
                            m_GroupDescriptor.DestructEntity( pPage, i );
                        }
                    
                        m_PageMgr.FreePage( pPage );
                    });

                    nEntities -= m_GroupDescriptor.m_EntriesPerPage;
                }
                Channel.join();
            }

            constexpr   auto        size()                  const   noexcept { return m_Count; }
            inline      auto        newSize()               const   noexcept { return m_NewCount.load(std::memory_order_relaxed); }

            xforceinline
            void* findComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) noexcept
            {
                if( auto pOffset = m_GroupDescriptor.findMutableOffset(Ref,TypeGuid); pOffset )
                {
                    const auto  Index   = pOffset->m_Offset + Ref.m_LocalIndex.m_Value * pOffset->m_Size;
                    std::byte*  p       = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[Index];
                    return p;
                }

                return nullptr;
            }

            xforceinline
            void* findComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) const noexcept
            {
                if( auto pOffset = m_GroupDescriptor.findReadOnlyOffset(Ref,TypeGuid); pOffset )
                {
                    const auto  Index   = pOffset->m_Offset + Ref.m_LocalIndex.m_Value * pOffset->m_Size;
                    std::byte*  p       = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[Index];
                    return p;
                }

                return nullptr;
            }

            entity::tmp_ref AllocEntity( entity::map_t::entry* pMapEntry ) noexcept
            {
                const auto          Index           = m_NewCount++;
                const local_index   RelativeIndex   { Index % m_GroupDescriptor.m_EntriesPerPage };
                const page_index    PageIndex       { static_cast<std::uint16_t>(Index / m_GroupDescriptor.m_EntriesPerPage) };

                do
                {
                    const auto nPages   = m_nPages.load(std::memory_order_relaxed);

                    if (PageIndex < nPages)
                    {
                        // Create entry here
                        m_GroupDescriptor.ConstructEntity( m_lPages[PageIndex.m_Value], RelativeIndex, pMapEntry );
                        break;
                    }
                    else if (PageIndex == nPages && RelativeIndex.m_Value == 0)
                    {
                        m_lPages[PageIndex.m_Value] = m_PageMgr.AllocPage();

                        m_nPages.store(page_index{static_cast<std::uint16_t>(nPages.m_Value+1)});
                        m_GroupDescriptor.ConstructEntity( m_lPages[PageIndex.m_Value], RelativeIndex, pMapEntry );
                        break;
                    }
                    else
                    {
                        // We will have to wait...
                        XCORE_PERF_ZONE_SCOPED_NC( "CreateEntity::Wasting Time", tracy::Color::ColorType::Red )
                        XCORE_CMD_PROFILER( while (PageIndex >= m_nPages.load(std::memory_order_relaxed)); )
                    }

                } while (true);

                // Set officially the value for the entity
                return { this, local_index{RelativeIndex}, page_index{PageIndex} };
            }

            xforceinline
            void DeleteEntity( entity::instance& Entity ) noexcept
            {
                assert(Entity.isZombie() == false);

                auto& Entry         = m_DeleteList.append();
                auto TmpRef         = Entity.getTmpRef();
                Entry.m_LocalIndex  = TmpRef.m_LocalIndex;
                Entry.m_PageIndex   = TmpRef.m_PageIndex;
                    
                // Mark entity as dead/zombie
                reinterpret_cast<std::size_t&>(Entity.m_pInstance) |= 1;
            }

            void DeletePendingEntries( void ) noexcept
            {
                std::uint32_t   nDelete     = m_DeleteList.size();
                const auto      WaterMark   = m_Count - nDelete;

                // Are we erasing everything?
                if( WaterMark == 0 )
                {
                    local_index     I{0};
                    page_index      P{0};
                    while( nDelete-- )
                    {
                        m_GroupDescriptor.DestructEntity( m_lPages[P.m_Value], I.m_Value );
                        I.m_Value++;
                        if( I.m_Value == m_GroupDescriptor.m_EntriesPerPage )
                        {
                            P.m_Value++;
                            I.m_Value = 0;
                        }
                    }

                    xassert( (m_GroupDescriptor.m_EntriesPerPage * P.m_Value + I.m_Value ) == m_Count );
                }
                else
                {
                    std::uint16_t   iTopPage    = static_cast<std::uint16_t>(m_Count/m_GroupDescriptor.m_EntriesPerPage);
                    std::uint16_t   iTopIndex   = static_cast<std::uint16_t>(m_Count%m_GroupDescriptor.m_EntriesPerPage);
                    xassert( (iTopPage * m_GroupDescriptor.m_EntriesPerPage + iTopIndex) == m_Count );
                    xassert( ((static_cast<std::uint32_t>(iTopPage) * m_GroupDescriptor.m_EntriesPerPage + iTopIndex)) >= WaterMark );

                    auto ProcessFromTheTop = [&]
                    {
                        do  
                        {
                            xassert_block_basic()
                            {
                                auto Index = (static_cast<std::uint32_t>(iTopPage) * m_GroupDescriptor.m_EntriesPerPage + iTopIndex);
                                xassert(Index >= WaterMark );
                            }

                            iTopIndex--;
                            if( iTopIndex == 0xffff )
                            {
                                xassert(iTopPage);
                                iTopPage--;
                                iTopIndex = m_GroupDescriptor.m_EntriesPerPage-1;
                            }

                            if( false == m_GroupDescriptor.getEntity( m_lPages[iTopPage], local_index{iTopIndex} ).isZombie() ) break;

                            m_GroupDescriptor.DestructEntity( m_lPages[iTopPage], iTopIndex );
                            --nDelete;

                        } while( nDelete );
                    };

                    // Find the real top
                    ProcessFromTheTop();

                    for( auto E : m_DeleteList )
                    {
                        if( E.m_PageIndex.m_Value >= iTopPage )
                        {
                            if( E.m_PageIndex.m_Value > iTopPage || E.m_LocalIndex.m_Value > iTopIndex ) 
                            {
                                xassert( m_GroupDescriptor.getEntity( m_lPages[E.m_PageIndex.m_Value], E.m_LocalIndex ).isZombie() );
                                continue;
                            }
                        }

                        if( nDelete-- == 0 ) break;
                        xassert( (static_cast<std::uint32_t>(iTopPage) * m_GroupDescriptor.m_EntriesPerPage + iTopIndex) >= WaterMark );

                        // This entity is completely gone at this point
                        xassert( m_GroupDescriptor.getEntity( m_lPages[E.m_PageIndex.m_Value], E.m_LocalIndex ).isZombie() );
                        m_GroupDescriptor.DestructEntity( m_lPages[E.m_PageIndex.m_Value], E.m_LocalIndex.m_Value );

                        // Move components to new location
                        for( int i=0; i<m_GroupDescriptor.m_nComponents; i++ )
                        {
                            const auto& ComponentDetails = m_GroupDescriptor.m_lComponentOffsets[i];
                            auto*       pDestinaltion    = &m_lPages[E.m_PageIndex.m_Value]->m_RawData[ ComponentDetails.m_Offset + E.m_LocalIndex.m_Value * ComponentDetails.m_Size ];
                            const auto* pSource          = &m_lPages[iTopPage]             ->m_RawData[ ComponentDetails.m_Offset + iTopIndex              * ComponentDetails.m_Size ];
                            std::memcpy( pDestinaltion, pSource, ComponentDetails.m_Size );
                        }

                        // Update the moved entity reference
                        {
                            const auto& ComponentDetails = m_GroupDescriptor.m_lComponentOffsets[0];
                            const auto& MovedEntity      = m_GroupDescriptor.getEntity( m_lPages[E.m_PageIndex.m_Value], E.m_LocalIndex );
                            assert( MovedEntity.isZombie() == false );
                            MovedEntity.m_pInstance->m_Value.m_LocalIndex = E.m_LocalIndex;
                            MovedEntity.m_pInstance->m_Value.m_PageIndex  = E.m_PageIndex;
                        }

                        // Update the top count
                        ProcessFromTheTop();
                    }

                    xassert_block_basic()
                    {
                        local_index     I{0};
                        page_index      P{0};
                        for( auto E = WaterMark; E--; )
                        {
                            xassert( false == m_GroupDescriptor.getEntity( m_lPages[P.m_Value], I ).isZombie() );
                            I.m_Value++;
                            if( I.m_Value == m_GroupDescriptor.m_EntriesPerPage )
                            {
                                P.m_Value++;
                                I.m_Value = 0;
                            }
                        }
                    }
                }

                // Clear all the list
                m_DeleteList.clear();

                // Set the new count of entries
                m_Count = WaterMark;
            }

            void MemoryBarrierSync( sync_point::instance& SyncPoint ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED()

                //
                // Avoid updating multiple times for a given sync_point done
                //
                if( m_pLastSyncPoint == &SyncPoint && m_LastTick == SyncPoint.m_Tick )
                    return;

                // Save state of previous update to make sure no one ask us to update twice
                m_pLastSyncPoint = &SyncPoint;
                m_LastTick       = SyncPoint.m_Tick;

                //
                // Make sure to lock for writing
                //
                xcore::lock::scope Lock(m_SemaphoreLock);

                //
                // Officially add all the new entries
                //
                m_Count += m_NewCount.load(std::memory_order_relaxed);
                m_NewCount.store(0,std::memory_order_relaxed );

                //
                // Delete all pending entities
                //
                DeletePendingEntries();

                //
                // Free any pages that we are not using any more
                //
                {
                    const std::uint16_t nPages      = m_nPages.load( std::memory_order_relaxed ).m_Value;
                    const std::uint16_t NeededPages = m_Count / m_GroupDescriptor.m_EntriesPerPage + 1u;
                
                    for( auto i=NeededPages; i<nPages; i++ )
                    {
                        m_PageMgr.FreePage( m_lPages[i] );
                    }

                    m_nPages.store( group::page_index{NeededPages}, std::memory_order_relaxed );
                }
            }

            constexpr static std::size_t max_pages_v = 1024*32;
            using page_list = xcore::array<tools::page*, max_pages_v>;

            struct component_lock_entry
            {
                std::uint8_t    m_nWritters{0};
                std::uint8_t    m_nReaders{0};
            };

            using component_lock_list_t = xcore::lock::object<std::array<component_lock_entry,64>,xcore::lock::semaphore>;

            group::guid                                                                 m_Guid;
            tools::page_mgr&                                                            m_PageMgr;
            tools::pool<details::delete_entity,max_pages_v>                             m_DeleteList;
            xcore::lock::semaphore                                                      m_SemaphoreLock     {};
            std::atomic<std::uint32_t>                                                  m_NewCount          { 0 };
            std::uint32_t                                                               m_Count             { 0 };
            std::atomic<group::page_index>                                              m_nPages            { group::page_index{0} };
            page_list                                                                   m_lPages            {};
            component_lock_list_t                                                       m_lComponentLocks   {};
            events                                                                      m_Events            {};
            const group::descriptor&                                                    m_GroupDescriptor;
            sync_point::instance*                                                       m_pLastSyncPoint    { nullptr };
            std::uint32_t                                                               m_LastTick          { 0 };
            void*                                                                       m_pDataBaseRef      { nullptr };
        };
    }

    //---------------------------------------------------------------------------------
    // QUERY
    //---------------------------------------------------------------------------------
    namespace query
    {
        //---------------------------------------------------------------------------------
        // QUERY::FILTER_BITS
        //---------------------------------------------------------------------------------
        struct filter_bits
        {
            std::array<std::uint64_t,2> m_Bits;

            constexpr filter_bits()=default;
            constexpr filter_bits(std::nullptr_t)    noexcept : m_Bits{ 0ull, 0ull}{}
            constexpr filter_bits(xcore::not_null_t) noexcept : m_Bits{~0ull,~0ull}{}

            xforceinline constexpr int AllocBit( void ) noexcept
            {
                for( auto& E : m_Bits ) 
                {
                    const auto Index = xcore::bits::ctz64(E);
                    if(Index != 64 )
                    {
                        E &= ~(1ull<<Index);
                        return Index;
                    }
                }

                xassert(false);
                return -1;
            }

            xforceinline void AddBit( int Index ) noexcept
            {
                m_Bits[Index/64] |= (1ull<<(Index%64));
            }

            xforceinline void Add( filter_bits Bits ) noexcept
            {
                for( int i=0; i<m_Bits.size(); ++i ) m_Bits[i] |= Bits.m_Bits[i];
            }
            xforceinline bool getBit( int Index ) const noexcept
            {
                return !!((m_Bits[Index/64] >> (Index%64))&1);
            }

            xforceinline constexpr bool Query( filter_bits All, filter_bits Any, filter_bits None ) const noexcept
            {
                bool    bAny1 = false;
                bool    bAny2 = false;
                for( int i=0; i<m_Bits.size(); ++i )
                { 
                    const auto B = m_Bits[i];
                    if( None.m_Bits[i] & B ) return false;
                    if( const auto A = All.m_Bits[i]; (A&B) != A ) return false;
                    if( const auto A = Any.m_Bits[i]; (A&B) ) bAny1 = true;
                    else bAny2 = A != 0;
                }
                return bAny2 == bAny1;
            }
        };

        namespace details
        {
            template< typename...T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED >
            struct access_components
            {
                std::tuple<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED& ...> m_Tuple;
                template< typename T > xforceinline constexpr
                T& get( void ) const noexcept
                {
                    return std::get<T&>(m_Tuple);
                }
            };
        }

        //---------------------------------------------------------------------------------
        // QUERY::ALL NONE ANY
        //---------------------------------------------------------------------------------
        template< typename...T_COMPONENTS_AND_TAGS >
        struct all : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            static_assert( ((std::is_reference_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
            static_assert( ((std::is_const_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct none : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            static_assert( ((std::is_reference_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
            static_assert( ((std::is_const_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct any : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            static_assert( ((std::is_reference_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
            static_assert( ((std::is_const_v<T_COMPONENTS_AND_TAGS> == false) && ...) );
        };

        template< typename...T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED >
        struct access : component::group::details::guid_arrays_from_types< T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED... >
        {
            static_assert( ((std::is_reference_v<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED> == false) && ...) );

            using type = details::access_components< T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED... >;
            std::array<component::write_type, sizeof...(T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED)> write_v
            { 
                std::is_base_of_v<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED, component::quantum_mutable> ? component::write_type::QUANTUM_WRITE :
                std::is_const_v<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED> == false                      ? component::write_type::READ_ONLY     :
                                                                                                          component::write_type::LINEAR_WRITE
                , ...
            };
        };

        //---------------------------------------------------------------------------------
        // QUERY::SYSTEM FUNCTION
        //---------------------------------------------------------------------------------
        namespace details
        {
            template<typename...T_ARGS>
            constexpr auto TypeArray( std::tuple<T_ARGS...>* ) noexcept
            {
                static_assert( (std::is_reference_v<T_ARGS> && ...) );
                return std::array
                {
                    std::decay_t<T_ARGS>::type_guid_v ...
                };
            }

            template<typename...T_ARGS>
            constexpr auto ConstArray( std::tuple<T_ARGS...>* ) noexcept
            {
                static_assert( (std::is_reference_v<T_ARGS> && ...) );
                return std::array
                {
                    std::is_const_v<std::remove_reference_t<T_ARGS>> ...
                };
            }

            template< typename T > struct remove_references;
            template< typename...T_ARGS >
            struct remove_references< std::tuple< T_ARGS &... >>
            {
                using type = std::tuple<T_ARGS...>;
            };

            template< std::size_t T_SIZE, typename T_FUNCTION, typename T_SORTED >
            constexpr auto remap( T_FUNCTION& Fun, T_SORTED& Sort ) noexcept
            {
                std::array<int,T_SIZE> Array{};
                for( int i=0; i<Array.size(); ++i )
                {
                    for( int j=0; j<Array.size(); ++j )
                        if( Sort[i] == Fun[j] ) 
                        {
                            Array[i] = j - i;
                        }
                }
                return Array;
            }

            template<typename T, typename...T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED>
            struct get_access_type;

            template<>
            struct get_access_type<void>
            {
                using type = access<>;
            };

            template<typename T, typename...T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED>
            struct get_access_type
            {
                using type = typename std::conditional_t
                <
                     xcore::types::is_specialized_v< access, T >
                    , T
                    , typename get_access_type<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED...>::type
                >;
            };

            template< typename...T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED >
            using access_t = typename get_access_type<T_COMPONENTS_ONLY_WITH_CONST_WHEN_NEEDED... , void>::type;
        }

        template< typename T >
        struct system_function
        {
            using args_t        = typename xcore::function::traits<T>::args_tuple;
            using sorted_args_t = typename xcore::types::tuple_sort_t< component::group::details::compare, args_t >;
            constexpr static std::array list_v          { details::TypeArray ( reinterpret_cast<args_t*>(nullptr) ) };
            constexpr static std::array const_list_v    { details::ConstArray( reinterpret_cast<args_t*>(nullptr) ) };
            constexpr static std::array sorted_list_v   { details::TypeArray ( reinterpret_cast<sorted_args_t*>(nullptr) ) };
            constexpr static std::array remap_v         { details::remap<list_v.size()>( list_v, sorted_list_v ) };
        };

        //---------------------------------------------------------------------------------
        // QUERY::DEFINE
        //---------------------------------------------------------------------------------
        struct define_data
        {
            struct query
            {
                std::span<const type_guid>    m_All;            // Which components/tags we must have
                std::span<const type_guid>    m_Any;            // Must have at least one component/tag from the set
                std::span<const type_guid>    m_None;           // Which components/tags you must not have
            };

            struct access
            {
                std::span<const type_guid>          m_Components;     // Which components/tags we must have
                std::span<component::write_type>    m_WriteType;          // Which components/tags we must have
            };

            query   m_ComponentQuery;
            query   m_TagQuery;
            access  m_Access;
        };

        template< typename...T_QUERY_FILTERS >
        struct define : define_data
        {
            using access = typename details::access_t<T_QUERY_FILTERS...>::type;
            
            template< typename T >
            constexpr static void Initializer( define& This ) noexcept
            {
                if constexpr ( xcore::types::is_specialized_v<all, T> ) 
                {
                    if(T::tag_guid_list_v.size()>1)         This.m_TagQuery.m_All        = std::span<const type_guid>(&T::tag_guid_list_v[1],       T::tag_guid_list_v.size()-1);
                    if(T::component_guid_list_v.size()>1)   This.m_ComponentQuery.m_All  = std::span<const type_guid>(&T::component_guid_list_v[1], T::component_guid_list_v.size()-1);
                }
                else if constexpr ( xcore::types::is_specialized_v<any, T> )
                {
                    if(T::tag_guid_list_v.size()>1)         This.m_TagQuery.m_Any        = std::span<const type_guid>(&T::tag_guid_list_v[1],       T::tag_guid_list_v.size()-1);
                    if(T::component_guid_list_v.size()>1)   This.m_ComponentQuery.m_Any  = std::span<const type_guid>(&T::component_guid_list_v[1], T::component_guid_list_v.size()-1);
                }
                else if constexpr ( xcore::types::is_specialized_v<none, T> )
                {
                    if(T::tag_guid_list_v.size()>1)         This.m_TagQuery.m_None       = std::span<const type_guid>(&T::tag_guid_list_v[1],       T::tag_guid_list_v.size()-1);
                    if(T::component_guid_list_v.size()>1)   This.m_ComponentQuery.m_None = std::span<const type_guid>(&T::component_guid_list_v[1], T::component_guid_list_v.size()-1);
                }
                else if constexpr ( xcore::types::is_specialized_v<access, T> )
                {
                    static_assert( T::tag_guid_list_v.size() == 0 );
                    if(T::component_guid_list_v.size())   This.m_Access.m_Components    = std::span<const type_guid>            (&T::component_guid_list_v[0], T::component_guid_list_v.size());
                    if(T::component_guid_list_v.size())   This.m_Access.m_WriteType     = std::span<const component::write_type>(&T::write_v[0], T::write_v.size());
                }
            };

            constexpr define( void ) noexcept
            {
                ( Initializer<T_QUERY_FILTERS>( *this ), ... );
            }
        };

        //---------------------------------------------------------------------------------
        // QUERY::RESULT ENTRY
        //---------------------------------------------------------------------------------
        struct result_entry
        {
            component::group::instance*                         m_pGroup;
            std::array<std::uint16_t,15>                        m_lRemaps;      // Offsets into the actual page where the component is
            std::array<std::uint8_t,15>                         m_lLockRemaps;  // Offsets into the actual page where the component is
            std::span<const bool>                               m_isWrite;
            std::uint8_t                                        m_nRemaps;
        };

        //---------------------------------------------------------------------------------
        // QUERY::INSTANCE
        //---------------------------------------------------------------------------------
        struct instance
        {
            struct component_query
            {
                filter_bits                 m_All                   {nullptr};
                filter_bits                 m_Any                   {nullptr};
                filter_bits                 m_None                  {nullptr};
            };

            struct tag_query
            {
                filter_bits                 m_All                   {nullptr};
                filter_bits                 m_Any                   {nullptr};
                filter_bits                 m_None                  {nullptr};
            };

            xcore::vector<result_entry> m_lResults              {};
            bool                        m_isInitialized         {false};
            filter_bits                 m_Function              {nullptr};
            filter_bits                 m_Write                 {nullptr};
            component_query             m_ComponentQuery        {};
            tag_query                   m_TagQuery              {};
        };
    }

    //------------------------------------------------------------------------------------------
    // COMMANDS - What kinds of things can the sync points do for us...
    //------------------------------------------------------------------------------------------
    /*
    namespace commands
    {
        using command_pool = containers::cmd_pool<1024>;

        namespace details
        {
            struct cmd_add_entity
            {
                entity::guid            m_Guid;
                int                     m_Size;
                component::type_guid*   m_pComponentTypes;
                void*                   m_pNewComponentData;
            };

            struct cmd_modify_components
            {
                entity::guid            m_Guid;
                int                     m_DelComponentCount     {0};
                component::type_guid*   m_pDelComponentTypes    {nullptr};
                int                     m_NewComponentCount     {0};
                component::type_guid*   m_pNewComponentTypes    {nullptr};
                void*                   m_pNewComponentData     { nullptr };
            };

            struct cmd_set_components
            {
                entity::guid            m_Guid;
                component::type_guid*   m_pComponentTypes       { nullptr };
                int                     m_ComponentCount        { 0 };
                void*                   m_pNewComponentData     { nullptr };
            };

            using cmds = std::variant
            <
                  cmd_add_entity
                , cmd_modify_components
                , cmd_set_components
            >;

            template< typename T_CLASS, typename...T_ARGS >
            struct set_component_handler
            {
                using component_tuple = std::tuple< T_ARGS... >;

                T_CLASS& m_Ref;

                template< typename T >
                auto& getComponentData(void) noexcept
                {
                    return std::get<T&>(*reinterpret_cast<component_tuple*>(m_Ref.m_pNewComponentData));
                }
            };

            struct modify_components_handler
            {
                cmd_modify_components&  m_Ref;
                command_pool&           m_Pool;
                command_pool::handle    m_Handel;

                template< typename...T_COMPONENTS >
                auto AddComponents( void ) noexcept
                {
                    using component_tuple = std::tuple<T_COMPONENTS...>;
                    std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };

                    m_Ref.m_NewComponentCount   = sizeof...(T_COMPONENTS);
                    m_Ref.m_pNewComponentData   = &m_Pool.NewData<component_tuple>(m_Handel);
                    m_Ref.m_pNewComponentTypes  = m_Pool.NewData<decltype(ComponentData)>(m_Handel, ComponentData).data();
                    return details::set_component_handler<cmd_modify_components, T_COMPONENTS...>{ m_Ref };
                }

                template< typename...T_COMPONENTS >
                void RemoveComponents(void) noexcept
                {
                    std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };
                    m_Ref.m_DelComponentCount   = sizeof...(T_COMPONENTS);
                    m_Ref.m_pDelComponentTypes  = m_Pool.NewData<decltype(ComponentData)>(m_Handel, ComponentData).data();
                }
            };
        }

        struct command_buffer
        {
            template< typename...T_COMPONENTS >
            auto AddEntity(entity::guid Guid) noexcept
            {
                using component_tuple = std::tuple< T_COMPONENTS... >;

                auto&       EntityCmd           = m_Pool.NewCmd<details::cmd_add_entity>(m_Handle);
                std::array  ComponentData       { T_COMPONENTS::factory::guid_component_type_v ... };

                EntityCmd.m_Guid                = Guid;
                EntityCmd.m_pComponentTypes     = m_Pool.NewData<decltype(ComponentData)>(m_Handle, ComponentData).data();
                EntityCmd.m_Size                = ComponentData.size();
                EntityCmd.m_pData               = &m_Pool.NewData<component_tuple>(m_Handle);

                return details::set_component_handler<details::cmd_add_entity, T_COMPONENTS...>{ EntityCmd };
            }

            auto EntityModifyComponets(entity::guid Guid) noexcept
            {
                auto& ModifyCmd = m_Pool.NewCmd<details::cmd_modify_components>(m_Handle);
                ModifyCmd.m_Guid = Guid;
                return details::modify_components_handler{ ModifyCmd, m_Pool, m_Handle };
            }

            template< typename...T_COMPONENTS >
            void SetComponents(entity::guid Guid) noexcept
            {
                using component_tuple = std::tuple< T_COMPONENTS... >;

                auto&       SetCmd      = m_Pool.NewCmd<details::cmd_set_components>(m_Handle);
                std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };

                SetCmd.m_Guid                   = Guid;
                SetCmd.m_ComponentCount         = sizeof...(T_COMPONENTS);
                SetCmd.m_pComponentTypes        = m_Pool.NewData<decltype(ComponentData)>(m_Handle, ComponentData).data();
                SetCmd.m_pNewComponentData      = &m_Pool.NewData<component_tuple>(m_Handle);

                return details::set_component_handler<details::cmd_set_components, T_COMPONENTS...>{ SetCmd };
            }

            command_pool&           m_Pool;
            command_pool::handle    m_Handle;
        };
    }
            */


    //---------------------------------------------------------------------------------
    // SYSTEM - System execute logic for components
    //---------------------------------------------------------------------------------
    namespace system
    {
        //---------------------------------------------------------------------------------
        // SYSTEM::INSTANCE 
        //---------------------------------------------------------------------------------
        struct instance : xcore::scheduler::job<4>
        {
            using                                   parent_t        = xcore::scheduler::job<4>;

            // Defaults that can be overwritten by the user
            constexpr static auto                   system_name_v   = xconst_universal_str("mecs::System");
            constexpr static mecs::query::define<>  query_v         {};

            // Messages that we can handle these can be overwritten by the user
            inline void msgSyncPointDone    ( sync_point::instance& ){}
            inline void msgSyncPointStart   ( sync_point::instance& ){}
            inline void msgWorldStart       ( void ){}
            inline void msgFrameStart       ( void ){}
            inline void msgFrameDone        ( void ){}

            // Functions
            template<typename K = world::instance>
            xforceinline void deleteEntity( entity::instance& Entity )
            {
                reinterpret_cast<K&>(m_World).deleteEntity( Entity );
            }

            template<typename K = world::instance>
            xforceinline auto& getTime( void ) const noexcept
            {
                return reinterpret_cast<const K&>(m_World).m_Time;
            }

            template< typename T, typename K = world::instance >
            xforceinline bool getGroup( component::group::guid gGroup, T&& CallBack ) noexcept
            {
                return reinterpret_cast<K&>(m_World).m_mapGroupPerTag.getExlusive( gGroup, [&](std::unique_ptr<component::group::instance>& upGroup )
                {
                    const auto& Semaphore = upGroup->m_SemaphoreLock;
                    Semaphore.lock();
                    CallBack( *upGroup );
                    Semaphore.unlock();
                });
            }

            template< typename T_TUPLE_ADD_COMPONENTS_AND_TAGS, typename T_TUPLE_REMOVE_COMPONENTS_AND_TAGS, typename T_CALLBACK, typename K = world::instance >
            xforceinline void getGroupBy( entity::instance& Entity, T_CALLBACK&& CallBack ) const noexcept
            {
                auto& Group = reinterpret_cast<K&>(m_World).getOrCreateGroupBy
                                ( *Entity.getTmpRef().m_pGroupInstance, (T_TUPLE_ADD_COMPONENTS_AND_TAGS*)nullptr, (T_TUPLE_REMOVE_COMPONENTS_AND_TAGS*)nullptr );

                {
                    const auto& Semaphore = Group.m_SemaphoreLock;
                    Semaphore.lock();
                    CallBack( Group );
                    Semaphore.unlock();
                };
            }

            template< typename K = world::instance >
            xforceinline void moveEntityToGroup( component::group::instance& Group, entity::instance& Entity, xcore::function::view<void(entity::tmp_ref)> CallBack = [](entity::tmp_ref){}) const noexcept
            {
                reinterpret_cast<K&>(m_World).moveEntityToGroup(Group, Entity, CallBack );
            }

            template<typename T_HANDLE_CALLBACK >
            void getEntity( entity::guid gEntity, T_HANDLE_CALLBACK&& CallBack )
            {
            
            }

            template<typename K = world::instance>
            xforceinline void createEntity( component::group::instance& Group, xcore::function::view<void(entity::tmp_ref)> UserFunction = [](entity::tmp_ref){}, entity::guid Guid = entity::guid{xcore::not_null} ) noexcept
            {
                reinterpret_cast<K&>(m_World).createEntity( Group, UserFunction, Guid );
            }

            instance( world::instance& World, xcore::string::const_universal Str, xcore::scheduler::definition Def = {} ) noexcept 
                : parent_t{ Def | xcore::scheduler::triggers::DONT_CLEAN_COUNT }
                , m_World{World}
                {setupName(Str);}

            // TODO: This will be private
            world::instance&    m_World;
            query::instance     m_Query;
        };

        namespace details
        {
            //-----------------------------------------------------------------------------------------
            inline
            const char* LockQueryComponents( query::instance& Query, const std::span<const std::int16_t> IndexList ) noexcept
            {
                const char* pError = nullptr;

                xassert(Query.m_lResults.size());

                // Lock all the groups that we need to work with so that no one tries to add/remove entities
                for( const auto& E : Query.m_lResults )
                {
                    auto& Group = const_cast<component::group::instance&>(*E.m_pGroup);

                    // TODO: Must lock each component base on r/w also must lock the group

                    // We lock as read only even if one of our components is going to write to it since really what we are saying is that we 
                    // are going to use the group. Unlock will happen when we are done using this group.
                    std::as_const(Group.m_SemaphoreLock).lock();
                    
                    // Now we are going to tell the system which components in fact we are going to use. 
                    // This is mainly for debugging purposes to make sure that we don't have two system writing to the same component
                    static_assert( std::is_const_v<decltype(Group)> == false, "We need to lock here as read only" );
                    xcore::lock::scope Lk( Group.m_lComponentLocks );
                    auto& ComponentLockArray = Group.m_lComponentLocks.get();

                    auto LockComponent = [&]( std::int16_t I ) constexpr noexcept -> const char*
                    {
                        const auto iComp = E.m_lLockRemaps[I];
                        auto&      CLock = ComponentLockArray[iComp];

                        if( E.m_isWrite[I] )
                        {
                            // Deal with warnings of writing a component while erasing it
                            if(I==0)
                            {
                                for( int i=0; i<Group.m_GroupDescriptor.m_nComponents; i++ )
                                {
                                    if( ComponentLockArray[i].m_nWritters )
                                    {
                                        if( i == 0 )
                                        {
                                            // TODO: WARNING: Chance of double erase here
                                        }
                                        else
                                        {
                                            // TODO: WARNING someone may be trying to delete an entity while a system is writing to it
                                        }
                                    }
                                }
                            }
                            else if(ComponentLockArray[0].m_nWritters)
                            {
                                // TODO: WARNING, A system is trying to write to one of the components while another system may be erasing the entity
                            }

                            if(CLock.m_nWritters) return "I found two systems trying to write to the same component";
                            if(CLock.m_nReaders)  return "I found a systems that is trying to write to a component while other systems where trying to read it [1]";
                            CLock.m_nWritters++;
                        }
                        else
                        {
                            if(CLock.m_nWritters)  return "I found a systems that is trying to write to a component while other systems where trying to read it [2]";
                            CLock.m_nReaders++;
                        }

                        return nullptr;
                    };

                    // Lock component
                    for( auto x : IndexList )
                    {
                        if( pError = LockComponent(x) )
                            return pError;
                    }
                }

                return nullptr;
            }

            //-----------------------------------------------------------------------------------------
            inline
            void UnlockQueryComponents( query::instance& Query, const std::span<const std::int16_t> IndexList ) noexcept
            {
                for( const auto& E : Query.m_lResults )
                {
                    auto& Group = const_cast<component::group::instance&>(*E.m_pGroup);

                    // Release each of the components that we locked before
                    static_assert( std::is_const_v<decltype(Group)> == false, "We need to lock here as read only" );
                    xcore::lock::scope Lk( Group.m_lComponentLocks );
                    auto& ComponentLockArray = Group.m_lComponentLocks.get();

                    auto UnlockComponent = [&]( std::int16_t I ) constexpr noexcept
                    {
                        const auto iComp = E.m_lLockRemaps[I];
                        auto&      CLock = ComponentLockArray[iComp];
                        if( E.m_isWrite[I] )
                        {
                            xassert(CLock.m_nWritters);
                            CLock.m_nWritters--;
                        }
                        else
                        {
                            xassert(CLock.m_nReaders);
                            CLock.m_nReaders--;
                        }
                    };

                    // Unlock component
                    for( auto x : IndexList )
                    {
                        UnlockComponent(x);
                    }

                    // We officially release this group
                    std::as_const(Group.m_SemaphoreLock).unlock();
                }
            }

            //-----------------------------------------------------------------------------------------
            template< typename T, typename...T_ARGS >
            void ProcessGroup( xcore::scheduler::channel& Channel, query::result_entry& ResultEntry, T&& Functor ) noexcept
            {
                using       function_t      = query::system_function<decltype(Functor)>;
                using       pool_tuple      = typename std::tuple< std::remove_reference_t<T_ARGS>*... >;
                auto&       Group           = *ResultEntry.m_pGroup;
                auto        TotalEntries    = Group.m_Count;
                auto        Index           = 0;

                xassert( TotalEntries );

                do 
                {
                    const auto  nEntities  = std::min( (int)5000, (int)TotalEntries );

                    Channel.SubmitJob( [ &Group, Index, nEntities, &ResultEntry, &Functor ]
                    {
                        XCORE_PERF_ZONE_SCOPED_N("ForEach::LightJob")

                        const auto  epp    = static_cast<std::int32_t>(Group.m_GroupDescriptor.m_EntriesPerPage);
                        auto        I      { static_cast<std::uint16_t>( Index % epp) };
                        auto        P      { static_cast<std::uint16_t>( Index / epp) };
                        auto        TotalRemainingForThisPage   = std::min( nEntities, (epp - I) );
                        auto        TotalRemaining              = nEntities - TotalRemainingForThisPage;

                        for(;;) 
                        {
                            const auto Pools  = pool_tuple
                            { 
                                reinterpret_cast<std::remove_reference_t<T_ARGS>*>
                                ( &Group.m_lPages[P]->m_RawData
                                    [ 
                                        ResultEntry.m_lRemaps[ xcore::types::tuple_t2i_v< T_ARGS, function_t::args_t > ] 
                                    ]
                                ) ... 
                            };
                            
                            // execute each entry
                            for( const auto End = I + TotalRemainingForThisPage; I<End; ++I )
                            {
                                Functor
                                ( 
                                    std::get
                                    <
                                        std::remove_reference_t<T_ARGS>*
                                    >(Pools)[I] ... 
                                );
                            }

                            // are we done?
                            if(TotalRemaining==0) break;

                            // get ready for next page
                            P++;
                            I                         = 0;
                            TotalRemainingForThisPage = std::min( TotalRemaining, epp );
                            TotalRemaining           -= TotalRemainingForThisPage;
                        }

                    });

                    Index        += nEntities;
                    TotalEntries -= nEntities;

                } while( TotalEntries );
            }

            //-----------------------------------------------------------------------------------------

            template< typename T, typename...T_ARGS >
            const char* ForEach( query::instance& Query, T&& Functor, std::tuple<T_ARGS...>* ) noexcept
            {
                using                           function_t  = query::system_function<decltype(Functor)>;
                constexpr static std::array     IndexList   { static_cast<std::int16_t>(xcore::types::tuple_t2i_v< T_ARGS, function_t::args_t >) ... };
                const char*                     pError      = nullptr;

                // If there is nothing to do then lets bail out
                if( Query.m_lResults.size() == 0 )
                    return pError;

                //
                // Lock all the groups
                //
                if( pError = LockQueryComponents( Query, IndexList ) ) return pError;

                //
                // Do all the groups
                //
                xcore::scheduler::channel Channel( xconst_universal_str("ForEach") );
                for (auto& R : Query.m_lResults )
                {
                    if( R.m_pGroup->m_Count == 0 ) continue;
                    ProcessGroup<T,T_ARGS...>( Channel, R, std::forward<T&&>(Functor) );
                }
                Channel.join();

                //
                // unlock all the groups that we need to work with so that no one tries to add/remove entities
                //
                UnlockQueryComponents( Query, IndexList );

                return pError;
            }

            //---------------------------------------------------------------------------------
            // SYSTEM::CUSTOM SYSTEM 
            //---------------------------------------------------------------------------------
            template< typename T_SYSTEM >
            struct custom_system final : T_SYSTEM
            {
                using user_system_t     = T_SYSTEM;
                using world_instance_t  = std::decay_t< decltype(user_system_t::m_World) >;

                sync_point::events::start::delegate                         m_SyncPointStartDelegate{       &custom_system::msgSyncPointStart };
                sync_point::events::done::delegate                          m_SyncPointDoneDelegate { this, &custom_system::msgSyncPointDone  };
                typename world_instance_t::events::world_start::delegate    m_WorldStartDelegate    {       &custom_system::msgWorldStart     };
                typename world_instance_t::events::frame_start::delegate    m_FrameStartDelegate    {       &custom_system::msgFrameStart     };
                typename world_instance_t::events::frame_done::delegate     m_FrameDoneDelegate     {       &custom_system::msgFrameDone      };

                using user_system_t::user_system_t;

                custom_system( world::instance& World ) noexcept 
                    : T_SYSTEM { World, user_system_t::system_name_v }
                {}

                void msgSyncPointDone( sync_point::instance& Syncpoint ) noexcept
                {
                    // Call the user function if he overwrote it
                    if constexpr ( &T_SYSTEM::msgSyncPointDone != &system::instance::msgSyncPointDone )
                    {
                        T_SYSTEM::msgSyncPointDone(Syncpoint);
                    }

                    // Lock all the groups that we need to work with so that no one tries to add/remove entities
                    for( const auto& E : user_system_t::m_Query.m_lResults )
                    {
                        // TODO: Must lock each component base on r/w also must lock the group
                        E.m_pGroup->MemoryBarrierSync( Syncpoint );
                    }

                    user_system_t::m_Query.m_lResults.clear();
                }

                virtual void qt_onRun( void ) noexcept override
                {
                    user_system_t::m_World.DoQuery(user_system_t::m_Query, *this, user_system_t::query_v );
                    
                    // Allow the system to compute
                    auto pErrorString = ForEach( user_system_t::m_Query, *this );
                    xassert(pErrorString==nullptr);
                }
            };
        }

        //---------------------------------------------------------------------------------
        // SYSTEM::FOR EACH 
        //---------------------------------------------------------------------------------
        template< typename T > constexpr xforceinline
        auto ForEach( query::instance& Query, T&& Functor ) noexcept
        {
            using function_t  = query::system_function<decltype(Functor)>;
            return details::ForEach( Query, Functor, reinterpret_cast< typename function_t::args_t* >(nullptr) );
        }
    }

    //---------------------------------------------------------------------------------
    // WORLD
    //---------------------------------------------------------------------------------
    namespace world
    {
        namespace details
        {
            struct component_desc
            {
                using fn_construct = void(void*);
                using fn_destruct  = void(void*);

                type_guid       m_Guid;
                fn_construct&   m_Construct;
                fn_destruct&    m_Destruct;
                std::uint16_t   m_Size;
                bool            m_isDoubleBuffer;
            };

            struct dbase
            {
                struct tag_entry
                {
                    struct group_entry
                    {
                        query::filter_bits                          m_Bits;
                        component::group::instance*                 m_pGroup{nullptr};
                    };

                    type_guid                                                   m_Guid;         // Tag UID (Sum of all individual tags)
                    query::filter_bits                                          m_Bits;         // Tag bitsets (Unique bit set for each tag)
                    std::uint32_t                                               m_nGroups;      
                    xcore::array<group_entry,32>                                m_lGroups;
                    std::uint32_t                                               m_nTags;      
                    xcore::array<type_guid,settings::max_tags_in_entity_v>      m_lTagGuids;    // Individual tag guids that make this entry
                };

                std::uint32_t                                       m_nTagEntries;
                xcore::array<tag_entry,128>                         m_lTagEntries;
            };

            struct tag_lookup
            {
                dbase::tag_entry*   m_pEntry;
                std::uint8_t        m_BitIndex;
            };

            #define CHECK_FUNCTION( FUNCTION_NAME )                                         \
            template<typename, typename T>                                                  \
            struct has_##FUNCTION_NAME                                                      \
            {                                                                               \
                static_assert( std::integral_constant<T, false>::value,                     \
                    "Second template parameter needs to be of function type.");             \
            };                                                                              \
            /* specialization that does the checking */                                     \
            template<typename T_CLASS, typename Ret, typename... Args>                      \
            struct has_##FUNCTION_NAME <T_CLASS, Ret(Args...)>                              \
            {                                                                               \
                template<typename T>                                                        \
                static constexpr auto check(T*) -> typename std::is_same                    \
                <                                                                           \
                    decltype(std::declval<T>().FUNCTION_NAME(std::declval<Args>()...))      \
                    , Ret /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   */    \
                >::type;  /* attempt to call it and see if the return type is correct */    \
                template<typename>                                                          \
                static constexpr std::false_type check(...);                                \
                using type = decltype(check<T_CLASS>(nullptr));                             \
                static constexpr bool value = type::value;                                  \
            };                                                                              \
            template<typename T, typename...T_ARGS>                                         \
            std::enable_if_t<true == has_##FUNCTION_NAME<T, void(T_ARGS...)>::value, void > \
            Call##FUNCTION_NAME(T& A, T_ARGS...Args) noexcept { A.FUNCTION_NAME(Args...); } \
            template<typename T, typename...T_ARGS>                                         \
            std::enable_if_t<false == has_##FUNCTION_NAME<T, void(T_ARGS...)>::value, void >\
            Call##FUNCTION_NAME(T&, T_ARGS...Args) noexcept {}                          

            CHECK_FUNCTION(Update)                          // W/R Components
          //  CHECK_FUNCTION(ExclusiveUpdate)                 // W/R Components + add/remove components
          //  CHECK_FUNCTION(BarrierUpdate)

            #undef CHECK_FUNCTION
        }

        //---------------------------------------------------------------------------------
        // WORLD::time
        //---------------------------------------------------------------------------------
        struct time
        {
            constexpr static float fix_step = 16/1000.0f;

            float m_DeltaTime           = 0;
            float m_FixDeltaTime        = 0;
            float m_FixTotalDeltaTime   = 0;
            float m_FixAccumulation     = 0;
            int   m_FixSteps            = 0;

            std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
            void clear(void) noexcept 
            {
                m_DeltaTime         = 0;
                m_FixSteps          = 1;
                m_FixAccumulation   = 0;
            }

            void Start(void) noexcept
            {
                m_Start               = std::chrono::high_resolution_clock::now();
                m_DeltaTime           = 0;
                m_FixDeltaTime        = 0;
                m_FixTotalDeltaTime   = 0;
                m_FixAccumulation     = 0;
                m_FixSteps            = 0;
            }

            void UpdateDeltaTimeRoundTrip(void) noexcept
            {
                const auto                      End  = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double>   Diff = End - m_Start;

                m_Start             = End;
                m_DeltaTime         = std::chrono::duration_cast<std::chrono::duration<float>>(Diff).count();
                m_FixDeltaTime      = settings::fixed_deltatime_size_v;
                m_FixAccumulation  += m_DeltaTime;
                m_FixSteps          = 0;

                while( m_FixAccumulation >= settings::fixed_deltatime_size_v )
                {
                    m_FixSteps++;
                    m_FixAccumulation -= settings::fixed_deltatime_size_v;
                }

                constexpr static auto max_steps = 5;
                if( m_FixSteps >= max_steps ) 
                {
                    m_FixSteps          = max_steps;
                    m_FixTotalDeltaTime = m_DeltaTime = settings::fixed_deltatime_size_v * m_FixSteps;
                    m_FixAccumulation   = 0;
                }
                else
                {
                    m_FixTotalDeltaTime = m_FixSteps * settings::fixed_deltatime_size_v;
                }
            }
        };

        //---------------------------------------------------------------------------------
        // WORLD::events
        //---------------------------------------------------------------------------------
        struct events
        {
            using world_start   = xcore::types::make_unique< xcore::event::type<>, struct world_start_tag >;
            using frame_start   = xcore::types::make_unique< xcore::event::type<>, struct frame_start_tag >;
            using frame_done    = xcore::types::make_unique< xcore::event::type<>, struct frame_done_tag >;

            world_start m_WorldStart;
            frame_start m_FrameStart;
            frame_done  m_FrameDone;
        };

        //---------------------------------------------------------------------------------
        // WORLD::instance
        //---------------------------------------------------------------------------------
        struct instance : xcore::scheduler::job<1>
        {
            sync_point::instance    m_StartSyncPoint;
            sync_point::instance    m_EndSyncPoint;

            using events = mecs::world::events;

            void msgSyncPointDone( sync_point::instance& SyncPoint )
            {
                auto& Scheduler = xcore::get().m_Scheduler;
                EndFrameUpdate();
                Scheduler.MainThreadStopWorking();
            }

            void EndFrameUpdate( void )
            {
                XCORE_PERF_ZONE_SCOPED()

                m_bFrameStarted = false;

                //
                // Clean up all deleted entities
                //
                for( auto pNext = m_EntityFreeList.load(std::memory_order_relaxed); pNext; pNext = reinterpret_cast<entity::map_t::entry*>(pNext->m_Value.m_pGroupInstance))
                {
                    m_mapEntity.free( pNext->m_Key, [](auto&){} );
                }
                m_EntityFreeList.store( nullptr, std::memory_order_relaxed );

                //
                // Let everyone know that we are done
                //
                m_Events.m_FrameDone.NotifyAll();

                //
                // Deal with profiler
                //
                XCORE_PERF_FRAME_MARK_END("mecs::Frame")
                XCORE_PERF_PLOT("Total Pages", static_cast<int64_t>(m_PageMgr.m_TotalPages.load(std::memory_order_relaxed)) )
                XCORE_PERF_PLOT("Free Pages",  static_cast<int64_t>(m_PageMgr.m_FreePages.load(std::memory_order_relaxed)) )
            }

            const sync_point::events::done::delegate m_SyncPointDoneDelegate{ this, &instance::msgSyncPointDone, m_EndSyncPoint.m_Events.m_Done };

            instance()
            {
                m_Definition.m_Triggers = xcore::scheduler::triggers::DONT_CLEAN_COUNT;
                registerComponents<entity::instance>();
                m_EndSyncPoint.AddJobToBeTrigger(*this);
                m_EndSyncPoint.FreezCount();
                m_EndSyncPoint.m_bDisable = true;

                m_StartSyncPoint.JobWillNotifyMe(*this);
                m_StartSyncPoint.FreezCount();
            }

            virtual void qt_onRun( void ) noexcept override
            {
                XCORE_PERF_FRAME_MARK_START("mecs::Frame")
                m_bFrameStarted = true;
                m_FrameNumber++;
                m_Events.m_FrameStart.NotifyAll();
            }

            void Start( void )
            {
                //
                // Make sure that the scene is ready to go
                //
                for( const auto& TagE : m_TagDBase.m_lTagEntries.ViewTo(m_TagDBase.m_nTagEntries) )
                    for( auto& GroupE : TagE.m_lGroups.ViewTo(TagE.m_nGroups) )
                    {
                        GroupE.m_pGroup->m_Count = GroupE.m_pGroup->m_NewCount.load(std::memory_order_relaxed);
                        GroupE.m_pGroup->m_NewCount.store( 0, std::memory_order_relaxed );
                    }

                //
                // Notify All
                //
                m_Events.m_WorldStart.NotifyAll();

                //
                // Begin Time
                //
                m_Time.Start();

                //
                // Get started
                //
                XCORE_PERF_FRAME_MARK()
                auto& Scheduler = xcore::get().m_Scheduler;
                Scheduler.AddJobToQuantumWorld(*this);
                Scheduler.MainThreadStartsWorking();
            }

            void Resume( void )
            {
                //
                // Update Time
                //
                m_Time.UpdateDeltaTimeRoundTrip();

                //
                // Get started
                //
                XCORE_PERF_FRAME_MARK()
                auto& Scheduler = xcore::get().m_Scheduler;
                Scheduler.AddJobToQuantumWorld(*this);
                Scheduler.MainThreadStartsWorking();
            }

            void Stop( void )
            {
            }

            xforceinline
            void createEntity( component::group::instance& Group, xcore::function::view<void(entity::tmp_ref)> UserFunction, entity::guid EntityGuid = mecs::entity::guid{xcore::not_null} ) noexcept
            {
                entity::tmp_ref Ref;
                m_mapEntity.alloc( EntityGuid, [&]( entity::map_t::entry& E )
                {
                    Ref = E.m_Value = Group.AllocEntity(&E);
                });

                // Let the user do whatever he needs to do
                UserFunction(Ref);

                // Notify anyone it needs to know that there is a new entity
                if( Group.m_Events.m_CreatedEntity.hasSubscribers() )
                    Group.m_Events.m_CreatedEntity.NotifyAll(Ref);
            }

            component::group::instance& getOrCreateGroup( const std::span<const type_guid>    lComponentTypesGuids
                                                        , const std::span<const type_guid>    lTagTypesGuids ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED()

                bool                bHasEntity          = false;
                const auto          ComponentGroupGuid  = component::group::guid{ [&] 
                {
                    std::uint64_t Key = 0; 
                    for (auto E : lComponentTypesGuids) 
                    {
                        xassert( E != entity::instance::type_guid_v );
                        Key += E.m_Value;
                    }
                    return Key + entity::instance::type_guid_v.m_Value; 
                 }() };
                const auto          TagGuid             = type_guid{ [&] {std::uint64_t Key = 0; for (auto E : lTagTypesGuids) Key += E.m_Value; return Key + entity::instance::type_guid_v.m_Value; }() };
                const auto          TagGroupGuid        = component::group::guid{ ComponentGroupGuid.m_Value + TagGuid.m_Value };
                auto&               TagEntry            = *m_mapTagLookup.getOrCreate( TagGuid,[&]() constexpr noexcept
                {
                    auto& Entry = m_TagDBase.m_lTagEntries[ m_TagDBase.m_nTagEntries++ ];
                    Entry.m_Guid      = TagGuid;
                    Entry.m_nGroups   = 0;
                    Entry.m_nTags     = 0;

                    for( auto E : lTagTypesGuids )
                    {
                        Entry.m_Bits.AddBit( m_mapTagDescriptor.get(E) );
                        Entry.m_lTagGuids[ Entry.m_nTags++ ] = E;
                    }

                    return &Entry;
                });

                return *m_mapGroupPerTag.getOrCreate( TagGroupGuid, [&]
                {
                    //
                    // Create a new group if we need to
                    //
                    //                             XCORE_PERF_ZONE_SCOPED_N("getOrCreateGroup::Wasting Time")
                    auto&               GroupEntry          = *new( &TagEntry.m_lGroups[TagEntry.m_nGroups++] ) details::dbase::tag_entry::group_entry {};
                    auto                nComponents         = lComponentTypesGuids.size();
                    
                    xcore::array<const component::descriptor*, settings::max_components_in_entity_v> lComponetDesc;
                    for( int i=0; i<nComponents; ++i )
                    {
                        auto& C = lComponentTypesGuids[i];
                        xassert(C.m_Value);

                        // Collect all the component descriptors
                        lComponetDesc[i] = m_mapComponentDescriptor.get(C);

                        // Make sure we set all the bits for the query
                        GroupEntry.m_Bits.AddBit(lComponetDesc[i]->m_BitNumber);
                    }

                    // the entity if we have to
                    if(bHasEntity == false)
                    {
                        lComponetDesc[nComponents] = m_mapComponentDescriptor.get(entity::instance::type_guid_v);
                        GroupEntry.m_Bits.AddBit(lComponetDesc[nComponents]->m_BitNumber);
                        nComponents++;
                    }

                    // Make sure that we store data in the ascending order
                    std::sort(  lComponetDesc.begin()
                            ,   lComponetDesc.begin() + nComponents
                            ,   [](const component::descriptor* pA, const component::descriptor* pB) { return pA->m_Guid.m_Value < pB->m_Guid.m_Value;} 
                    );

                    auto pGroupDesc = m_mapGroupDescriptor.getOrCreate
                    ( 
                        ComponentGroupGuid
                        , [&]
                        {
                            return new component::group::descriptor
                            {
                                lComponetDesc.ViewTo(nComponents)
                            };
                        }
                    );

                    GroupEntry.m_pGroup = new component::group::instance
                    { 
                          TagGroupGuid
                        , m_PageMgr
                        , *pGroupDesc
                    };

                    GroupEntry.m_pGroup->m_pDataBaseRef = &TagEntry;

                    return std::unique_ptr<component::group::instance>{ GroupEntry.m_pGroup };
                });
            }

            template< typename...T_COMPONENTS_AND_TAGS >
            xforceinline component::group::instance& getOrCreateGroup( void ) noexcept
            {
                constexpr static component::group::details::guid_arrays_from_types_with_duplication<T_COMPONENTS_AND_TAGS...> arrays_v;
                return getOrCreateGroup( arrays_v.component_guid_list_v, arrays_v.tag_guid_list_v );
            }

            template< typename T_COMPONENTS_AND_TAGS >
            inline void registerComponent(void) noexcept
            {
                if constexpr ( std::is_base_of_v<mecs::component::tag, T_COMPONENTS_AND_TAGS> )
                {
                    static_assert( sizeof(T_COMPONENTS_AND_TAGS)==1, "Tags should not contain any data" );
                    m_mapTagDescriptor.alloc( T_COMPONENTS_AND_TAGS::type_guid_v, [&]( auto& E )
                    {
                        E.m_Value = m_TagRegistrationFreeBits.AllocBit();
                    });
                }
                else
                {
                    constexpr static component::descriptor component_descriptor_v
                    {
                            T_COMPONENTS_AND_TAGS::type_guid_v
                        ,   [](void* p) noexcept { new(p) T_COMPONENTS_AND_TAGS{}; }
                        ,   [](void* p) noexcept { std::destroy_at( reinterpret_cast<T_COMPONENTS_AND_TAGS*>(p) ); }
                        ,   [](void* d, void* s) noexcept { *reinterpret_cast<T_COMPONENTS_AND_TAGS*>(d) = std::move(*reinterpret_cast<T_COMPONENTS_AND_TAGS*>(s)); }
                            // //property::table&  m_PropertyTable;
                        ,   static_cast<std::uint16_t>(sizeof(T_COMPONENTS_AND_TAGS))
                        ,   static_cast<std::uint16_t>(std::alignment_of_v<T_COMPONENTS_AND_TAGS>)
                        ,   std::is_base_of_v<mecs::component::double_buffer, T_COMPONENTS_AND_TAGS>
                        ,   -1
                    };

                    component_descriptor_v.m_BitNumber = m_ComponentRegistrationFreeBits.AllocBit();
                    m_mapComponentDescriptor.alloc(T_COMPONENTS_AND_TAGS::type_guid_v, [&]( auto& E )
                    {
                        E.m_Value = &component_descriptor_v;
                    });
                }
            }
 
            template< typename...T_COMPONENTS_AND_TAGS >
            inline void registerComponents(void) noexcept
            {
                (registerComponent<T_COMPONENTS_AND_TAGS>(), ... );
            }

            template< typename T_SYSTEM, typename...T_ARGS >
            T_SYSTEM& AddGraphConnection(sync_point::instance& StartSynpoint, T_ARGS&&... EndSyncPoints) noexcept
            {
                static_assert(sizeof...(EndSyncPoints) > 0, "You must pass at least one end Sync_point");
                static_assert(std::is_base_of_v<sync_point::instance, std::common_type_t<T_ARGS...>>, "All the end sync_points must be non const sync_points");
            
                using custom_system_t  = system::details::custom_system<T_SYSTEM>;
                auto pSystem = new custom_system_t{*this};

                // Add all end triggers
                ( EndSyncPoints.JobWillNotifyMe(*pSystem), ... );
                ( EndSyncPoints.FreezCount(), ... );

                // Set the start trigger
                StartSynpoint.AddJobToBeTrigger(*pSystem);
                StartSynpoint.FreezCount();

                //
                // Register to SyncPoint messages as needed
                //

                // This message is a must have since we need to deal with the memory barrier
                pSystem->m_SyncPointDoneDelegate.Connect( xcore::types::PickFirstArgument( std::forward<T_ARGS>(EndSyncPoints) ... ).m_Events.m_Done );

                if constexpr ( &custom_system_t::msgSyncPointStart != &system::instance::msgSyncPointStart )
                {
                    pSystem->m_WorldStartDelegate.setupThis( *pSystem );
                    pSystem->m_SyncPointStartDelegate.Connect( StartSynpoint.m_Events.m_Start );
                }

                //
                // Register to World Instances messages
                //
                if constexpr ( &custom_system_t::msgWorldStart != &system::instance::msgWorldStart )
                {
                    pSystem->m_WorldStartDelegate.setupThis( *pSystem );
                    pSystem->m_WorldStartDelegate.Connect( m_Events.m_WorldStart );
                }

                if constexpr ( &custom_system_t::msgFrameStart != &system::instance::msgFrameStart )
                {
                    pSystem->m_FrameStartDelegate.setupThis( *pSystem );
                    pSystem->m_FrameStartDelegate.Connect( m_Events.m_FrameStart );
                }

                if constexpr ( &custom_system_t::msgFrameDone != &system::instance::msgFrameDone )
                {
                    pSystem->m_FrameEndDelegate.setupThis( *pSystem );
                    pSystem->m_FrameEndDelegate.Connect( m_Events.m_FrameDone );
                }

                return *pSystem;
            }

            template<typename T >
            void DoQuery( query::instance& Instance, T&& Functor, const query::define_data& Defined ) const noexcept
            {
                XCORE_PERF_ZONE_SCOPED()
                using function  = query::system_function<decltype(Functor)>;

                if( Instance.m_isInitialized == false )
                {
                    Instance.m_isInitialized = true;

                    //
                    // Handle the Component Side
                    //
                    for( auto E : Defined.m_ComponentQuery.m_All )
                        Instance.m_ComponentQuery.m_All.AddBit( m_mapComponentDescriptor.get(E)->m_BitNumber );

                    for( auto E : Defined.m_ComponentQuery.m_Any )
                        Instance.m_ComponentQuery.m_Any.AddBit( m_mapComponentDescriptor.get(E)->m_BitNumber );

                    for( auto E : Defined.m_ComponentQuery.m_None )
                        Instance.m_ComponentQuery.m_None.AddBit( m_mapComponentDescriptor.get(E)->m_BitNumber );

                    for( int i=0; i<function::list_v.size(); ++i )
                    {
                        const auto BitNum = m_mapComponentDescriptor.get(function::list_v[i])->m_BitNumber;
                        Instance.m_Function.AddBit( BitNum );
                        if( function::const_list_v[i] == false ) Instance.m_Write.AddBit(BitNum);
                    }

                    // Function argument are a must have
                    Instance.m_ComponentQuery.m_All.Add(Instance.m_Function);

                    //
                    // Handle the tag side
                    //
                    for( auto E : Defined.m_TagQuery.m_All )
                        Instance.m_TagQuery.m_All.AddBit( m_mapTagDescriptor.get(E) );

                    for( auto E : Defined.m_TagQuery.m_Any )
                        Instance.m_TagQuery.m_Any.AddBit( m_mapTagDescriptor.get(E) );

                    for( auto E : Defined.m_TagQuery.m_None )
                        Instance.m_TagQuery.m_None.AddBit( m_mapTagDescriptor.get(E) );
                }
                
                //
                // Do query
                //
                Instance.m_lResults.clear();
                for( const auto& TagE : m_TagDBase.m_lTagEntries.ViewTo(m_TagDBase.m_nTagEntries) )
                {
                    if( false == TagE.m_Bits.Query( Instance.m_TagQuery.m_All, Instance.m_TagQuery.m_Any, Instance.m_TagQuery.m_None ) ) continue;

                    for( auto& GroupE : TagE.m_lGroups.ViewTo(TagE.m_nGroups) )
                    {
                        if( false == GroupE.m_Bits.Query( Instance.m_ComponentQuery.m_All, Instance.m_ComponentQuery.m_Any, Instance.m_ComponentQuery.m_None ) ) continue;

                        //
                        // Create new entry for this query
                        //
                        auto&       Entry       = Instance.m_lResults.append();
                        Entry.m_pGroup          = const_cast<component::group::instance*>( GroupE.m_pGroup);

                        const auto& Group       = *GroupE.m_pGroup;
                        const auto& lOffsets    = Group.m_GroupDescriptor.m_lComponentOffsets;
                        const auto& lCTypeGuid  = Group.m_GroupDescriptor.m_lComponentTypeGuids;
                        const int   Size        = Group.m_GroupDescriptor.m_nComponents;
                        int         F           = 0;
                        for( int i=0; i<Size; ++i)
                        {
                            if( lCTypeGuid[i] == function::sorted_list_v[F] )
                            {
                                // Are we a double buffer component?
                                if( lOffsets[ i ].m_HasT1 )
                                {
                                    // First find the base for this double buffer component
                                    int iBase = (i > 0)? (lCTypeGuid[i-1] == function::sorted_list_v[F]? (i-1) : i) : 0;

                                    // Are we looking for T0?
                                    if( function::const_list_v[F] ) iBase += lOffsets[ iBase ].m_T0;
                                    else                            iBase += ( 1 - lOffsets[ iBase ].m_T0);

                                    // Okay ready to set the offsets now
                                    Entry.m_lRemaps[F + function::remap_v[F]]     = lOffsets[ iBase ].m_Offset;
                                    Entry.m_lLockRemaps[F + function::remap_v[F]] = iBase;
                                }
                                else
                                {
                                    Entry.m_lRemaps[F + function::remap_v[F]]     = lOffsets[ i ].m_Offset;
                                    Entry.m_lLockRemaps[F + function::remap_v[F]] = F;
                                }

                                Entry.m_isWrite = function::const_list_v;

                                // Move to next component
                                F++;
                                if(F==function::list_v.size()) break;
                            }
                        }
                        xassert(F==function::list_v.size());
                        Entry.m_nRemaps = static_cast<std::uint8_t>(function::list_v.size()); 
                    }
                }
            }

            void deleteEntity( entity::instance& Entity ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED()
                if( Entity.isZombie() ) return;

                auto& Node   = *Entity.m_pInstance;
                auto  TmpRef =  Node.m_Value;

                // Mark for deletion in the group
                TmpRef.m_pGroupInstance->DeleteEntity( Entity );

                //
                // Notify that we are deleting an entity
                //
                auto& Group = *TmpRef.m_pGroupInstance;
                if( Group.m_Events.m_DeletedEntity.hasSubscribers() )
                    Group.m_Events.m_DeletedEntity.NotifyAll(TmpRef);

                //
                // Add entity into list to be completely deleted at the end of the frame
                //
                {
                    auto  pHead = m_EntityFreeList.load(std::memory_order_relaxed);
                    auto& pNext = reinterpret_cast<entity::map_t::entry*&>( Node.m_Value.m_pGroupInstance );

                    pNext = pHead;
                    while( false == m_EntityFreeList.compare_exchange_weak( pNext, &Node) );
                }

                //
                // Mark entity as officially deleted
                //

                // Make sure we mark the tmp_ref as invalid
                Node.m_Value.clear();
            }

            template< typename...T_ADD_COMPONENTS_OR_TAGS, typename...T_REMOVE_COMPONENTS_OR_TAGS >
            component::group::instance& getOrCreateGroupBy( 
                    component::group::instance&                 Group
                ,   std::tuple<T_ADD_COMPONENTS_OR_TAGS...>*
                ,   std::tuple<T_REMOVE_COMPONENTS_OR_TAGS...>*
                ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED()
                constexpr static auto ModifyComponentList = []( 
                        std::span<type_guid>        OutputList
                    ,   std::span<const type_guid>  CurrentList
                    ,   std::span<const type_guid>  AddList
                    ,   std::span<const type_guid>  RemoveList ) constexpr noexcept -> int
                {
                    int nComponents = 0;
                    for( int i=0, a=0 ;;)
                    {
                        if( a == AddList.size() )
                        {
                            while( i < CurrentList.size() )
                                OutputList[nComponents++] = CurrentList[i++];
                            break;
                        }
                        else if( i == CurrentList.size() )
                        {
                            while( a < AddList.size() )
                                OutputList[nComponents++] = AddList[a++];
                            break;
                        }

                        if( CurrentList[i] > AddList[a] )
                        {
                            OutputList[nComponents++] = AddList[a++];
                        }
                        else
                        {
                            if( CurrentList[i] == AddList[a] )
                            {
                                //TODO: WARNING User is trying to add a duplicate component. We should issue a warning
                                i++;
                            }
                            else
                            {
                                OutputList[nComponents++] = CurrentList[i++];
                            }
                        }
                    }

                    // Delete components
                    if( RemoveList.size() )
                    {
                        int n=0;
                        for( int i=0, r=0;;  )
                        {
                            if( r == RemoveList.size() )
                            {
                                while( i<nComponents )
                                    OutputList[n++] = OutputList[i++];
                                break;
                            }
                            else if( i == (nComponents - r) ) break;

                            if( CurrentList[i] > RemoveList[r] ) 
                            { 
                                ++r;
                            }
                            else 
                            {
                                if( CurrentList[i] < RemoveList[r] ) OutputList[n++] = OutputList[i];
                                ++i;
                            }
                        }
                        nComponents = n;
                    }

                    return nComponents;
                };

                constexpr static component::group::details::guid_arrays_from_types_with_duplication<T_ADD_COMPONENTS_OR_TAGS...>    add_v;
                constexpr static component::group::details::guid_arrays_from_types_with_duplication<T_REMOVE_COMPONENTS_OR_TAGS...> remove_v;

                auto&   GroupDescriptor          = Group.m_GroupDescriptor;
                auto&   CurrentTagEntry          = *reinterpret_cast<details::dbase::tag_entry*>(Group.m_pDataBaseRef);

                std::array<type_guid, settings::max_components_in_entity_v >    Components;
                std::array<type_guid, settings::max_tags_in_entity_v >          Tags;

                const int nComponents = ModifyComponentList(    Components
                                                            ,   std::span{ std::as_const(GroupDescriptor.m_lComponentTypeGuids).data(), GroupDescriptor.m_nComponents }
                                                            ,   add_v.component_guid_list_v
                                                            ,   remove_v.component_guid_list_v );

                const int nTags       = ModifyComponentList(    Tags
                                                            ,   std::span{ std::as_const(CurrentTagEntry.m_lTagGuids).data(), CurrentTagEntry.m_nTags }
                                                            ,   add_v.tag_guid_list_v
                                                            ,   remove_v.tag_guid_list_v );

                // Add components
                auto& NewGroup = getOrCreateGroup( std::span<const type_guid>{Components.data()+1, static_cast<std::size_t>(nComponents-1)}
                                                 , std::span<const type_guid>{Tags.data(), static_cast<std::size_t>(nTags) } );

                if( &NewGroup == &Group )
                {
                    // TODO: WARNING, This move resulted in no operation!
                }

                return NewGroup;
            }

            template< typename T_TUPLE_ADD_COMPONENTS_OR_TAGS, typename T_TUPLE_REMOVE_COMPONENTS_OR_TAGS >
            xforceinline component::group::instance& getOrCreateGroupBy( component::group::instance& Group ) noexcept
            {
                return getOrCreateGroupBy(Group, (T_TUPLE_ADD_COMPONENTS_OR_TAGS*)nullptr, (T_TUPLE_REMOVE_COMPONENTS_OR_TAGS*)nullptr );
            }

            void moveEntityToGroup( component::group::instance& NewGroup, entity::instance& Entity, xcore::function::view<void(entity::tmp_ref)> Callback = [](entity::tmp_ref){} )
            {
                XCORE_PERF_ZONE_SCOPED()
                // This entity has been mark for deletion.... there is nothing we can accomplish here 
                xassert( Entity.isZombie() == false );

                auto  OldTmpRef = Entity.m_pInstance->m_Value;
                auto& OldGroup  = *OldTmpRef.m_pGroupInstance;

                // If we are going to move it to the same group there is nothing to do
                if( &NewGroup == &OldGroup ) return;

                // Let anyone that is interested know
                if( OldGroup.m_Events.m_MovedOutEntity.hasSubscribers() )
                    OldGroup.m_Events.m_MovedOutEntity.NotifyAll(OldTmpRef);

                auto& NewGDescriptor = NewGroup.m_GroupDescriptor;
                auto& OldGDescriptor = OldGroup.m_GroupDescriptor;

                // Allocate our new entity in the new group
                auto  NewTmpRef      = NewGroup.AllocEntity( Entity.m_pInstance );
                    
                //
                // Move/construct the components over 
                //
                NewGDescriptor.ConstructOnlyEntityComponent( NewGroup.m_lPages[NewTmpRef.m_PageIndex.m_Value], NewTmpRef.m_LocalIndex, Entity.m_pInstance );
                for( int inew=1, iold=1; inew < NewGDescriptor.m_nComponents; )
                {
                    if( iold == OldGDescriptor.m_nComponents ) 
                    {
                        do 
                        {
                            NewGDescriptor.ConstructOnlyComponent( NewGroup.m_lPages[NewTmpRef.m_PageIndex.m_Value], NewTmpRef.m_LocalIndex, inew );
                            inew++;
                        } while (inew < NewGDescriptor.m_nComponents);
                        break;
                    }

                    if( NewGDescriptor.m_lComponentTypeGuids[inew] < OldGDescriptor.m_lComponentTypeGuids[iold] )
                    {
                        // New group has a component that the old group did not so we need to construct this entry
                        NewGDescriptor.ConstructOnlyComponent( NewGroup.m_lPages[NewTmpRef.m_PageIndex.m_Value], NewTmpRef.m_LocalIndex, inew );
                        inew++;
                    }
                    else if( NewGDescriptor.m_lComponentTypeGuids[inew] > OldGDescriptor.m_lComponentTypeGuids[iold] )
                    {
                        // We are missing this component so skip it
                        iold++;
                    }
                    else
                    {
                        const auto& NewOffsets = NewGDescriptor.m_lComponentOffsets[inew];
                        const auto& OldOffsets = OldGDescriptor.m_lComponentOffsets[iold];
                        auto&       MoveFn     = NewGDescriptor.m_lComponentDescriptors[inew]->m_fnMove;
                        void*       pSrc       = &OldGroup.m_lPages[OldTmpRef.m_PageIndex.m_Value]->m_RawData[OldOffsets.m_Offset + iold * OldOffsets.m_Size ];
                        void*       pDst       = &NewGroup.m_lPages[NewTmpRef.m_PageIndex.m_Value]->m_RawData[NewOffsets.m_Offset + inew * NewOffsets.m_Size ];

                        // Move this component to new location
                        MoveFn( pDst, pSrc );

                        // Get ready for next component
                        inew++;
                        iold++;
                    }
                }

                //
                // Lets tell the system to delete the old component
                //
                OldGroup.DeleteEntity( Entity );

                //
                // Release the new entity. We must lock it before making any changes
                //
                {
                    xcore::lock::scope Lock( Entity.getPointer().m_pLock.load(std::memory_order_relaxed)->m_Lock );
                    Entity.m_pInstance->m_Value = NewTmpRef;
                }

                //
                // Let the user know that there is has been officially moved
                //
                Callback(NewTmpRef);

                if( NewGroup.m_Events.m_MovedInEntity.hasSubscribers() )
                    NewGroup.m_Events.m_MovedInEntity.NotifyAll(NewTmpRef);
            }

            ~instance()
            {
                XCORE_PERF_ZONE_SCOPED()
                
                xcore::scheduler::channel MyChannel(xconst_universal_str("World Destruction"));
                MyChannel.SubmitJob([&]{ m_mapEntity.clear(); });
                MyChannel.SubmitJob([&]{ m_mapGroupPerTag.clear(); });
                MyChannel.SubmitJob([&]{ m_mapTagLookup.clear(); });
                MyChannel.join();
                MyChannel.SubmitJob([&]{ m_mapGroupDescriptor.clear(); });
                MyChannel.SubmitJob([&]{ m_mapComponentDescriptor.clear(); });
                MyChannel.SubmitJob([&]{ m_mapTagDescriptor.clear(); });
                MyChannel.SubmitJob([&]{ m_PageMgr.clear(); });
                MyChannel.join();
            }

            constexpr static std::size_t                                max_groups_count_v = 1024;

            using map_group_per_tag_t           = tools::map_fixed<component::group::guid,  std::unique_ptr<component::group::instance>,  max_groups_count_v>;
            using map_group_descriptor_t        = tools::map_fixed<component::group::guid,  const component::group::descriptor*,          max_groups_count_v>;
            using map_component_descriptor_t    = tools::map_fixed<type_guid,               const component::descriptor*,                 256>;
            using map_tag_descriptor_t          = tools::map_fixed<type_guid,               std::uint8_t,                                 256>;
            using map_tag_lookup_t              = tools::map_fixed<type_guid,               details::dbase::tag_entry*,                   max_groups_count_v>;

            bool                                        m_bFrameStarted                 { false };
            bool                                        m_bPause                        { false };
            tools::page_mgr                             m_PageMgr                       {};
            entity::map_t                               m_mapEntity                     {};
            details::dbase                              m_TagDBase                      {};
            map_group_per_tag_t                         m_mapGroupPerTag                {};
            map_group_descriptor_t                      m_mapGroupDescriptor            {};
            map_component_descriptor_t                  m_mapComponentDescriptor        {};
            map_tag_descriptor_t                        m_mapTagDescriptor              {};
            map_tag_lookup_t                            m_mapTagLookup                  {};
            query::filter_bits                          m_ComponentRegistrationFreeBits {xcore::not_null};
            query::filter_bits                          m_TagRegistrationFreeBits       {xcore::not_null};
            std::atomic<entity::map_t::entry*>          m_EntityFreeList                {nullptr};
            std::uint64_t                               m_FrameNumber                   {0};
            time                                        m_Time                          {};
            events                                      m_Events                        {};
        };
    }

    //------------------------------------------------------------------------------------
    // ENTITY:FUNCTIONS
    //------------------------------------------------------------------------------------
    namespace entity
    {
        template< typename T >
        T& tmp_ref::getComponent(type_guid TypeGuid) noexcept
        {
            auto p = reinterpret_cast<T*>(m_pGroupInstance->findComponent(*this, TypeGuid));
            assert(p);
            return *p;
        }

        template< typename T >
        constexpr const T& tmp_ref::getComponent(type_guid TypeGuid) const noexcept
        {
            auto p = reinterpret_cast<T*>(m_pGroupInstance->findComponent(*this, TypeGuid));
            assert(p);
            return *p;
        }
    }
}
