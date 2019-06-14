
namespace xcore::string
{
    //------------------------------------------------------------------------------
    // Determine the length of a null '0' terminated string
    //------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_CHAR > constexpr xforceinline
        auto Length( const T_CHAR* const pStr ) noexcept
        {
            xassume( pStr );
            const T_CHAR* pEnd = pStr;
            if( pStr ) while( *pEnd++ ) {}
            return string::units<T_CHAR>{ static_cast<int>( pEnd - pStr - 1 ) };
        }
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr xforceinline
    auto Length( const T& Obj ) noexcept 
    { 
        if constexpr(is_obj_v<T>)   return details::Length<T::char_t>(Obj); 
        else                        return details::Length(Obj); 
    }

    //------------------------------------------------------------------------------
    // Copies N characters from one string buffer to another
    //------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_C1, typename T_C2 > constexpr xforceinline 
        auto CopyN( view<T_C1> Dest, const T_C2* const pSrc, units<decltype(pSrc[0])> Count ) noexcept
        {
            xassume( Dest  );
            xassume( pSrc  );
            xassume( Count.m_Value >= 0 );
            xassume( Dest.size() > Count.m_Value );

            Dest[Count.m_Value--] = 0;
            while( Count.m_Value >= 0 )
            {
                Dest[Count.m_Value] = pSrc[Count.m_Value];
                Count.m_Value--;
            }
    
            return units<T_C1>{ Count.m_Value };
        }
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto CopyN( T1& Dest, const T2& Src, units<decltype(Src[0])> Count ) noexcept 
    { 
        // Make sure Destination is ready to get the right data
        if constexpr(is_ref_v<T1>) Dest.ResetToSize( Count );

        if constexpr(is_obj_v<T2>)  return details::CopyN<T1::char_t,T2::char_t>(Dest,Src,Count); 
        else                        return details::CopyN<T1::char_t>(Dest,Src,Count); 
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto CopyN( T1&& Dest, const T2& Src, units<decltype(Src[0])> Count ) noexcept
    {
        static_assert( is_view_v<T1> );
        return CopyN(Dest,Src,Count);
    }

    //------------------------------------------------------------------------------
    // Copy one string buffer to another
    //------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_C1, typename T_C2 > constexpr 
        auto Copy( view<T_C1> Dest, const T_C2* pSrc ) noexcept
        {
            xassume( pSrc );
            xassume( Dest );
            xassume( Dest.size() > 0 );
    
            const auto Max =  Dest.size() - 1;
            int i;
            for( i=0; (Dest[i] = pSrc[i]) && (i < Max); i++ );
    
            xassert_block_medium()
            {
                for( auto j = i; j < Dest.size(); j++ ) Dest[j] = T_C1{'?'};
            }
    
            Dest[i] = 0;
    
            return units<T_C1>{ i };
        }
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr
    auto Copy( T1& Dest, const T2& Src ) noexcept
    {
        // Make sure Destination is ready to get the right data
        if constexpr ( is_ref_v<T1> ) 
        {
            // Quick copies (Share pointer or constant strings)
            if constexpr( is_ref_v<T2> ) 
                if constexpr ( std::is_same_v<std::decay_t<T1::char_t>,std::decay_t<T2::char_t>> )
            {
                if( Src.m_Ref.index() == T2::index_share_v )
                {
                    Dest.m_Ref.emplace<T1::share_t>( std::get<T2::share_t>(Src.m_Ref) );
                    return Length(Dest);
                }
                else if( Src.m_Ref.index() == T2::index_const_v )
                {
                    Dest.m_Ref.emplace<T1::constant_t>( std::get<T2::constant_t>(Src.m_Ref) );
                    return Length(Dest);
                }
            }

            // Copy string
            return CopyN( Dest, Src, Length(Src) );
        }
        else
        {
            if constexpr(is_obj_v<T2>)  return details::Copy<T1::char_t,T2::char_t>(Dest,Src); 
            else                        return details::Copy<T1::char_t>(Dest,Src); 
        }
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto Copy( T1&& Dest, const T2& Src ) noexcept
    {
        static_assert( is_view_v<T1> );
        return Copy(Dest,Src);
    }

    //------------------------------------------------------------------------------
    // Append
    //------------------------------------------------------------------------------

    template< typename T1, typename T2 > constexpr
    auto Append( T1& Dest, const T2& Src ) noexcept
    {
        const auto LD    = Length(Dest);
        const auto LS    = Length(Src);
        const auto Total = LD + LS;

        // Make sure Destination is ready to get the right data
        if constexpr ( is_ref_v<T1> ) 
        {
            // Resize if we need to
            Dest.ResetToSize( Total );
        }
    
        // Copy string
        return CopyN( view<T1::char_t>{ &Dest[LD.m_Value], (Total - LD).m_Value }, Src, LS );
    }

    //------------------------------------------------------------------------------

    template< typename T1, typename T2 > constexpr
    auto Append( T1&& Dest, const T2& Src ) noexcept
    {
        static_assert( is_view_v<T1> );
        return Append(Dest,Src);
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    inline
    units<char> sprintf ( view<char> Dest, const char* pFmt, ... ) noexcept
    {
        va_list Args;
        va_start( Args, pFmt );
        auto n = vsnprintf ( Dest.data(), Dest.size(), pFmt, Args );
        va_end(Args);
        return units<char>{n};
    }

    //------------------------------------------------------------------------------
    // Converts a string to a guid
    //------------------------------------------------------------------------------
    template< typename T_CHAR > constexpr
    std::uint64_t ToGuid( const T_CHAR* const pStr ) noexcept
    {
        // read first part 
        std::uint64_t   N = 0;
        T_CHAR          c = pStr[0];
        int i = 0;
        while((c >= T_CHAR{'0'} && c <= T_CHAR{'9'}) || (c >= T_CHAR{'a'} && c <= T_CHAR{'f'} ) || (c >= T_CHAR{'A'} && c <= T_CHAR{'F'}))
        {
            int v;
            if ( c > T_CHAR{'9'} ) v = (ToCharLower(c) - T_CHAR{'a'} ) + 10;
            else                   v = (ToCharLower(c) - '0');
            N <<= 4;
            N  |= (v & 0xF);
            c   = pStr[++i];
        }

        return N;
    }


}



