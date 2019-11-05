
namespace mecs
{
    namespace group
    {
        struct instance;
        struct local_index  : xcore::units::type<local_index,   std::uint32_t> {using type::type;};
        struct page_index   : xcore::units::type<page_index,    std::uint32_t> {using type::type;};
        struct global_index : xcore::units::type<global_index,  std::uint32_t> {using type::type;};
    }

    using type_guid = xcore::guid::unit<64, struct component_type_guid_tag >;

    //---------------------------------------------------------------------------------
    // COMPONENT
    //---------------------------------------------------------------------------------
    namespace component
    {
        struct double_buffer_tag {};
        template< typename T_COMPONENT >
        struct double_buffer : double_buffer_tag
        {
            struct t0 : T_COMPONENT { using T_COMPONENT::T_COMPONENT; constexpr static auto type_guid_v = T_COMPONENT::type_guid_v; };
            struct t1 : T_COMPONENT { using T_COMPONENT::T_COMPONENT; constexpr static auto type_guid_v = T_COMPONENT::type_guid_v; };
        };
        struct tag {};

        struct descriptor
        {
            using fn_construct  = void(void*);
            using fn_destruct   = void(void*);

            type_guid           m_Guid;
            fn_construct&       m_fnConstruct;
            fn_construct&       m_fnDestruct;
            //property::table&  m_PropertyTable;
            std::uint16_t       m_Size;
            std::uint16_t       m_Aligment;
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
            constexpr static type_guid type_guid_v{0ull};
            entity::guid    m_Guid;     // This is 64 bits
            entity::flags   m_Flags;    // This is 32 bits
                                        // Means 32bits are been used to padding here
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
        /*
        namespace details
        {
            template< typename T_COMPONENTS, typename T_TAGS >
            struct page_info;

            template< typename...T_COMPONENTS >
            struct page_info_components
            {
                using                   ptr_tuple_t         = std::tuple< T_COMPONENTS *... >;
                using                   ptr_list_t          = std::array<std::byte*,    sizeof...(T_COMPONENTS)>;
                using                   offset_list_t       = std::array<std::uint32_t, sizeof...(T_COMPONENTS)>;
                using                   data_aligment_as_t  = mecs::group::page;
                constexpr static auto   raw_data_size_v     = static_cast<std::uint32_t>(sizeof(mecs::group::page));
                constexpr static auto   aligments_v         = offset_list_t{ static_cast<std::uint32_t>(std::alignment_of_v<T_COMPONENTS>) ... };
                constexpr static auto   sizes_v             = offset_list_t{ static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) ... };
                constexpr static auto   entry_count_v       = []() constexpr
                {
                    const std::uint32_t     SizeOfOne           = (static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) + ...);
                    const std::uint32_t     OriginalAddress     = sizeof(data_aligment_as_t);
                    std::uint32_t           WastedToAligment    = 0;
                    std::uint32_t           p                   = OriginalAddress;

                    for (int i = 0; i < aligments_v.size(); i++)
                    {
                        WastedToAligment += xcore::bits::Align(p, aligments_v[i]) - p;
                        p                += sizes_v[i];
                    }

                    return (raw_data_size_v - WastedToAligment) / SizeOfOne;
                }();
                constexpr static auto   offset_list_v = []() constexpr
                {
                    const std::uint32_t     OriginalAddress     = sizeof(data_aligment_as_t);
                    std::uint32_t           p                   = OriginalAddress;
                    offset_list_t           FinalOffsets{};

                    for (int i = 0; i < aligments_v.size(); i++)
                    {
                        FinalOffsets[i] = xcore::bits::Align( p, aligments_v[i] ) - OriginalAddress;
                        p              += entry_count_v * sizes_v[i];
                    }
                    assert((p - OriginalAddress) <= raw_data_size_v);
                    return FinalOffsets;
                }();

                constexpr static auto  is_double_buffer_v = std::array
                { (std::is_base_of_v<T_COMPONENTS, mecs::component::double_buffer_tag> ? std::uint8_t{0x0} : std::uint8_t{0xff}) ... };

                constexpr static auto  type_guid_v = std::array
                { T_COMPONENTS::type_guid_v ... };

                constexpr static void Construct(page* pPage, std::uint32_t Index) noexcept
                {
                    (
                        new( &pPage->m_RawData
                            [
                                offset_list_v[xcore::types::tuple_t2i_v<T_COMPONENTS*, ptr_tuple_t>]
                                + Index * sizeof(T_COMPONENTS)
                            ]
                        ) T_COMPONENTS,
                        ... 
                    );
                }

                constexpr static void Destruct(page* pPage, std::uint32_t Index) noexcept
                {
                    (
                        std::destroy_at
                        (
                            reinterpret_cast<T_COMPONENTS*>(&pPage->m_RawData
                            [
                                offset_list_v[xcore::types::tuple_t2i_v<T_COMPONENTS*, ptr_tuple_t>]
                                + Index * sizeof(T_COMPONENTS)
                            ])
                        ),
                        ...
                    );
                }
            };

            template< typename...T_TAGS >
            struct page_info_tags
            {

            };

            template< typename...T_COMPONENTS, typename...T_TAGS >
            struct page_info< std::tuple<T_COMPONENTS...>, std::tuple<T_TAGS...> > 
                : page_info_components<T_COMPONENTS...>
                , page_info_tags<T_TAGS...>
            {};

            //---------------------------------------------------------------------------------

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                  true == std::is_base_of_v<mecs::component::double_buffer_tag,     T_COMPONENT>
                , std::tuple<typename T_COMPONENT::t0, typename T_COMPONENT::t1>
            > FilterOutTagsExtendDoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                   false == std::is_base_of_v<mecs::component::double_buffer_tag,   T_COMPONENT>
                && false == std::is_base_of_v<mecs::component::tag, T_COMPONENT>
                , std::tuple<T_COMPONENT>
            > FilterOutTagsExtendDoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                   false == std::is_base_of_v<mecs::component::double_buffer_tag,   T_COMPONENT>
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
                constexpr static type_guid type_guid_v{ 0ull };
            };

            //---------------------------------------------------------------------------------
            template< typename T_A, typename T_B >
            struct compare
            {
                constexpr static bool value = T_A::type_guid_v.m_Value
                                            > T_B::type_guid_v.m_Value;
            };
        }
        */


        //---------------------------------------------------------------------------------
        // GROUP::PAGE
        //---------------------------------------------------------------------------------
        struct alignas(std::uint64_t) page
        {
            std::byte    m_RawData[1024*64];
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
        // GROUP::INSTANCE
        //---------------------------------------------------------------------------------

        namespace details
        {
            struct offsets
            {
                std::uint16_t m_Offset;
                std::uint16_t m_Size;
            };
        }


        struct instance
        {
            instance( page_mgr& PageMgr, group::instance& GroupInstance, std::span<component::descriptor*> lComponetDesc, const std::span<const type_guid> TagTypesGuids) noexcept
                : m_PageMgr             { PageMgr }
                , m_lComponentTypeGuids { [&]() constexpr { std::array<type_guid,64>                D; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i]->m_Guid;  return D; }() }
                , m_lTagGuids           { [&]() constexpr { std::array<type_guid,64>                D; for (int i = 0; i < TagTypesGuids.size(); ++i) D[i] = TagTypesGuids[i];          return D; }() }
                , m_lDescriptors        { [&]() constexpr { std::array<component::descriptor*,64>   D; for (int i = 0; i < lComponetDesc.size(); ++i) D[i] = lComponetDesc[i];          return D; }() }
                , m_nTagGuids           { static_cast<std::uint16_t>(TagTypesGuids.size()) }
                , m_nComponentTypeGuids { static_cast<std::uint16_t>(lComponetDesc.size()) }
                , m_EntriesPerPage      { [&]() constexpr
                                            {
                                                const std::uint32_t     SizeOfOne           = [&]() constexpr { std::uint32_t S = 0; for (auto p : lComponetDesc) S += p->m_Size; return S; }();
                                                const std::uint32_t     OriginalAddress     = sizeof(page);
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
                                                std::array<details::offsets, 64>      lComponentOffsets;
                                                const std::uint32_t                   OriginalAddress     = sizeof(page);
                                                std::uint32_t                         p                   = OriginalAddress;

                                                for (int i = 0; i < lComponetDesc.size(); i++)
                                                {
                                                    lComponentOffsets[i].m_Offset = xcore::bits::Align( p, lComponetDesc[i]->m_Aligment ) - OriginalAddress;
                                                    p                            += m_EntriesPerPage * lComponetDesc[i]->m_Size;
                                                }
                                                assert((p - OriginalAddress) <= sizeof(page));
                                                return lComponentOffsets;
                                            }()
                                        }
                {}

            void ConstructEntity( page* Page, std::uint32_t LocalIndex ) noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_lComponentOffsets.size(); ++i )
                {
                    auto& E = m_lComponentOffsets[i];
                    m_lDescriptors[i]->m_fnConstruct( &(&p[ E.m_Offset ])[ LocalIndex * E.m_Size ] );
                }
            }

            void DestructEntity( page* Page, std::uint32_t LocalIndex ) noexcept
            {
                std::byte* p = reinterpret_cast<std::byte*>(Page);
                for( int i=0; i<m_lComponentOffsets.size(); ++i )
                {
                    auto& E = m_lComponentOffsets[i];
                    m_lDescriptors[i]->m_fnDestruct( &(&p[ E.m_Offset ])[ LocalIndex * E.m_Size ] );
                }
            }

            entity::tmp_ref CreateEntity( entity::guid Guid )
            {
                const auto Index         = m_NewCount++;
                const auto RelativeIndex = Index % m_EntriesPerPage;
                const auto PageIndex     = Index / m_EntriesPerPage;
                do
                {
                    const auto nPages   = m_nPages.load(std::memory_order_relaxed);

                    if (PageIndex < nPages)
                    {
                        // Create entry here
                        ConstructEntity( m_lPages[PageIndex], RelativeIndex );
                        break;
                    }
                    else if (PageIndex == nPages && RelativeIndex == 0)
                    {
                        m_lPages[PageIndex] = m_PageMgr.AllocPage();
                        m_nPages++;
                        ConstructEntity( m_lPages[PageIndex], RelativeIndex );
                        break;
                    }
                    else
                    {
                        // We will have to wait...
                        XCORE_PERF_ZONE_SCOPED_N("Wasting Time")
                        XCORE_CMD_PROFILER( while (PageIndex >= m_nPages.load(std::memory_order_relaxed)); )
                    }

                } while (true);

                return { this, local_index{RelativeIndex}, page_index{PageIndex} };
            }

            using page_list = std::array<page*, 1024*8>;

            page_mgr&                                       m_PageMgr;
            std::uint32_t                                   m_NewCount      { 0 };
            std::uint32_t                                   m_Count         { 0 };
            std::atomic<std::uint32_t>                      m_nPages        { 0 };
            page_list                                       m_lPages        {};
            const std::array<type_guid, 64>                 m_lComponentTypeGuids;
            const std::array<type_guid, 64>                 m_lTagGuids;
            const std::array<component::descriptor*, 64>    m_lDescriptors;
            const std::array<details::offsets, 64>          m_lComponentOffsets;
            const std::uint16_t                             m_nComponentTypeGuids;
            const std::uint16_t                             m_nTagGuids;
            const std::uint16_t                             m_EntriesPerPage;
        };

        /*
        struct instance
        {

            template< typename T_PAGE_INFO >
            constexpr instance(bool, page_mgr& PageMgr, T_PAGE_INFO*) noexcept
                : m_PageMgr             { PageMgr }
                , m_TypeSizes           (T_PAGE_INFO::sizes_v)
                , m_Offsets             (T_PAGE_INFO::offset_list_v)
                , m_EntriesPerPage      (T_PAGE_INFO::entry_count_v)
                , m_isDoubleBuffer      (T_PAGE_INFO::is_double_buffer_v)
                , m_ComponentTypeGuid   (T_PAGE_INFO::type_guid_v)
                , m_fnConstruction       (T_PAGE_INFO::Construct)
                , m_fnDestruction        (T_PAGE_INFO::Destruct)
            {}

            template< typename...T_COMPONENTS_AND_TAGS >
            constexpr instance(page_mgr& PageMgr, T_COMPONENTS_AND_TAGS* ... ) noexcept
                : instance
                ( 
                    false
                    , PageMgr
                    , reinterpret_cast
                    <
                        details::page_info
                        <
                            xcore::types::tuple_sort_t
                            < 
                                details::compare, xcore::types::tuple_cat_t
                                < 
                                    decltype(details::FilterOutTagsExtendDoubleBuffer<T_COMPONENTS_AND_TAGS>())...
                                >
                            >
                            , 
                            xcore::types::tuple_sort_t
                            < 
                                details::compare, xcore::types::tuple_cat_t
                                <
                                    decltype(details::FilterOutComponents<T_COMPONENTS_AND_TAGS>())...
                                >
                            >
                        >*
                    >(nullptr)
                )
            {}

            constexpr   const auto  begin()                 const   noexcept { return iterator{ const_cast<instance*>(this), local_index{0u}, page_index{0u} }; }
                        auto        begin()                         noexcept { return iterator{ this, local_index{0u}, page_index{0u} }; }
            constexpr   auto        end()                   const   noexcept { return nullptr; }
            constexpr   auto        size()                  const   noexcept { return m_Count; }
            inline      auto        newSize()               const   noexcept { return m_NewCount.load(std::memory_order_relaxed); }


            entity::tmp_ref CreateEntity( entity::guid Guid )
            {
                const auto Index         = m_NewCount++;
                const auto RelativeIndex = Index % m_EntriesPerPage;
                const auto PageIndex     = Index / m_EntriesPerPage;
                do
                {
                    const auto nPages   = m_nPages.load(std::memory_order_relaxed);

                    if (PageIndex < nPages)
                    {
                        // Create entry here
                        m_fnConstruction( m_lPages[PageIndex], RelativeIndex );
                        break;
                    }
                    else if (PageIndex == nPages && RelativeIndex == 0)
                    {
                        m_lPages[PageIndex] = m_PageMgr.AllocPage();
                        m_nPages++;
                        m_fnConstruction(m_lPages[PageIndex], RelativeIndex);
                        break;
                    }
                    else
                    {
                        // We will have to wait...
                        XCORE_PERF_ZONE_SCOPED_N("Wasting Time")
                        XCORE_CMD_PROFILER( while (PageIndex >= m_nPages.load(std::memory_order_relaxed)); )
                    }

                } while (true);

                return { this, local_index{RelativeIndex}, page_index{PageIndex} };
            }

            inline
            void* getComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) noexcept
            {
                for (int i = 0; i < m_ComponentTypeGuid.size(); ++i)
                {
                    if (m_ComponentTypeGuid[i] >= TypeGuid )
                    {
                        if (m_ComponentTypeGuid[i] > TypeGuid) break;

                        std::byte* p = &reinterpret_cast<std::byte*>(m_lPages[Ref.m_PageIndex.m_Value])[m_Offsets[i]];
                        const auto d = m_isDoubleBuffer[i];
                        if (d == 0xff)  return &p[Ref.m_LocalIndex.m_Value       * m_TypeSizes[i]];
                        else            return &p[(Ref.m_LocalIndex.m_Value + d) * m_TypeSizes[i]];
                    }
                }

                return nullptr;
            }

            inline
            const void* getComponent(const entity::tmp_ref& Ref, type_guid TypeGuid) const noexcept
            {
                return const_cast<instance*>(this)->getComponent(Ref, TypeGuid);
            }

            using construction = void (page * pPage, std::uint32_t Index);
            using destruction  = void (page * pPage, std::uint32_t Index);

            std::uint32_t                               m_Count             {0u};
            std::atomic<std::uint32_t>                  m_nPages            {0u};
            std::atomic<std::uint32_t>                  m_NewCount          {0u};
            page_list                                   m_lPages            {};
            page_mgr&                                   m_PageMgr;
            construction&                               m_fnConstruction;
            destruction&                                m_fnDestruction;
            const std::uint32_t                         m_EntriesPerPage;
            const std::span<const std::uint32_t>        m_TypeSizes;
            const std::span<const std::uint32_t>        m_Offsets;
            const std::span<const std::uint8_t>         m_isDoubleBuffer;
            const std::span<const type_guid>            m_ComponentTypeGuid;
        };

        //---------------------------------------------------------------------------------
        // GROUP::MAKE INSTANCE
        //---------------------------------------------------------------------------------
        template< typename...T_COMPONENTS_AND_TAGS >
        auto MakeInstance(page_mgr& PageMgr) noexcept
        {
            return std::unique_ptr<instance>
            { 
                new instance
                { 
                    PageMgr
                    , reinterpret_cast<entity::instance*>(nullptr)              // Entity always go first
                    , reinterpret_cast<details::default_tag*>(nullptr)          // We must have a least one tag
                    , reinterpret_cast<T_COMPONENTS_AND_TAGS*>(nullptr) ...
                } 
            };
        }
        */

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
                    using fn_destruct = void(void*);

                    type_guid       m_Guid;
                    fn_construct& m_Construct;
                    fn_destruct& m_Destruct;
                    std::uint16_t   m_Size;
                    bool            m_isDoubleBuffer;
                };

                struct entity_dec
                {
                    std::atomic<entity::guid>                       m_Guid{ 0 };
                    entity::tmp_ref                                 m_EntityRef{ nullptr, group::local_index{0u}, group::page_index{0u} };
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

                template< typename T, std::size_t T_MAXSIZE_V > 
                struct map
                {
                    constexpr static auto max_size_v = T_MAXSIZE_V;

                    template< typename T_GUID >
                    T& append(T_GUID Guid) noexcept
                    {
                        for (auto Index = Guid.m_Value % max_size_v; true; ++Index)
                        {
                            auto&   Entry = m_Map[Index];
                            auto    Local = Entry.m_Guid.load(std::memory_order_relaxed);

                            if (Local.isValid()) continue;

                            if (Entry.m_Guid.compare_exchange_weak(Local, Guid))
                            {
                                return Entry;
                            }
                            else
                            {
                                // Lets try again
                                Index--;
                            }
                        }

                        xassert(false);
                    }

                    template< typename T_GUID, typename T_WAIT_LAMBDA >
                    T& appendOrGet(T_GUID Guid, T_WAIT_LAMBDA&& WaitLambda) noexcept
                    {
                        for (auto Index = Guid.m_Value % max_size_v; true; ++Index)
                        {
                            auto& Entry = m_Map[Index];
                            auto  Local = Entry.m_Guid.load(std::memory_order_relaxed);

                            if (Local == Guid)
                            {
                                WaitLambda(Entry);
                                return Entry;
                            }

                            if ( Local.isValid() ) continue;

                            if (Entry.m_Guid.compare_exchange_weak(Local, Guid))
                            {
                                return Entry;
                            }
                            else
                            {
                                // Lets try again
                                Index--;
                            }
                        }
                        xassert(false);
                    }

                    template< typename T_GUID >
                    T* find(T_GUID Guid) noexcept
                    {
                        for (auto Index = Guid.m_Value % max_size_v; true; ++Index)
                        {
                            auto&   Entry = m_Map[Index];
                            auto    Local = Entry.m_Guid.load(std::memory_order_relaxed);
                            if (Local == Guid) return &Entry;
                            if (Local.isValid() == false) return nullptr;;
                        }

                        xassert(false);
                    }

                    template< typename T_GUID >
                    T& get(T_GUID Guid) noexcept
                    {
                        return *find(Guid);
                    }

                    xcore::unique_span<T> m_Map{ T_MAXSIZE_V };
                };
            }

            //---------------------------------------------------------------------------------
            // WORLD::instance
            //---------------------------------------------------------------------------------
            struct instance
            {
                entity::tmp_ref createEntity( group::instance& Group, entity::guid EntityGuid )
                {
                    auto  EntityRef = Group.CreateEntity(EntityGuid);
                    auto& Entry     = m_mapEntity.append(EntityGuid);

                    Entry.m_EntityRef.m_LocalIndex      = EntityRef.m_LocalIndex;
                    Entry.m_EntityRef.m_PageIndex       = EntityRef.m_PageIndex;
                    Entry.m_EntityRef.m_pGroupInstance  = EntityRef.m_pGroupInstance;
                    return EntityRef;
                }

                group::instance& getOrCreateGroup( const std::span<const type_guid>    ComponentTypesGuids
                                                 , const std::span<const type_guid>    TagTypesGuids )
                {
                    const auto          ComponentGroupGuid  = [&] {std::uint64_t Key = 0; for (auto E : ComponentTypesGuids) Key += E; return Key; }();
                    const auto          TagGroupGuid        = [&] {std::uint64_t Key = 0; for (auto E : TagTypesGuids)       Key += E; return Key; }();
                    const auto          GroupGuid           = group::guid{ ComponentGroupGuid + TagGroupGuid };

                    //
                    // Create a new group if we need to
                    //
                    if( auto pGroupDesc = m_mapGroup.find(GroupGuid); pGroupDesc == nullptr )
                    {
                        pGroupDesc = &m_mapGroup.appendOrGet(GroupGuid, [&](const details::group_dec& Entry)
                        { 
                            while( Entry.m_upGroup.load(std::memory_order_relaxed).get() == nullptr )
                            {
                                // Spin lock
                            };
                        });

                        xcore::array<const component::descriptor*, 128> m_lComponetDesc;
                        for( int i=0; i< ComponentTypesGuids.size(); ++i )
                            m_lComponetDesc[i] = m_mapComponentDescriptor.get(ComponentTypesGuids[i]).m_pDescriptor;
                        
                        std::sort(  m_lComponetDesc.begin()
                                ,   m_lComponetDesc.begin() + ComponentTypesGuids.size()
                                ,   [](const component::descriptor* pA, const component::descriptor* pB) { return pA->m_Guid.m_Value < pA->m_Guid.m_Value;} 
                        );

                        pGroupDesc->m_pGroup = new group::instance{ *this, m_lComponetDesc.ViewTo(ComponentTypesGuids.size()), TagTypesGuids };
                        
                        return *pGroupDesc->m_upGroup;
                    }
                    else
                    {
                        return *pGroupDesc->m_upGroup;
                    }
                }

                entity::tmp_ref createEntity(group::guid GroupGuid, entity::guid EntityGuid)
                {
                    return createEntity(m_mapGroup.get(GroupGuid), EntityGuid);
                }

                template< typename T_COMPONENT >
                void registerComponent(void) noexcept
                {
                    constexpr static component::descriptor component_v
                    {
                            T_COMPONENT::type_guid_v
                        ,   [](void* p) { new(p) T_COMPONENT{}; }
                        ,   [](void* p) { std::destroy_at( reinterpret_cast<T_COMPONENT*>(p) ); }
                            // //property::table&  m_PropertyTable;
                        ,   static_cast<std::uint16_t>(sizeof(T_COMPONENT))
                        ,   static_cast<std::uint16_t>(std::alignment_of_v<T_COMPONENT>)
                    };

                    m_mapComponentDescriptor.append(T_COMPONENT::type_guid_v).m_pDescriptor = &component_v;
                }

                constexpr static std::size_t                            max_entity_count_v = xcore::bits::RoundToNextPowOfTwo(100000000ull);

                details::map<details::entity_dec, max_entity_count_v>   m_mapEntity                 {};
                details::map<details::group_dec, 1024>                  m_mapGroup                  {};
                details::map<details::component_dec, 256>               m_mapComponentDescriptor    {};
            };
        }

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

    //------------------------------------------------------------------------------------
    // ENTITY:FUNCTIONS
    //------------------------------------------------------------------------------------
    namespace entity
    {
        template< typename T >
        T& tmp_ref::getComponent(type_guid TypeGuid) noexcept
        {
            return *reinterpret_cast<T*>(m_pGroupInstance->getComponent(*this,TypeGuid));
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

        struct double_buff : component::double_buffer<double_buff>
        {
            constexpr static type_guid type_guid_v{ "double_buff" };
            int m_Value{ 123 };
        };

        struct my_tag : component::tag 
        {
            constexpr static type_guid type_guid_v{ "my_tag" };
        };

        void Test()
        {
            group::page_mgr PageMgr;
            auto upGroup = group::MakeInstance<simple,double_buff,my_tag>(PageMgr);

            xcore::scheduler::channel MyChannel(xconst_universal_str("Test"));

            for(int i=0; i<xcore::get().m_Scheduler.getWorkerCount(); i++)
                MyChannel.SubmitJob([&]
                {
                    while (upGroup->newSize() < 10000000)
                    {
                        auto    tmpEntityRef = upGroup->appendEntity();
                        auto&   Entity = tmpEntityRef.getComponent<entity::instance>();
                        auto&   Simple = tmpEntityRef.getComponent<simple>();
                        auto&   Double = tmpEntityRef.getComponent<double_buff>();
                        assert(Simple.m_Value == 22);
                        assert(Double.m_Value == 123);
                    }
                });

            MyChannel.join();

            int b = 0;
        }
    }
}
