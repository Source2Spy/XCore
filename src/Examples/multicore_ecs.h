
namespace mecs
{
    //---------------------------------------------------------------------------------
    // TOOLS
    //---------------------------------------------------------------------------------
    namespace tools
    {
        //---------------------------------------------------------------------------------
        // TOOLS::map_fixed
        //---------------------------------------------------------------------------------
        template< typename T_KEY, typename T_VALUE, std::size_t T_MAXSIZE_V > 
        struct map_fixed
        {
            constexpr static auto max_size_v = T_MAXSIZE_V;

            struct entry
            {
                T_KEY               m_Key   {};
                T_VALUE             m_Value {};
                entry*              m_pNext {nullptr};        // Next for cases where collisions happen
            };
                
            struct lock
            {
                xcore::lock::semaphore  m_Lock;
                entry*                  m_pEntry{ nullptr };
            };

            map_fixed()
            {
                for( int i=1; i<m_Entries.size(); ++i )
                {
                    m_Entries[i-1].m_pNext = &m_Entries[i];
                }

                m_Entries.back().m_pNext = nullptr;
                m_pFreeNodes.store( &m_Entries[0], std::memory_order_relaxed );
            }

            template< typename T_CALLBACK >
            auto& push( T_KEY Key, T_CALLBACK&& Callback ) noexcept
            {
                assert(Key.isValid());

                // Allocate an overflow entry
                auto pNode = m_pFreeNodes.load(std::memory_order_relaxed);
                do 
                {
                    // Run out of free entries!
                    xassert(pNode);

                    if( m_pFreeNodes.compare_exchange_weak( pNode, pNode->m_pNext ) )
                        break;
                } while ( true );

                // Lock our entry
                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope( MapEntry.m_Lock );

                // Do we have a collision? 
                // Make sure we have not collisions with any one
                xassert_block_basic()
                {
                    for( auto p = MapEntry.m_pEntry; p ; p = p->m_pNext )
                    {
                        xassert( Key.m_Value != p->m_Key.m_Value );
                    }
                }

                // Setup the node
                pNode->m_Key        = Key;
                pNode->m_pNext      = MapEntry.m_pEntry;
                Callback(*pNode);           // should set ( pNode->m_Value = ... )
                MapEntry.m_pEntry   = pNode;

                return *pNode;
            }

            template< typename T > 
            const bool find( T_KEY Key, T&& CallBack ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope( const_cast<const lock&>( MapEntry.m_Lock ) );

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

            template< typename T > 
            const bool pop( T_KEY Key, T&& CallBack ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                xcore::lock::scope( const_cast<const lock&>( MapEntry.m_Lock ) );

                xassert( MapEntry.m_pEntry );
                for( auto p = &MapEntry.m_pEntry; (**p).m_pNext; p = &p->m_pNext )
                {
                    if( (**p).m_Key == Key ) 
                    {
                        *p = (**p).m_pNext;
                        CallBack( **p );
                        return true;
                    }
                }

                return false;
            }

            std::atomic<entry*>                         m_pFreeNodes    { nullptr };        // Link list of overflow entries
            xcore::unique_span<lock>                    m_Map           { T_MAXSIZE_V };
            xcore::unique_span<entry>                   m_Entries       { T_MAXSIZE_V };    // 10% overflow
        };

        //---------------------------------------------------------------------------------
        // TOOLS::map
        //---------------------------------------------------------------------------------
        template< typename T_KEY, typename T_VALUE, std::size_t T_MAXSIZE_V > 
        struct map
        {
            constexpr static auto max_size_v = T_MAXSIZE_V;

            map()
            {
                for( int i=1; i<m_Extras.size(); ++i )
                {
                    m_Extras[i-1].m_pNext = &m_Extras[i];
                }

                m_Extras.back().m_pNext = nullptr;
                m_pFreeOverflow.store( &m_Extras[0] );
            }

            auto& push( T_KEY Key, T_VALUE&& Data ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                auto            Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);

                //
                // Lock
                //
                do 
                {
                    if( Local.m_isLocked == true ) 
                    {
                        // Spin....
                        Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);
                        continue;
                    }

                    if( MapEntry.m_Lock.compare_exchange_weak(Local, lock{ true, Local.m_isValid } ) )
                        break;

                } while( true );

                //
                // Set new data
                //

                // Do we have a collision? 
                if( Local.m_isValid )
                {
                    // Make sure we have not collisions with any one
                    xassert_block_basic()
                    {
                        xassert( Key.m_Value != MapEntry.m_Key.m_Value );
                        for( auto p = MapEntry.m_pNext; p ; p = p->m_pNext )
                        {
                            xassert( Key.m_Value != MapEntry.m_Key.m_Value );
                        }
                    }

                    // Allocate an overflow entry
                    auto pNode = m_pFreeOverflow.load(std::memory_order_relaxed);
                    do 
                    {
                        // Run out of free entries!
                        xassert(pNode);

                        if( m_pFreeOverflow.compare_exchange_weak( pNode, pNode->m_pNext ) )
                            break;
                    } while ( true );

                    // Link new node
                    pNode->m_pNext = MapEntry.m_pNext;
                    MapEntry.m_pNext = pNode;

                    // Copy data
                    pNode->m_Key    = Key;
                    pNode->m_Value  = std::move(Data);
                }
                else
                {
                    MapEntry.m_Key   = Key;
                    MapEntry.m_Value = std::move(Data);
                }

                //
                // Unlock
                //
                MapEntry.m_Lock.store( lock{ false, true } );

                return MapEntry;
            }

            template< typename T > 
            const T_VALUE& getOrCreate( T_KEY Key, T&& SafeCreateCallBack ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                auto            Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);

                //
                // Lock
                //
                do 
                {
                    if( Local.m_isLocked == true ) 
                    {
                        // Spin....
                        Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);
                        continue;
                    }

                    if( MapEntry.m_Lock.compare_exchange_weak(Local, lock{ true, Local.m_isValid } ) )
                        break;

                } while( true );

                //
                // Search for the node
                //
                T_VALUE*      pResult     = nullptr;

                if( Local.m_isValid )
                {
                    if( Key.m_Value == MapEntry.m_Key.m_Value )
                    {
                        pResult = &MapEntry.m_Value;
                    }
                    else
                    {
                        for( auto p = MapEntry.m_pNext; p; p = p->m_pNext )
                        {
                            if( Key.m_Value == p->m_Key.m_Value ) 
                            {
                                pResult = &p->m_Value;
                                break;
                            }

                        }
                    }
                }

                if( pResult == nullptr )
                {
                    // Do we have a collision? 
                    if( Local.m_isValid )
                    {
                        // Make sure we have not collisions with any one
                        xassert_block_basic()
                        {
                            xassert( Key.m_Value != MapEntry.m_Key.m_Value );
                            for( auto p = MapEntry.m_pNext; p ; p = p->m_pNext )
                            {
                                xassert( Key.m_Value != MapEntry.m_Key.m_Value );
                            }
                        }

                        // Allocate an overflow entry
                        auto pNode = m_pFreeOverflow.load(std::memory_order_relaxed);
                        do 
                        {
                            // Run out of free entries!
                            xassert(pNode);

                            if( m_pFreeOverflow.compare_exchange_weak( pNode, pNode->m_pNext ) )
                                break;
                        } while ( true );

                        // Link new node
                        pNode->m_pNext = MapEntry.m_pNext;
                        MapEntry.m_pNext = pNode;

                        // Copy data
                        pNode->m_Key   = Key;
                        pNode->m_Value = SafeCreateCallBack();

                        pResult = &pNode->m_Value;
                    }
                    else
                    {
                        MapEntry.m_Key   = Key;
                        MapEntry.m_Value = SafeCreateCallBack();

                        pResult = &MapEntry.m_Value;
                    }
                }

                //
                // Unlock
                //
                MapEntry.m_Lock.store( lock{ false, true } );

                return *pResult;
            }

            template< typename T_CALLBACK >
            void pop( T_KEY Key, T_CALLBACK&& CallBack = [](T_VALUE&){} ) noexcept
            {
                assert(Key.isValid());

                auto&           MapEntry    = m_Map[Key.m_Value % max_size_v];
                auto            Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);

                //
                // Lock
                //
                do 
                {
                    if( Local.m_isLocked == true ) 
                    {
                        // Spin....
                        Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);
                        continue;
                    }

                    if( MapEntry.m_Lock.compare_exchange_weak(Local, lock{ true, Local.m_isValid } ) )
                        break;

                } while( true );

                //
                // Remove node
                //

                // Better have that node
                xassert(Local.m_isValid);
                entry* pCleanUp = nullptr;

                if( Key.m_Value == MapEntry.m_Key.m_Value  )
                {
                    CallBack(MapEntry.m_Value);
                    if( MapEntry.m_pNext )
                    {
                        pCleanUp = MapEntry.m_pNext;

                        MapEntry.m_Key      = MapEntry.m_pNext->m_Key;
                        MapEntry.m_Value    = std::move( MapEntry.m_pNext->m_Value );
                        MapEntry.m_pNext    = MapEntry.m_pNext->m_pNext;
                    }
                    else
                    {
                        MapEntry.m_Lock.store( lock{ false, false }, std::memory_order_relaxed );
                        return;
                    }
                }
                else
                {
                    for( entry* p = &MapEntry; p->m_pNext; p = p->m_pNext )
                    {
                        if( Key.m_Value == p->m_pNext->m_Key.m_Value )
                        {
                            CallBack(p->m_pNext->m_Value);
                            pCleanUp    = p->m_pNext;
                            p->m_pNext  = p->m_pNext->m_pNext;
                            break;
                        }
                    }
                }

                if( pCleanUp )
                {
                    pCleanUp->m_pNext = m_pFreeOverflow.load(std::memory_order_relaxed);
                    while( m_pFreeOverflow.compare_exchange_weak( pCleanUp->m_pNext, pCleanUp) == false );
                }
                
                //
                // Unlock
                //
                MapEntry.m_Lock.store( lock{ false, true } );
            }

            T_VALUE* find(T_KEY Key) const noexcept
            {
                assert(Key.isValid());
                auto&   MapEntry    = m_Map[Key.m_Value % max_size_v];
                auto    Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);

                //
                // Lock
                //
                do 
                {
                    if( Local.m_isValid == false ) return nullptr; 
                    if( Local.m_isLocked == true ) 
                    {
                        // Spin....
                        Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);
                        continue;
                    }

                    if( MapEntry.m_Lock.compare_exchange_weak(Local, lock{ true, true } ) )
                        break;

                } while( true );

                //
                // Search for the node
                //
                xassert( Local.m_isValid );
                T_VALUE*      pResult     = nullptr;

                if( Key.m_Value == MapEntry.m_Key.m_Value )
                {
                    pResult = &MapEntry.m_Value;
                }
                else
                {
                    for( auto p = MapEntry.m_pNext; p; p = p->m_pNext )
                    {
                        if( Key.m_Value == p->m_Key.m_Value ) 
                        {
                            pResult = &p->m_Value;
                            break;
                        }
                    }
                }
                    
                //
                // Unlock
                //
                MapEntry.m_Lock.store( lock{ false, true } );
                    
                return pResult;
            }

            T_VALUE& get(T_KEY Key) noexcept
            {
                auto p = find(Key);
                xassert(p);
                return *p;
            }

            T_VALUE& get(T_KEY Key) const noexcept
            {
                auto p = find(Key);
                xassert(p);
                return *p;
            }

            template< typename T_CALLBACK >
            bool getExlusive( T_KEY Key, T_CALLBACK&& CallBack ) noexcept
            {
                assert(Key.isValid());
                auto&   MapEntry    = m_Map[Key.m_Value % max_size_v];
                auto    Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);

                //
                // Lock
                //
                do 
                {
                    if( Local.m_isValid == false ) return false; 
                    if( Local.m_isLocked == true ) 
                    {
                        // Spin....
                        Local       = MapEntry.m_Lock.load(std::memory_order_relaxed);
                        continue;
                    }

                    if( MapEntry.m_Lock.compare_exchange_weak(Local, lock{ true, true } ) )
                        break;

                } while( true );

                //
                // Search for the node
                //
                xassert( Local.m_isValid );
                T_VALUE*      pResult     = nullptr;

                if( Key.m_Value == MapEntry.m_Key.m_Value )
                {
                    pResult = &MapEntry.m_Value;
                }
                else
                {
                    for( auto p = MapEntry.m_pNext; p; p = p->m_pNext )
                    {
                        if( Key.m_Value == p->m_Key.m_Value ) 
                        {
                            pResult = &p->m_Value;
                            break;
                        }
                    }
                }
                    
                if( pResult )
                {
                    CallBack(*pResult);
                }

                //
                // Unlock
                //
                MapEntry.m_Lock.store( lock{ false, true } );
                    
                return !!pResult;
            }

            struct entry
            {
                T_KEY               m_Key   {};
                T_VALUE             m_Value {};
                entry*              m_pNext {nullptr};        // Next for cases where collisions happen
            };
                
            struct alignas(std::uint16_t) lock
            {
                bool            m_isLocked  { false };
                bool            m_isValid   { false };
            };

            struct lock_entry : entry
            {
                std::atomic<lock>   m_Lock{};
            };

            std::atomic<entry*>             m_pFreeOverflow { nullptr };                // Link list of overflow entries
            xcore::unique_span<lock_entry>  m_Map           { T_MAXSIZE_V };
            xcore::unique_span<entry>       m_Extras        { (T_MAXSIZE_V*10)/100 };   // 10% overflow
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

            std::atomic<next*> m_pFreeHead{ nullptr };

            page* AllocPage( void ) noexcept
            {
                next* pLocal = m_pFreeHead.load(std::memory_order_relaxed);
                do 
                {
                    if(pLocal == nullptr) return new page;
                } while( m_pFreeHead.compare_exchange_weak( pLocal, pLocal->m_pNext) == false );

                return reinterpret_cast<page*>(pLocal);
            }

            void FreePage(page* pPage) noexcept
            {
                next* p      = reinterpret_cast<next*>(pPage);
                p->m_pNext   = m_pFreeHead.load(std::memory_order_relaxed);
                while (m_pFreeHead.compare_exchange_weak(p->m_pNext, p) == false);
            }
        };

        //---------------------------------------------------------------------------------
        // TOOLS::POOL
        //---------------------------------------------------------------------------------
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

            template< typename...T_ARGS >
            T& append( T_ARGS&&... Args ) noexcept
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
                            m_lPages[NewCounts.m_nPages] = p;
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
                    else if( m_Counts.compare_exchange_weak( Counts, counts{ 0, 0, 1 } ) )
                    {
                        if constexpr (std::is_destructible_v<T>)
                        {
                            for( int i=0; i<Counts.m_nEntries; ++i )
                            {
                                std::destroy_at
                                (
                                    reinterpret_cast<T*>(&m_lPages[i/entries_per_page_v]->m_RawData[i%entries_per_page_v])
                                );
                            }                        
                        }

                        for( int i=0; i<Counts.m_Pages; ++i )
                        {
                            m_PageMgr.FreePage(m_lPages[i]);
                        }

                        m_Counts.store( counts{ 0, 0, 0 } );
                    }

                    // Spin Lock...
                } while (true);

                xassert(false);            
            }

            std::atomic<counts>             m_Counts    {counts{0,0,0}};
            std::array<page*, MAX_PAGES>    m_lPages    {};
            page_mgr&                       m_PageMgr;
        };

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
                            Position    = sizeof(page*);
                            m_Position.store( Position );
                            break;
                        }
                    }

                    // See if someone has unlock us
                    Position = m_Position.load(std::memory_order_relaxed);
                }

                auto*   p           = xcore::bits::Align( &m_pPageHead->m_Data[Position], std::alignment_of_v(T) );
                if( p > &m_pPageHead->m_Data[sizeof(page)] )
                {
                    // Lock the command pool
                    if( m_Position.compare_exchange_weak( Position, 0 ) == false ) goto START;

                    // Need new page
                    auto  pNewPage = m_PageMgr.AllocPage();
                    auto& pNewNext = reinterpret_cast<page*&>(pNewPage->m_Data);
                    pNewNext    = m_pPageHead;
                    m_pPageHead = pNewPage;

                    // Allocate our entry
                    p                = xcore::bits::Align( &pNewPage->m_Data[sizeof(page*)], std::alignment_of_v(T) );
                    auto NewPosition = reinterpret_cast<std::uint32_t>( p - m_pPageHead->m_Data );

                    // Release the lock and set the new position
                    m_Position.store( NewPosition );
                }
                else
                {
                    auto NewPosition = reinterpret_cast<std::uint32_t>( p - &m_pPageHead->m_Data[Position] );
                    if( m_Position.compare_exchange_weak( Position, NewPosition ) == false ) goto START;
                }

                return *new(p) T{ std::forward<T_ARGS>(Args)... };
            }

            template< typename T, typename...T_ARGS >
            T& NewCmd( T_ARGS...Args ) noexcept
            {
                auto& Cmd       = NewAlloc<cmd>();
                auto& Requested = Cmd.m_Cmd.construct<T>( std::forward<T_ARGS>(Args)... );
                Cmd.m_pNext = m_pCmdHead.load( std::memory_order_relaxed );
                while( m_pCmdHead.compare_exchange_weak( Cmd.m_pNext, &Cmd ) );
                return Cmd.m_Cmd;
            }

            page_mgr&                           m_PageMgr;
            page*                               m_pPageHead     { nullptr };
            std::atomic<cmd*>                   m_pCmdHead      { nullptr };
            std::atomic<std::uint32_t>          m_Position      {1};                // 1 means that needs to be initialize
        };
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
    constexpr static std::size_t max_ingroup_components_v = 64;

    namespace world { struct instance; }

    //---------------------------------------------------------------------------------
    // COMPONENT
    //---------------------------------------------------------------------------------
    namespace component
    {
        struct double_buffer{};
        struct tag {};

        struct descriptor
        {
            using fn_construct  = void(void*) noexcept;
            using fn_destruct   = void(void*) noexcept;

            const type_guid         m_Guid;
            fn_construct* const     m_fnConstruct;
            fn_construct* const     m_fnDestruct;
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
            constexpr bool isValid(void) const noexcept { return !!m_pGroupInstance; }

            template< typename T >
            T& getComponent(type_guid TypeGuid = T::type_guid_v) noexcept;

            template< typename T >
            constexpr const T& getComponent(type_guid TypeGuid = T::type_guid_v) const noexcept;

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

            struct default_tag : component::tag
            {
                constexpr static type_guid type_guid_v{ 1ull };
            };

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
                                  std::tuple<entity::instance>
                                , decltype(FilterOutTags<std::decay_t<T_COMPONENTS_AND_TAGS>>())...
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
                                  std::tuple<default_tag>
                                , decltype(FilterOutComponents<std::decay_t<T_COMPONENTS_AND_TAGS>>())...
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
                                  std::tuple<entity::instance>
                                , decltype(FilterOutTagsExtendDoubleBuffer<T_COMPONENTS_AND_TAGS>())...
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
                                  std::tuple<default_tag>
                                , decltype(FilterOutComponents<T_COMPONENTS_AND_TAGS>())...
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

        }

        //---------------------------------------------------------------------------------
        // GROUP::DESCRIPTOR
        //---------------------------------------------------------------------------------
        struct descriptor
        {
            constexpr descriptor( xcore::span<const component::descriptor*> lComponetDesc ) noexcept
                : m_lComponentTypeGuids     { [&]() constexpr { std::array<type_guid,64>                    D{}; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i]->m_Guid;  return D; }() }
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

            
            void ConstructEntity( tools::page* Page, group::local_index LocalIndex, entity::map_t::entry* pEntry ) const noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_nComponents; ++i )
                {
                    auto&       E       = m_lComponentOffsets[i];
                    const auto  Index   = E.m_Offset + static_cast<std::uint32_t>(LocalIndex.m_Value) * E.m_Size;
                    m_lComponentDescriptors[i]->m_fnConstruct( &p[Index] );
                    if( i == 0 ) reinterpret_cast< entity::instance* >( &p[Index] )->m_pInstance = pEntry;
                }
            }

            void DestructEntity( tools::page* Page, std::uint32_t LocalIndex ) const noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_nComponents; ++i )
                {
                    auto&       E       = m_lComponentOffsets[i];
                    const auto  Index   = E.m_Offset + LocalIndex * E.m_Size;
                    m_lComponentDescriptors[i]->m_fnDestruct( &p[Index] );
                }
            }

            constexpr const details::offsets* findMutableOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) const noexcept
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

            constexpr const details::offsets* findReadOnlyOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) const noexcept
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


            const std::uint16_t                                                         m_EntriesPerPage;
            const std::uint16_t                                                         m_nComponents;
            const std::array<type_guid, max_ingroup_components_v>                       m_lComponentTypeGuids;
            const std::array<const component::descriptor*, max_ingroup_components_v>    m_lComponentDescriptors;
            const std::array<details::offsets, max_ingroup_components_v>                m_lComponentOffsets;
        };


        struct events
        {
            // onNewEntities...      (which new entities where created this frame)
            // onDeleteEntities...   (which entities got deleted this frame)
            // onUpdatedEntities...  (Some system may have change this entity (Had write access to it))
            // onMovedInEntities...  (Some new entity got moved in here, from another group)
            // onMovedOutEntities... (By Adding or Removing Componets)
        };

        //---------------------------------------------------------------------------------
        // GROUP::INSTANCE
        //---------------------------------------------------------------------------------
        struct instance
        {
            instance( tools::page_mgr& PageMgr, const descriptor& GroupDescriptor ) noexcept
                : m_PageMgr             { PageMgr }
                , m_GroupDescriptor     { GroupDescriptor }
                , m_lDeleteEntries      { m_PageMgr }
                {}

            constexpr   auto        size()                  const   noexcept { return m_Count; }
            inline      auto        newSize()               const   noexcept { return m_NewCount.load(std::memory_order_relaxed); }

            inline
            void* getComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) noexcept
            {
                if( auto pOffset = m_GroupDescriptor.findMutableOffset(Ref,TypeGuid); pOffset )
                {
                    const auto  Index   = pOffset->m_Offset + Ref.m_LocalIndex.m_Value * pOffset->m_Size;
                    std::byte*  p       = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[Index];
                    return p;
                }

                return nullptr;
            }

            inline
            void* getComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) const noexcept
            {
                if( auto pOffset = m_GroupDescriptor.findReadOnlyOffset(Ref,TypeGuid); pOffset )
                {
                    const auto  Index   = pOffset->m_Offset + Ref.m_LocalIndex.m_Value * pOffset->m_Size;
                    std::byte*  p       = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[Index];
                    return p;
                }

                return nullptr;
            }

            entity::tmp_ref CreateEntity( entity::map_t::entry* pMapEntry )
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
                        XCORE_PERF_ZONE_SCOPED_N("CreateEntity::Wasting Time")
                        XCORE_CMD_PROFILER( while (PageIndex >= m_nPages.load(std::memory_order_relaxed)); )
                    }

                } while (true);

                return { this, local_index{RelativeIndex}, page_index{PageIndex} };
            }

            struct delete_entry
            {
                page_index  m_PageIndex;
                local_index m_LocalEntry;
            };

            /*
            void DeleteEntity( entity::tmp_ref Entry ) noexcept
            {
                auto& Entity = *reinterpret_cast<entity::instance*>
                (
                    &m_lPages[Entry.m_PageIndex.m_Value]->m_RawData
                    [
                          m_GroupDescriptor.m_lComponentOffsets[0].m_Offset 
                        + m_GroupDescriptor.m_lComponentOffsets[0].m_Size 
                        * static_cast<std::uint32_t>(Entry.m_LocalIndex.m_Value)
                    ]
                );

                auto Guid       = Entity.m_Guid;
                auto Flags      = Entity.m_SystemFlags.load(std::memory_order_relaxed);
                auto NewFlags   = Flags;

                do 
                {
                    NewFlags = Flags;
                    NewFlags.m_isDeleted = true;
                } while( Entity.m_SystemFlags.compare_exchange_weak( Flags, NewFlags) == false );

                auto& Del = m_lDeleteEntries.append();
                Del.m_PageIndex  = Entry.m_PageIndex;
                Del.m_LocalEntry = Entry.m_LocalIndex;
            }
                */

            constexpr static std::size_t max_pages_v = 1024*32;
            using page_list = xcore::array<tools::page*, max_pages_v>;

            tools::page_mgr&                                                            m_PageMgr;
            std::atomic<std::uint32_t>                                                  m_NewCount          { 0 };
            std::uint32_t                                                               m_Count             { 0 };
            std::atomic<group::page_index>                                              m_nPages            { group::page_index{0} };
            page_list                                                                   m_lPages            {};
            const group::descriptor&                                                    m_GroupDescriptor;
            tools::pool<delete_entry,max_pages_v>                                       m_lDeleteEntries;
        };

        //---------------------------------------------------------------------------------
        // GROUP::ITERATOR FUNCTIONS
        //---------------------------------------------------------------------------------
        /*
        inline
        iterator iterator::operator++ (void) noexcept
        {
            m_LocalIndex.m_Value++;
            if (m_LocalIndex.m_Value == m_pGroupInstance->m_EntriesPerPage)
            {
                m_PageIndex.m_Value++;
                m_LocalIndex.m_Value = 0;
                if (m_PageIndex.m_Value < m_pGroupInstance->m_lPages.size())
                    _mm_prefetch((const char*)m_pGroupInstance->m_lPages[m_PageIndex.m_Value], _MM_HINT_NTA);
            }
            return *this;
        }

        inline constexpr
        bool iterator::operator !=  (std::nullptr_t) const noexcept
        {
            const auto l = m_pGroupInstance->m_lPages.size() - 1;
            return !(l == m_PageIndex.m_Value
                 && (m_pGroupInstance->m_Count - l * m_pGroupInstance->m_EntriesPerPage) == m_LocalIndex.m_Value);
        }
        */
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

            constexpr int AllocBit( void ) noexcept
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

            inline void AddBit( int Index ) noexcept
            {
                m_Bits[Index/64] |= (1ull<<(Index%64));
            }

            inline void Add( filter_bits Bits ) noexcept
            {
                for( int i=0; i<m_Bits.size(); ++i ) m_Bits[i] |= Bits.m_Bits[i];
            }

            constexpr bool Query( filter_bits All, filter_bits Any, filter_bits None ) const noexcept
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

        //---------------------------------------------------------------------------------
        // QUERY::ALL NONE ANY
        //---------------------------------------------------------------------------------
        template< typename...T_COMPONENTS_AND_TAGS >
        struct all : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            constexpr static int type_v = 0;
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct none : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            constexpr static int type_v = 1;
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct any : component::group::details::guid_arrays_from_types< T_COMPONENTS_AND_TAGS... >
        {
            constexpr static int type_v = 2;
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
                    std::is_const_v<T_ARGS> ...
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
        }

        template< typename T >
        struct system_function
        {
            using args_t        = typename xcore::function::traits<decltype(&T::Update)>::args_tuple;
            using sorted_args_t = typename xcore::types::tuple_sort_t< component::group::details::compare, args_t >;
            constexpr static std::array list_v          { details::TypeArray ( reinterpret_cast<args_t*>(nullptr) ) };
            constexpr static std::array const_list_v    { details::ConstArray( reinterpret_cast<args_t*>(nullptr) ) };
            constexpr static std::array sorted_list_v   { details::TypeArray ( reinterpret_cast<sorted_args_t*>(nullptr) ) };
            constexpr static std::array remap_v         { details::remap<list_v.size()>( list_v, sorted_list_v ) };

            static_assert(sorted_list_v[0].m_Value < sorted_list_v[1].m_Value);
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

            query m_ComponentQuery;
            query m_TagQuery;
        };

        template< typename...T_QUERY_FILTERS >
        struct define : define_data
        {
            template< bool T_IS_TAG, int I >
            constexpr static void Initializer( define& This, std::span<const type_guid> Componets, std::span<const type_guid> Tags ) noexcept
            {
                // We filter out the first component since it is always the entity or the default_tag
                if constexpr (T_IS_TAG)
                {
                         if constexpr ( I == 0 ) if(Tags.size()>1) This.m_TagQuery.m_All  = std::span<const type_guid>(&Tags[1], Tags.size()-1);
                    else if constexpr ( I == 1 ) if(Tags.size()>1) This.m_TagQuery.m_Any  = std::span<const type_guid>(&Tags[1], Tags.size()-1);
                    else if constexpr ( I == 2 ) if(Tags.size()>1) This.m_TagQuery.m_None = std::span<const type_guid>(&Tags[1], Tags.size()-1);
                }
                else
                {
                         if constexpr ( I == 0 ) if(Componets.size()>1) This.m_ComponentQuery.m_All  = std::span<const type_guid>(&Componets[1], Componets.size()-1);
                    else if constexpr ( I == 1 ) if(Componets.size()>1) This.m_ComponentQuery.m_Any  = std::span<const type_guid>(&Componets[1], Componets.size()-1);
                    else if constexpr ( I == 2 ) if(Componets.size()>1) This.m_ComponentQuery.m_None = std::span<const type_guid>(&Componets[1], Componets.size()-1);
                }
            };

            constexpr define( void ) noexcept
            {
                ( 
                    Initializer
                    <
                        std::is_base_of_v<mecs::component::tag, T_QUERY_FILTERS>
                        , T_QUERY_FILTERS::type_v
                    >( *this, T_QUERY_FILTERS::component_guid_list_v, T_QUERY_FILTERS::tag_guid_list_v )
                    , ... 
                );
            }
        };

        //---------------------------------------------------------------------------------
        // QUERY::RESULT ENTRY
        //---------------------------------------------------------------------------------
        struct result_entry
        {
            component::group::instance*                         m_pGroup;
            std::array<std::uint16_t,15>                        m_lRemaps;  // Offsets into the actual page where the component is
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
        constexpr static std::size_t max_systems_v = 256;

        namespace details
        {
            //---------------------------------------------------------------------------------
            // SYSTEM::CUSTOM SYSTEM 
            //---------------------------------------------------------------------------------
            template< typename T_SYSTEM >
            struct custom_system final : T_SYSTEM
            {
                using user_system_t     = T_SYSTEM;
                using system_function_t = query::system_function<T_SYSTEM>;
                using user_system_t::user_system_t;

                query::instance     m_Query;
                world::instance&    m_World;

                custom_system( world::instance& World ) noexcept 
                    : m_World       { World }
                    , T_SYSTEM      {}
                { user_system_t::setupName(user_system_t::system_name_v); }

                template< typename...T_ARGS >
                void ForEach( std::tuple<T_ARGS...>* ) noexcept
                {
                    using pool_tuple = typename std::tuple< std::remove_reference_t<T_ARGS>*... >;
                    xcore::scheduler::channel Channel( xconst_universal_str("ForEach") );

                    for (auto& R : m_Query.m_lResults )
                    {
                        auto&               Group           = *R.m_pGroup;
                        auto                TotalEntries    = Group.m_Count;
                        const auto          EntriesPerPage  = Group.m_GroupDescriptor.m_EntriesPerPage;

                        for( auto& Page : Group.m_lPages.ViewTo(Group.m_nPages.load(std::memory_order_relaxed).m_Value) )
                        {
                            auto        pPage = reinterpret_cast<std::byte*>( Page->m_RawData );
                            const auto  iEnd  = std::min( static_cast<std::uint16_t>(TotalEntries), EntriesPerPage );
                            TotalEntries -= iEnd;
                            Channel.SubmitJob( [ &R, iEnd, pPage, this ]
                            {
                                XCORE_PERF_ZONE_SCOPED_N("ForEach::LightJob")

                                const pool_tuple Pools
                                {
                                    reinterpret_cast<std::remove_reference_t<T_ARGS>*>
                                    ( &pPage
                                        [ 
                                            R.m_lRemaps[ xcore::types::tuple_t2i_v< T_ARGS, system_function_t::args_t > ] 
                                        ]
                                    ) ...
                                };

                                for( auto i = 0; i < iEnd; ++i )
                                {
                                    // Call the system update function
                                    user_system_t::Update
                                    ( 
                                        std::get
                                        <
                                            std::remove_reference_t<T_ARGS>*
                                        >(Pools)[i] ... 
                                    );
                                }
                            });

                            // Just in case we added new entries and new pages got added
                            if( iEnd == TotalEntries ) break;
                        }
                    }

                    Channel.join();
                }

                virtual void qt_onRun( void ) noexcept override
                {
                    m_World.DoQuery<user_system_t>(m_Query);
                    ForEach( reinterpret_cast< typename system_function_t::args_t* >(nullptr) );
                }
            };
        }

        struct instance : xcore::scheduler::job<4>
        {
            using                   parent_t        = xcore::scheduler::job<4>;
            constexpr static auto   system_name_v   = xconst_universal_str("System");

            constexpr instance(xcore::scheduler::definition Def) noexcept 
                : parent_t{ Def | xcore::scheduler::triggers::DONT_CLEAN_COUNT }
            {}

            constexpr instance(void) noexcept
                : parent_t( xcore::scheduler::definition::Flags(xcore::scheduler::triggers::DONT_CLEAN_COUNT) )
            {}
        };
    }

    //---------------------------------------------------------------------------------
    // SYNC_POINT - A sync point establish a moment in time where systems can be run. You can
    //              also build complex graphs this way.
    //---------------------------------------------------------------------------------
    namespace sync_point
    {
        struct events
        {
            using done  = xcore::types::make_unique< xcore::event::type<>, struct done_tag >;
            using start = xcore::types::make_unique< xcore::event::type<>, struct start_tag >;

            start m_Start;
            done  m_Done;
        };

        struct instance : xcore::scheduler::trigger<system::max_systems_v>
        {
            events  m_Events;
            bool    m_bDisable = false;
            int     m_FreethCount;

            void FreezCount() noexcept
            {
                m_FreethCount = m_NotificationCounter.load();
            }

            virtual void qt_onTriggered ( void ) noexcept override
            {
                m_NotificationCounter = m_FreethCount;
                m_Events.m_Done.NotifyAll();                
                if(m_bDisable == false)
                {
                    m_Events.m_Start.NotifyAll();
                    xcore::scheduler::trigger<system::max_systems_v>::qt_onTriggered();
                }
            }

            ~instance()
            {
                m_NotificationCounter = 0;
            }

        };
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

                    type_guid                                       m_Guid;         // Tag UID (Sum of all individual tags)
                    query::filter_bits                              m_Bits;         // Tag bitsets (Unique bit set for each tag)
                    std::uint32_t                                   m_nGroups;      
                    xcore::array<group_entry,32>                    m_lGroups;
                    std::uint32_t                                   m_nTags;      
                    xcore::array<type_guid,64>                      m_lTagGuids;    // Individual tag guids that make this entry
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
        // WORLD::instance
        //---------------------------------------------------------------------------------
        struct instance : xcore::scheduler::job<1>
        {
            sync_point::instance    m_StartSyncPoint;
            sync_point::instance    m_EndSyncPoint;

            void msgSyncPointDone()
            {
                auto& Scheduler = xcore::get().m_Scheduler;
                EndFrameUpdate();
                Scheduler.MainThreadStopWorking();
            }

            void EndFrameUpdate( void )
            {
                m_bFrameStarted = false;

                for( const auto& TagE : m_TagDBase.m_lTagEntries.ViewTo(m_TagDBase.m_nTagEntries) )
                    for( auto& GroupE : TagE.m_lGroups.ViewTo(TagE.m_nGroups) )
                        GroupE.m_pGroup->m_Count = GroupE.m_pGroup->m_NewCount.load(std::memory_order_relaxed);
            }

            const sync_point::events::done::delegate m_SyncPointDoneDelegate{ this, &instance::msgSyncPointDone, m_EndSyncPoint.m_Events.m_Done };

            instance()
            {
                m_Definition.m_Triggers = xcore::scheduler::triggers::DONT_CLEAN_COUNT;
                registerComponents<entity::instance,component::group::details::default_tag>();
                m_EndSyncPoint.AddJobToBeTrigger(*this);
                m_EndSyncPoint.FreezCount();
                m_EndSyncPoint.m_bDisable = true;

                m_StartSyncPoint.JobWillNotifyMe(*this);
                m_StartSyncPoint.FreezCount();
            }

            virtual void qt_onRun( void ) noexcept override
            {
                m_bFrameStarted = true;
                printf("Hello world\n");
            }

            void Start( void )
            {
                auto& Scheduler = xcore::get().m_Scheduler;
                EndFrameUpdate();
                Scheduler.AddJobToQuantumWorld(*this);
                Scheduler.MainThreadStartsWorking();
            }

            void Resume( void )
            {
            
            }

            void Stop( void )
            {
            }

            inline
            entity::tmp_ref createEntity( component::group::instance& Group, entity::guid EntityGuid ) noexcept
            {
                entity::tmp_ref Ref;
                m_mapEntity.push( EntityGuid, [&]( entity::map_t::entry& E )
                {
                    Ref = E.m_Value = Group.CreateEntity(&E);
                });
                return Ref;
            }

            component::group::instance& getOrCreateGroup( const std::span<const type_guid>    lComponentTypesGuids
                                                        , const std::span<const type_guid>    lTagTypesGuids ) noexcept
            {
                bool                bHasEntity          = false;
                const auto          ComponentGroupGuid  = component::group::guid{ [&] 
                {
                    std::uint64_t Key = 0; 
                    for (auto E : lComponentTypesGuids) 
                    {
                        if( E == entity::instance::type_guid_v ) bHasEntity = true;
                        Key += E.m_Value;
                    }
                    if(bHasEntity == false) Key += entity::instance::type_guid_v.m_Value; 
                    return Key; 
                 }() };
                const auto          TagGuid             = type_guid{ [&] {std::uint64_t Key = 0; for (auto E : lTagTypesGuids)       Key += E.m_Value; return Key; }() };
                const auto          TagGroupGuid        = type_guid{ ComponentGroupGuid.m_Value + TagGuid.m_Value };

                return *m_mapGroupPerTag.getOrCreate( TagGroupGuid, [&]
                {
                    auto& TagEntry = *m_mapTagLookup.getOrCreate( TagGuid,[&]()
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

                    //
                    // Create a new group if we need to
                    //
                    //                             XCORE_PERF_ZONE_SCOPED_N("getOrCreateGroup::Wasting Time")
                    auto&               GroupEntry          = *new( &TagEntry.m_lGroups[TagEntry.m_nGroups++] ) details::dbase::tag_entry::group_entry {};
                    auto                nComponents         = lComponentTypesGuids.size();
 
                    xcore::array<const component::descriptor*, max_ingroup_components_v> lComponetDesc;
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
                          m_PageMgr
                        , *pGroupDesc
                    };

                    return GroupEntry.m_pGroup;
                });
            }

            template< typename...T_COMPONENTS_AND_TAGS >
            inline component::group::instance& getOrCreateGroup( void ) noexcept
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
                    m_mapTagDescriptor.push( T_COMPONENTS_AND_TAGS::type_guid_v, m_TagRegistrationFreeBits.AllocBit() );
                }
                else
                {
                    constexpr static component::descriptor component_descriptor_v
                    {
                            T_COMPONENTS_AND_TAGS::type_guid_v
                        ,   [](void* p) noexcept { new(p) T_COMPONENTS_AND_TAGS{}; }
                        ,   [](void* p) noexcept { std::destroy_at( reinterpret_cast<T_COMPONENTS_AND_TAGS*>(p) ); }
                            // //property::table&  m_PropertyTable;
                        ,   static_cast<std::uint16_t>(sizeof(T_COMPONENTS_AND_TAGS))
                        ,   static_cast<std::uint16_t>(std::alignment_of_v<T_COMPONENTS_AND_TAGS>)
                        ,   std::is_base_of_v<mecs::component::double_buffer, T_COMPONENTS_AND_TAGS>
                        ,   -1
                    };

                    component_descriptor_v.m_BitNumber = m_ComponentRegistrationFreeBits.AllocBit();
                    m_mapComponentDescriptor.push(T_COMPONENTS_AND_TAGS::type_guid_v, &component_descriptor_v);
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
            
                auto pSystem = new system::details::custom_system<T_SYSTEM>{*this};

                // Add all end triggers
                ( EndSyncPoints.JobWillNotifyMe(*pSystem), ... );
                ( EndSyncPoints.FreezCount(), ... );

                // Set the start trigger
                StartSynpoint.AddJobToBeTrigger(*pSystem);
                StartSynpoint.FreezCount();

                return *pSystem;
            }

            template<typename T_SYSTEM >
            void DoQuery( query::instance& Instance ) const noexcept
            {
                using system    = T_SYSTEM;
                using function  = query::system_function<T_SYSTEM>;

                if( Instance.m_isInitialized == false )
                {
                    Instance.m_isInitialized = true;

                    //
                    // Handle the Component Side
                    //
                    for( auto E : system::query_v.m_ComponentQuery.m_All )
                        Instance.m_ComponentQuery.m_All.AddBit( m_mapComponentDescriptor.get(E)->m_BitNumber );

                    for( auto E : system::query_v.m_ComponentQuery.m_Any )
                        Instance.m_ComponentQuery.m_Any.AddBit( m_mapComponentDescriptor.get(E)->m_BitNumber );

                    for( auto E : system::query_v.m_ComponentQuery.m_None )
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
                    for( auto E : system::query_v.m_TagQuery.m_All )
                        Instance.m_TagQuery.m_All.AddBit( m_mapTagDescriptor.get(E) );

                    for( auto E : system::query_v.m_TagQuery.m_Any )
                        Instance.m_TagQuery.m_Any.AddBit( m_mapTagDescriptor.get(E) );

                    for( auto E : system::query_v.m_TagQuery.m_None )
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
                                    Entry.m_lRemaps[F + function::remap_v[F]] = lOffsets[ iBase ].m_Offset;
                                }
                                else
                                {
                                    Entry.m_lRemaps[F + function::remap_v[F]] = lOffsets[ i ].m_Offset;
                                }

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

            /*
            void DeleteEntity( entity::guid Guid ) noexcept
            {
                m_mapEntity.pop( Guid, [&]( entity::tmp_ref TmpRef )
                {
                    TmpRef.m_pGroupInstance->DeleteEntity(TmpRef);
                });
            }
            */
 
            constexpr static std::size_t                                max_groups_count_v = 1024;

            using map_group_per_tag_t           = tools::map<type_guid,               component::group::instance*,            max_groups_count_v>;
            using map_group_descriptor_t        = tools::map<component::group::guid,  const component::group::descriptor*,    max_groups_count_v>;
            using map_component_descriptor_t    = tools::map<type_guid,               const component::descriptor*,           256>;
            using map_tag_descriptor_t          = tools::map<type_guid,               std::uint8_t,                           256>;
            using map_tag_lookup_t              = tools::map<type_guid,               details::dbase::tag_entry*,             max_groups_count_v>;

            bool                                        m_bFrameStarted                 { false };
            bool                                        m_bPause                        { false };
            int                                         m_FrameNumber                   { 0 };
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
            return *reinterpret_cast<T*>(m_pGroupInstance->getComponent(*this, TypeGuid));
        }

        template< typename T >
        constexpr const T& tmp_ref::getComponent(type_guid TypeGuid) const noexcept
        {
            return *reinterpret_cast<T*>(m_pGroupInstance->getComponent(*this, TypeGuid));
        }
    }

    //------------------------------------------------------------------------------------
    // Test
    //------------------------------------------------------------------------------------
    namespace test
    {
        struct simple
        {
            constexpr static type_guid type_guid_v{"simple"};
            int m_Value{ 22 };
        };

        struct double_buff : component::double_buffer
        {
            constexpr static type_guid type_guid_v{ "double_buff" };
            int m_Value{ 123 };
        };

        struct my_tag : component::tag 
        {
            constexpr static type_guid type_guid_v{ "my_tag" };
        };

        struct my_system : system::instance
        {
            constexpr static query::define
            <
                query::all<simple>
              , query::none<my_tag>
            > query_v{};

            constexpr
            void Update( const double_buff& Buff, simple& Simple ) const noexcept
            {
                Simple.m_Value += Buff.m_Value;
            }
        };

        void Test()
        {
            mecs::world::instance World;
            World.registerComponents
            <
                    simple
                ,   double_buff
                ,   my_tag
            >();

            World.AddGraphConnection<my_system>( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Create a bunch of entities
            //
            {
                xcore::scheduler::channel MyChannel(xconst_universal_str("Create Entities"));
                auto& Group = World.getOrCreateGroup<simple,double_buff,my_tag>();

                for(int i=0; i<xcore::get().m_Scheduler.getWorkerCount(); i++)
                    MyChannel.SubmitJob([&]
                    {
                        while (Group.newSize() < 10000000ull XCORE_CMD_ASSERT( - 9500000ull ) )
                        {
                            auto    tmpEntityRef = World.createEntity( Group, entity::guid{xcore::not_null} );
                            auto&   Entity = tmpEntityRef.getComponent<entity::instance>();
                            auto&   Simple = tmpEntityRef.getComponent<simple>();
                            auto&   Double = tmpEntityRef.getComponent<double_buff>();
                            assert(Simple.m_Value == 22);
                            assert(Double.m_Value == 123);
                        }
                    });

                MyChannel.join();
            }

            World.Start();

            int b = 0;
        }
    }
}
