#ifndef _XCORE_UNITS_H
#define _XCORE_UNITS_H
#pragma once

namespace xcore::units
{
    //------------------------------------------------------------------------------
    // handle
    //------------------------------------------------------------------------------
    template< typename T_TAG, typename T = std::uint32_t >
    struct handle
    {
        using value_t = T;
        
        constexpr static value_t null_v = ~value_t{0}; 

        value_t m_Value{};

        constexpr                       handle              ( void )                        noexcept = default;
        constexpr explicit              handle              ( std::nullptr_t  )             noexcept : m_Value{ null_v } {}
        constexpr explicit              handle              ( std::uint32_t v )             noexcept : m_Value{ v }    {}
        constexpr           bool        isValid             ( void )                const   noexcept { return m_Value != null_v;    }
        constexpr           bool        isNull              ( void )                const   noexcept { return m_Value == null_v;    }
        inline              void        setNull             ( void )                        noexcept { m_Value = null_v;            }  
        constexpr           bool        operator    ==      ( handle H )            const   noexcept { return H.m_Value == m_Value; }
        constexpr           bool        operator    !=      ( handle H )            const   noexcept { return H.m_Value != m_Value; }
    };

    //------------------------------------------------------------------------------
    // type
    //------------------------------------------------------------------------------
    template< typename T_PARENT, typename T_BASIC_TYPE >
    struct type
    {
        using basic_t = T_BASIC_TYPE;
        using self_t  = type;
        T_BASIC_TYPE m_Value;

        constexpr                               type        ( void )                        noexcept = default;
        constexpr explicit                      type        ( basic_t Value )               noexcept : m_Value{ Value } {}
        constexpr               bool            operator == ( const T_PARENT X )    const   noexcept { return m_Value == X.m_Value;              }
        constexpr               bool            operator != ( const T_PARENT X )    const   noexcept { return m_Value != X.m_Value;              }
        constexpr               bool            operator >= ( const T_PARENT X )    const   noexcept { return m_Value >= X.m_Value;              }
        constexpr               bool            operator <= ( const T_PARENT X )    const   noexcept { return m_Value <= X.m_Value;              }
        constexpr               bool            operator >  ( const T_PARENT X )    const   noexcept { return m_Value >  X.m_Value;              }
        constexpr               bool            operator <  ( const T_PARENT X )    const   noexcept { return m_Value <  X.m_Value;              }
                                auto            operator ++ ( int )                         noexcept { T_PARENT temp{*this}; m_Value+=1; return temp;}
                                auto&           operator ++ ( void )                        noexcept { m_Value+=1; return *static_cast<T_PARENT*>(this); }
                                auto            operator -- ( int )                         noexcept { T_PARENT temp{*this}; m_Value-=1; return temp;}
                                auto&           operator -- ( void )                        noexcept { m_Value-=1; return *this;                 }
                                auto&           operator += ( const T_PARENT X )            noexcept { m_Value += X.m_Value; return *static_cast<T_PARENT*>(this);       }
                                auto&           operator -= ( const T_PARENT X )            noexcept { m_Value -= X.m_Value; return *static_cast<T_PARENT*>(this);       }
                                auto&           operator *= ( const T_PARENT X )            noexcept { m_Value *= X.m_Value; return *static_cast<T_PARENT*>(this);       }
                                auto&           operator /= ( const T_PARENT X )            noexcept { m_Value /= X.m_Value; return *static_cast<T_PARENT*>(this);       }
        template< typename = typename std::enable_if_t< std::is_signed<basic_t>::value, type > >
        constexpr               auto            operator -  ( void  )               const   noexcept { return T_PARENT{ -m_Value };                  }
        constexpr               auto            operator +  ( const T_PARENT X )    const   noexcept { return T_PARENT{ m_Value + X.m_Value };       }
        constexpr               auto            operator -  ( const T_PARENT X )    const   noexcept { return T_PARENT{ m_Value - X.m_Value };       }
        constexpr               auto            operator *  ( const T_PARENT X )    const   noexcept { return T_PARENT{ m_Value * X.m_Value };       }
        constexpr               auto            operator /  ( const T_PARENT X )    const   noexcept { return T_PARENT{ m_Value / X.m_Value };       }
    };

    template< typename T_BASIC_TYPE >
    struct type<T_BASIC_TYPE,T_BASIC_TYPE>
    {
        using basic_t = T_BASIC_TYPE;
        using self_t  = type;
        T_BASIC_TYPE m_Value;

        constexpr                               type        ( void )                        noexcept = default;
        constexpr explicit                      type        ( basic_t Value )               noexcept : m_Value{ Value } {}
        constexpr               bool            operator == ( const self_t X )      const   noexcept { return m_Value == X.m_Value;              }
        constexpr               bool            operator != ( const self_t X )      const   noexcept { return m_Value != X.m_Value;              }
        constexpr               bool            operator >= ( const self_t X )      const   noexcept { return m_Value >= X.m_Value;              }
        constexpr               bool            operator <= ( const self_t X )      const   noexcept { return m_Value <= X.m_Value;              }
        constexpr               bool            operator >  ( const self_t X )      const   noexcept { return m_Value >  X.m_Value;              }
        constexpr               bool            operator <  ( const self_t X )      const   noexcept { return m_Value <  X.m_Value;              }
                                auto            operator ++ ( int )                         noexcept { self_t temp{*this}; m_Value+=1; return temp;}
                                auto&           operator ++ ( void )                        noexcept { m_Value+=1; return *this;                   }
                                auto            operator -- ( int )                         noexcept { self_t temp{*this}; m_Value-=1; return temp;}
                                auto&           operator -- ( void )                        noexcept { m_Value-=1; return *this;                 }
                                auto&           operator += ( const self_t X )              noexcept { m_Value += X.m_Value; return *this;       }
                                auto&           operator -= ( const self_t X )              noexcept { m_Value -= X.m_Value; return *this;       }
                                auto&           operator *= ( const self_t X )              noexcept { m_Value *= X.m_Value; return *this;       }
                                auto&           operator /= ( const self_t X )              noexcept { m_Value /= X.m_Value; return *this;       }
        template< typename = typename std::enable_if_t< std::is_signed<basic_t>::value, type > >
        constexpr               auto            operator -  ( void  )               const   noexcept { return self_t{ -m_Value };                  }
        constexpr               auto            operator +  ( const self_t X )      const   noexcept { return self_t{ m_Value + X.m_Value };       }
        constexpr               auto            operator -  ( const self_t X )      const   noexcept { return self_t{ m_Value - X.m_Value };       }
        constexpr               auto            operator *  ( const self_t X )      const   noexcept { return self_t{ m_Value * X.m_Value };       }
        constexpr               auto            operator /  ( const self_t X )      const   noexcept { return self_t{ m_Value / X.m_Value };       }
    };

    struct bytes : type<bytes, std::int64_t>
    {
        using type::type;
        using type::operator =;
    };

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
}

//------------------------------------------------------------------------------
template< char... T_DIGITS > constexpr int operator "" _xbin() { return xcore::units::details::conv2bin< T_DIGITS ... >::value; }

#endif