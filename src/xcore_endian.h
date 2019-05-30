#ifndef _XCORE_ENDIAN_H
#define _XCORE_ENDIAN_H
#pragma once

namespace xcore::endian
{
    //------------------------------------------------------------------------------
    // Convert from one endian format to another
    //------------------------------------------------------------------------------
    namespace details
    {
        constexpr auto Convert( std::uint8_t  x ) noexcept { return x; }
        constexpr auto Convert( std::uint16_t x ) noexcept { return   ((x&0xff00)>>(8*1)) 
                                                                    | ((x&0x00ff)<<(8*1)); }
        constexpr auto Convert( std::uint32_t x ) noexcept { return   ((x&0xff000000)>>(8*3)) 
                                                                    | ((x&0x000000ff)<<(8*3)) 
                                                                    | ((x&0x00ff0000)>>(8*1)) 
                                                                    | ((x&0x0000ff00)<<(8*1)); }
        constexpr auto Convert( std::uint64_t x ) noexcept { return   ((x&0xff00000000000000)>>(8*7)) 
                                                                    | ((x&0x00000000000000ff)<<(8*7)) 
                                                                    | ((x&0x00ff000000000000)>>(8*5)) 
                                                                    | ((x&0x000000000000ff00)<<(8*5)) 
                                                                    | ((x&0x0000ff0000000000)>>(8*3)) 
                                                                    | ((x&0x0000000000ff0000)<<(8*3)) 
                                                                    | ((x&0x000000ff00000000)>>(8*1)) 
                                                                    | ((x&0x00000000ff000000)<<(8*1)); }
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr 
    T Convert( const T data ) noexcept
    {
        static_assert( std::is_integral<T>::value, "Only atomic values such int floats, etc. allowed for swapping endians" );
        return static_cast<T>(details::Convert( static_cast<types::to_uint_t<T>>(data)));
    }

    //------------------------------------------------------------------------------
    template<> inline 
    float Convert<float>( const float h ) noexcept
    {
        using T             = float;
        using base_type     = xcore::types::to_uint_t<T>;
        return *reinterpret_cast<const T*>( xcore::ptr::getAddressOfTemp( Convert( *reinterpret_cast<const base_type*>(&h)) ) );
    }

    //------------------------------------------------------------------------------
    template<> inline 
    double Convert<double>( const double h ) noexcept
    {
        using T             = double;
        using base_type     = xcore::types::to_uint_t<T>;
        return *reinterpret_cast<const T*>( xcore::ptr::getAddressOfTemp( Convert( *reinterpret_cast<const base_type*>(&h)) ) );
    }

    static_assert( Convert(std::uint64_t(0xabcdefAA123456ff)) == std::uint64_t(0xff563412AAefcdab), "" );
    static_assert( Convert(std::uint32_t(0xabcd12ff))         == std::uint32_t(0xff12cdab),         "" );
    static_assert( Convert(std::uint16_t(0xabff))             == std::uint16_t(0xffab),             "" );
    static_assert( Convert(std::uint8_t(0xff))                == std::uint8_t(0xff),                "" );

    //------------------------------------------------------------------------------
    // Determine the system endian
    //------------------------------------------------------------------------------
    constexpr 
    bool isSystemLittle( void ) noexcept
    {
        return static_cast<const std::uint8_t&>(0x01) == 0x01;
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool isSystemBig( void ) noexcept
    {
        return !isSystemLittle();
    }

    //------------------------------------------------------------------------------
    // System to a particular endian conversions
    //------------------------------------------------------------------------------
    template<typename T> constexpr
    T SystemToLittle( const T& data ) noexcept
    {
        if constexpr( isSystemLittle() ) return data; 
        else                             return Convert(data);
    }

    //------------------------------------------------------------------------------
    template<typename T> constexpr
    T SystemToBig( const T& data ) noexcept
    {
        if constexpr( isSystemLittle() ) return Convert(data);
        else                             return data;
    }

    //------------------------------------------------------------------------------
    template<typename T> constexpr
    T BigToSystem( const T& data ) noexcept
    {
        if constexpr( isSystemLittle() ) return Convert(data); 
        else                             return data;
    }

    //------------------------------------------------------------------------------
    template<typename T> constexpr
    T LittleToSystem( const T& data ) noexcept
    {
        if constexpr ( isSystemLittle() )   return data; 
        else                                return Convert(data);
    }

    //------------------------------------------------------------------------------
    #if _XCORE_ENDIAN_LITTLE
        static_assert( isSystemLittle() == true, "" );
    #elif  _XCORE_ENDIAN_BIG
        static_assert( isSystemLittle() == false, "" );
    #else
        #error "Not Endian defined"
    #endif
}

#endif