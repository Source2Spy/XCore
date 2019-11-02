
namespace mecs
{
    namespace component
    {
        using type_guid = xcore::guid::unit<64, struct component_type_guid_tag >;
    }

    namespace group
    {
        namespace details
        {
            template< std::size_t T_SIZE_V, typename...T_COMPONENTS >
            struct page
            {
                using                   ptr_tuple_t         = std::tuple< T_COMPONENTS *... >;
                using                   ptr_list_t          = std::array<std::byte*,    sizeof...(T_COMPONENTS)>;
                using                   offset_list_t       = std::array<std::uint32_t, sizeof...(T_COMPONENTS)>;
                using                   data_aligment_as_t  = std::uint64_t;
                constexpr static auto   raw_data_size_v     = static_cast<std::uint32_t>(T_SIZE_V);
                constexpr static auto   entry_count_v       = []() constexpr
                {
                    const offset_list_t     Aligments           { static_cast<std::uint32_t>(std::alignment_of_v<T_COMPONENTS>) ... };
                    const offset_list_t     Sizes               { static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) ... };
                    const std::uint32_t     SizeOfOne           = (static_cast<std::uint32_t>(sizeof(T_COMPONENTS)) + ...);
                    const std::uint32_t     OriginalAddress     = sizeof(data_aligment_as_t);
                    const std::uint32_t     WastedToAligment    = 0;
                    std::uint32_t           p                   = OriginalAddress;

                    for (int i = 0; i < Aligments.size(); i++)
                    {
                        WastedToAligment += xcore::bits::Align(p, Aligments[i]) - p;
                        p                += Sizes[i];
                    }

                    return (raw_data_size_v - WastedToAligment) / SizeOfOne;
                }();
                constexpr static auto   offset_list_v = []() constexpr
                {
                    const offset_list_t     Aligments           { static_cast<std::uint32_t>(std::alignment_of_v<T_COMPONENTS>) ... };
                    const std::uint32_t     OriginalAddress     = sizeof(std::uint64_t);
                    std::uint32_t           p                   = OriginalAddress;
                    offset_list_t           FinalOffsets;

                    for (int i = 0; i < FinalOffsets.size(); i++)
                    {
                        FinalOffsets[i] = xcore::bits::Align( p, Aligments[i] ) - OriginalAddress;
                        p              += entry_count_v * sizeof(T_COMPONENTS);
                    }
                    assert((p - OriginalAddress) <= raw_data_size_v);
                    return FinalOffsets;
                }();

                std::byte alignas(data_aligment_as_t)   m_RawData[raw_data_size_v];
            };
        }

        struct instance
        {
            using page_list = std::array<void*, 1024>;

            constexpr instance(const std::span<std::uint32_t> Offsets, std::uint32_t EntriesPerPage) noexcept
                : m_Offsets(Offsets)
                , m_EntriesPerPage(EntriesPerPage)
            {}

            std::uint32_t                   m_Count         {0};
            std::uint32_t                   m_nPages        {0};
            std::atomic<std::uint32_t>      m_NewCount      {0};
            page_list                       m_lPages        {nullptr};
            const std::uint32_t             m_EntriesPerPage;
            const std::span<std::uint32_t>  m_Offsets;
           // const std::span<type_guid>      m_IDs;
        };

        template< typename... T_COMPONENTS >
        struct custom final : instance
        {
            using my_page       = details::page<1024 * 64, T_COMPONENTS... >;
            constexpr custom(void) noexcept
                : instance{ my_page::offset_list_t, my_page::entry_count_v }
            {}
        };
    }




}
