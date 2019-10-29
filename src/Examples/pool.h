namespace containers
{
    template< typename T_TYPE, std::size_t T_PAGE_SIZE_V >
    class pool
    {
    public:

        constexpr static std::size_t    entries_per_page_v = T_PAGE_SIZE_V;
        using                           entry = T_TYPE;
        using                           self = pool;

        template< typename T >
        struct iterator
        {
            constexpr iterator (T& Class) noexcept
                : m_This(Class) {}

            iterator operator++ (void) noexcept
            {
                m_iEntry++;
                if (m_iEntry == entries_per_page_v)
                {
                    m_iPage++;
                    m_iEntry = 0;
                    if (m_iPage < m_This.m_lPages.size())
                        _mm_prefetch((const char*)m_This.m_lPages[m_iPage].get(), _MM_HINT_NTA);
                }
                return *this;
            }

            constexpr   bool    operator    !=  (std::nullptr_t)    const   noexcept
            {
                return m_This.m_Count != (m_iEntry + m_iPage * entries_per_page_v);
            }

            constexpr   auto& operator*   ()  const   noexcept
            {
                if constexpr (std::is_const_v<T>)
                    return *reinterpret_cast<const entry*>((*m_This.m_lPages[m_iPage])[m_iEntry].m_Data);
                else
                    return *reinterpret_cast<entry*>      ((*m_This.m_lPages[m_iPage])[m_iEntry].m_Data);
            }

            auto& operator*   ()  noexcept
            {
                if constexpr (std::is_const_v<T>)
                    return *reinterpret_cast<const entry*>((*m_This.m_lPages[m_iPage])[m_iEntry].m_Data);
                else
                    return *reinterpret_cast<entry*>      ((*m_This.m_lPages[m_iPage])[m_iEntry].m_Data);
            }

        private:

            T& m_This;
            std::size_t     m_iPage{ 0 };
            std::size_t     m_iEntry{ 0 };
        };

    public:

        constexpr   auto begin()                        const   noexcept { return iterator{ *this }; }
                    auto begin()                                noexcept { return iterator{ *this }; }
        constexpr   auto end()                          const   noexcept { return nullptr; }
        constexpr   auto size()                         const   noexcept { return m_Count; }

        ~pool(void) noexcept
        {
            if constexpr (std::is_destructible_v<entry>)
            {
                clear();
            }
        }

        template< typename...T_ARGS >
        std::size_t append(T_ARGS&&... Args) noexcept
        {
            // Grow if we need to
            if (m_Count == m_lPages.size() * entries_per_page_v)
                m_lPages.push_back(std::make_unique<page>());

            // construct the entry
            if constexpr ( std::is_constructible_v<entry> || sizeof...(T_ARGS) > 0 )
            {
                new(&(*m_lPages[m_Count / entries_per_page_v])[m_Count % entries_per_page_v].m_Data)
                    entry{ std::forward<T_ARGS>(Args)... };
            }
            else
            {
                m_Count++;
            }
            return m_Count;
        }

        template< typename...T_ARGS >
        std::size_t appendList( int Count, T_ARGS&&... Args) noexcept
        {
            const auto NewCount = m_Count + Count;

            // Grow if we need to
            while (NewCount > (m_lPages.size() * entries_per_page_v) )
                m_lPages.push_back(std::make_unique<page>());

            // construct the entry
            if constexpr (std::is_constructible_v<entry>)
            {
                for( ; m_Count < NewCount; m_Count++ )
                    new(&(*m_lPages[m_Count / entries_per_page_v])[m_Count % entries_per_page_v].m_Data)
                        entry{ std::forward<T_ARGS>(Args)... };
            }
            else
            {
                m_Count = NewCount;
            }

            return m_Count;
        }

        bool deleteBySwap(std::size_t Index) noexcept
        {
            m_Count--;
            auto& A = reinterpret_cast<entry&>((*m_lPages[Index / entries_per_page_v])[Index % entries_per_page_v].m_Data);
            if (Index != m_Count)
            {
                auto& B = reinterpret_cast<entry&>((*m_lPages[m_Count / entries_per_page_v])[m_Count % entries_per_page_v].m_Data);
                A = std::move(B);
                return true;
            }
            else if constexpr (std::is_destructible_v<entry>)
            {
                std::destroy_at(&A);
            }
            return false;
        }

        template< typename T >
        auto& operator[](T Index) noexcept
        {
            return reinterpret_cast<entry&>((*m_lPages[Index / entries_per_page_v])[Index % entries_per_page_v].m_Data);
        }

        template< typename T >
        constexpr auto& operator[](T Index) const noexcept
        {
            return reinterpret_cast<const entry&>((*m_lPages[Index / entries_per_page_v])[Index % entries_per_page_v].m_Data);
        }

        void clear(void) noexcept
        {
            if constexpr (std::is_destructible_v<entry>)
            {
                for (auto& E : *this) std::destroy_at(&E);
            }
            m_Count = 0;
        }

    protected:

        struct raw_entry
        {
            alignas(std::alignment_of_v<entry>) std::byte m_Data[sizeof(entry)];
        };

        using page = std::array<raw_entry, entries_per_page_v>;

    protected:

        std::vector<std::unique_ptr<page>>  m_lPages{};
        std::size_t                         m_Count{ 0 };
    };
}