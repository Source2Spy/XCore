
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
            return string::units<T_CHAR>{ static_cast<details::units_int_t>( pEnd - pStr - 1 ) };
        }
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr xforceinline
    auto Length( const T& Obj ) noexcept 
    { 
        if constexpr ( std::is_same_v<std::string, xcore::types::decay_full_t<T>> 
                    || std::is_same_v<std::string_view, xcore::types::decay_full_t<T>> )
        {
            return string::units<xcore::types::decay_full_t<decltype(Obj[0])>>{ static_cast<details::units_int_t>(Obj.length()) };
        }
        else
        {
            return details::Length(&Obj[0]);
        }
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
            xassume( Dest.size() > Count.m_Value );

            if(Count.m_Value)
            {
                Dest[Count.m_Value] = 0;
                do
                {
                    Count.m_Value--;
                    Dest[Count.m_Value] = pSrc[Count.m_Value];
                } while (Count.m_Value);
            }
    
            return units<T_C1>{ Count.m_Value };
        }
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto CopyN( T1& Dest, const T2& Src, units<decltype(Dest[0])> Count ) noexcept
    { 

        if constexpr( xcore::types::is_specialized_v<std::basic_string, T1> )
        {
            Dest = std::string( &Src[0], Count );
        }
        else
        {
            // Make sure Destination is ready to get the right data
            if constexpr (is_ref_v<T1>) Dest.ResetToSize(units<T1::char_t>{ Count.m_Value + 1 });

            if constexpr (is_obj_v<T2>) return details::CopyN<T1::char_t>(Dest, &Src[0], Count);
            else                        return details::CopyN<T1::char_t>(Dest, Src, Count);
        }
    }

    /*
    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto CopyN( T1&& Dest, const T2& Src, units<decltype(Src[0])> Count ) noexcept
    {
        static_assert( is_view_v<T1> );
        return CopyN(Dest,Src,Count);
    }
    */

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
            details::units_int_t i;
            for( i=0; (Dest[i] = static_cast<std::decay_t<decltype(Dest[i])>>(pSrc[i])) && (i < Max); i++ );
    
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

    template< typename T_CHAR, typename T > constexpr
    ref<T_CHAR> To( const T& Str ) noexcept
    {
        const auto StrLen = ref<T_CHAR>::units(Length(Str).m_Value + 1);
        ref<T_CHAR> Ref(StrLen);
        for( auto i=0u, end_i = StrLen.m_Value-1; i<end_i; ++i ) Ref[i] = static_cast<T_CHAR>(Str[i]);
        Ref[StrLen.m_Value-1] = 0;
        return Ref;
    }

    //------------------------------------------------------------------------------

    template< typename T_CHAR, typename T > constexpr
    ref<T_CHAR> To( T aVal, const int Base ) noexcept
    {
        static_assert( std::is_integral_v<T> );
        using t = xcore::types::to_uint_t<T>;
        using u = typename ref<T_CHAR>::units;
        constexpr auto  string_capacity_v   = 64;
        auto            String              = string::ref<T_CHAR>(u(string_capacity_v) );
        int             iCursor             = 0;
        int             i                   = 0;
        auto            Val                 = static_cast<t>(aVal);

        xassert( Base > 2 );
        xassert( Base <= (26+26+10) );

        if constexpr ( std::is_signed_v<T> )
        {
            if( Val < 0 )
            {
                String[iCursor++] = '-';
                i++;
            }
        }
        do 
        {
            const auto CVal = t(Val % Base);
            Val     /= Base;

            // convert to ascii and store 
            if ( CVal < 10 )
            {
                String[u(iCursor++)] = T_CHAR( CVal + T_CHAR{'0'} );
            }
            else if( CVal < (10+26) )
            {
                String[u(iCursor++)] = T_CHAR( CVal - 10 + T_CHAR{'a'} );
            }
            else if( CVal < (10+26+26) )
            {
                String[u(iCursor++)] = T_CHAR( CVal - 10 - 26 + T_CHAR{'A'} );
            }
            else
            {
                // The base is too big
                xassert( false );
            }

        } while( Val > 0 && iCursor < string_capacity_v);

        //  terminate string; 
        String[u(iCursor)] = 0;

        // Reverse string order
        for( iCursor--; iCursor > i ; i++, iCursor-- )
        {
            std::swap( String[u(iCursor)], String[u(i)] );
        }

        return String;
    }

    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    auto Copy( T1&& Dest, const T2& Src ) noexcept
    {
        static_assert( is_view_v<T1> );
        return Copy(Dest,Src);
    }

    //------------------------------------------------------------------------------
    // FindStr
    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    int FindStr(const T1& Where, const T2& What) noexcept
    {
        int i=0;
        while( Where[i] )
        {
            if( Where[i] == What[0] )
            {
                int j=1;
                while(What[j])
                {
                    if(Where[i+j] != What[j]) goto continue_loop;
                    j++;
                }
                return i;
                continue_loop:;
            }
            i++;
        }
        return -1;
    }

    //------------------------------------------------------------------------------
    // FindStr
    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    int FindStrI(const T1& Where, const T2& What) noexcept
    {
        int i=0;
        while( Where[i] )
        {
            if(xcore::string::ToCharUpper(Where[i]), xcore::string::ToCharUpper(What[0]) )
            {
                int j=1;
                while(What[j])
                {
                    if(xcore::string::ToCharUpper(Where[i + j]) != xcore::string::ToCharUpper(What[j])) goto continue_loop;
                    j++;
                }
                return i;
                continue_loop:;
            }
            i++;
        }
        return -1;
    }

    //------------------------------------------------------------------------------
    // CompareI
    //------------------------------------------------------------------------------
    template< typename T1, typename T2 > constexpr xforceinline
    int CompareI(const T1& Str1, const T2& Str2) noexcept
    {
        int i = 0;
        for( ; Str2[i]; ++i )
        {
            const auto a = ToCharUpper(Str1[i]);
            const auto b = ToCharUpper(Str2[i]);
            if( a == b ) continue;
            return (a > b)?1:-1;
        }
        return Str1[i]?1:0;
    }

    //------------------------------------------------------------------------------

    template< typename T1, typename T2 > constexpr xforceinline
    int Compare(const T1& Str1, const T2& Str2) noexcept
    {
        int i = 0;
        for( ; Str2[i]; ++i )
        {
            if( Str1[i] == Str2[i] ) continue;
            return (Str1[i] > Str2[i])?1:-1;
        }
        return Str1[i]?1:0;
    }

    //------------------------------------------------------------------------------

    template< typename T1, typename T2 > constexpr   
    int CompareN( const T1& Dest, const T2& Src, int Count ) noexcept
    {
        for (int i = 0; i < Count; i++)
        {
            std::uint16_t A = Dest[i];
            std::uint16_t B = Src[i];
            if (A < B) return -1;
            if (A > B) return 1;
            if (A == 0) return 0;
        }
        return 0;
    }

    //------------------------------------------------------------------------------

    template< typename T, typename T_CHAR > constexpr 
    int ReplaceChar( T& Str, T_CHAR FindChar, T_CHAR ReplaceChar ) noexcept
    {
        int i = 0;
        for (; Str[i]; ++i)
        {
            if (Str[i] == FindChar) Str[i] = ReplaceChar;
        }
        return i;
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_CHAR > constexpr
    int findLastInstance( const T& MainStr, const T_CHAR C ) noexcept
    {
        auto s = -1;
        if(MainStr) for (int i = 0; MainStr[i]; ++i) if (MainStr[i] == C) s = i;
        return s;
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    int CleanPath( T& Path ) noexcept
    {
        using char_t = xcore::types::decay_full_t<decltype(Path[0])>;

        // Convert to the right path symbol
        const int l = ReplaceChar( Path, char_t('\\'), char_t('/') );

        // Remove any duplicate symbols
        if constexpr( std::is_same_v<char, char_t> )
        {
            for( int i = findAndReplace( Path, "//", "/" ); i != l; i = l );
        }
        else
        {
            for( int i = findAndReplace( Path, L"//", L"/" ); i != l; i = l );
        }

        // Go up if requested
        int pos = 0;
        int x   = 0;
        constexpr auto pStr = []{if constexpr (std::is_same<char, char_t>::value) return "/.."; else return L"/..";}();

        for( int i=0; Path[i]; ++i )
        {
            if( pStr[x] != Path[i] ) x=0;
            if( pStr[x] == Path[i] ) x++;

            if( x == 3 )
            {
                // remove one path
                for( int y = pos - 3; y && Path[y]; --y )
                {
                    if( Path[y] == char_t('/') )
                    {
                        pos = y;
                        break;
                    }
                    XCORE_CMD_ASSERT( Path[y] = char_t('X') );
                }
                x = 0;
            }
            else
            {
                Path[ pos++ ] = Path[ i ];
            }
        }

        Path[ pos ] = 0;

        return pos;
    }

    //------------------------------------------------------------------------------

    template< typename T1, typename T2, typename T3 > constexpr
    int findAndReplace( T1& Str, const T2& FindStr, const T3& ReplaceChar ) noexcept
    {
        static_assert( sizeof(decltype(Str[0])) == sizeof(decltype(FindStr[0])) );
        static_assert( sizeof(decltype(Str[0])) == sizeof(decltype(ReplaceChar[0])) );

        const int LenFindStr        = Length(FindStr).m_Value;
        const int LenReplaceChar    = Length(ReplaceChar).m_Value;
        
        if( LenFindStr >= LenReplaceChar )
        {
            xassert(Str[0]);
            int w = 0, r = 0;
            while( Str[r] )
            {
                if( Str[r] == FindStr[0] )
                {
                    int j = 0;
                    for( ; j<LenFindStr; j++)
                    {
                        if( Str[r+j] != FindStr[j] )
                            break;
                    }

                    if( j == LenFindStr )
                    {
                        for( int k = 0; k<LenReplaceChar; k++ )
                            Str[w++] = ReplaceChar[k];

                        // update the read cursor
                        r += j;

                        continue;
                    }

                    // not match fall through
                }

                // Copy the char
                Str[w++] = Str[r++];
            }

            // Make sure that the last character is correctly terminated
            Str[w++] = 0;
            return w;
        }
        else
        {
            // We woould need to grow the Str and we currently don't support that
            xassert(false);
        }

        return 0;
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

    template< typename T_CHAR > inline 
    ref<T_CHAR>& ref<T_CHAR>::operator = ( const char_t* pStr ) noexcept
    {
        string::Copy( *this, pStr );
        return *this;
    }

    //------------------------------------------------------------------------------

    template< typename T_CHAR > inline 
    ref<T_CHAR>& ref<T_CHAR>::operator = ( const self& Str ) noexcept
    {
        string::Copy( *this, Str );
        return *this;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    /*
    inline
    units<char> sprintf ( view<char> Dest, const char* pFmt, ... ) noexcept
    {
        va_list Args;
        va_start( Args, pFmt );
        auto n = vsnprintf ( Dest.data(), Dest.size(), pFmt, Args );
        va_end(Args);
        return units<char>{n};
    }
    */
    namespace details
    {
        int vsprintf( xcore::string::view<char> inBuffer, const char* pFormatStr, const xcore::arglist::view inArgList ) noexcept;
    }

    template< typename... T_ARGS > inline
    units<char> sprintf ( view<char> Dest, const char* pFmt, T_ARGS&&... Args ) noexcept
    {
        return units<char>{ static_cast<string::details::units_int_t>(details::vsprintf( Dest, pFmt, xcore::arglist::out{ std::forward<T_ARGS&&>(Args)...} )) };
    }

    //------------------------------------------------------------------------------
    // Converts a string to a guid
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    std::uint64_t ToGuid(const T& Str ) noexcept
    {
        using char_t = xcore::types::decay_full_t<decltype(Str[0])>;

        // read first part 
        std::uint64_t   N = 0;
        char_t          c = Str[0];
        int             i = 0;
        while((c >= char_t('0') && c <= char_t('9')) || (c >= char_t('a') && c <= char_t('f') ) || (c >= char_t('A') && c <= char_t('F')))
        {
            int v;
            if ( c > char_t('9') ) v = (ToCharLower(c) - char_t('a') ) + 10;
            else                   v = (ToCharLower(c) - '0');
            N <<= 4;
            N |= (v & 0xF);
            c = Str[++i];
        }

        return N;
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    xcore::err ToFullGuid( std::array<std::uint64_t, 2>& FullGuid, const T& Path ) noexcept
    {
        using char_t = xcore::types::decay_full_t<decltype(Path[0])>;

        const auto Len = Length(Path);
        if (Len.m_Value == 0) return xerr_failure_s( "Badly formatted Resource Full GUID for path, String was empty" );

        static constexpr auto   ExpectedOther = std::array{ char_t('-'), char_t('.') };
        static constexpr auto   Expected      = std::array{ char_t('/'), char_t('.') };
        int                     iExpected = 0;
        for( int i = Len.m_Value - 1; i >= 0; --i )
        {
            if( false == isCharHex(Path[i]) )
            {
                // Deal with expected strings
                if( Expected[iExpected] != Path[i] && ExpectedOther[iExpected] != Path[i] )
                    return xerr_failure_s( "Badly formatted Resource Full GUID for path, Unexpected characters" );

                // set the resource guid
                if( iExpected == 0)
                {
                    FullGuid[1] = ToGuid( &Path[i + 1] );
                }
                // At we at the begging of the type guid
                else if (iExpected == 1)
                {
                    FullGuid[0] = ToGuid(&Path[i + 1]);
                }

                // Set the next expected character
                iExpected++;
            }
        }

        if (iExpected != 2)
            return xerr_failure_s( "Badly formatted Resource Full GUID for path, Unexpected string termination" );

        FullGuid[0] |= ToGuid(Path)<<32;
        return {};
    }


}



