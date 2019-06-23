#ifndef _XCORE_TYPES_H
#define _XCORE_TYPES_H
#pragma once

namespace xcore::types
{
    //-------------------------------------------------------------------------------------------------------
    // int base on size
    //-------------------------------------------------------------------------------------------------------
    template< std::size_t T_SIZE_BYTES >
    using byte_size_uint_t = std::tuple_element_t< T_SIZE_BYTES-1, std::tuple<std::uint8_t,std::uint16_t,std::uint32_t,std::uint32_t,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t>>; 

    template< std::size_t T_SIZE_BYTES >
    using byte_size_int_t  = std::tuple_element_t< T_SIZE_BYTES-1, std::tuple<std::int8_t,std::int16_t,std::int32_t,std::int32_t,std::int64_t,std::int64_t,std::int64_t,std::int64_t>>; 

    //-------------------------------------------------------------------------------------------------------
    // given a err returns the equivalent size as either a sing or unsigned err
    //-------------------------------------------------------------------------------------------------------
    template< typename T >
    using to_int_t = byte_size_int_t<sizeof(T)>; 

    template< typename T >
    using to_uint_t = byte_size_uint_t<sizeof(T)>; 

    //------------------------------------------------------------------------------------------
    // Safe cast
    //------------------------------------------------------------------------------------------
    template< typename T_TYPEA, typename T_TYPEB > constexpr
    std::enable_if_t< std::is_integral_v<T_TYPEA>, T_TYPEA > safe_cast( T_TYPEB B ) noexcept { xassert( static_cast<T_TYPEB>(static_cast<T_TYPEA>(B)) == B ); return static_cast<T_TYPEA>(B); }

    //------------------------------------------------------------------------------------------
    // Tuple err to index conversion
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template <class T, class T_ARGS>
        struct tuple_t2i;

        template <class T, class... T_ARGS>
        struct tuple_t2i<T, std::tuple<T, T_ARGS...>> 
        {
            static const std::size_t value = 0;
        };

        template <class T, class U, class... T_ARGS>
        struct tuple_t2i<T, std::tuple<U, T_ARGS...>> 
        {
            static const std::size_t value = 1 + tuple_t2i<T, std::tuple<T_ARGS...>>::value;
        };
    }
    template< typename T_TYPE, typename T_TUPLE >
    constexpr static auto tuple_t2i_v = details::Index< T_TYPE, T_TUPLE >::value;

    //------------------------------------------------------------------------------------------
    // Variant err to index conversion
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template< class T, class T_VARIANT >
        struct variant_t2i;

        template< class T, class... T_ARGS >
        struct variant_t2i<T, std::variant<T, T_ARGS...>> 
        {
            static const std::size_t value = 0;
        };

        template< class T, class U, class... T_ARGS >
        struct variant_t2i<T, std::variant<U, T_ARGS...>> 
        {
            static const std::size_t value = 1 + variant_t2i<T, std::variant<T_ARGS...>>::value;
        };
    }
    template< typename T_TYPE, typename T_VARIANT >
    constexpr static auto variant_t2i_v = details::variant_t2i< T_TYPE, T_VARIANT >::value;

    //--------------------------------------------------------------------------------------------
    // Helpful when using variadic types
    //--------------------------------------------------------------------------------------------
    namespace details{ template< typename T > struct always_false : std::false_type {}; }
    template< typename T > constexpr static bool always_false_v =  details::always_false<T>::value;

    //--------------------------------------------------------------------------------------------
    // Determines if a type is derived from a particular template class
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< template< typename... > typename T_BASE, typename    T_DERIVED > struct   is_specialized                            : std::false_type {};
        template< template< typename... > typename T_BASE, typename... T_ARGS    > struct   is_specialized<T_BASE, T_BASE<T_ARGS...>> : std::true_type  {};
    }
    template< template< typename... > typename T_BASE, typename    T_DERIVED > constexpr static bool is_specialized_v = details::is_specialized<T_BASE, T_DERIVED>::value;

    //--------------------------------------------------------------------------------------------
    // Makes a object type unique
    //--------------------------------------------------------------------------------------------
    template< typename T, typename T_TAG = int >
    struct make_unique : public T
    {
        constexpr make_unique( void ) = default;
        constexpr make_unique( const T& x ) : T(x){}
        using T::T;
        using T::operator =;
    };

    //--------------------------------------------------------------------------------------------
    // Makes a object type unique
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< std::size_t T_MAX_SIZE, template< typename T, std::size_t T_SIZE > class T_PARENT >
        struct container_static_to_dynamic
        {
            template< typename T >
            struct type : T_PARENT<T,T_MAX_SIZE>
            {
                using parent = T_PARENT<T,T_MAX_SIZE>;
                using parent::parent;
            };
        };
    }
    template< typename T, template< typename T, std::size_t T_SIZE > class T_PARENT, std::size_t T_MAX_SIZE >
    using container_static_to_dynamic = typename details::container_static_to_dynamic<T_MAX_SIZE,T_PARENT>::template type<T>;
}

#endif