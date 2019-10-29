
namespace containers
{
    // Multi-core circular memory pool
    template< std::size_t T_PAGE_SIZE_V >
    struct circular_pool
    {
        struct alignas(std::uint64_t) index
        {
            std::uint32_t                   m_Value;
            std::uint32_t                   m_Version;
        };

        struct context
        {
            std::vector<int>                m_lFullPages{};
            int                             m_iActivePage{ -1 };
            std::atomic<index>              m_iLevel{};
        };

        struct alignas(std::uint64_t) page
        {
            std::byte m_Data[T_PAGE_SIZE_V];
        };

        void PageFlip(void)
        {
            auto& Context = m_lContext[m_iContext];
            Context.m_iLevel        = 0;
            Context.m_iActivePage   = -1;

            m_lEmptyPages.insert(m_lEmptyPages.end(), Context.m_lFullPages.begin(), Context.m_lFullPages.end());
            Context.m_lFullPages.clear();

            m_iContext = 1 - m_iContext;
        }

        void getNewPage(void) noexcept
        {
            auto& Context = m_lContext[m_iContext];
            xcore::lock::scope LkScope(m_Lock);

            if (m_lEmptyPages.size() == 0)
            {
                m_lPages.push_back(std::unique_ptr<page>{new page});
                Context.m_iActivePage = static_cast<int>(m_lPages.size() - 1);
            }
            else
            {
                Context.m_iActivePage = m_lEmptyPages.back();
                m_lEmptyPages.erase(m_lEmptyPages.end());
            }
            Context.m_lFullPages.push_back(Context.m_iActivePage);

            // Set the iLevel
            for (auto iLevel = Context.m_iLevel.load(std::memory_order_relaxed); false == Context.m_iLevel.compare_exchange_weak(iLevel, index{0,iLevel.m_Version + 1}););
        }

        template< typename T, typename...T_ARGS >
        T& New(T_ARGS&&...Args) noexcept
        {
            auto& Context = m_lContext[m_iContext];

            if( Context.m_iActivePage == -1 ) getNewPage();

            do
            {
                auto&      Page     = *m_lPages[Context.m_iActivePage];
                auto       iLevel   = Context.m_iLevel.load(std::memory_order_relaxed);
                std::byte* pData    = xcore::bits::Align(&Page.m_Data[iLevel.m_Value], std::alignment_of_v<T>);
                std::byte* pNext    = pData + sizeof(T);
                if (pNext > &Page.m_Data[T_PAGE_SIZE_V])
                {
                    // Is it too much memory?
                    assert(Context.m_iActivePage);
                    getNewPage();
                    continue;
                }

                index       iNewLevel{ static_cast<std::uint32_t>(pNext - pData), iLevel.m_Version + 1 };

                if(Context.m_iLevel.compare_exchange_weak(iLevel,iNewLevel))
                    return *new(pData) T{std::forward<T_ARGS>(Args)... };

            } while (true);
        }

        xcore::lock::spin                   m_Lock{};
        int                                 m_iContext{ 0 };
        std::vector<std::unique_ptr<page>>  m_lPages{};
        std::vector<int>                    m_lEmptyPages{};
        std::array<context, 2>              m_lContext{};
    };




}