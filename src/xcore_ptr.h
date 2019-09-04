#ifndef _XCORE_PTR_H
#define _XCORE_PTR_H
#pragma once

namespace xcore::ptr
{
    //------------------------------------------------------------------------------
    // Description:
    //      Getting the address of a temporary variable (rval)
    //------------------------------------------------------------------------------
    template<typename T> constexpr 
    T* getAddressOfTemp(T&& v) { return &v; }
}

#endif
