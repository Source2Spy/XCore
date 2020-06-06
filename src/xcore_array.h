#ifndef _XCORE_RAW_ARRAY_H
#define _XCORE_RAW_ARRAY_H
#pragma once

namespace xcore::containers
{
    namespace raw
    {
        //------------------------------------------------------------------------------
        // Raw entry converts a standard type to a struct of bytes but with the right
        // size and alignment. This structure then can be used to allocate the memory
        // without constructing anything. Later the constructor can be call.
        //------------------------------------------------------------------------------
        template< typename T >
        struct entry
        {
            using type_value = T;
            alignas( std::alignment_of_v<T> ) char m_Data[sizeof(T)];
        };

        //------------------------------------------------------------------------------
        // This is a raw array which is similar to a standard array except it does not
        // construct anything.
        //------------------------------------------------------------------------------
        template< class T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
        struct array
        {
            using mirror_type               = typename std::array<T, T_MAX_SIZE>;
            using value_type                = T;
            using size_type                 = typename mirror_type::size_type;
            using difference_type           = typename mirror_type::difference_type;
            using pointer                   = typename mirror_type::pointer;
            using const_pointer             = typename mirror_type::const_pointer;
            using reference                 = typename mirror_type::reference;
            using const_reference           = typename mirror_type::const_reference;
            using iterator                  = typename mirror_type::iterator;
            using const_iterator            = typename mirror_type::const_iterator;
            using reverse_iterator          = typename mirror_type::reverse_iterator;
            using const_reverse_iterator    = typename mirror_type::const_reverse_iterator;

            template < typename C = size_type>  constexpr static    auto        size            ( void )                                        { return static_cast<C>(T_MAX_SIZE);                                  }
                                                constexpr           auto*       data            ( void )                                        { return reinterpret_cast<         mirror_type&>(m_Buffer).data();    }
                                                constexpr           auto*       data            ( void )                    const               { return reinterpret_cast<const    mirror_type&>(m_Buffer).data();    }
            template < typename C >             constexpr           auto&       operator []     ( C i )                                         { return reinterpret_cast<         mirror_type&>(m_Buffer)[i];        }
            template < typename C >             constexpr           auto&       operator []     ( C i )                     const               { return reinterpret_cast<const    mirror_type&>(m_Buffer)[i];        }
                                                constexpr           auto        begin           ( void )                            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).begin();   }
                                                constexpr           auto        begin           ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).begin();   }
                                                constexpr           auto        end             ( void )                            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).end();     }
                                                constexpr           auto        end             ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).end();     }
                                                constexpr           auto        rbegin          ( void )                            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).rbegin();  }
                                                constexpr           auto        rbegin          ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).rbegin();  }
                                                constexpr           auto        rend            ( void )                            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).rend();    }
                                                constexpr           auto        rend            ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).rend();    }
                                                constexpr           auto        cbegin          ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).cbegin();  }
                                                constexpr           auto        cend            ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).cend();    }
                                                constexpr           auto        crbegin         ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).crbegin(); }
                                                constexpr           auto        crend           ( void )                    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).crend();   }
            template < typename C > static      constexpr           bool        isIndexValid    ( C i )                             noexcept    { return i >= 0 && i <= static_cast<decltype(i)>(T_MAX_SIZE);         }
        
            // More of the interface is found here
            #include "Implementation/xcore_linear_buffers_hardness.h"

            std::array<entry<T>, T_MAX_SIZE> m_Buffer;
        };
    }
}

//------------------------------------------------------------------------------
// Shortcuts
//------------------------------------------------------------------------------
namespace xcore
{
    template< class T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
    struct t : std::array<T,T_MAX_SIZE>{};

    //------------------------------------------------------------------------------
    // standard C++17 array but give additional functions to make it more useful
    //------------------------------------------------------------------------------
    template< class T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
    struct array : t<T,T_MAX_SIZE,T_COUNTER>
    {
        using parent                    = t<T,T_MAX_SIZE,T_COUNTER>;
        using value_type                = typename parent::value_type;
        using size_type                 = typename parent::size_type;
        using difference_type           = typename parent::difference_type;
        using pointer                   = typename parent::pointer;
        using const_pointer             = typename parent::const_pointer;
        using reference                 = typename parent::reference;
        using const_reference           = typename parent::const_reference;
        using iterator                  = typename parent::iterator;
        using const_iterator            = typename parent::const_iterator;
        using reverse_iterator          = typename parent::reverse_iterator;
        using const_reverse_iterator    = typename parent::const_reverse_iterator;

        //using parent::array;
        using parent::operator =;
        using parent::operator [];
        using parent::data;

        template < typename C = size_type>  constexpr static    auto        size            ( void )                                        { return static_cast<C>(T_MAX_SIZE);                                  }
        template < typename C >             constexpr           bool        isIndexValid    ( C i )                     const   noexcept    { return i >= 0 && i <= static_cast<decltype(i)>(T_MAX_SIZE);         }

        // More of the interface is found here
        #include "Implementation/xcore_linear_buffers_hardness.h"
    };

    // Construction guide
    template <class... T_ARGS >
    array(T_ARGS...) -> array<typename std::common_type_t<T_ARGS...>, sizeof...(T_ARGS)>;

    //------------------------------------------------------------------------------
    // raw array
    //------------------------------------------------------------------------------
    template< class T, std::size_t T_MAX_SIZE, typename T_COUNTER = std::size_t >
    struct rawarray : xcore::containers::raw::array<T,T_MAX_SIZE,T_COUNTER> {};

    // Construction guide
    template <class... T_ARGS >
    rawarray(T_ARGS...) -> rawarray<typename std::common_type_t<T_ARGS...>, sizeof...(T_ARGS)>;
}


#endif