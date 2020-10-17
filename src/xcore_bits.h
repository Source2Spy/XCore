#ifndef _XCORE_BITS_H
#define _XCORE_BITS_H
#pragma once

namespace xcore::bits
{
    //------------------------------------------------------------------------------
    // Description:
    //      returns a power of 2 integer given n. result = 2^n  
    //------------------------------------------------------------------------------
    template< typename T > constexpr xforceinline
    T SLeft( T n ) noexcept
    { 
        static_assert( std::is_integral<T>::value,"" ); 
        return static_cast<T>(1 << n); 
    }

    //------------------------------------------------------------------------------
    // Description:
    //      Takes an Address or integer and aligns it up base on the given alignment.
    //      The result in the next number greater than or equal to "n" 
    //      which is a multiple of "a".  For example, x_Align(57,16) is 64.
    // Arguments:
    //     Addr       - This is the address/number/offset to align
    //     AlignTo    - This is a power of 2 number that the user_types wants it to be align to
    //------------------------------------------------------------------------------
    template< typename T > constexpr xforceinline
    T Align( T Address, const int AlignTo ) noexcept
    {
        static_assert( std::is_integral<T>::value, "This function only works with integer values" );
        using unsigned_t = xcore::types::to_uint_t<T>; 
        return static_cast<T>( (unsigned_t( Address ) + (static_cast<unsigned_t>(AlignTo) - 1)) & static_cast<unsigned_t>(-AlignTo) );
    }

    //------------------------------------------------------------------------------

    template< typename T > xforceinline
    T* Align( T* Address, const int AlignTo ) noexcept
    {
        return reinterpret_cast<T*>( Align( reinterpret_cast<const std::size_t>( Address ), AlignTo ) );
    }

    //------------------------------------------------------------------------------
    // Description:
    //      Takes an Address or integer and aligns it up base on the given alignment.
    //      The result in the next number less than or equal to "n" 
    //      which is a multiple of "a".  For example, x_AlignLower(57,16) is 48.
    // Arguments:
    //     Addr       - This is the address/number/offset to align
    //     AlignTo    - This is a power of 2 number that the user_types wants it to be align to
    //------------------------------------------------------------------------------

    template< typename T > constexpr xforceinline
    T AlignLower( T Address, const int AlignTo ) noexcept
    {
        static_assert( std::is_integral<T>::value, "This function only works with integer values" );
        using unsigned_t = typename xcore::types::to_uint_t<T>::type; 
        return static_cast<T>( unsigned_t( Address ) & (-AlignTo) );
    }

    //------------------------------------------------------------------------------

    template< typename T > xforceinline
    T* AlignLower( T* Address, const int AlignTo ) noexcept
    {
        return reinterpret_cast<T*>( AlignLower( reinterpret_cast<const std::size_t>( Address ), AlignTo ) );
    }

    //------------------------------------------------------------------------------
    // Description:
    //     This macro result in a TRUE/FALSE which indicates whether a number/address/offset
    //     is align to a certain power of two provided by the user_types.
    // Arguments:
    //     Addr       - This is the address/number/offset to align
    //     AlignTo    - This is a power of 2 number that the user_types wants it to be align to
    //------------------------------------------------------------------------------
    template< typename T > constexpr xforceinline
    bool isAlign( T Addr, const int AlignTo ) noexcept
    {
        static_assert( std::is_pointer<T>::value || std::is_integral<T>::value,"" );
        using unsigned_t = xcore::types::to_uint_t<T>; 
        return ( unsigned_t(Addr) & (static_cast<unsigned_t>(AlignTo)-1) ) == 0;
    }

    //------------------------------------------------------------------------------
    // Description:
    //       Functions to help deal with flags
    //------------------------------------------------------------------------------
    template< class T > constexpr void    FlagToggle(       T& N, const std::uint32_t F ) noexcept;
    template< class T > constexpr void    FlagOn    (       T& N, const std::uint32_t F ) noexcept;
    template< class T > constexpr void    FlagOff   (       T& N, const std::uint32_t F ) noexcept;
    template< class T > constexpr bool    FlagIsOn  ( const T  N, const std::uint32_t F ) noexcept;
    template< class T > constexpr bool    FlagsAreOn( const T  N, const std::uint32_t F ) noexcept;

    //------------------------------------------------------------------------------
    // Description:
    //      Computes the Log2 of an integral value. 
    //      It answer the question: how many bits do I need to rshift 'y' to make this expression true: 
    //      (input) x == 1 << 'y'. Assuming x was originally a power of 2.
    //------------------------------------------------------------------------------
    template< typename T> constexpr xforceinline
    T Log2Int( T x, int p = 0 ) noexcept
    {
        return (x <= 1) ? p : Log2Int(x >> 1, p + 1);
    }

    //------------------------------------------------------------------------------
    // Description:
    //      Determines the minimum power of two that encapsulates the given number
    //      Other cool int bits magic: http://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign
    //------------------------------------------------------------------------------
    namespace details
    {
        //------------------------------------------------------------------------------
        template< typename T > constexpr xforceinline
        T NextPowOfTwo( const T x, const int s ) noexcept
        {
            static_assert( std::is_integral<T>::value, "" );
            return static_cast<T>( ( s == 0 ) ? 1 + x : details::NextPowOfTwo<T>( x | (x >> s), s>>1 ) );
        }
    }

    //------------------------------------------------------------------------------
    template< class T > constexpr xforceinline
    bool isPowTwo( const T x ) noexcept
    { 
        static_assert( std::is_integral<T>::value, "" ); 
        return !((x - 1) & x); 
    }


    //------------------------------------------------------------------------------
    // Description:
    //      Checks if a integral number is divisible by some 2^m number
    //------------------------------------------------------------------------------
    template< class T1, class T2 > constexpr xforceinline
    bool isDivBy2PowerX( const T1 n, const T2 x ) noexcept
    { 
        static_assert( std::is_integral<T1>::value ); 
        static_assert( std::is_integral<T2>::value );
        // n is divisible by pow(2, x) 
        return ((n & ((1 << x) - 1)) == 0); 
    } 

    //------------------------------------------------------------------------------
    // Description:
    //      Determines the minimum power of two that encapsulates the given number
    // Example:
    //      Log2IntRoundUp(3) == 2 // it takes 2bits to store #3
    //------------------------------------------------------------------------------
    template< typename T> constexpr xforceinline
    T Log2IntRoundUp( T x ) noexcept
    {
        static_assert( std::is_integral<T>::value, "" ); 
        return x < 1 ? 0 : Log2Int(x) + 1; 
    }
    static_assert( 0  == Log2IntRoundUp(0), "" );
    static_assert( 1  == Log2IntRoundUp(1), "" );
    static_assert( 2  == Log2IntRoundUp(2), "" );
    static_assert( 2  == Log2IntRoundUp(3), "" );
    static_assert( 3  == Log2IntRoundUp(4), "" );
    static_assert( 3  == Log2IntRoundUp(5), "" );
    static_assert( 3  == Log2IntRoundUp(6), "" );
    static_assert( 3  == Log2IntRoundUp(7), "" );
    static_assert( 4  == Log2IntRoundUp(8), "" );
    static_assert( 4  == Log2IntRoundUp(9), "" );
    static_assert( 4  == Log2IntRoundUp(10), "" );
    static_assert( 4  == Log2IntRoundUp(11), "" );
    static_assert( 4  == Log2IntRoundUp(12), "" );
    static_assert( 4  == Log2IntRoundUp(13), "" );
    static_assert( 10 == Log2IntRoundUp(1023), "" );
    static_assert( 11 == Log2IntRoundUp(1024), "" );

    //------------------------------------------------------------------------------
    // Description:
    //      Rounds a number to the next power of two
    // Example:
    //      RoundToNextPowOfTwo(3) == 4   // The next power from #3 is #4
    //------------------------------------------------------------------------------
    template< typename T> constexpr xforceinline
    T RoundToNextPowOfTwo( const T x ) noexcept
    { 
        static_assert( std::is_integral<T>::value, "" ); 
        return ( x == 0 ) ? 0 : details::NextPowOfTwo<T>( x - 1, static_cast<int>(4*sizeof(T)) ); 
    }

    //------------------------------------------------------------------------------
    // Description:
    //       a murmur Hash Key Generator.
    // Algorithm:
    //      from code.google.com/p/smhasher/wiki/MurmurHash3
    //------------------------------------------------------------------------------
    namespace details
    {
        //------------------------------------------------------------------------------
        template< int T_SIZE >
        struct murmurHash3_by_size {};

        //------------------------------------------------------------------------------
        template<>
        struct murmurHash3_by_size<4> 
        {
            xforceinline constexpr
            static auto Compute( std::uint32_t h ) noexcept
            {
                h ^= h >> 16;
                h *= 0x85ebca6b;
                h ^= h >> 13;
                h *= 0xc2b2ae35;
                h ^= h >> 16;
                return h;
            }
        };

        //------------------------------------------------------------------------------
        template<> 
        struct murmurHash3_by_size<8> 
        {
            xforceinline constexpr
            static auto Compute( std::uint64_t h ) noexcept
            {
                h ^= h >> 33;
                h *= 0xff51afd7ed558ccd;
                h ^= h >> 33;
                h *= 0xc4ceb9fe1a85ec53;
                h ^= h >> 33;
                return h;
            }
        };
    }

    //------------------------------------------------------------------------------
    template< typename T > xforceinline constexpr 
    T MurmurHash3( T h ) noexcept
    {
        static_assert( sizeof(T) >= 4 && sizeof(T) <= 8, "" );
        static_assert( std::is_integral<T>::value,"" );
        return static_cast<T>(details::murmurHash3_by_size<sizeof(T)>::Compute( static_cast< typename xcore::types::template to_uint_t<T> >(h) ));
    }

    //------------------------------------------------------------------------------
    // Scans from Least significant bit to most until it finds a bit turn on and returns that position
    //------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #pragma intrinsic(_BitScanForward)
    inline
    std::uint32_t ctz32( std::uint32_t value ) noexcept
    {
        unsigned long leading_zero = 0;

        if ( _BitScanForward( &leading_zero, value ) )
        {
            return leading_zero;
        }
        else
        {
            // undefine....
            return 32;
        }
    }
    inline
    std::uint32_t ctz64( std::uint64_t value ) noexcept
    {
        unsigned long leading_zero = 0;

        if ( _BitScanForward64( &leading_zero, value ) )
        {
            return leading_zero;
        }
        else
        {
            // undefine....
            return 64;
        }
    }
#else
    constexpr
    std::uint32_t popcnt32( uint32_t x ) noexcept
    {
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return x & 0x0000003f;
    }

    constexpr
    std::uint32_t clz32( uint32_t x ) noexcept
    {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return 32 - popcnt32(x);
    }

    constexpr
    std::uint32_t ctz32( std::uint32_t x ) noexcept
    {
        return popcnt32((x & -x) - 1);
    }
#endif

}

#endif
