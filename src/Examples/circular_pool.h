
namespace containers
{
    // Multi-core circular memory pool
    template< std::size_t T_PAGE_SIZE_V >
    struct cmd_pool
    {
        struct handle
        {
            int                             m_Index;
        };

        struct alignas(std::uint64_t) page
        {
            page*                           m_pNext { nullptr };
            std::byte                       m_Data[T_PAGE_SIZE_V];
        };

        struct cmd
        {
            cmd* m_pNext;
        };

        struct context
        {
            page*                           m_pFullPages{nullptr};
            int                             m_iLevel{0};
            cmd*                            m_pCmdList{nullptr};
            cmd*                            m_pCmdLast{ nullptr };
        };

        template< typename T_CMD >
        struct custom_cmd : cmd
        {
            T_CMD                           m_Cmd;
        };

        void getNewPage( context& Context ) noexcept
        {
            if( m_pEmptyPages == nullptr )
            {
                Context.m_pFullPages = new page;
            }
            else
            {
                auto pNext              = Context.m_pFullPages;
                Context.m_pFullPages    = m_pEmptyPages.load(std::memory_order_relaxed);
                while( m_pEmptyPages.compare_exchange_weak(Context.m_pFullPages, Context.m_pFullPages->m_pNext) == false);
                Context.m_pFullPages->m_pNext = pNext;
            }

            // Set the iLevel
            Context.m_iLevel = 0;
        }

        template< typename T, typename...T_ARGS >
        T& NewData(handle hHandle, T_ARGS&&...Args) noexcept
        {
            auto& Context = m_lContext[hHandle.m_Index];

            if (Context.m_pFullPages == nullptr) getNewPage(Context);

            do
            {
                auto& Page = *Context.m_pFullPages;
                std::byte* pData = xcore::bits::Align(&Page.m_Data[Context.m_iLevel], std::alignment_of_v<T>);
                std::byte* pNext = pData + sizeof(T);
                if (pNext > & Page.m_Data[T_PAGE_SIZE_V])
                {
                    // Is it too much memory?
                    getNewPage(Context);
                    continue;
                }

                Context.m_iLevel = static_cast<std::uint32_t>(pNext - pData);
                assert(Context.m_iLevel <= T_PAGE_SIZE_V);
                return *new(pData) T{ std::forward<T_ARGS>(Args)... };

            } while (true);
        }


        template< typename T, typename...T_ARGS >
        T& NewCmd(handle hHandle, T_ARGS&&...Args) noexcept
        {
            auto& Context = m_lContext[hHandle.m_Index];

            if( Context.m_pFullPages == nullptr ) getNewPage(Context);

            using type = custom_cmd<T>;
            type& Data = NewData<type>( hHandle, nullptr, std::forward<T_ARGS>(Args)... );
            
            if (Context.m_pCmdList == nullptr)
            {
                Context.m_pCmdLast = Context.m_pCmdList = &Data;
            }
            else
            {
                Context.m_pCmdLast = &Data;
            }
        }

        handle newContext( void ) noexcept
        {
            return { m_iContext++ };
        }

        std::array<context, 16>             m_lContext      {};
        std::atomic<page*>                  m_pEmptyPages   { nullptr };
        std::vector<std::unique_ptr<page>>  m_lPages        {};
        std::atomic<int>                    m_iContext      { 0 };
    };




}