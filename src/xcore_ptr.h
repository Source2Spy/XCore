#ifndef _XCORE_PTR_H
#define _XCORE_PTR_H
#pragma once

#include "xcore_span.h"

namespace xcore::ptr
{
    //------------------------------------------------------------------------------
    // Description:
    //      Getting the address of a temporary variable (rval)
    //------------------------------------------------------------------------------
    template<typename T> constexpr 
    T* getAddressOfTemp(T&& v) { return &v; }


    //------------------------------------------------------------------------------
    //
    //------------------------------------------------------------------------------
    template <typename T>
    void MemCopy( std::span<T> dest, const std::span<T> src ) 
    {
        xassert( src.size() == dest.size() );
        std::memcpy( dest.data(), src.data(), src.size() );
    }
}

#endif
