#ifndef _XCORE_RTTI_H
#define _XCORE_RTTI_H
#pragma once

namespace xcore::rtti
{
    //--------------------------------------------------------------------------------------------------------

    struct base
    {
        template< std::size_t N >
        constexpr               base                    ( const char (&Name)[N] )               noexcept : m_CRC{ crc::FromString<64>(Name) }, m_pName{Name} {}
                                base                    ( void )                                noexcept = delete;
        constexpr   int         getNumberOfParents      ( void )                        const   noexcept { return 0; }
        virtual const void*     RTTIStrongCastHelperV   ( const base& A, const void* )  const   noexcept { xassert(false); return nullptr; }

        template< typename T >
        constexpr     bool      isKindofV               ( void )                        const   noexcept { return m_CRC == T::m_RTTI.m_CRC; }
        virtual       bool      isKindofV               ( crc::units<64> CRC )          const   noexcept { return m_CRC == CRC; }

        template< typename T >
        struct kindof
        {
            template <typename T_ARG>
            constexpr static bool isKindof( void )                  noexcept { return T::m_RTTI.m_CRC == T_ARG::m_RTTI.m_CRC; }
            constexpr static bool isKindof( crc::units<64> CRC )    noexcept { return T::m_RTTI.m_CRC == CRC; }
        };

        constexpr const void* RTTIStrongCastCheck( const base& Rtti, const void* )      const   noexcept { return nullptr; }

        const char* const       m_pName;
        const crc::units<64>    m_CRC;
    };

    //--------------------------------------------------------------------------------------------------------

    template< typename T_CLASS, typename ...T_ARGS >
    struct entry : base 
    {
        static constexpr int total_parents_v    { sizeof...(T_ARGS) };
        static constexpr int start_index_v      { total_parents_v - 1};
        static_assert(total_parents_v>=1);
        static_assert(start_index_v>=0);

        using base::base;
        constexpr static    int     getNumberOfParents      ( void )                                  noexcept          { return total_parents_v;  }
        template< typename T >
        constexpr           bool    isKindofV               ( void )                            const noexcept          { return m_CRC == T::m_RTTI.m_CRC || kindof< T_CLASS >::template isKindof<T>(); }
        virtual             bool    isKindofV               ( crc::units<64> CRC )              const noexcept override { return m_CRC == CRC             || kindof< T_CLASS >::isKindof( CRC );        }
        virtual       const void*   RTTIStrongCastHelperV   ( const base& A, const void* P )    const noexcept override { return RTTIStrongCastCheck( A, reinterpret_cast<const T_CLASS*>(P) );}

        template< int T_I >
        struct arg_t      
        { 
            using type = typename std::tuple_element< T_I, std::tuple<T_ARGS...> >::type;
        
            template< typename T_ARG >
            constexpr static bool isKindof( void ) noexcept
            {
                return type::m_RTTI.m_CRC == T_ARG::m_RTTI.m_CRC || std::remove_reference<decltype(type::m_RTTI)>::type::template kindof<type>::template isKindof<T_ARG>();
            }
            constexpr static bool isKindof( crc::units<64> CRC ) noexcept
            {
                return type::m_RTTI.m_CRC == CRC || std::remove_reference<decltype(type::m_RTTI)>::type::template kindof<type>::isKindof(CRC);
            }
        };

        template< typename T, int T_COUNT = start_index_v >
        struct get_parent_index
        {
            static constexpr int value = std::is_same_v<T, arg_t<T_COUNT>::type > ? T_COUNT : std::is_same_v<T, arg_t<T_COUNT - 1>::type >::value;
        };
    
        template< typename T >
        struct get_parent_index< T, -1 >
        {
            static constexpr int value = -1;
        };

        template< typename T, int T_COUNT = start_index_v >
        struct kindof
        {
            template< typename T_ARG >
            constexpr static bool isKindof( void ) noexcept
            { 
                if constexpr( arg_t<T_COUNT>::template isKindof<T_ARG>() )  return true;
                else                                                        return kindof<T,T_COUNT-1>::template isKindof<T_ARG>();
            }           
            constexpr static bool isKindof( crc::units<64> CRC ) noexcept
            { 
                if ( arg_t<T_COUNT>::isKindof(CRC) )  return true;
                else                                  return kindof<T,T_COUNT-1>::isKindof(CRC);
            }           
        };

        template< typename T >
        struct kindof< T, -1 >
        {
            template< typename T_ARG >
            constexpr static bool isKindof( void ) noexcept
            { 
                return false ;
            }
            constexpr static bool isKindof( crc::units<64> CRC ) noexcept
            { 
                return false ;
            }
        };  

        constexpr const void* RTTIStrongCastCheck( const base& Rtti, const T_CLASS* Ptr ) const noexcept
        {
            return strong_cast<>::CastCheck(Rtti, Ptr);
        }

        template< int T_COUNT = start_index_v >
        struct strong_cast
        {
            using parent = typename arg_t< T_COUNT >::type;
            constexpr static const void* CastCheck( const base& Rtti, const T_CLASS* Ptr ) noexcept
            {
                static constexpr auto& ParentRTTI = parent::m_RTTI;

                // check if it is the parent, and if so cast it properly and return the final pointer
                if constexpr ( std::is_base_of_v< parent, T_CLASS > )
                {
                    if( ParentRTTI.m_CRC == Rtti.m_CRC )
                        return static_cast<const parent*>(Ptr);
                }

                // if it is not the parent ask the parent to check all its children
                if( auto x = ParentRTTI.RTTIStrongCastCheck(Rtti, Ptr); x ) return x;

                // if we did not find it lets try our next parent
                return strong_cast<T_COUNT - 1>::CastCheck( Rtti, Ptr );
            }
        };

        template<>
        struct strong_cast< -1 >
        {
            constexpr static const void* CastCheck( const base& Rtti, const void* ) noexcept
            {
                // stop the loop
                return nullptr;
            }
        };
    };

    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_ARG > constexpr
    bool isKindof( void ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_ARG > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_ARG>>)
            return true;
        else // Cast up
            return T_ARG::m_RTTI.isKindofV<T_TYPE>();
    }

    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_CLASS > inline
    bool isKindof( const T_CLASS& Class ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_CLASS > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_CLASS>>) return true;
        // Cast up
        else return Class.getVRTTI().isKindofV( T_TYPE::m_RTTI.m_CRC );
    }

}

#endif

