#ifndef _XCORE_VECTORS_H
#define _XCORE_VECTORS_H
#pragma once

namespace xcore::containers::vector
{
    namespace details
    {
        template<typename T_ALLOCATOR >
        struct has_malloc_internal
        {
            template<typename T>
            static constexpr auto check(T*) -> typename std::is_same
            <
                decltype(std::declval<T>().malloc(0))
                , xcore::err
            >::type;

            template<typename>
            static constexpr std::false_type check(...);

            using type = decltype(check<T_ALLOCATOR>(nullptr));
        };

        template<typename T_ALLOCATOR >
        static constexpr bool has_malloc_v = has_malloc_internal<T_ALLOCATOR>::type::value;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR >
    class type
    {
    public:

        using self          = type;
        using value_type    = T;
        using size_type     = typename T_ALLOCATOR::size_type;
        using view          = typename std::span<value_type>;

        constexpr static size_type DEFAULT_GROWTH = 127;

        // it must have a function to allocate memory
        static_assert( details::has_malloc_v<T_ALLOCATOR> );

        inline                                                  type                        ( self&& Container )                                noexcept : m_Allocator( std::move(Container.m_Allocator) ), m_Count(Container.m_Count) { Container.m_Count=0; }
        constexpr                                               type                        ( void )                                            noexcept = default;
        inline                                                 ~type                        ( void )                                            noexcept { clear(); }

        inline      auto&                                       operator =                  ( self&& Container )                                noexcept { clear(); m_Allocator = std::move(Container.m_Allocator); m_Count = Container.m_Count; Container.m_Count=0; return *this; }
        inline      auto&                                       operator =                  ( const self& Container )                           noexcept { m_Allocator = Container.m_Allocator; m_Count = Container.m_Count; return *this; }
        constexpr   auto*                                       data                        ( void )                                            noexcept { return reinterpret_cast<T*>      (m_Allocator.data()); }
        constexpr   auto*                                       data                        ( void )                                    const   noexcept { return reinterpret_cast<const T*>(m_Allocator.data()); }
        constexpr   bool                                        isIndexValid                ( size_type i )                             const   noexcept { return i >= 0 && i < m_Count; }
        inline      auto&                                       operator []                 ( size_type i )                                     noexcept { xassert(isIndexValid(i)); return data()[i]; }
        constexpr   auto&                                       operator []                 ( size_type i )                             const   noexcept { xassert(isIndexValid(i)); return data()[i]; }
        inline      auto&                                       at                          ( size_type i )                                     noexcept { xassert(isIndexValid(i)); return data()[i]; }
        constexpr   auto&                                       at                          ( size_type i )                             const   noexcept { xassert(isIndexValid(i)); return data()[i]; }
        constexpr   auto                                        size                        ( void )                                    const   noexcept { return m_Count; }
        constexpr   auto&                                       front                       ( void )                                    const   noexcept { return *data(); }
        inline      auto&                                       front                       ( void )                                            noexcept { return *data(); }
        constexpr   auto&                                       back                        ( void )                                    const   noexcept { return data()[m_Count-1]; }
        inline      auto&                                       back                        ( void )                                            noexcept { return data()[m_Count-1]; }
        constexpr   bool                                        empty                       ( void )                                    const   noexcept { return m_Count==0; }
        constexpr   size_type                                   capacity                    ( void )                                    const   noexcept { return m_Allocator.size(); }
        inline      void                                        clear                       ( void )                                            noexcept;
        inline      void                                        reserve                     ( size_type NewCap )                                noexcept;
        inline      xcore::err                                  GrowBy                      ( size_type NewItems )                              noexcept;

        inline      value_type&                                 insert                      ( const size_type Index )                           noexcept { insert(Index,1); return m_Allocator[Index]; }
        inline      void                                        insert                      ( const size_type Index, const size_type Count )    noexcept;
        inline      void                                        insert                      ( const size_type Index, const view View )          noexcept;

        inline      void                                        CopyFromTo                  ( const view View, size_type DestinationOffset = size_type{0} )  noexcept;
        inline      void                                        eraseCollapse               ( const size_type Index, const size_type Count=1 )  noexcept;
        inline      void                                        eraseSwap                   ( const size_type Index, const size_type Count=1 )  noexcept;
    
        template<typename ...T_ARG>
        inline      value_type&                                 append                      ( T_ARG&&...Args )                                  noexcept { auto Index = m_Count; appendList(1, std::forward<T_ARG&&>(Args)... ); return data()[Index]; }
    
        template<typename ...T_ARG>
        inline      value_type&                                 append                      ( size_type& Index, T_ARG&&...Args )                noexcept {      Index = m_Count; appendList(1, std::forward<T_ARG>(Args)... ); return data()[Index]; }
    
        template<typename ...T_ARG>
        inline      void                                        appendList                  ( const size_type Count, T_ARG&&...Args )           noexcept;

        inline      void                                        appendList                  ( const view View )                                 noexcept;
        
        inline      auto                                        begin                       ( void )                                            noexcept { return xcore::span{ data(), m_Count }.begin(); }
        inline      auto                                        begin                       ( void )                                      const noexcept { return xcore::span{ data(), m_Count }.begin(); }
        inline      auto                                        end                         ( void )                                      const noexcept { return xcore::span{ data(), m_Count }.end();   }
        inline      auto                                        end                         ( void )                                            noexcept { return xcore::span{ data(), m_Count }.end();   }

        // More of the interface is found here
        #include "Implementation/xcore_linear_buffers_hardness.h"

    private:

        T_ALLOCATOR         m_Allocator         {};
        size_type           m_Count             {0};
    };
}

namespace xcore
{
    template< typename T >
    using vector = xcore::containers::vector::type< T, xcore::unique_rawspan< T, std::uint64_t > >;
}

#endif