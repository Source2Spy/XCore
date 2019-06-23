#ifndef _XCORE_VECTOR_H
#define _XCORE_VECTOR_H
#pragma once

namespace xcore::container::vector
{
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    template< typename T_ALLOCATOR, typename T_COUNTER >
    class base
    {
    public:
        

    private:

        T_ALLOCATOR         m_Mem               {};
        T_COUNTER           m_Count             {0};
    };
}

namespace xcore
{
    template< typename T >
    using vector = xcore::container::vector::base< std::unique_ptr<T[]>, std::uint32_t >;
}

#endif