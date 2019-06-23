#ifndef _XCORE_RAW_ARRAY_H
#define _XCORE_RAW_ARRAY_H
#pragma once

namespace xcore::array
{
    template< typename T >
    struct raw_entry
    {
        alignas( std::alignment_of_v<T> ) char m_Data[sizeof(T)];
    };

    template< class T, std::size_t T_MAX_SIZE >
    struct raw
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

        template < typename C = size_type>  constexpr static    auto        size            ( void )                        { return static_cast<C>(T_MAX_SIZE);                                  }
                                            constexpr           auto*       data            ( void )                        { return reinterpret_cast<         mirror_type&>(m_Buffer).data();    }
                                            constexpr           auto*       data            ( void )    const               { return reinterpret_cast<const    mirror_type&>(m_Buffer).data();    }
        template < typename C >             constexpr           auto&       operator []     ( C i )                         { return reinterpret_cast<         mirror_type&>(m_Buffer)[i];        }
        template < typename C >             constexpr           auto&       operator []     ( C i )     const               { return reinterpret_cast<const    mirror_type&>(m_Buffer)[i];        }
                                            constexpr           auto        begin           ( void )            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).begin();   }
                                            constexpr           auto        begin           ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).begin();   }
                                            constexpr           auto        end             ( void )            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).end();     }
                                            constexpr           auto        end             ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).end();     }
                                            constexpr           auto        rbegin          ( void )            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).rbegin();  }
                                            constexpr           auto        rbegin          ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).rbegin();  }
                                            constexpr           auto        rend            ( void )            noexcept    { return reinterpret_cast<         mirror_type&>(m_Buffer).rend();    }
                                            constexpr           auto        rend            ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).rend();    }
                                            constexpr           auto        cbegin          ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).cbegin();  }
                                            constexpr           auto        cend            ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).cend();    }
                                            constexpr           auto        crbegin         ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).crbegin(); }
                                            constexpr           auto        crend           ( void )    const   noexcept    { return reinterpret_cast<const    mirror_type&>(m_Buffer).crend();   }

        std::array<raw_entry<T>, T_MAX_SIZE> m_Buffer;
    };

    template< class T, std::size_t T_MAX_SIZE >
    using type = typename std::array<T,T_MAX_SIZE>;
}
#endif