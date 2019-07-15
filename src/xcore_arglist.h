#ifndef _XCORE_ARGLIST_H
#define _XCORE_ARGLIST_H
#pragma once

namespace xcore::arglist
{
    //------------------------------------------------------------------------------
    using out_types = std::variant<  bool,
                                     std::uint8_t,  std::uint16_t,  std::uint32_t,  std::uint64_t,
                                     std::int8_t,   std::int16_t,   std::int32_t,   std::int64_t, 
                                     bool*,
                                     std::uint8_t*, std::uint16_t*, std::uint32_t*, std::uint64_t*,
                                     std::int8_t*,  std::int16_t*,  std::int32_t*,  std::int64_t*, 
                                     float*, double*, 
                                     void*,
                                     float, double, 
                                     const char*, xcore::string::view<char>,
                                     xcore::string::view<char>*, 
                                     xcore::string::ref<char>* >;

    //------------------------------------------------------------------------------
    using view = xcore::span<out_types>;

    //------------------------------------------------------------------------------
    template< typename... T_ARGS >
    struct out 
    {
        xcore::array< out_types, sizeof...(T_ARGS) > m_Params;
        constexpr out( T_ARGS... Args ) : m_Params { Args... } {}
        operator view () noexcept { return m_Params; }
    };

    //------------------------------------------------------------------------------
    constexpr bool isConvertible( const out_types& Out, std::size_t iTo ) noexcept
    {
        if( Out.index() <= xcore::types::variant_t2i_v<std::uint64_t,out_types> )
            return iTo  <= xcore::types::variant_t2i_v<std::uint64_t,out_types> && iTo >= Out.index();

        if( Out.index() <= xcore::types::variant_t2i_v<std::int64_t,out_types> )
            return ((iTo  <= xcore::types::variant_t2i_v<std::int64_t,out_types>  && iTo >= Out.index()) 
            ||      (iTo  <= xcore::types::variant_t2i_v<std::uint64_t,out_types> && iTo >= (Out.index()-4)));

        if( Out.index() <= xcore::types::variant_t2i_v<std::int64_t*,out_types> )
            return iTo == Out.index();

        if( iTo == xcore::types::variant_t2i_v<void*,out_types> )
            return  Out.index() >= xcore::types::variant_t2i_v<std::uint8_t*,out_types>
              &&    Out.index() <= xcore::types::variant_t2i_v<void*,out_types>;

        if( Out.index() <= xcore::types::variant_t2i_v<double,out_types> )
            return iTo  <= xcore::types::variant_t2i_v<double,out_types> && iTo >= Out.index();

        return iTo == xcore::types::variant_t2i_v<const char*,out_types>
               && Out.index() >= xcore::types::variant_t2i_v<const char*,out_types> ;
    }

    //------------------------------------------------------------------------------
    template< typename T > 
    constexpr bool isConvertible( const out_types& Out ) noexcept
    {
        return isConvertible( Out, xcore::types::variant_t2i_v<T,xcore::arglist::out_types> );
    }

    //------------------------------------------------------------------------------
    template< class T >
    constexpr const T get( const out_types& Out ) noexcept
    {
        xassert( arglist::isConvertible<T>( Out ) );

        T Answer;
        std::visit( [&]( auto Value )
        {
            using t = std::decay_t<decltype(Value)>;
            if constexpr (std::is_same_v<T,const char*> && types::variant_t2i_v<t,out_types> >= types::variant_t2i_v<const char*,out_types> )
            {
                     if constexpr ( types::variant_t2i_v<t,out_types> == types::variant_t2i_v<xcore::string::view<char>*,out_types> )  Answer = Value->data();
                else if constexpr ( types::variant_t2i_v<t,out_types> == types::variant_t2i_v<xcore::string::ref<char>*,out_types> )   Answer = Value->data();
                else                                                                                                                   Answer = Value;
            }
            else if constexpr( std::is_integral_v<T> 
                && types::variant_t2i_v<t,out_types> >= types::variant_t2i_v<std::uint8_t,out_types> 
                && types::variant_t2i_v<t,out_types> <= types::variant_t2i_v<std::int64_t,out_types> )
            {
                Answer = static_cast<T>(Value);
            }
            else if constexpr( std::is_floating_point_v<T> 
                && types::variant_t2i_v<t,out_types> >= types::variant_t2i_v<float,out_types> 
                && types::variant_t2i_v<t,out_types> <= types::variant_t2i_v<double,out_types> )
            {
                Answer = static_cast<T>(Value);
            }
            else if constexpr( std::is_reference_v<T> 
                && types::variant_t2i_v<t,out_types> >= types::variant_t2i_v<std::uint8_t*,out_types> 
                && types::variant_t2i_v<t,out_types> <= types::variant_t2i_v<void*,out_types> )
            {
                Answer = static_cast<T>(Value);
            }
            else
            {
                xassume( types::always_false_v<t> );
            }
        }, Out );

        return Answer;
    }

}

#endif
