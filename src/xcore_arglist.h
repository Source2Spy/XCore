#ifndef _XCORE_ARGLIST_H
#define _XCORE_ARGLIST_H
#pragma once

namespace xcore::arglist
{
    using out_types = std::variant< std::uint8_t,std::uint16_t,std::uint32_t,std::uint64_t,
                                    std::int8_t, std::int16_t, std::int32_t, std::int64_t, 
                                    float, double, void*, const char*, 
                                    std::string* >;

    using out_types = std::variant< std::uint8_t,std::uint16_t,std::uint32_t,std::uint64_t,
                                    std::int8_t, std::int16_t, std::int32_t, std::int64_t, 
                                    float, double, void*, const char*, 
                                    std::string* >;

    template< typename... T_ARGS >
    struct out 
    {
        std::array< out_types, sizeof...(T_ARGS) > m_Params;
        constexpr out( T_ARGS... Args ) : m_Params { Args... } {}
        operator std::span<out_types> () noexcept { return std::make_span(m_Params); }
    };
}

#endif
