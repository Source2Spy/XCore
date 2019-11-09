
namespace mecs
{
    namespace group
    {
        struct instance;
        struct local_index  : xcore::units::type<local_index,   std::uint16_t> {using type::type;};
        struct page_index   : xcore::units::type<page_index,    std::uint16_t> {using type::type;};
        struct global_index : xcore::units::type<global_index,  std::uint32_t> {using type::type;};
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
        using guid = xcore::guid::unit<64, struct ecs_entity_tag>;

        union flags
        {
            std::uint32_t m_Value{ 0 };
            struct
            {
                bool  m_isDeleted : 1
                    , m_isDisable : 1
                    , m_isInWorld : 1
                    ;
            };
        };

        struct instance
        {
            constexpr static type_guid type_guid_v{1ull};
            std::atomic<entity::flags>  m_SystemFlags;  // This is 32 bits
            std::uint32_t               m_Layer;        // Means 32bits are been used to padding here
            entity::guid                m_Guid;         // This is 64 bits
        };

        struct tmp_ref
        {
            constexpr bool isValid(void) const noexcept { return !!m_pGroupInstance; }

            template< typename T >
            T& getComponent(type_guid TypeGuid = T::type_guid_v) noexcept;

            template< typename T >
            constexpr const T& getComponent(type_guid TypeGuid = T::type_guid_v) const noexcept;

            //std::vector<properties_tuple> getProperties(void) const noexcept;

            group::instance*        m_pGroupInstance;
            group::local_index      m_LocalIndex;
            group::page_index       m_PageIndex;
        };
    }

    //---------------------------------------------------------------------------------
    // GROUP
    //---------------------------------------------------------------------------------
    namespace group
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
            
            //---------------------------------------------------------------------------------
            struct default_tag : component::tag
            {
                constexpr static type_guid type_guid_v{ 2ull };
            };

            //---------------------------------------------------------------------------------
            template< typename T_A, typename T_B >
            struct compare
            {
                constexpr static bool value = T_A::type_guid_v.m_Value
                                            < T_B::type_guid_v.m_Value;
            };
        }

        //---------------------------------------------------------------------------------
        // GROUP::PAGE
        //---------------------------------------------------------------------------------
        struct alignas(128) page
        {
            std::byte    m_RawData[1024*16];
        };

        //---------------------------------------------------------------------------------
        // GROUP::PAGE MANAGER
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
        // GROUP::SIMPLE POOL
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
        // GROUP::INSTANCE
        //---------------------------------------------------------------------------------
        struct instance
        {
            instance( page_mgr& PageMgr, xcore::span<const component::descriptor*> lComponetDesc, const std::span<const type_guid> TagTypesGuids ) noexcept
                : m_PageMgr             { PageMgr }
                , m_lComponentTypeGuids { [&]() constexpr { std::array<type_guid,64>                    D{}; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i]->m_Guid;  return D; }() }
                , m_lTagGuids           { [&]() constexpr { std::array<type_guid,64>                    D{}; for (int i = 0; i < TagTypesGuids.size(); ++i) D[i] = TagTypesGuids[i];          return D; }() }
                , m_lDescriptors        { [&]() constexpr { std::array<const component::descriptor*,64> D{}; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i];          return D; }() }
                , m_nTagGuids           { static_cast<std::uint16_t>(TagTypesGuids.size()) }
                , m_nComponents         { static_cast<std::uint16_t>(lComponetDesc.size()) }
                , m_EntriesPerPage      { [&]() constexpr
                                            {
                                                const std::uint32_t     SizeOfOne           = [&]() constexpr { std::uint32_t S = 0; for (auto p : lComponetDesc) S += p->m_Size; return S; }();
                                                const std::uint32_t     OriginalAddress     = std::alignment_of_v<page>;
                                                std::uint32_t           WastedToAligment    = 0;
                                                std::uint32_t           p                   = OriginalAddress;

                                                for( auto pE : lComponetDesc )
                                                {
                                                    WastedToAligment += xcore::bits::Align(p, pE->m_Aligment) - p;
                                                    p += pE->m_Size;
                                                }

                                                return static_cast<std::uint16_t>((sizeof(page) - WastedToAligment) / SizeOfOne);
                                            }()
                                        }
                , m_lComponentOffsets   { [&]() constexpr 
                                            {
                                                std::array<details::offsets, 64>      lComponentOffsets{};
                                                const std::uint32_t                   OriginalAddress     = std::alignment_of_v<page>;
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
                                                assert((p - OriginalAddress) <= sizeof(page));
                                                return lComponentOffsets;
                                            }()
                                        }
                , m_lDeleteEntries      { m_PageMgr }
                {}

            constexpr   auto        size()                  const   noexcept { return m_Count; }
            inline      auto        newSize()               const   noexcept { return m_NewCount.load(std::memory_order_relaxed); }

            void ConstructEntity( page* Page, group::local_index LocalIndex ) noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_nComponents; ++i )
                {
                    auto&       E       = m_lComponentOffsets[i];
                    const auto  Index   = E.m_Offset + static_cast<std::uint32_t>(LocalIndex.m_Value) * E.m_Size;
                    m_lDescriptors[i]->m_fnConstruct( &p[Index] );
                }
            }

            void DestructEntity( page* Page, std::uint32_t LocalIndex ) noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_nComponents; ++i )
                {
                    auto&       E       = m_lComponentOffsets[i];
                    const auto  Index   = E.m_Offset + LocalIndex * E.m_Size;
                    m_lDescriptors[i]->m_fnDestruct( &p[Index] );
                }
            }

            const details::offsets* findOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) noexcept
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

            const details::offsets* findOffset( const entity::tmp_ref& Ref, type_guid TypeGuid ) const noexcept
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

            inline
            void* getComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) noexcept
            {
                if( auto pOffset = findOffset(Ref,TypeGuid); pOffset )
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
                if( auto pOffset = findOffset(Ref,TypeGuid); pOffset )
                {
                    const auto  Index   = pOffset->m_Offset + Ref.m_LocalIndex.m_Value * pOffset->m_Size;
                    std::byte*  p       = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[Index];
                    return p;
                }

                return nullptr;
            }

            entity::tmp_ref CreateEntity( entity::guid Guid )
            {
                const auto          Index           = m_NewCount++;
                const local_index   RelativeIndex   { Index % m_EntriesPerPage };
                const page_index    PageIndex       { static_cast<std::uint16_t>(Index / m_EntriesPerPage) };

                do
                {
                    const auto nPages   = m_nPages.load(std::memory_order_relaxed);

                    if (PageIndex < nPages)
                    {
                        // Create entry here
                        ConstructEntity( m_lPages[PageIndex.m_Value], RelativeIndex );
                        break;
                    }
                    else if (PageIndex == nPages && RelativeIndex.m_Value == 0)
                    {
                        m_lPages[PageIndex.m_Value] = m_PageMgr.AllocPage();

                        m_nPages.store(page_index{static_cast<std::uint16_t>(nPages.m_Value+1)});
                        ConstructEntity( m_lPages[PageIndex.m_Value], RelativeIndex );
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

            void DeleteEntity( entity::tmp_ref Entry ) noexcept
            {
                auto& Entity = *reinterpret_cast<entity::instance*>
                (
                    &m_lPages[Entry.m_PageIndex.m_Value]->m_RawData
                    [
                        m_lComponentOffsets[0].m_Offset + m_lComponentOffsets[0].m_Size * static_cast<std::uint32_t>(Entry.m_LocalIndex.m_Value)
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

            constexpr static std::size_t max_pages_v = 1024*32;
            using page_list = xcore::array<page*, max_pages_v>;

            group::instance*                                                            m_pNext         {nullptr};
            page_mgr&                                                                   m_PageMgr;
            std::atomic<std::uint32_t>                                                  m_NewCount      { 0 };
            std::uint32_t                                                               m_Count         { 0 };
            std::atomic<group::page_index>                                              m_nPages        { group::page_index{0} };
            page_list                                                                   m_lPages        {};
            const std::uint16_t                                                         m_EntriesPerPage;
            const std::uint16_t                                                         m_nComponents;
            const std::uint16_t                                                         m_nTagGuids;
            const std::array<type_guid, max_ingroup_components_v>                       m_lComponentTypeGuids;
            const std::array<type_guid, max_ingroup_components_v>                       m_lTagGuids;
            const std::array<const component::descriptor*, max_ingroup_components_v>    m_lDescriptors;
            const std::array<details::offsets, max_ingroup_components_v>                m_lComponentOffsets;
            pool<delete_entry,max_pages_v>                                              m_lDeleteEntries;
        };

        //---------------------------------------------------------------------------------
        // GROUP::ITERATOR FUNCTIONS
        //---------------------------------------------------------------------------------
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
                bool bAny = false;
                for( int i=0; i<m_Bits.size(); ++i )
                { 
                    const auto B = m_Bits[i];
                    if( None.m_Bits[i] & B ) return false;
                    if( const auto A = All.m_Bits[i]; (A&B) != A ) return false;
                    if( const auto A = Any.m_Bits[i]; (A&B) ) bAny = true;
                }
                return bAny;
            }
        };

        //---------------------------------------------------------------------------------
        // QUERY::ALL NONE ANY
        //---------------------------------------------------------------------------------
        template< typename...T_COMPONENTS_AND_TAGS >
        struct all
        {
            constexpr static int type_v = 0;
            constexpr static std::array list_v{ T_COMPONENTS_AND_TAGS::type_guid_v ... };
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct none
        {
            constexpr static int type_v = 1;
            constexpr static std::array list_v{ T_COMPONENTS_AND_TAGS::type_guid_v ... };
        };

        template< typename...T_COMPONENTS_AND_TAGS >
        struct any
        {
            constexpr static int type_v = 2;
            constexpr static std::array list_v{ T_COMPONENTS_AND_TAGS::type_guid_v ... };
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

            template< typename B, typename A >
            struct compare
            {
                constexpr static bool value = std::decay_t<A>::type_guid_v.m_Value 
                                            < std::decay_t<B>::type_guid_v.m_Value;
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
            using sorted_args_t = typename xcore::types::tuple_sort_t< details::compare, args_t >;
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
            std::span<const type_guid>    m_All;            // Which components/tags we must have
            std::span<const type_guid>    m_Any;            // Must have at least one component/tag from the set
            std::span<const type_guid>    m_None;           // Which components/tags you must not have
        };

        template< typename...T_ARGS >
        struct define : define_data
        {
            template< int I >
            constexpr static void Initializer( define& This, std::span<const type_guid> X ) noexcept
            {
                     if constexpr ( I == 0 ) This.m_All         = X;
                else if constexpr ( I == 1 ) This.m_Any         = X;
                else if constexpr ( I == 2 ) This.m_None        = X;
                else if constexpr ( I == 3 ) This.m_Function    = X;
            };

            constexpr define( void ) noexcept
            {
                ( Initializer<T_ARGS::type_v>( *this, T_ARGS::list_v ), ... );
            }
        };

        //---------------------------------------------------------------------------------
        // QUERY::RESULT ENTRY
        //---------------------------------------------------------------------------------
        struct result_entry
        {
            group::instance*                                    m_pGroup;
            std::array<std::uint16_t,15>                        m_lRemaps;
            std::uint8_t                                        m_nRemaps;
        };

        //---------------------------------------------------------------------------------
        // QUERY::INSTANCE
        //---------------------------------------------------------------------------------
        struct instance
        {
            xcore::vector<result_entry> m_lResults              {};
            bool                        m_isInitialized         {false};
            filter_bits                 m_Function              {nullptr};
            filter_bits                 m_Write                 {nullptr};
            filter_bits                 m_All                   {nullptr};
            filter_bits                 m_Any                   {nullptr};
            filter_bits                 m_None                  {nullptr};
        };
    }

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
                        const auto          EntriesPerPage  = Group.m_EntriesPerPage;
                        const std::uint16_t EntityOffset    = Group.m_lComponentOffsets[0].m_Offset;

                        for( auto& Page : Group.m_lPages.ViewTo(Group.m_nPages.load(std::memory_order_relaxed)) )
                        {
                            auto        pPage = reinterpret_cast<std::byte*>( Page->m_RawData );
                            const auto  iEnd  = std::min( static_cast<std::uint16_t>(TotalEntries), EntriesPerPage );
                            TotalEntries -= iEnd;
                            Channel.SubmitJob( [ EntityOffset, &R, iEnd, pPage, this ]
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

                                auto pEntityPool = reinterpret_cast<const entity::instance* const>(&pPage[EntityOffset]);

                                for( auto i = 0; i < iEnd; ++i )
                                {
                                    auto Flags = pEntityPool[i].m_SystemFlags.load(std::memory_order_relaxed);

                                    // Make sure we are running a valid entity
                                    if(    Flags.m_isDeleted 
                                        || Flags.m_isDisable
                                        || Flags.m_isInWorld == false )
                                        continue;

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

            struct entity_desc : xcore::lock::spin
            {
                entity::guid                                    m_Guid;
                entity::tmp_ref                                 m_EntityRef;
            };

            struct group_dec
            {
                std::atomic<group::guid>                        m_Guid{ 0 };
                std::atomic<group::instance*>                   m_pGroup{nullptr};
            };

            struct component_dec
            {
                std::atomic<type_guid>                          m_Guid{ 0 };
                const component::descriptor*                    m_pDescriptor{nullptr};
            };

            struct query_table_entry
            {
                query::filter_bits                              m_Bits;
                const group::instance*                          m_pGroup{nullptr};
            };

            template< typename T, std::size_t T_MAXSIZE_V > 
            struct map
            {
                constexpr static auto max_size_v = T_MAXSIZE_V;

                template< typename T_GUID >
                T& append(const T_GUID Guid) noexcept
                {
                    assert(Guid.isValid());
                    auto Index = Guid.m_Value % max_size_v;
                    do 
                    {

                        auto Local = m_Map[Index].m_Guid.load(std::memory_order_relaxed);
                        while( Local.m_Value == 0 )
                            if( auto& Entry = m_Map[Index]; Entry.m_Guid.compare_exchange_weak(Local, Guid) )
                                return Entry;
                        Index = (Index+1) % max_size_v;
                    } while(true);

                    xassert(false);
                }

                template< typename T_GUID, typename T_WAIT_LAMBDA >
                T& appendOrGet(T_GUID Guid, T_WAIT_LAMBDA&& WaitLambda) noexcept
                {
                    assert(Guid.isValid());
                    auto Index = Guid.m_Value % max_size_v;
                    do 
                    {
                        auto Local = m_Map[Index].m_Guid.load(std::memory_order_relaxed);
                        if (Local == Guid)
                        {
                            auto& Entry = m_Map[Index];
                            WaitLambda(Entry);
                            return Entry;
                        }
                        else while( Local.m_Value == 0 )
                            if( auto& Entry = m_Map[Index]; Entry.m_Guid.compare_exchange_weak(Local, Guid) )
                                return Entry;
                        Index = (Index+1) % max_size_v;
                    } while(true);

                    xassert(false);
                }

                template< typename T_GUID >
                T* find(T_GUID Guid) const noexcept
                {
                    assert(Guid.isValid());
                    auto Index = Guid.m_Value % max_size_v;
                    do 
                    {
                        auto Local = m_Map[Index].m_Guid.load(std::memory_order_relaxed);
                        if (Local == Guid)
                        {
                            auto& Entry = m_Map[Index];
                            return &Entry;
                        }
                        if( Local.m_Value == 0 ) return nullptr;
                        Index = (Index+1) % max_size_v;
                    } while(true);

                    xassert(false);
                }

                template< typename T_GUID >
                T* find(T_GUID Guid) noexcept
                {
                    return const_cast<T*>(const_cast<const map*>(this)->find(Guid));
                }

                template< typename T_GUID >
                T& get(T_GUID Guid) noexcept
                {
                    return *find(Guid);
                }

                template< typename T_GUID >
                T& get(T_GUID Guid) const noexcept
                {
                    return *find(Guid);
                }

                xcore::unique_span<T> m_Map{ T_MAXSIZE_V };
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
                for( auto pGroupInstance = m_pGroupHead.load(std::memory_order_relaxed); pGroupInstance; pGroupInstance = pGroupInstance->m_pNext )
                {
                    pGroupInstance->m_Count = pGroupInstance->m_NewCount.load(std::memory_order_relaxed);
                }
            }

            const sync_point::events::done::delegate m_SyncPointDoneDelegate{ this, &instance::msgSyncPointDone, m_EndSyncPoint.m_Events.m_Done };

            instance()
            {
                m_Definition.m_Triggers = xcore::scheduler::triggers::DONT_CLEAN_COUNT;
                registerComponents<entity::instance,group::details::default_tag>();
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
            entity::tmp_ref createEntity( group::instance& Group, entity::guid EntityGuid ) noexcept
            {
                auto  EntityRef = Group.CreateEntity(EntityGuid);
                auto& Entry     = m_mapEntity.append(EntityGuid);

                Entry.m_EntityRef.m_LocalIndex      = EntityRef.m_LocalIndex;
                Entry.m_EntityRef.m_PageIndex       = EntityRef.m_PageIndex;
                Entry.m_EntityRef.m_pGroupInstance  = EntityRef.m_pGroupInstance;
                return EntityRef;
            }

            group::instance& getOrCreateGroup( const std::span<const type_guid>    ComponentTypesGuids
                                             , const std::span<const type_guid>    TagTypesGuids ) noexcept
            {
                const auto          ComponentGroupGuid  = [&] {std::uint64_t Key = 0; for (auto E : ComponentTypesGuids) Key += E.m_Value; return Key; }();
                const auto          TagGroupGuid        = [&] {std::uint64_t Key = 0; for (auto E : TagTypesGuids)       Key += E.m_Value; return Key; }();
                const auto          GroupGuid           = group::guid{ ComponentGroupGuid + TagGroupGuid };

                //
                // Create a new group if we need to
                //
                if( auto pGroupDesc = m_mapGroup.find(GroupGuid); pGroupDesc == nullptr )
                {
                    pGroupDesc = &m_mapGroup.appendOrGet(GroupGuid, [&](const details::group_dec& Entry)
                    { 
                        if( Entry.m_pGroup.load(std::memory_order_relaxed) == nullptr )
                        {
                            // Spin lock
                            XCORE_PERF_ZONE_SCOPED_N("getOrCreateGroup::Wasting Time")
                            while( Entry.m_pGroup.load(std::memory_order_relaxed) == nullptr );
                        }
                    });

                    bool                bAddEntityComponent = true;
                    auto                nComponents         = ComponentTypesGuids.size();
                    query::filter_bits  Bits                {nullptr};
 
                    xcore::array<const component::descriptor*, max_ingroup_components_v> lComponetDesc;
                    for( int i=0; i<nComponents; ++i )
                    {
                        auto& C = ComponentTypesGuids[i];
                        xassert(C.m_Value);

                        // Collect all the component descriptors
                        lComponetDesc[i] = m_mapComponentDescriptor.get(C).m_pDescriptor;

                        // Make sure we set all the bits for the query
                        Bits.AddBit(lComponetDesc[i]->m_BitNumber);

                        // Did we found the entity in the list if so we dont need to add it
                        if( C == entity::instance::type_guid_v) bAddEntityComponent = false;
                    }

                    // Add the bits for the tags as well
                    for(auto E : TagTypesGuids)
                    {
                        Bits.AddBit(m_mapComponentDescriptor.get(E).m_pDescriptor->m_BitNumber);
                    }

                    // the entity if we have to
                    if(bAddEntityComponent)
                        lComponetDesc[nComponents++] = m_mapComponentDescriptor.get(entity::instance::type_guid_v).m_pDescriptor;

                    // Make sure that we store data in the ascending order
                    std::sort(  lComponetDesc.begin()
                            ,   lComponetDesc.begin() + nComponents
                            ,   [](const component::descriptor* pA, const component::descriptor* pB) { return pA->m_Guid.m_Value < pB->m_Guid.m_Value;} 
                    );

                    auto pGroupInstance = new group::instance
                    { 
                          m_PageMgr
                        , lComponetDesc.ViewTo(nComponents)
                        , TagTypesGuids 
                    };

                    // Add new entry into the query table
                    auto& QueryEntry    = m_lQueryTable[m_nQueryTables++];
                    QueryEntry.m_Bits   = Bits;
                    QueryEntry.m_pGroup = pGroupInstance;
                    
                    // Officially release the lock/set the group
                    for( pGroupInstance->m_pNext = m_pGroupHead.load(std::memory_order_relaxed); 
                         m_pGroupHead.compare_exchange_weak( pGroupInstance->m_pNext, pGroupInstance ) == false; );
                    pGroupDesc->m_pGroup.store( pGroupInstance, std::memory_order_relaxed );
                        
                    return *pGroupInstance;
                }
                else
                {
                    return *pGroupDesc->m_pGroup.load(std::memory_order_relaxed);
                }
            }

            template< typename...T_COMPONENTS_AND_TAGS >
            inline group::instance& getOrCreateGroup( void ) noexcept
            {
                constexpr static auto ComponentGuid = group::details::MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            group::details::compare, xcore::types::tuple_cat_t
                            < 
                                  std::tuple<entity::instance>
                                , decltype(group::details::FilterOutTagsExtendDoubleBuffer<T_COMPONENTS_AND_TAGS>())...
                            >
                        >*
                    >(nullptr)
                );

                constexpr static auto TagsGuid = group::details::MakeArrayTypeGuids
                ( reinterpret_cast
                    <
                        xcore::types::tuple_sort_t
                        < 
                            group::details::compare, xcore::types::tuple_cat_t
                            <
                                  std::tuple<group::details::default_tag>
                                , decltype(group::details::FilterOutComponents<T_COMPONENTS_AND_TAGS>())...
                            >
                        >*
                    >(nullptr)
                );

                return getOrCreateGroup(ComponentGuid,TagsGuid);
            }

            
            inline entity::tmp_ref createEntity(group::guid GroupGuid, entity::guid EntityGuid) noexcept
            {
                return createEntity( *m_mapGroup.get(GroupGuid).m_pGroup.load(std::memory_order_relaxed), EntityGuid);
            }

            template< typename T_COMPONENT >
            inline void registerComponent(void) noexcept
            {
                constexpr static component::descriptor component_descriptor_v
                {
                        T_COMPONENT::type_guid_v
                    ,   [](void* p) noexcept { new(p) T_COMPONENT{}; }
                    ,   [](void* p) noexcept { std::destroy_at( reinterpret_cast<T_COMPONENT*>(p) ); }
                        // //property::table&  m_PropertyTable;
                    ,   static_cast<std::uint16_t>(sizeof(T_COMPONENT))
                    ,   static_cast<std::uint16_t>(std::alignment_of_v<T_COMPONENT>)
                    ,   std::is_base_of_v<mecs::component::double_buffer, T_COMPONENT>
                    ,   -1
                };

                component_descriptor_v.m_BitNumber = m_RegistrationFreeBits.AllocBit();
                m_mapComponentDescriptor.append(T_COMPONENT::type_guid_v).m_pDescriptor = &component_descriptor_v;
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
                    for( auto E : system::query_v.m_All )
                    {
                        Instance.m_All.AddBit( m_mapComponentDescriptor.get(E).m_pDescriptor->m_BitNumber );
                    }

                    for( auto E : system::query_v.m_Any )
                    {
                        Instance.m_Any.AddBit( m_mapComponentDescriptor.get(E).m_pDescriptor->m_BitNumber );
                    }

                    for( auto E : system::query_v.m_None )
                    {
                        Instance.m_None.AddBit( m_mapComponentDescriptor.get(E).m_pDescriptor->m_BitNumber );
                    }

                    for( int i=0; i<function::list_v.size(); ++i )
                    {
                        const auto BitNum = m_mapComponentDescriptor.get(function::list_v[i]).m_pDescriptor->m_BitNumber;
                        Instance.m_Function.AddBit( BitNum );
                        if( function::const_list_v[i] == false ) Instance.m_Write.AddBit(BitNum);
                    }

                    // All arguments are a must
                    Instance.m_All.Add(Instance.m_Function);
                }
                
                //
                // Do query
                //
                Instance.m_lResults.clear();
                for( const auto& E : m_lQueryTable.ViewTo(m_nQueryTables) )
                {
                    if( false == E.m_Bits.Query( Instance.m_All, Instance.m_Any, Instance.m_None ) ) continue;

                    //
                    // Create new entry for this query
                    //
                    auto&       Entry       = Instance.m_lResults.append();
                    Entry.m_pGroup          = const_cast<group::instance*>(E.m_pGroup);

                    const auto& Group       = *E.m_pGroup;
                    const auto& lOffsets    = Group.m_lComponentOffsets;
                    const auto& lCTypeGuid  = Group.m_lComponentTypeGuids;
                    const int   Size        = Group.m_nComponents;
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

            void DeleteEntity( entity::guid Guid ) noexcept
            {
                auto pEntity = m_mapEntity.find(Guid);
                if( pEntity == nullptr ) return;

                entity::tmp_ref TmpRef{ nullptr,group::local_index{0u},group::page_index{0u}};
                if( xcore::lock::scope Lk(*pEntity); pEntity->m_Guid.m_Value )
                {
                    pEntity->m_Guid.m_Value = 0;
                    TmpRef = pEntity->m_EntityRef;
                }

                if(TmpRef.m_pGroupInstance) TmpRef.m_pGroupInstance->DeleteEntity(TmpRef);
            }
 
            constexpr static std::size_t                                max_entity_count_v = xcore::bits::RoundToNextPowOfTwo(100000000ull)
                                                                        XCORE_CMD_ASSERT(  - xcore::bits::RoundToNextPowOfTwo(100000000ull)
                                                                                           + xcore::bits::RoundToNextPowOfTwo(  1000000ull));
            constexpr static std::size_t                                max_groups_count_v = 1024;

            std::atomic<group::instance*>                                   m_pGroupHead                {nullptr};
            std::uint32_t                                                   m_nQueryTables              {0};
            group::page_mgr                                                 m_PageMgr                   {};
            details::map<details::entity_desc, max_entity_count_v>           m_mapEntity                 {};
            details::map<details::group_dec, max_groups_count_v>            m_mapGroup                  {};
            details::map<details::component_dec, 256>                       m_mapComponentDescriptor    {};
            xcore::array<details::query_table_entry, max_groups_count_v>    m_lQueryTable               {};
            bool                                                            m_bFrameStarted             { false };
            bool                                                            m_bPause                    { false };
            int                                                             m_FrameNumber               { 0 };
            query::filter_bits                                              m_RegistrationFreeBits      {xcore::not_null};
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

            World.AddGraphConnection<my_system>(World.m_StartSyncPoint, World.m_EndSyncPoint );

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
