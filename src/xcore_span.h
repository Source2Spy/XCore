#ifndef _XCORE_SPAN_H
#define _XCORE_SPAN_H
#pragma once

namespace xcore
{
    template< typename T, std::ptrdiff_t T_SIZE = -1 >
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

        template < typename C = size_type, typename = std::enable_if_t<T_SIZE!=-1> >  constexpr static auto     size            ( void )                noexcept
        { 
            return T_SIZE; 
        }        

        template < typename C,             typename = std::enable_if_t<T_SIZE==-1> >  constexpr static bool     isIndexValid    ( C i )                 noexcept    { return i >= 0 && i <= static_cast<decltype(i)>(T_SIZE);         }

        // More of the interface is found here
        #include "Implementation/xcore_linear_buffers_hardness.h"
    };
}

#endif

