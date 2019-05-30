#ifndef _XCORE_UNITS_H
#define _XCORE_UNITS_H
#pragma once

namespace xcore::units
{
    //------------------------------------------------------------------------------
    // type
    //------------------------------------------------------------------------------
    template< typename T, typename T_TAGS >
    struct type
    {
        T m_Value;

        constexpr                               type        ( void )                    noexcept = default;
        constexpr explicit                      type        ( T Value )                 noexcept : m_Value{ Value } {}
        constexpr               bool            operator == ( const type X )    const   noexcept { return m_Value == X.m_Value;              }
        constexpr               bool            operator >= ( const type X )    const   noexcept { return m_Value >= X.m_Value;              }
        constexpr               bool            operator <= ( const type X )    const   noexcept { return m_Value <= X.m_Value;              }
        constexpr               bool            operator >  ( const type X )    const   noexcept { return m_Value >  X.m_Value;              }
        constexpr               bool            operator <  ( const type X )    const   noexcept { return m_Value <  X.m_Value;              }
                                type            operator ++ ( int )                     noexcept { auto temp{*this}; m_Value+=1; return temp;}
                                type&           operator ++ ( void )                    noexcept { m_Value+=1; return *this;                 }
                                type            operator -- ( int )                     noexcept { auto temp{*this}; m_Value-=1; return temp;}
                                type&           operator -- ( void )                    noexcept { m_Value-=1; return *this;                 }
                                type&           operator += ( const type X )            noexcept { m_Value += X.m_Value; return *this;       }
                                type&           operator -= ( const type X )            noexcept { m_Value -= X.m_Value; return *this;       }
                                type&           operator *= ( const type X )            noexcept { m_Value *= X.m_Value; return *this;       }
                                type&           operator /= ( const type X )            noexcept { m_Value /= X.m_Value; return *this;       }
        template< typename = typename std::enable_if_t< std::is_signed<T>::value, type > >
        constexpr               type            operator -  ( void  )           const   noexcept { return type{ -m_Value };                  }
        constexpr               type            operator +  ( const type X )    const   noexcept { return type{ m_Value + X.m_Value };       }
        constexpr               type            operator -  ( const type X )    const   noexcept { return type{ m_Value - X.m_Value };       }
        constexpr               type            operator *  ( const type X )    const   noexcept { return type{ m_Value * X.m_Value };       }
        constexpr               type            operator /  ( const type X )    const   noexcept { return type{ m_Value / X.m_Value };       }
    };

    struct bytes : type<int, struct bytes>
    {
        using type::type;
        using type::operator =;
    };

    //------------------------------------------------------------------------------
    // degree
    //------------------------------------------------------------------------------
    struct degree : type<float, struct degree>
    {
        using type::type;
        using type::operator =;
        constexpr explicit                      degree                  ( const unsigned long long    X )       noexcept : type{ static_cast<float>(X) } {} 
        constexpr explicit                      degree                  ( const double X )                      noexcept : type{ static_cast<float>(X) } {} 
    };

    //------------------------------------------------------------------------------
    // radian
    //------------------------------------------------------------------------------
    struct radian : type<float, struct radian>
    {
        using type::type;
        using type::operator =;
        constexpr explicit                      radian                  ( const int X )                 noexcept : type{ static_cast<float>(X) }                      {} 
        constexpr explicit                      radian                  ( const long double X )         noexcept : type{ static_cast<float>(X) }                      {} 
        constexpr                               radian                  ( const degree X )              noexcept : type{ X.m_Value * 0.017453292519943295769f }       {} 
        constexpr degree                        getDegrees              ( void )                const   noexcept { return degree{ m_Value * 57.29577951308232087685f }; }
    };

    constexpr degree   operator "" _deg    ( long double deg )          noexcept { return degree{ static_cast<float>(                    deg  ) }; }
    constexpr degree   operator "" _deg    ( unsigned long long deg )   noexcept { return degree{ static_cast<float>(static_cast<double>(deg) ) }; }

    //------------------------------------------------------------------------------
    // binary number constants support
    // int a = 101101_bin;
    //------------------------------------------------------------------------------
    namespace details
    {
        template< char... T_DIGITS >
        struct conv2bin;

        //------------------------------------------------------------------------------
        template< char T_HIGH, char... T_DIGITS >
        struct conv2bin< T_HIGH, T_DIGITS ...> 
        {
            static_assert( T_HIGH == '0' || T_HIGH == '1', "no binary num!" );
            constexpr static std::size_t value = (T_HIGH - '0') * (1 << sizeof...(T_DIGITS)) + conv2bin<T_DIGITS...>::value;
        };

        //------------------------------------------------------------------------------
        template< char T_HIGH >
        struct conv2bin< T_HIGH > 
        {
            static_assert( T_HIGH == '0' || T_HIGH == '1', "There is a digit here that is not 0 || 1, so this is not a binary number!");
            constexpr static std::size_t value = (T_HIGH - '0');
        };
    };

    //------------------------------------------------------------------------------
    template< char... T_DIGITS > constexpr int operator "" _bin() { return details::conv2bin< T_DIGITS ... >::value; }
}

//------------------------------------------------------------------------------
// Add as a potential hash 
//------------------------------------------------------------------------------
template<typename T, typename B> 
struct std::hash<xcore::units::type<T,B>>
{
    typedef xcore::units::type<T,B> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
        return s.m_Value;
    }
};

#endif