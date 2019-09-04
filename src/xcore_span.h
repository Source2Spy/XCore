#ifndef _XCORE_SPAN_H
#define _XCORE_SPAN_H
#pragma once

namespace xcore
{
    template< typename T, std::size_t T_SIZE = std::dynamic_extent >
    struct span : std::span<T, T_SIZE>
    {
        using value_type = T;
        using size_type  = std::size_t;

        using parent = std::span<T, T_SIZE>;
        using parent::span;
        using parent::operator [];
        using parent::operator =;
        using parent::data;
        using parent::size; 

        template < typename C = size_type, typename = std::enable_if_t<T_SIZE!=std::dynamic_extent> >  
        constexpr static auto size ( void ) noexcept
        { 
            return T_SIZE; 
        }        

        template < typename C, typename = std::enable_if_t<T_SIZE==std::dynamic_extent> >  
        constexpr static bool isIndexValid ( C i ) noexcept    
        { 
            return i >= 0 && i <= static_cast<decltype(i)>(T_SIZE);         
        }

        // More of the interface is found here
        #include "Implementation/xcore_linear_buffers_hardness.h"
    };

    // Deduction Guides
    template <class T, size_t N>
    span(T (&)[N])->span<T, N>;

    template <class T, size_t N>
    span(std::array<T, N>&)->span<T, N>;

    template <class T, size_t N>
    span(const std::array<T, N>&)->span<const T, N>;

    template <class T>
    span(T*,int)->span<T>;

    template <class Container>
    span(Container&)->span<typename Container::value_type>;

    template <class Container>
    span(const Container&)->span<const typename Container::value_type>;

    //------------------------------------------------------------------------------
    // Copy data from one span to another
    //------------------------------------------------------------------------------
    template <typename T>
    void MemCopy( std::span<T> dest, const std::span<T> src ) 
    {
        xassert( src.size() <= dest.size() );
        std::memcpy( dest.data(), src.data(), src.size()*sizeof(T) );
    }
}

#endif

