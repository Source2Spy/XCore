#ifndef _XCORE_STRING_H
#define _XCORE_STRING_H
#pragma once

namespace xcore::string
{
    //------------------------------------------------------------------------------
    // units - characters, units_chars<8> == char, units_chars<16> == wchar
    //------------------------------------------------------------------------------
    namespace details
    {
        using size_t = std::uint32_t;

        template< typename T >
        struct units_imp : xcore::units::type<units_imp<T>, size_t>
        {
            static_assert( std::is_same_v<T,std::decay_t<T>> );
            using parent = xcore::units::type<units_imp<T>, size_t>;
            using parent::type;
            using parent::operator =;
                                            operator xcore::units::bytes    ( void )        const   noexcept { return units::bytes{ parent::m_Value * sizeof(T) }; }
            constexpr xcore::units::bytes   getBytes                        ( void )        const   noexcept { return units::bytes{ parent::m_Value * sizeof(T) }; }
        };
    }
    template< typename T > using units = details::units_imp<std::decay_t<T>>;

    //------------------------------------------------------------------------------------
    // view - string view
    //------------------------------------------------------------------------------------
    template< typename T_CHAR, std::size_t T_SIZE_V = std::dynamic_extent >
    struct view : xcore::span<T_CHAR,T_SIZE_V>
    {
        using char_t = T_CHAR;
        using units  = units<char_t>;
        constexpr static auto size_v = T_SIZE_V;
        using parent = xcore::span<char_t,size_v>;
        using parent::operator =;
        using parent::span;

        constexpr                   operator const char_t*          ( void )                    const   noexcept { xassume(parent::empty()==false); return parent::data(); }
                                    operator       char_t*          ( void )                            noexcept { xassume(parent::empty()==false); return parent::data(); }
        constexpr                   operator       std::string_view ( void )                            noexcept { xassume(parent::empty()==false); return { parent::data(), std::strlen(parent::data()) }; }
        constexpr                   operator const std::string_view ( void )                    const   noexcept { xassume(parent::empty()==false); return { parent::data(), std::strlen(parent::data()) }; }
        constexpr   bool            empty                           ( void )                    const   noexcept { return parent::data() == nullptr || parent::data()[0] == 0; }
        constexpr   auto            ViewFrom                        ( int Start )               const   noexcept { return static_cast<view&&>(parent::ViewFrom(Start)); }
        constexpr   auto            ViewFrom                        ( int Start )                       noexcept { return static_cast<view&&>(parent::ViewFrom(Start)); }
        constexpr   auto            ViewFromTo                      ( int Start, int To )       const   noexcept { return static_cast<view&&>(parent::ViewFromTo(Start,To)); }
        constexpr   auto            ViewFromTo                      ( int Start, int To )               noexcept { return static_cast<view&&>(parent::ViewFromTo(Start,To)); }
        constexpr   auto            ViewTo                          ( int To )                  const   noexcept { return static_cast<view&&>(parent::ViewFromTo(0, To)); }
        constexpr   auto            ViewTo                          ( int To )                          noexcept { return static_cast<view&&>(parent::ViewFromTo(0, To)); }
    };
     
    // Deduction Guides
    template <class T, size_t N>
    view(T (&)[N])->view<T, N>;

    template <class T, size_t N>
    view(std::array<T, N>&)->view<T, N>;

    template <class T, size_t N>
    view(const std::array<T, N>&)->view<const T, N>;

    template <class T>
    view(T*,int)->view<T>;

    template <class Container>
    view(Container&)->view<typename Container::value_type>;

    template <class Container>
    view(const Container&)->view<const typename Container::value_type>;
    
    //------------------------------------------------------------------------------------
    // constant - string can not be modified
    //------------------------------------------------------------------------------------
    template< typename T_CHAR >
    struct constant 
    { 
        static_assert( std::is_const_v<T_CHAR> == false );
        using char_t    = T_CHAR;
        using units     = units<char_t>;
        using type      = const char_t*; 

        const char_t*   m_pValue      = nullptr; 
        units           m_Size        = units{0};

        template< std::size_t N >
        constexpr                   constant                            ( const char_t(&Str)[N] )           noexcept : m_pValue{Str}, m_Size{N} {}
        constexpr                   constant                            ( const constant& )                 noexcept = default;
        constexpr                   constant                            ( void )                            noexcept = default;
        constexpr   const char_t&   operator[]                          ( units I )                 const   noexcept { xassume( m_pValue ); xassume( I < m_Size ); return m_pValue[I.m_Value]; }
//                    char_t&         operator[]                          ( units I )                         noexcept { xassume( m_pValue ); xassume( I < m_Size ); return m_pValue[I.m_Value]; }
        constexpr                   operator const view<const char_t>   ( void )                    const   noexcept { xassume( m_pValue ); return { m_pValue, m_Size.m_Value }; }
        constexpr                   operator const view<char_t>         ( void )                    const   noexcept { xassume( m_pValue ); return { const_cast<char_t*>(m_pValue), m_Size.m_Value }; }
        constexpr                   operator const char_t*              ( void )                    const   noexcept { xassume( m_pValue ); return m_pValue; }
        constexpr   int             size                                ( void )                    const   noexcept { return m_Size.m_Value; }
        constexpr   auto            get                                 ( void )                    const   noexcept { return m_pValue; }
        constexpr   auto            getView                             ( void )                    const   noexcept { return static_cast<view<const char_t>>(*this); }
        constexpr   auto            getViewFrom                         ( int Start )               const   noexcept { return view<const char_t>{ &(*this)[Start], m_Size.m_Value - Start }; }
        constexpr   bool            empty                               ( void )                    const   noexcept { return m_pValue == nullptr || m_pValue[0] == 0; }
    };

    //------------------------------------------------------------------------------------

    struct const_universal
    {
        constant<char>          m_Str;
        constant<wchar_t>       m_WStr;
    };

    #define xconst_universal_str(A)  xcore::string::const_universal{ A, L##A }

    //------------------------------------------------------------------------------------
    template< typename T_CHAR >
    struct const_crc
    {
        template< std::size_t T_LENGTH >
        constexpr const_crc(const T_CHAR(&Str)[T_LENGTH])   noexcept : m_Str{ Str }, m_CRC{ xcore::crc<32>::FromString(Str) } {}
        constexpr const_crc(void)                           noexcept = default;
        constexpr const_crc(const const_crc&)               noexcept = default;

        constant<T_CHAR>    m_Str;
        crc<32>             m_CRC;
    };

    // Deduction Guides
    //template <class T, std::size_t N>
    //const_crc(T (&)[N])->const_crc<T>;

    //------------------------------------------------------------------------------------
    // fixed - fixed size string class
    //------------------------------------------------------------------------------------
    template< typename T_CHAR, std::size_t T_SIZE_V >
    struct fixed : std::array<T_CHAR, T_SIZE_V>
    {
        using char_t    = T_CHAR;
        using units     = units<char_t>;
        using parent    = std::array<T_CHAR, T_SIZE_V>;
        constexpr static units      ms_BufferSize  = units{static_cast<int>(T_SIZE_V)};

        using parent::array;
        using parent::operator =;

        constexpr                   fixed                                       ( const view<char_t,T_SIZE_V> Str )         noexcept { xassume(Str.size()<=ms_BufferSize.m_Value); for(int i=0; i<Str.size(); ++i) parent::at(i) = Str[i]; } //-V573
        constexpr                   fixed                                       ( const view<const char_t,T_SIZE_V> Str )   noexcept { xassume(Str.size()<=ms_BufferSize.m_Value); for(int i=0; i<Str.size(); ++i) parent::at(i) = Str[i]; } //-V573
        constexpr                   fixed                                       ( const view<char_t> Str )                  noexcept { xassume(Str.size()<=ms_BufferSize.m_Value); for(int i=0; i<Str.size(); ++i) parent::at(i) = Str[i]; } //-V573
        constexpr                   fixed                                       ( const view<const char_t> Str )            noexcept { xassume(Str.size()<=ms_BufferSize.m_Value); for(int i=0; i<Str.size(); ++i) parent::at(i) = Str[i]; } //-V573
        constexpr                   fixed                                       ( const char_t (&Str)[T_SIZE_V] )           noexcept : parent{ [&]()constexpr{ parent A{}; for(int i=0; i<T_SIZE_V; ++i) A[i] = Str[i]; return A;}() }{} //-V573
        template< std::size_t N >
        constexpr                   fixed                                       ( const char_t (&Str)[N] )                  noexcept : parent{ [&]()constexpr{ static_assert(N <= T_SIZE_V); parent A{}; for (int i = 0; i < N; ++i) A[i] = Str[i]; return A; }() }{} //-V573
        constexpr                   fixed                                       ( const char_t* pStr )                      noexcept : parent{ [&]()constexpr{ parent A{}; for (int i = 0; A[i] = pStr[i]; ++i); return A; }() }{} //-V573 

        inline                      operator view<char_t,T_SIZE_V>              ( void )                                    noexcept { return { parent::data() }; }
        inline                      operator view<char_t>                       ( void )                                    noexcept { return { parent::data(), size() }; }
        constexpr                   operator const view<const char_t,T_SIZE_V>  ( void )                            const   noexcept { return { parent::data() }; }
        constexpr                   operator const view<const char_t>           ( void )                            const   noexcept { return { parent::data(), size() }; }
        constexpr                   operator const char_t*                      ( void )                            const   noexcept { return parent::data(); }
        constexpr static    auto    size                                        ( void )                                    noexcept { return ms_BufferSize.m_Value; }
        inline              auto    get                                         ( void )                                    noexcept { return parent::data(); }
        constexpr           auto    get                                         ( void )                            const   noexcept { return parent::data(); }
        inline              auto    getView                                     ( void )                                    noexcept { return static_cast<view<char_t>>(*this); }
        constexpr           auto    getView                                     ( void )                            const   noexcept { return static_cast<const view<char_t>>(*this); }
        inline              auto    getViewFrom                                 ( int Start )                               noexcept { return string::view{ &(*this)[Start], size()-Start }; }
        constexpr           auto    getViewFrom                                 ( int Start )                       const   noexcept { return string::view{ &(*this)[Start], size()-Start }; }
        constexpr           bool    empty                                       ( void )                            const   noexcept { return parent::data()[0] == 0; }
        inline              void    clear                                       ( void )                                    noexcept { parent::data()[0]=0; }
    };

    //------------------------------------------------------------------------------------
    // unique - string owner
    //------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_CHAR >
        struct unique : std::unique_ptr<T_CHAR[]>
        {
            using char_t = T_CHAR;
            using units  = units<T_CHAR>;
            using parent = std::unique_ptr<T_CHAR[]>;
            using parent::unique_ptr;
            using parent::operator =;
            units m_Size{0};
            constexpr int size( void ) const noexcept { return m_Size.m_Value; }
            constexpr unique( unique&& U ) noexcept : parent{ std::move(U) }, m_Size{U.m_Size}{}
            constexpr unique( T_CHAR* p, int Size ) noexcept : parent{ p }, m_Size{Size}{}
        };
    }

    //------------------------------------------------------------------------------------
    // String manipulations
    //------------------------------------------------------------------------------------
    template< typename T >                  constexpr   auto            Length          ( const T& Obj )                                                            noexcept;
    template< typename T1, typename T2 >    constexpr   auto            CopyN           ( T1& Dest, const T2& Src,  units<decltype(Src[0])> Count )                 noexcept;
    template< typename T1, typename T2 >    constexpr   auto            Copy            ( T1& Dest, const T2& Src )                                                 noexcept;
    template< typename T1, typename T2 >    constexpr   auto            Append          ( T1& Dest, const T2& Src )                                                 noexcept;
    template< typename T1, typename T2 >    constexpr   int             CompareN        ( const T1& Dest, const T2& Src, int Count )                 noexcept;

    template< typename... T_ARGS >
    inline      units<char>     sprintf         ( view<char> Dest, const char*, T_ARGS... Args )                                       noexcept;

    template< typename T_CHAR >             constexpr   std::uint64_t   ToGuid          ( const T_CHAR* const pStr )                                                noexcept;

    template< typename T_CHAR >             constexpr   int             ToCharUpper     ( const T_CHAR C )                                                          noexcept { return( (C >= T_CHAR{'a'}) && (C <= T_CHAR{'z'}) )? C + (T_CHAR{'A'} - T_CHAR{'a'}) : C; }
    template< typename T_CHAR >             constexpr   int             ToCharLower     ( const T_CHAR C )                                                          noexcept { return( (C >= T_CHAR{'A'}) && (C <= T_CHAR{'Z'}) )? C + (T_CHAR{'a'} - T_CHAR{'A'}) : C; }
    template< typename T_CHAR >             constexpr   bool            isCharSpace     ( const T_CHAR C )                                                          noexcept { return  (C == 0x09) || (C == 0x0A) || (C == 0x0D) || (C == 32 ); }
    template< typename T_CHAR >             constexpr   bool            isCharDigit     ( const T_CHAR C )                                                          noexcept { return( (C >=  T_CHAR{'0'}) && (C <= T_CHAR{'9'}) ); }
    template< typename T_CHAR >             constexpr   bool            isCharAlpha     ( const T_CHAR C )                                                          noexcept { return( ((C >= T_CHAR{'A'}) && (C <= T_CHAR{'Z'})) || ((C >= T_CHAR{'a'}) && (C <= T_CHAR{'z'})) ); }
    template< typename T_CHAR >             constexpr   bool            isCharUpper     ( const T_CHAR C )                                                          noexcept { return( (C >=  T_CHAR{'A'}) && (C <= T_CHAR{'Z'}) ); }
    template< typename T_CHAR >             constexpr   bool            isCharLower     ( const T_CHAR C )                                                          noexcept { return( (C >=  T_CHAR{'a'}) && (C <= T_CHAR{'z'}) ); }
    template< typename T_CHAR >             constexpr   bool            isCharHex       ( const T_CHAR C )                                                          noexcept { return( ((C >= T_CHAR{'A'}) && (C <= T_CHAR{'F'})) || ((C >= T_CHAR{'a'}) && (C <= T_CHAR{'f'})) || ((C >=  T_CHAR{'0'}) && (C <= T_CHAR{'9'})) ); }

#if 0
    template< typename T_CHAR > constexpr       chars<T_CHAR>   cpy             ( view<T_CHAR> Dest, const T_CHAR* pSrc )                                           noexcept;
    template< typename T_CHAR > constexpr       void*           memchr          ( void* pBuf, const T_CHAR C, const units_chars_any<T_CHAR> aCount )                noexcept;
    template< typename T_CHAR > constexpr       chars<T_CHAR>   dtoa            ( const u64 Val, xbuffer_view<T_CHAR> Buffer, const int Base )                      noexcept;
    template< typename T_CHAR > constexpr       chars<T_CHAR>   dtoa            ( const s64 Val, xbuffer_view<T_CHAR> Buffer, const int Base )                      noexcept;
    template< typename T_CHAR > constexpr       chars<T_CHAR>   dtoa            ( const u32 Val, xbuffer_view<T_CHAR> Buffer, const int Base )                      noexcept;
    template< typename T_CHAR > constexpr       chars<T_CHAR>   dtoa            ( const int Val, xbuffer_view<T_CHAR> Buffer, const int Base )                      noexcept;
    template< typename T_CHAR > x_inline        s64         atod64              ( const T_CHAR* pStr, const int Base )                                              noexcept;
    template< typename T_CHAR > x_inline        int         x_atod32            ( const char* pStr, const int Base )                                                noexcept;
    template< typename T_CHAR > x_inline        u32         Hash                ( const T_CHAR* pStr, const u32 Range = 0xffffffff, const u32 hVal = 1 )            noexcept;
    template< typename T_CHAR > x_inline        u32         IHash               ( const T_CHAR* pStr, const u32 Range = 0xffffffff, const u32 hVal = 1 )            noexcept;
    template< typename T_CHAR > x_inline        u64         IHash64             ( const T_CHAR* pStr, const u64 hVal )                                              noexcept;
    template< typename T_CHAR > x_inline        u64         toguid              ( const T_CHAR* const pStr, const T_CHAR Separator = T_CHAR{ ':' } )                noexcept;
    template< typename T_CHAR > x_inline        int         ncmp                ( const T_CHAR* pStr1, const T_CHAR* pStr2, int Count )                             noexcept;
    template< typename T_CHAR > x_inline        int         cmp                 ( const T_CHAR* pStr1, const T_CHAR* pStr2 )                                        noexcept;
    template< typename T_CHAR > x_inline        int         icmp                ( const T_CHAR* pStr1, const T_CHAR* pStr2 )                                        noexcept;
    template< typename T_CHAR > x_inline        int         str                 ( const T_CHAR* const pMainStr, const T_CHAR* const pSubStr )                       noexcept;
    template< typename T_CHAR > x_inline        int         istr                ( const T_CHAR* const pMainStr, const T_CHAR* const pSubStr )                       noexcept;
    template< typename T_CHAR > x_inline        int         lastchar            ( const T_CHAR* const pMainStr, const T_CHAR C )                                    noexcept;
    template< typename T_CHAR > x_inline        int         firstchar           ( const T_CHAR* const pMainStr, const T_CHAR C )                                    noexcept;
    template< typename T_CHAR > x_inline   const T_CHAR*    lastchar_ptr        ( const T_CHAR* const pMainStr, const T_CHAR C )                                    noexcept;
    template< typename T_CHAR > x_inline   const T_CHAR*    firstchar_ptr       ( const T_CHAR* const pMainStr, const T_CHAR C )                                    noexcept;
    template< typename T_CHAR > x_inline        int         tolower             ( T_CHAR* const pMainStr )                                                          noexcept;
    template< typename T_CHAR > x_inline        int         toupper             ( T_CHAR* const pMainStr )                                                          noexcept;
    template< typename T_CHAR > x_inline        int         atoi32              ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        s64         atoi64              ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        bool        isstrint            ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        bool        isstrfloat          ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        bool        isstrhex            ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        bool        isstrguid           ( const T_CHAR* pStr, const T_CHAR Separator = T_CHAR{ ':' } )                      noexcept;
    template< typename T_CHAR > x_inline        f32         atof32              ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        f64         atof64              ( const T_CHAR* pStr )                                                              noexcept;
    template< typename T_CHAR > x_inline        int         toupper             ( const T_CHAR C )                                                                  noexcept { return( (C >= T_CHAR{'a'}) && (C <= T_CHAR{'z'}) )? C + (T_CHAR{'A'} - T_CHAR{'a'}) : C; }
    template< typename T_CHAR > x_inline        int         tolower             ( const T_CHAR C )                                                                  noexcept { return( (C >= T_CHAR{'A'}) && (C <= T_CHAR{'Z'}) )? C + (T_CHAR{'a'} - T_CHAR{'A'}) : C; }
    template< typename T_CHAR > x_inline        bool        isspace             ( const T_CHAR C )                                                                  noexcept { return( (C == 0x09) || (C == 0x0A) || (C == 0x0D) || (C == T_CHAR{' '}) ); }
    template< typename T_CHAR > x_inline        bool        isdigit             ( const T_CHAR C )                                                                  noexcept { return( (C >=  T_CHAR{'0'}) && (C <= T_CHAR{'9'}) ); }
    template< typename T_CHAR > x_inline        bool        isalpha             ( const T_CHAR C )                                                                  noexcept { return( ((C >= T_CHAR{'A'}) && (C <= T_CHAR{'Z'})) || ((C >= T_CHAR{'a'}) && (C <= T_CHAR{'z'})) ); }
    template< typename T_CHAR > x_inline        bool        isupper             ( const T_CHAR C )                                                                  noexcept { return( (C >=  T_CHAR{'A'}) && (C <= T_CHAR{'Z'}) ); }
    template< typename T_CHAR > x_inline        bool        islower             ( const T_CHAR C )                                                                  noexcept { return( (C >=  T_CHAR{'a'}) && (C <= T_CHAR{'z'}) ); }
    template< typename T_CHAR > x_inline        bool        ishex               ( const T_CHAR C )                                                                  noexcept { return( ((C >= T_CHAR{'A'}) && (C <= T_CHAR{'F'})) || ((C >= T_CHAR{'a'}) && (C <= T_CHAR{'f'})) || ((C >=  T_CHAR{'0'}) && (C <= T_CHAR{'9'})) ); }
    template< typename T_CHAR > x_inline        u32         strCRC              ( const T_CHAR* pStr, u32 crcSum = 0x00000000u )                                    noexcept;
    template< typename T_CHAR > x_inline        int         replace             ( xbuffer_view<T_CHAR> Str, const T_CHAR  FindChar, const T_CHAR ReplaceChar )      noexcept;
    template< typename T_CHAR > x_inline        int         replace             ( xbuffer_view<T_CHAR> Str, const T_CHAR* const pFindStr, const T_CHAR* const pReplaceChar ) noexcept;
    template< typename T_CHAR > x_inline        int         cleanpath           ( xbuffer_view<T_CHAR> Path )                                                       noexcept;
#endif

    //------------------------------------------------------------------------------------
    // ref - reference string class this class handle all kinds of types of strings at the same time
    //------------------------------------------------------------------------------------
    template< typename T_CHAR >
    struct ref 
    { 
        using                           self            = ref<T_CHAR>;
        using                           char_t          = T_CHAR;
        using                           units           = xcore::string::units<char_t>;
        constexpr static int            cache_size_v    = (64 - sizeof(std::size_t))/sizeof(char_t); // 64bytes - (the variant type index with alignment)
        using                           cache_t         = fixed<char_t,cache_size_v>; 
        using                           unique_t        = details::unique<char_t>;
        using                           share_t         = std::shared_ptr<ref<char_t>>;
        using                           constant_t      = constant<char_t>;
        using                           fixed_t         = xcore::types::make_unique<unique_t, struct fixed_tag>;
        using                           type            = std::variant< 
                                                              cache_t                           // This is a mini cache used for unique_t strings 
                                                            , unique_t                          // Can be reallocated and I am the owner
                                                            , share_t                           // This string is been share with others, can only be written if only one person looks at it else it will decay to a unique_t
                                                            , constant_t                        // This string can only be read, but if written to it will decay to a unique_t
                                                            , fixed_t                           // can not be reallocated for resizing and I am the owner
                                                          >;
        constexpr static std::size_t    index_cache_v   = xcore::types::variant_t2i_v<cache_t,type>;
        constexpr static std::size_t    index_unique_v  = xcore::types::variant_t2i_v<unique_t,type>;
        constexpr static std::size_t    index_share_v   = xcore::types::variant_t2i_v<share_t,type>;
        constexpr static std::size_t    index_const_v   = xcore::types::variant_t2i_v<constant_t,type>;
        constexpr static std::size_t    index_fixed_v   = xcore::types::variant_t2i_v<fixed_t,type>;
        
        type    m_Ref           {};

        template< std::size_t N >
        constexpr       ref     ( const char_t(&Str)[N] ) noexcept { m_Ref.emplace<constant_t>(Str); };
        constexpr       ref     ( void ) noexcept {std::get<cache_t>(m_Ref)[0u]=0;}
        constexpr       ref     ( ref&& Str ) noexcept
        {
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                        if constexpr( std::is_same_v<T,unique_t> )        m_Ref.emplace<unique_t>( std::move(X) );
                else    if constexpr( std::is_same_v<T,fixed_t>  )        m_Ref.emplace<fixed_t> ( std::move(X) );
                else                                                      m_Ref = X;
            }, Str.m_Ref );
        }
        constexpr       ref     ( const ref& Str ) noexcept
        {
            if( Str.empty() ) 
            {
                std::get<cache_t>(m_Ref)[0u]=0;
                return; 
            }

            if( Str.m_Ref.index() ==  index_fixed_v ) MakeFixed( string::Length(Str) );
            string::Copy( *this, Str );
        }

        constexpr       ref     ( const constant_t Str ) noexcept
        {
            m_Ref.emplace<constant_t>(Str);
        }

        template< std::size_t N >
        constexpr       ref     ( const string::fixed<char_t,N> Str ) noexcept
        {
            MakeFixed( units{N-1} );
            string::Copy( *this, Str );
        }

        constexpr       ref     ( units Characters ) noexcept
        {
            MakeUnique( Characters );
        }

        template< typename T_C > 
        constexpr       ref     ( string::view<T_C> View ) noexcept
        {
            string::Copy(*this, View);
        }

        inline ref<T_CHAR>& operator = ( const char_t* pStr ) noexcept;
        inline ref<T_CHAR>& operator = ( const self& Str ) noexcept;

        constexpr int size( void ) const noexcept 
        { 
            int Size;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if constexpr( std::is_same_v<T,share_t> ) Size = X->size();
                else                                      Size = static_cast<int>(X.size());
            }, m_Ref );
            return Size;
        }

        void clear( void ) noexcept
        {
            m_Ref.emplace<cache_t>();
            (*this)[0] = 0;
        }

        ref& MakeUnique( units Characters = units{ cache_size_v-1 } ) noexcept
        {
            xassert(Characters.m_Value>0);

            // Size in number of buffer size "a" == Length/Characters == 1, == 'a/0' == Size == 2
            const int Size = Characters.m_Value + 1;

            // If we currently have the right type
            if( index_cache_v == m_Ref.index() || index_unique_v == m_Ref.index() )
            {
                // Also the minimum require sized then we are done.
                if( Size < size() ) return *this;
            }

            // If we can use the cache choose that first
            if( Size <= cache_size_v )
            {
                m_Ref.emplace<cache_t>();
            }
            else
            {
                // Other wise we must allocate
                m_Ref.emplace<unique_t>( new char_t[Size], Size );
            }
            (*this)[0]=0;
            return *this;
        }

        ref& MakeFixed( units Characters ) noexcept
        {
            xassert(Characters.m_Value>0);

            // Size in number of buffer size "a" == Length/Characters == 1, == 'a/0' == Size == 2
            const int Size = Characters.m_Value + 1;

            auto& Unique = m_Ref.emplace<fixed_t>( new char_t[Size], Size );
            (*this)[0]=0;
            return *this;
        }

        ref& MakeShare( units Characters ) noexcept
        {
            xassert(Characters.m_Value>0);

            // Size in number of buffer size "a" == Length/Characters == 1, == 'a/0' == Size == 2
            const details::size_t Size = Characters.m_Value + 1;

            m_Ref.emplace<share_t>(new xcore::string::ref<char_t>{ units{Size} });
            (*this)[0]=0;
            return *this;
        }

        bool ResetToSize( units Characters ) noexcept
        {
            xassert(Characters.m_Value>0);

            // Size in number of buffer size "a" == Length/Characters == 1, == 'a/0' == Size == 2
            const int Size = Characters.m_Value + 1;

            // If we are a constant string we can decay into a Unique string
            if( m_Ref.index() == index_const_v )
            {
                MakeUnique(Characters);
                return true;
            }
            else if( m_Ref.index() == index_share_v )
            {
                // Multiple people looking at this pointer. We can not longer change it
                // at that point we need to decay to a unique string.
                if ( std::get<share_t>(m_Ref).use_count() != 1 ) 
                {
                    MakeUnique( Characters );
                    return true;
                }
                else if( size() < Size )
                {
                    MakeShare( Characters );
                    return true;
                }
            }
            else if( m_Ref.index() == index_fixed_v )
            {
                xassert( size() >= Size );
            }
            else if( size() < Size ) 
            {
                MakeUnique( Characters );
                return true;
            }

            return false;
        }

        constexpr bool empty ( void ) const noexcept
        {
            return data()[0] == 0;
        }
        
        constexpr const char_t& operator[] ( units I ) const noexcept
        { 
            xassert( empty() == false );
            xassert( I < size() );
            const char_t* p;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if constexpr( std::is_same_v<T,share_t> ) p = &(*X)[I.m_Value]; 
                else                                      p = &X[I.m_Value];
            },m_Ref );
            return *p;
        }

        char_t& operator[] ( units I ) noexcept
        { 
            xassert( empty() == false );
            xassert( I < size() );
            char_t* p;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                     if constexpr( std::is_same_v<T,constant_t> )  assert(false); 
                else if constexpr( std::is_same_v<T,share_t> )     p = &(*X)[I.m_Value];
                else                                               p = &X[I.m_Value]; 
            }, m_Ref );
            return *p;
        }

        operator view<char_t>() noexcept
        {
            char_t* p;
            units   Size;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if constexpr( std::is_same_v<T,constant_t> )       assert(false);  //p = X.m_pValue;
                else if constexpr( std::is_same_v<T,share_t> )      
                {
                    p               = X->data();
                    Size.m_Value    = X->size();
                }
                else
                {
                    p               = X.get();
                    Size.m_Value    = X.size();
                }
            }, m_Ref );
            return view<char_t>{ p, Size.m_Value };
        }

        operator const view<const char_t>() const noexcept
        {
            xassert( empty() == false );
            const char_t* p;
            units         Size;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if constexpr( std::is_same_v<T,share_t> )      
                {
                    p               = X->data();
                    Size            = X->size();
                }
                else
                {
                    p               = X.get();
                    Size.m_Value    = X.size();
                }
            }, m_Ref );
            return { p, Size.m_Value };
        }
        constexpr   const char_t*   data                                ( void )                    const   noexcept 
        { 
            const char_t* p;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if constexpr( std::is_same_v<T,share_t> )   p = X->data();
                else                                        p = X.get();
            }, m_Ref );
            return p;
        }
        constexpr char_t* data ( void ) noexcept 
        { 
            char_t* p;
            std::visit( [&]( auto& X )
            { 
                using T = std::decay_t<decltype( X )>;
                if      constexpr( std::is_same_v<T,constant_t> ) assert( false );
                else if constexpr( std::is_same_v<T,share_t> )    p = X->data();
                else                                              p = X.get();
            }, m_Ref );
            return p;
        }
                  const char_t*   c_str   ( void )                   const       noexcept { return data(); }
                  char_t*   c_str   ( void )                          noexcept { return data(); }
                  void      assign  ( const char* p )                 noexcept { Copy( *this, p ); }
                  void      assign  ( const self& V )                 noexcept { Copy( *this, V ); }
                  void      copy    ( char_t* p, int L )      const   noexcept { for( int i=0; i<L; ++i ) p[i] = (*this)[i]; }
        constexpr auto      length  ( void )                  const   noexcept { return Length(*this).m_Value; }
        constexpr auto      getView ( void )          const   noexcept { return static_cast<const string::view<const char>>(*this); }
        constexpr auto      getView ( void )                  noexcept { return static_cast<string::view<char>>(*this); }

        constexpr                   operator const char_t*              ( void )                    const   noexcept { return data(); }
                                    operator       char_t*              ( void )                            noexcept { return data(); }

    };

    // Confirm we are getting the size that we expect
    static_assert( sizeof(ref<char>)  == 64 );
    static_assert( sizeof(ref<short>) == 64 );


    template< typename T_CHAR, typename... T_ARGS >
    xforceinline    ref<T_CHAR> Fmt( const T_CHAR* pFmt, T_ARGS&&...Args ) noexcept
    {
        ref<T_CHAR> Ref;
        Ref.MakeFixed( ref<T_CHAR>::units( 256 ) );
        sprintf( Ref.getView(), pFmt, std::forward<T_ARGS>(Args)... );
        return std::move(Ref);
    }

    //------------------------------------------------------------------------------
    // Simple == functions
    //------------------------------------------------------------------------------
    template< typename T, int N2 >          constexpr bool operator == ( const ref<T>& Str1,        const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N >           constexpr bool operator == ( const ref<T>& Str1,        const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const ref<T>& Str1,        const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const ref<T>& Str1,        const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const ref<T>& Str1,        const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const ref<T>& Str1,        nullptr_t )                 noexcept { return Str1.empty(); }

    template< typename T, int N1, int N2 >  constexpr bool operator == ( const view<T,N1> Str1,     const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1, int N2 >  constexpr bool operator == ( const view<T,N1> Str1,     const fixed<T,N2>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const view<T,N1> Str1,     const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const view<T,N1> Str1,     const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const view<T,N1> Str1,     const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const view<T,N1> Str1,     nullptr_t )                 noexcept { return Str1.empty(); }

    template< typename T, int N2 >          constexpr bool operator == ( const constant<T>& Str1,   const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N >           constexpr bool operator == ( const constant<T>& Str1,   const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const constant<T>& Str1,   const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const constant<T>& Str1,   const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const constant<T>& Str1,   const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const constant<T>& Str1,   nullptr_t )                 noexcept { return Str1.empty(); }

    template< typename T, int N1, int N2 >  constexpr bool operator == ( const fixed<T,N1>& Str1,   const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1, int N2 >  constexpr bool operator == ( const fixed<T,N1>& Str1,   const fixed<T,N2>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const fixed<T,N1>& Str1,   const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const fixed<T,N1>& Str1,   const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const fixed<T,N1>& Str1,   const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N1 >          constexpr bool operator == ( const fixed<T,N1>& Str1,   nullptr_t )                 noexcept { return Str1.empty(); }

    template< typename T, int N2 >          constexpr bool operator == ( const T* Str1,             const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T, int N >           constexpr bool operator == ( const T* Str1,             const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const T* Str1,             const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) == 0; }
    template< typename T >                  constexpr bool operator == ( const T* Str1,             const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) == 0; }

    //------------------------------------------------------------------------------
    // Simple != functions
    //------------------------------------------------------------------------------
    template< typename T, int N2 >          constexpr bool operator != ( const ref<T>& Str1,        const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N >           constexpr bool operator != ( const ref<T>& Str1,        const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const ref<T>& Str1,        const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const ref<T>& Str1,        const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const ref<T>& Str1,        const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const ref<T>& Str1,        nullptr_t )                 noexcept { return !Str1.empty(); }
                                                                                                                                                                            
    template< typename T, int N1, int N2 >  constexpr bool operator != ( const view<T,N1> Str1,     const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1, int N2 >  constexpr bool operator != ( const view<T,N1> Str1,     const fixed<T,N2>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const view<T,N1> Str1,     const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const view<T,N1> Str1,     const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const view<T,N1> Str1,     const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const view<T,N1> Str1,     nullptr_t )                 noexcept { return !Str1.empty(); }
                                                                                                                                                                            
    template< typename T, int N2 >          constexpr bool operator != ( const constant<T>& Str1,   const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N >           constexpr bool operator != ( const constant<T>& Str1,   const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const constant<T>& Str1,   const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const constant<T>& Str1,   const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const constant<T>& Str1,   const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const constant<T>& Str1,   nullptr_t )                 noexcept { return !Str1.empty(); }
                                                                                                                                                                            
    template< typename T, int N1, int N2 >  constexpr bool operator != ( const fixed<T,N1>& Str1,   const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1, int N2 >  constexpr bool operator != ( const fixed<T,N1>& Str1,   const fixed<T,N2>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const fixed<T,N1>& Str1,   const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const fixed<T,N1>& Str1,   const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const fixed<T,N1>& Str1,   const T* Str2 )             noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N1 >          constexpr bool operator != ( const fixed<T,N1>& Str1,   nullptr_t )                 noexcept { return !Str1.empty(); }
                                                                                                                                                                            
    template< typename T, int N2 >          constexpr bool operator != ( const T* Str1,             const view<T,N2> Str2 )     noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T, int N >           constexpr bool operator != ( const T* Str1,             const fixed<T,N>& Str2 )    noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const T* Str1,             const constant<T>& Str2 )   noexcept { return std::strcmp( Str1, Str2 ) != 0; }
    template< typename T >                  constexpr bool operator != ( const T* Str1,             const ref<T>& Str2 )        noexcept { return std::strcmp( Str1, Str2 ) != 0; }

    //------------------------------------------------------------------------------
    // is_obj_v - Helper to determine if it is a string object or a built in string
    //------------------------------------------------------------------------------
    template< typename T >
    constexpr static bool is_obj_v = std::is_object_v<T> && !std::is_array_v<T> && !std::is_pointer_v<T>;

    //------------------------------------------------------------------------------------
    // is_ref_v - Checks if a type is a kind of string::ref 
    //------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T, bool B >
        struct is_ref       : std::false_type  {};

        template< typename T >
        struct is_ref<T,true> { constexpr static bool value = std::is_same_v<T, ref<T::char_t>>; };
    }
    template< typename T >
    constexpr static bool is_ref_v = details::is_ref<T, is_obj_v<T> >::value;

    //------------------------------------------------------------------------------------
    // is_view_v - is a particular type a view
    //------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T > struct is_view : std::false_type  {};
        template< typename T, std::size_t V >
        struct is_view< string::view<T,V> > : std::true_type  {};
    }
    template< typename T >
    constexpr static bool is_view_v = details::is_view<std::decay_t<T>>::value;
}



#endif
