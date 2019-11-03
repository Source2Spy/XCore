
namespace mecs
{
    namespace group
    {
        struct instance;
    }

    namespace entity
    {
        using guid = xcore::guid::unit<64, struct ecs_entity_tag>;
        struct index { std::uint32_t m_Value; };

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
            //using factory = factory::custom_instance<0, instance, 1024>;
            entity::guid    m_Guid;     // This is 64 bits
            entity::flags   m_Flags;    // This is 32 bits
                                        // Means 32bits are been used to padding here
        };

        struct tmp_ref
        {
            constexpr bool isValid(void) const noexcept { return !!m_pGroupInstance; }

            //template< typename T >
            //T& getComponent(component::type_guid TypeGuid = T::factory::guid_component_type_v) noexcept;

            //template< typename T >
            //constexpr const T& getComponent(component::type_guid TypeGuid = T::factory::guid_component_type_v) const noexcept;

            //std::vector<properties_tuple> getProperties(void) const noexcept;

            group::instance*        m_pGroupInstance{ nullptr };
            index                   m_Index{};
        };
    }

    namespace component
    {
        using type_guid = xcore::guid::unit<64, struct component_type_guid_tag >;

        struct double_buffer_tag {};
        template< typename T_COMPONENT >
        struct double_buffer : double_buffer_tag
        {
            struct t0 : T_COMPONENT { using T_COMPONENT::T_COMPONENT; using factory = typename T_COMPONENT::factory; };
            struct t1 : T_COMPONENT { using T_COMPONENT::T_COMPONENT; using factory = typename T_COMPONENT::factory; };
        };
        struct tag {};
    }

    namespace group
    {
        struct alignas(std::uint64_t) page
        {
            std::byte    m_RawData[1024*64];
        };

        namespace details
        {
            template< typename...T_COMPONENTS >
            struct page_info
            {
                using                   ptr_tuple_t         = std::tuple< T_COMPONENTS *... >;
                using                   ptr_list_t          = std::array<std::byte*,    sizeof...(T_COMPONENTS)>;
                using                   offset_list_t       = std::array<std::uint32_t, sizeof...(T_COMPONENTS)>;
                using                   data_aligment_as_t  = mecs::group::page;
                constexpr static auto   raw_data_size_v     = static_cast<std::uint32_t>(sizeof(mecs::group::page));
                constexpr static offset_list_t aligments_v         { static_cast<std::uint32_t>(std::alignment_of_v<T_COMPONENTS>) ... };
                constexpr static offset_list_t sizes_v             { static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) ... };
                constexpr static auto   entry_count_v       = []() constexpr
                {
                    const std::uint32_t     SizeOfOne           = (static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) + ...);
                    const std::uint32_t     OriginalAddress     = sizeof(data_aligment_as_t);
                    const std::uint32_t     WastedToAligment    = 0;
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
                    const std::uint32_t     OriginalAddress     = sizeof(std::uint64_t);
                    std::uint32_t           p                   = OriginalAddress;
                    offset_list_t           FinalOffsets;

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

                constexpr static auto  component_type_guid_v = std::array
                { T_COMPONENTS::factory::component_type_guid_v ... };


                constexpr static void Construct(page* pPage, std::uint32_t Index) noexcept
                {
                    std::array<void*, sizeof...(T_COMPONENTS)> X =
                    {
                        new( &pPage->m_RawData
                            [
                                offset_list_v[xcore::types::tuple_t2i_v<T_COMPONENTS*, ptr_tuple_t>]
                                + Index * sizeof(T_COMPONENTS)
                            ]
                        ) T_COMPONENTS
                        ... 
                    };
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
                            ]
                        )
                        ,
                        ...
                    );
                }
            };

            //---------------------------------------------------------------------------------

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                true == std::is_base_of_v<T_COMPONENT, mecs::component::double_buffer_tag>
                , std::tuple<typename T_COMPONENT::t0, typename T_COMPONENT::t1>
            > DoubleBuffer();

            template< typename T_COMPONENT >
            std::enable_if_t
            <
                false == std::is_base_of_v<T_COMPONENT, mecs::component::double_buffer_tag>
                , std::tuple<T_COMPONENT>
            > DoubleBuffer();

            //---------------------------------------------------------------------------------

            template< typename...T_COMPONENTS >
            page_info<T_COMPONENTS...> MakePageType(std::tuple<T_COMPONENTS...>*);
        }

        struct page_mgr
        {
            struct next
            {
                next* m_pNext;
            };

            std::atomic<next*> m_pFreeHead;

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

        struct instance
        {
            using page_list = std::array<page*, 1024>;

            template< typename T_PAGE_INFO >
            constexpr instance(T_PAGE_INFO*) noexcept
                : m_Offsets             (T_PAGE_INFO::offset_list_t)
                , m_EntriesPerPage      (T_PAGE_INFO::entry_count_v)
                , m_isDoubleBuffer      (T_PAGE_INFO::is_double_buffer_v)
                , m_ComponentTypeGuid   (T_PAGE_INFO::component_type_guid_v)
                , m_pConstruction       (&T_PAGE_INFO::Construct)
                , m_pDestruction        (&T_PAGE_INFO::Destruct)
            {}

            template< typename...T_COMPONENTS >
            constexpr instance(page_mgr& PageMgr, T_COMPONENTS* ... ) noexcept
                : m_PageMgr{ PageMgr }
                , instance( false, reinterpret_cast<decltype(details::MakePageType< xcore::types::tuple_cat_t< decltype(details::DoubleBuffer<T_COMPONENTS>())... >>)>(nullptr))
            {}

            entity::tmp_ref appendEntity()
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
                        m_pConstruction( m_lPages[PageIndex], RelativeIndex );
                        break;
                    }
                    else if (PageIndex == nPages && RelativeIndex == 0)
                    {
                        m_lPages[++m_nPages] = m_PageMgr.AllocPage();
                        m_pConstruction(m_lPages[PageIndex], RelativeIndex);
                        break;
                    }
                    else
                    {
                        // We will have to wait...
                    }

                } while (true);
            }

            using construction = void (page * pPage, std::uint32_t Index);
            using destruction  = void (page * pPage, std::uint32_t Index);

            std::uint32_t                               m_Count         {0};
            std::atomic<std::uint32_t>                  m_nPages        {0};
            std::atomic<std::uint32_t>                  m_NewCount      {0};
            page_list                                   m_lPages        {};
            page_mgr&                                   m_PageMgr;
            const construction                          m_pConstruction;
            const destruction                           m_pDestruction;
            const std::uint32_t                         m_EntriesPerPage;
            const std::span<std::uint32_t>              m_Offsets;
            const std::span<std::uint8_t>               m_isDoubleBuffer;
            const std::span<component::type_guid>       m_ComponentTypeGuid;
        };

        template< typename...T_COMPONENTS >
        auto MakeInstance(page_mgr& PageMgr) noexcept
        {
            return std::unique_ptr<instance>
            { 
                new instance
                { 
                    PageMgr
                    , reinterpret_cast<entity::instance>(nullptr)
                    , reinterpret_cast<T_COMPONENTS>(nullptr) ... 
                } 
            };
        }

    }




}
