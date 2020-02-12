#ifndef _XCORE_RTTI_H
#define _XCORE_RTTI_H
#pragma once

//--------------------------------------------------------------------------------------------------------
// Example
//--------------------------------------------------------------------------------------------------------
// struct unknown
// {
//     xcore_rtti_start(unknown)
// };
// 
// struct unknown2 : unknown
// {
//     xcore_rtti(unknown2,unknown)
// };
//--------------------------------------------------------------------------------------------------------
namespace xcore::rtti
{
    //--------------------------------------------------------------------------------------------------------
    // Define the actual information that will be used to compare between RTTIs
    //--------------------------------------------------------------------------------------------------------
    constexpr static auto   crc_unit_bits_v    = 32;
    using                   crc_unit           = types::make_unique< xcore::crc<crc_unit_bits_v>, struct rtti_crc_units_tag >;

    //--------------------------------------------------------------------------------------------------------
    // base - base class for the rtti. Used to start a new rtti chain and to serve as the base for all the entries
    //--------------------------------------------------------------------------------------------------------
    struct base
    {
        constexpr                   base                    ( crc_unit X, const char* pName )               noexcept : m_CRC{ X }, m_pName{pName} {}
                                    base                    ( void )                                        noexcept = delete;
        constexpr   int             getNumberOfParents      ( void )                                const   noexcept { return 0; }
        constexpr   const void*     RTTIStrongCastCheck     ( const base&, const void* )            const   noexcept { return nullptr; }
        constexpr         void*     RTTIStrongCastCheck     ( const base&, void* )                  const   noexcept { return nullptr; }

        template< typename T >
        constexpr   bool            isKindOf                ( void )                                const   noexcept { return m_CRC == T::ms_RTTI.m_CRC; }
        constexpr   bool            isKindOf                ( crc_unit CRC )                        const   noexcept { return m_CRC == CRC; }

        template< typename T >
        struct kindof
        {
            template <typename T_ARG>
            constexpr static bool isKindOf( void )              noexcept { return T::ms_RTTI.m_CRC == T_ARG::ms_RTTI.m_CRC; }
            constexpr static bool isKindOf( crc_unit CRC )      noexcept { return T::ms_RTTI.m_CRC == CRC; }
        };

        const char* const   m_pName;
        const crc_unit      m_CRC;
    };

    //--------------------------------------------------------------------------------------------------------
    // entry - details
    //--------------------------------------------------------------------------------------------------------
    namespace details
    {
        //--------------------------------------------------------------------------------------------------------
        // args
        //--------------------------------------------------------------------------------------------------------
        template< int T_I, typename... T_ARGS >
        struct arg_t      
        { 
            using type = typename std::tuple_element< T_I, std::tuple<T_ARGS...> >::type;
        
            template< typename T_ARG >
            constexpr static bool isKindOf( void ) noexcept
            {
                return type::ms_RTTI.m_CRC == T_ARG::ms_RTTI.m_CRC || std::remove_reference<decltype(type::ms_RTTI)>::type::template kindof<type>::template isKindOf<T_ARG>();
            }
            constexpr static bool isKindOf( crc_unit CRC ) noexcept
            {
                return type::ms_RTTI.m_CRC == CRC || std::remove_reference<decltype(type::ms_RTTI)>::type::template kindof<type>::isKindOf(CRC);
            }
        };

        //--------------------------------------------------------------------------------------------------------
        // kindof
        //--------------------------------------------------------------------------------------------------------
        template< typename T_CLASS, typename T, int T_COUNT >
        struct kindof
        {
            template< typename T_ARG >
            constexpr static bool isKindOf( void ) noexcept
            { 
                if constexpr( T_CLASS::arg_t<T_COUNT>::template isKindOf<T_ARG>() ) return true;
                else if constexpr( T_COUNT == 0 )                                   return false;
                     else                                                           return kindof<T_CLASS,T,T_COUNT-1>::template isKindOf<T_ARG>();
            }           
            constexpr static bool isKindOf( crc_unit CRC ) noexcept
            { 
                if ( T_CLASS::arg_t<T_COUNT>::isKindOf(CRC) )   return true;
                else if constexpr( T_COUNT == 0 )               return false;
                     else                                       return kindof<T_CLASS,T,T_COUNT-1>::isKindOf(CRC);
            }           
        };

        //--------------------------------------------------------------------------------------------------------
        // strong cast
        //--------------------------------------------------------------------------------------------------------
        template< typename T_ENTRY, typename T_CLASS, int T_COUNT >
        struct strong_cast
        {
            using parent = typename T_ENTRY::template arg_t< T_COUNT >::type;

            constexpr static const void* CastCheck( const base& Rtti, const T_CLASS* Ptr ) noexcept
            {
                static constexpr auto& ParentRTTI = parent::ms_RTTI;

                // check if it is the parent, and if so cast it properly and return the final pointer
                if constexpr ( std::is_base_of_v< parent, T_CLASS > )
                {
                    if( ParentRTTI.m_CRC == Rtti.m_CRC )
                        return static_cast<const parent*>(Ptr);
                }

                // if it is not the parent ask the parent to check all its children
                if( auto x = ParentRTTI.RTTIStrongCastCheck(Rtti, Ptr); x ) return x;

                // if we did not find it lets try our next parent
                if constexpr( T_COUNT == 0 )    return nullptr;
                else                            return strong_cast<T_ENTRY, T_CLASS, T_COUNT - 1>::CastCheck( Rtti, Ptr );
            }

            constexpr static void* CastCheck( const base& Rtti, T_CLASS* Ptr ) noexcept
            {
                static constexpr auto& ParentRTTI = parent::ms_RTTI;

                // check if it is the parent, and if so cast it properly and return the final pointer
                if constexpr ( std::is_base_of_v< parent, T_CLASS > )
                {
                    if( ParentRTTI.m_CRC == Rtti.m_CRC )
                        return static_cast<parent*>(Ptr);
                }

                // if it is not the parent ask the parent to check all its children
                if( auto x = ParentRTTI.RTTIStrongCastCheck(Rtti, Ptr); x ) return x;

                // if we did not find it lets try our next parent
                if constexpr( T_COUNT == 0 )    return nullptr;
                else                            return strong_cast<T_ENTRY, T_CLASS, T_COUNT - 1>::CastCheck( Rtti, Ptr );
            }
        };

        //--------------------------------------------------------------------------------------------------------
        // Safe Cast - Cast a type instance to another type in a safe manner.
        //--------------------------------------------------------------------------------------------------------
        template< typename T, typename T_DST > inline
        typename std::enable_if
        <
            // Up cast to DST
               false == std::is_same< typename std::decay<T>::type, typename std::decay<T_DST>::type >::value 
            && true  == std::is_base_of< T, T_DST >::value
            , T_DST*
        >::type
        internal_cast( T* A )
        {
            const static int Offset = static_cast<int>
            ( 
                reinterpret_cast<const std::byte*> //-V650
                (
                    static_cast<const T*>(reinterpret_cast<const T_DST*>(0x10000000))
                ) 
                - reinterpret_cast<const std::byte*>(0x10000000)
            );
            return reinterpret_cast<T_DST*>( &((std::byte*)A)[ -Offset ] );
        }

        //--------------------------------------------------------------------------------------------------------
        template< typename T, typename T_DST > constexpr
        typename std::enable_if
        <
            // Down cast to T_DST
               true == std::is_same< typename std::decay<T>::type, typename std::decay<T_DST>::type >::value 
            || true == std::is_base_of< T_DST, T >::value
            , T_DST*
        >::type
        internal_cast( T* A )
        {
            return static_cast<T_DST*>(A);
        }

        //--------------------------------------------------------------------------------------------------------
        template< typename T, typename T_DST > inline
        typename std::enable_if
        <
               false == std::is_base_of< T_DST, T >::value 
            && false == std::is_base_of< T, T_DST >::value
            , T_DST*
        >::type
        internal_cast( T* A )
        {
            xassert(A);
            auto p = A->StrongCastV( T_DST::ms_RTTI );
            xassert(p);
            return reinterpret_cast<T_DST*>(const_cast<void*>(p));
        }
    }

    //--------------------------------------------------------------------------------------------------------
    // entry
    //--------------------------------------------------------------------------------------------------------
    template< typename T_CLASS, typename ...T_ARGS >
    struct entry final : base 
    {
        static constexpr int total_parents_v    { sizeof...(T_ARGS) };
        static constexpr int start_index_v      { total_parents_v - 1};
        static_assert(total_parents_v>=1);
        static_assert(start_index_v>=0);

        template< int T_I >
        using arg_t = details::arg_t<T_I,T_ARGS...>;

        template< typename T >
        using kindof = details::kindof< entry, T, start_index_v >;

        using strong_cast = details::strong_cast< entry, T_CLASS, start_index_v >;

        using base::base;
        constexpr static    int     getNumberOfParents      ( void )                                            noexcept            { return total_parents_v;  }
        template< typename T >
        constexpr           bool    isKindOf                ( void )                                    const   noexcept            { return m_CRC == T::ms_RTTI.m_CRC || kindof< T_CLASS >::template isKindOf<T>(); }
        constexpr           bool    isKindOf                ( crc_unit CRC )                            const   noexcept            { return m_CRC == CRC             || kindof< T_CLASS >::isKindOf( CRC );        }
        constexpr     const void*   RTTIStrongCastCheck     ( const base& Rtti, const T_CLASS* Ptr )    const   noexcept            { return strong_cast::CastCheck(Rtti, Ptr); }
        constexpr           void*   RTTIStrongCastCheck     ( const base& Rtti, T_CLASS* Ptr )          const   noexcept            { return strong_cast::CastCheck(Rtti, Ptr); }
    };

    //--------------------------------------------------------------------------------------------------------
    // is Kind Of - Determine if a type is castable to another type. Two versions:
    //      - isKindOf( void ).     This version only deals with types and so it has not idea of type instances.
    //      - isKindOf( CLASS& ).   This versions deal with variable instances. Has only one virtual call.
    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_ARG > constexpr
    bool [[nodiscard]] isKindOf( void ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_ARG > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_ARG>>)
            return true;
        else // Cast up
            return T_ARG::ms_RTTI.isKindOf<T_TYPE>();
    }

    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_CLASS > constexpr
    bool [[nodiscard]] isKindOf( const T_CLASS& Class ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_CLASS > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_CLASS>>) return true;
        // Cast up
        else return Class.isKindOfV( T_TYPE::ms_RTTI.m_CRC );
    }

    //--------------------------------------------------------------------------------------------------------
    // Safe Cast - Cast a type instance to another type in a safe manner.
    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_CLASS > inline
    T_TYPE& [[nodiscard]] SafeCast( T_CLASS& Class ) noexcept
    {
        xassert( isKindOf<T_TYPE>(Class) );
        return *details::internal_cast<std::remove_reference_t<T_CLASS>, T_TYPE >( &Class );
    }
}

//------------------------------------------------------------------------------
// Description:
//      Convert an argument to a string
//------------------------------------------------------------------------------
#define xcore_rtti_start(CLASS)     public: constexpr static rtti::base                     ms_RTTI     { []{ return xcore::crc<xcore::rtti::crc_unit_bits_v>::FromString(X_STRINGIFY( CLASSNAME ) "->" __FUNCSIG__ ":" X_STRINGIFY( __LINE__ )); }(), #CLASS };\
                                    public: virtual const void*                             StrongCastV ( const rtti::base& B )         const   noexcept { return ms_RTTI.RTTIStrongCastCheck( B, this ); }                                                     \
                                    public: virtual void*                                   StrongCastV ( const rtti::base& B )                 noexcept { return ms_RTTI.RTTIStrongCastCheck( B, this ); }                                                     \
                                    public: virtual bool [[nodiscard]]                      isKindOfV   ( xcore::rtti::crc_unit CRC )   const   noexcept { return ms_RTTI.m_CRC == CRC;      }                                                                  \

#define xcore_rtti(CLASS, ... )     public: constexpr static rtti::entry<CLASS,__VA_ARGS__> ms_RTTI     { []{ return xcore::crc<xcore::rtti::crc_unit_bits_v>::FromString(X_STRINGIFY( CLASSNAME ) "->" __FUNCSIG__ ":" X_STRINGIFY( __LINE__ )); }(), #CLASS };\
                                    public: virtual const void*                             StrongCastV ( const rtti::base& B )         const   noexcept override { return ms_RTTI.RTTIStrongCastCheck( B, this ); }                                            \
                                    public: virtual void*                                   StrongCastV ( const rtti::base& B )                 noexcept override { return ms_RTTI.RTTIStrongCastCheck( B, this ); }                                            \
                                    public: virtual bool [[nodiscard]]                      isKindOfV   ( xcore::rtti::crc_unit CRC )   const   noexcept override { return ms_RTTI.isKindOf( CRC ); }                                                           \



#endif

