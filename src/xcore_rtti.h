#ifndef _XCORE_RTTI_H
#define _XCORE_RTTI_H
#pragma once

namespace xcore::rtti
{
    //--------------------------------------------------------------------------------------------------------
    // base - base class for the rtti. Used to start a new rtti chain and to serve as the base for all the entries
    //--------------------------------------------------------------------------------------------------------
    struct base
    {
        constexpr                   base                    ( crc::units<64> X, const char* pName )         noexcept : m_CRC{ X }, m_pName{pName} {}
                                    base                    ( void )                                        noexcept = delete;
        constexpr   int             getNumberOfParents      ( void )                                const   noexcept { return 0; }

        virtual     const void*     RTTIStrongCastHelperV   ( const base&, const void* )            const   noexcept { return nullptr; }
        constexpr   const void*     RTTIStrongCastCheck     ( const base& , const void* )           const   noexcept { return nullptr; }

        template< typename T >
        constexpr     bool          isKindofV               ( void )                                const   noexcept { return m_CRC == T::m_RTTI.m_CRC; }
        virtual       bool          isKindofV               ( crc::units<64> CRC )                  const   noexcept { return m_CRC == CRC; }

        template< typename T >
        struct kindof
        {
            template <typename T_ARG>
            constexpr static bool isKindOf( void )                  noexcept { return T::m_RTTI.m_CRC == T_ARG::m_RTTI.m_CRC; }
            constexpr static bool isKindOf( crc::units<64> CRC )    noexcept { return T::m_RTTI.m_CRC == CRC; }
        };

        const char* const       m_pName;
        const crc::units<64>    m_CRC;
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
                return type::m_RTTI.m_CRC == T_ARG::m_RTTI.m_CRC || std::remove_reference<decltype(type::m_RTTI)>::type::template kindof<type>::template isKindOf<T_ARG>();
            }
            constexpr static bool isKindOf( crc::units<64> CRC ) noexcept
            {
                return type::m_RTTI.m_CRC == CRC || std::remove_reference<decltype(type::m_RTTI)>::type::template kindof<type>::isKindOf(CRC);
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
                if constexpr( T_CLASS::arg_t<T_COUNT>::template isKindOf<T_ARG>() )  return true;
                else                                                                 return kindof<T_CLASS,T,T_COUNT-1>::template isKindOf<T_ARG>();
            }           
            constexpr static bool isKindOf( crc::units<64> CRC ) noexcept
            { 
                if ( T_CLASS::arg_t<T_COUNT>::isKindOf(CRC) )  return true;
                else                                           return kindof<T_CLASS,T,T_COUNT-1>::isKindOf(CRC);
            }           
        };

        //--------------------------------------------------------------------------------------------------------
        template< typename T_CLASS, typename T >
        struct kindof< T_CLASS, T, -1 >
        {
            template< typename T_ARG >
            constexpr static bool isKindOf( void ) noexcept
            { 
                return false ;
            }
            constexpr static bool isKindOf( crc::units<64> CRC ) noexcept
            { 
                return false ;
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
                static constexpr auto& ParentRTTI = parent::m_RTTI;

                // check if it is the parent, and if so cast it properly and return the final pointer
                if constexpr ( std::is_base_of_v< parent, T_CLASS > )
                {
                    if( ParentRTTI.m_CRC == Rtti.m_CRC )
                        return static_cast<const parent*>(Ptr);
                }

                // if it is not the parent ask the parent to check all its children
                if( auto x = ParentRTTI.RTTIStrongCastHelperV(Rtti, Ptr); x ) return x;

                // if we did not find it lets try our next parent
                return strong_cast<T_ENTRY, T_CLASS, T_COUNT - 1>::CastCheck( Rtti, Ptr );
            }
        };

        //--------------------------------------------------------------------------------------------------------
        template< typename T_ENTRY, typename T_CLASS >
        struct strong_cast< T_ENTRY, T_CLASS, -1 >
        {
            constexpr static const void* CastCheck( const base&, const void* ) noexcept
            {
                // stop the loop
                return nullptr;
            }
        };
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
        constexpr           bool    isKindofV               ( void )                                    const   noexcept            { return m_CRC == T::m_RTTI.m_CRC || kindof< T_CLASS >::template isKindOf<T>(); }
        virtual             bool    isKindofV               ( crc::units<64> CRC )                      const   noexcept override   { return m_CRC == CRC             || kindof< T_CLASS >::isKindOf( CRC );        }
        virtual       const void*   RTTIStrongCastHelperV   ( const base& A, const void* P )            const   noexcept override   { return RTTIStrongCastCheck( A, reinterpret_cast<const T_CLASS*>(P) );}
        constexpr     const void*   RTTIStrongCastCheck     ( const base& Rtti, const T_CLASS* Ptr )    const   noexcept            { return strong_cast::CastCheck(Rtti, Ptr); }

/*
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
*/
    };

    //--------------------------------------------------------------------------------------------------------
    // is Kind Of
    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_ARG > constexpr
    bool [[nodiscard]] isKindOf( void ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_ARG > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_ARG>>)
            return true;
        else // Cast up
            return T_ARG::m_RTTI.isKindofV<T_TYPE>();
    }

    //--------------------------------------------------------------------------------------------------------
    template< typename T_TYPE, typename T_CLASS > inline
    bool [[nodiscard]] isKindOf( const T_CLASS& Class ) noexcept
    {
        // Cast down easy
        if constexpr( std::is_base_of_v< T_TYPE, T_CLASS > || std::is_same_v<std::decay_t<T_TYPE>,std::decay_t<T_CLASS>>) return true;
        // Cast up
        else return Class.getVRTTI().first.isKindofV( T_TYPE::m_RTTI.m_CRC );
    }

    //--------------------------------------------------------------------------------------------------------
    // Safe Cast
    //--------------------------------------------------------------------------------------------------------
    namespace details
    {
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
                reinterpret_cast<const std::byte*>
                (
                    (const T*)(reinterpret_cast<const T_DST*>(0x10000000))
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
            auto [RTTI,Ptr] = A->getVRTTI();
            auto p = RTTI.RTTIStrongCastHelperV( T_DST::m_RTTI, Ptr );

            // Seems it did not find the type to cast
            xassert(p);
            return reinterpret_cast<T_DST*>(const_cast<void*>(p));
        }
    }

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
#define __xcore_rtti_class_name( CLASSNAME )  []() constexpr noexcept ->crc::units<64> { return crc::FromString<64>(X_STRINGIFY( CLASSNAME ) "->" __FUNCSIG__ ":" X_STRINGIFY( __LINE__ )); }()

#define xcore_rtti_start(CLASS)     public: constexpr static rtti::base m_RTTI{ __xcore_rtti_class_name(CLASS), #CLASS };                                   \
                                    public: virtual std::pair<const rtti::base&,const void*> getVRTTI( void ) const noexcept { return {m_RTTI,this}; }      \
                                    public: virtual std::pair<const rtti::base&,      void*> getVRTTI( void )       noexcept { return {m_RTTI,this}; }      \

#define xcore_rtti(CLASS, ... )     public: constexpr static rtti::entry<CLASS,__VA_ARGS__> m_RTTI{ __xcore_rtti_class_name(CLASS), #CLASS };               \
                                    public: virtual std::pair<const rtti::base&,const void*> getVRTTI( void ) const noexcept { return {m_RTTI,this}; }      \
                                    public: virtual std::pair<const rtti::base&,      void*> getVRTTI( void )       noexcept { return {m_RTTI,this}; }      \

#endif

