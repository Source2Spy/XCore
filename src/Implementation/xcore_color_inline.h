namespace xcore::color
{
    //------------------------------------------------------------------------------
    // Description:
    //      This constructor builds by passing a single U32 which represents a 
    //      packed version of the color structure.
    // Arguments:
    //      K       - This is the color packed into a 32 bit variable. Note 
    //                 that this may have issues in different endian machines. The Rule is that
    //                 the color is packed little endian.
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T>::unit( const std::uint32_t K ) noexcept
    {
        if constexpr( std::is_integral_v<T> )
        {
            parent_t::m_Value = xcore::endian::SystemToBig(K);
        }
        else
        {
            const u32_elements E{ xcore::endian::SystemToBig(K) };
            parent_t::m_R = E.m_R * (1.0f / 0xff);
            parent_t::m_G = E.m_G * (1.0f / 0xff);
            parent_t::m_B = E.m_B * (1.0f / 0xff);
            parent_t::m_A = E.m_A * (1.0f / 0xff);
        }
    }

    namespace details
    {
        //==============================================================================
        // MACROS
        //==============================================================================
        #define COMP_BIT(A,B) std::uint32_t(((1<<(A))-1)<<(B)) 
        #define COMP_SFT(A,B) (((A)-8)+(B))

        #define DESC_ARGB(A,AL,R,RL,G,GL,B,BL) (A+R+G+B),                                                      \
                                                       COMP_SFT(A,AL), COMP_SFT(R,RL), COMP_SFT(G,GL), COMP_SFT(B,BL), \
                                                       COMP_BIT(A,AL), COMP_BIT(R,RL), COMP_BIT(G,GL), COMP_BIT(B,BL), 


        #define BUILD_RGBA(R,G,B,A) DESC_ARGB( A,R+G+B,   R, 0,      G,B,     B,G+B )
        #define BUILD_RGBU(R,G,B,U) DESC_ARGB( 0,R+G+B,   R, 0,      G,B,     B,G+B )

        #define BUILD_ABGR(A,B,G,R) DESC_ARGB( A,0,       R,G+B+A,   G,B+A,   B,A )
        #define BUILD_UBGR(U,B,G,R) DESC_ARGB( 0,0,       R,G+B+U,   G,B+U,   B,U )

        #define BUILD_URGB(U,R,G,B) DESC_ARGB( 0,0,       R,U,       G,R+U,   B,G+R+U )
        #define BUILD_ARGB(A,R,G,B) DESC_ARGB( A,0,       R,A,       G,R+A,   B,G+R+A )

        #define BUILD_BGRA(B,G,R,A) DESC_ARGB( A,R+G+B,   R,G+B,     G,B,     B,0 )
        #define BUILD_BGRU(B,G,R,U) DESC_ARGB( 0,R+G+B,   R,G+B,     G,B,     B,0 )

        //==============================================================================
        // VARIABLES
        //==============================================================================
        static constexpr auto g_FormatDesc = []()->auto
        {
            std::array<format::descriptor, format::count_v> FormatDesc = {};

            FormatDesc[(int)format::type::UINT_16_ABGR_4444] = format::descriptor{ format::type::UINT_16_ABGR_4444, std::uint32_t(1u << (int)format::type::UINT_16_ABGR_4444), 16, BUILD_ABGR(4,4,4,4) };
            FormatDesc[(int)format::type::UINT_16_ARGB_4444] = format::descriptor{ format::type::UINT_16_ARGB_4444, std::uint32_t(1u << (int)format::type::UINT_16_ARGB_4444), 16, BUILD_ARGB(4,4,4,4) };
            FormatDesc[(int)format::type::UINT_16_RGBA_4444] = format::descriptor{ format::type::UINT_16_RGBA_4444, std::uint32_t(1u << (int)format::type::UINT_16_RGBA_4444), 16, BUILD_RGBA(4,4,4,4) };
            FormatDesc[(int)format::type::UINT_16_RGB_565  ] = format::descriptor{ format::type::UINT_16_RGB_565  , std::uint32_t(1u << (int)format::type::UINT_16_RGB_565)  , 16, BUILD_RGBU(5,6,5,0) };
            FormatDesc[(int)format::type::UINT_16_BGR_565  ] = format::descriptor{ format::type::UINT_16_BGR_565  , std::uint32_t(1u << (int)format::type::UINT_16_BGR_565)  , 16, BUILD_BGRU(5,6,5,0) };
            FormatDesc[(int)format::type::UINT_16_ARGB_1555] = format::descriptor{ format::type::UINT_16_ARGB_1555, std::uint32_t(1u << (int)format::type::UINT_16_ARGB_1555), 16, BUILD_ARGB(1,5,5,5) };
            FormatDesc[(int)format::type::UINT_16_RGBA_5551] = format::descriptor{ format::type::UINT_16_RGBA_5551, std::uint32_t(1u << (int)format::type::UINT_16_RGBA_5551), 16, BUILD_RGBA(5,5,5,1) };
            FormatDesc[(int)format::type::UINT_16_URGB_1555] = format::descriptor{ format::type::UINT_16_URGB_1555, std::uint32_t(1u << (int)format::type::UINT_16_URGB_1555), 16, BUILD_URGB(1,5,5,5) };
            FormatDesc[(int)format::type::UINT_16_RGBU_5551] = format::descriptor{ format::type::UINT_16_RGBU_5551, std::uint32_t(1u << (int)format::type::UINT_16_RGBU_5551), 16, BUILD_RGBU(5,5,5,1) };
            FormatDesc[(int)format::type::UINT_16_ABGR_1555] = format::descriptor{ format::type::UINT_16_ABGR_1555, std::uint32_t(1u << (int)format::type::UINT_16_ABGR_1555), 16, BUILD_ABGR(1,5,5,5) };
            FormatDesc[(int)format::type::UINT_16_UBGR_1555] = format::descriptor{ format::type::UINT_16_UBGR_1555, std::uint32_t(1u << (int)format::type::UINT_16_UBGR_1555), 16, BUILD_UBGR(1,5,5,5) };
            FormatDesc[(int)format::type::UINT_16_BGRA_5551] = format::descriptor{ format::type::UINT_16_BGRA_5551, std::uint32_t(1u << (int)format::type::UINT_16_BGRA_5551), 16, BUILD_BGRA(5,5,5,1) };
            FormatDesc[(int)format::type::UINT_24_RGB_888  ] = format::descriptor{ format::type::UINT_24_RGB_888  , std::uint32_t(1u << (int)format::type::UINT_24_RGB_888)  , 24, BUILD_RGBU(8,8,8,0) };
            FormatDesc[(int)format::type::UINT_24_ARGB_8565] = format::descriptor{ format::type::UINT_24_ARGB_8565, std::uint32_t(1u << (int)format::type::UINT_24_ARGB_8565), 24, BUILD_ARGB(8,5,6,5) };
            FormatDesc[(int)format::type::UINT_32_RGBU_8888] = format::descriptor{ format::type::UINT_32_RGBU_8888, std::uint32_t(1u << (int)format::type::UINT_32_RGBU_8888), 32, BUILD_RGBU(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_URGB_8888] = format::descriptor{ format::type::UINT_32_URGB_8888, std::uint32_t(1u << (int)format::type::UINT_32_URGB_8888), 32, BUILD_URGB(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_ARGB_8888] = format::descriptor{ format::type::UINT_32_ARGB_8888, std::uint32_t(1u << (int)format::type::UINT_32_ARGB_8888), 32, BUILD_ARGB(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_RGBA_8888] = format::descriptor{ format::type::UINT_32_RGBA_8888, std::uint32_t(1u << (int)format::type::UINT_32_RGBA_8888), 32, BUILD_RGBA(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_ABGR_8888] = format::descriptor{ format::type::UINT_32_ABGR_8888, std::uint32_t(1u << (int)format::type::UINT_32_ABGR_8888), 32, BUILD_ABGR(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_BGRA_8888] = format::descriptor{ format::type::UINT_32_BGRA_8888, std::uint32_t(1u << (int)format::type::UINT_32_BGRA_8888), 32, BUILD_BGRA(8,8,8,8) };
            FormatDesc[(int)format::type::UINT_32_BGRU_8888] = format::descriptor{ format::type::UINT_32_BGRU_8888, std::uint32_t(1u << (int)format::type::UINT_32_BGRU_8888), 32, BUILD_BGRU(8,8,8,0) };

            return FormatDesc;
        }();
        #undef COMP_BIT
        #undef COMP_SFT
        #undef DESC_ARGB
        #undef BUILD_RGBA
        #undef BUILD_RGBU
        #undef BUILD_ABGR
        #undef BUILD_UBGR
        #undef BUILD_URGB
        #undef BUILD_ARGB
        #undef BUILD_BGRA
        #undef BUILD_BGRU

        struct best_match
        {
            std::array<format::type, format::count_v> m_Format;
        };

        static constexpr auto g_Match = []()->auto
        {
            std::array<best_match, format::count_v> Match = {};

            Match[(int)format::type::UINT_16_ARGB_4444] = best_match{ { format::type::UINT_16_ARGB_4444, format::type::UINT_16_RGBA_4444, format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,                                   format::type::INVALID } };
            Match[(int)format::type::UINT_16_RGBA_4444] = best_match{ { format::type::UINT_16_RGBA_4444, format::type::UINT_16_ARGB_4444, format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,                                   format::type::INVALID } };
            Match[(int)format::type::UINT_16_RGB_565  ] = best_match{ { format::type::UINT_16_RGB_565  , format::type::UINT_16_URGB_1555, format::type::UINT_16_RGBU_5551, format::type::UINT_16_UBGR_1555, format::type::UINT_24_RGB_888,   format::type::UINT_32_RGBU_8888, format::type::UINT_32_URGB_8888,                                   format::type::INVALID } };
            Match[(int)format::type::UINT_16_ARGB_1555] = best_match{ { format::type::UINT_16_ARGB_1555, format::type::UINT_16_RGBA_5551, format::type::UINT_16_ABGR_1555, format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,  format::type::INVALID } };
            Match[(int)format::type::UINT_16_RGBA_5551] = best_match{ { format::type::UINT_16_RGBA_5551, format::type::UINT_16_ARGB_1555, format::type::UINT_16_ABGR_1555, format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,  format::type::INVALID } };
            Match[(int)format::type::UINT_16_URGB_1555] = best_match{ { format::type::UINT_16_URGB_1555, format::type::UINT_16_RGBU_5551, format::type::UINT_16_UBGR_1555, format::type::UINT_16_RGB_565,   format::type::UINT_24_RGB_888,   format::type::UINT_32_RGBU_8888, format::type::UINT_32_URGB_8888,                                   format::type::INVALID } };
            Match[(int)format::type::UINT_16_RGBU_5551] = best_match{ { format::type::UINT_16_RGBU_5551, format::type::UINT_16_URGB_1555, format::type::UINT_16_UBGR_1555, format::type::UINT_16_RGB_565,   format::type::UINT_24_RGB_888,   format::type::UINT_32_RGBU_8888, format::type::UINT_32_URGB_8888,                                   format::type::INVALID } };
            Match[(int)format::type::UINT_16_ABGR_1555] = best_match{ { format::type::UINT_16_ABGR_1555, format::type::UINT_16_RGBA_5551, format::type::UINT_16_ARGB_1555, format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,  format::type::INVALID } };
            Match[(int)format::type::UINT_24_RGB_888  ] = best_match{ { format::type::UINT_24_RGB_888  , format::type::UINT_32_RGBU_8888, format::type::UINT_32_URGB_8888,                                                                                                                                                                       format::type::INVALID } };
            Match[(int)format::type::UINT_24_ARGB_8565] = best_match{ { format::type::UINT_24_ARGB_8565, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,                                                                                                     format::type::INVALID } };
            Match[(int)format::type::UINT_32_RGBU_8888] = best_match{ { format::type::UINT_32_RGBU_8888, format::type::UINT_32_URGB_8888, format::type::UINT_24_RGB_888  ,                                                                                                                                                                       format::type::INVALID } };
            Match[(int)format::type::UINT_32_URGB_8888] = best_match{ { format::type::UINT_32_URGB_8888, format::type::UINT_32_RGBU_8888, format::type::UINT_24_RGB_888  ,                                                                                                                                                                       format::type::INVALID } };
            Match[(int)format::type::UINT_32_ARGB_8888] = best_match{ { format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,                                                                                                                                      format::type::INVALID } };
            Match[(int)format::type::UINT_32_RGBA_8888] = best_match{ { format::type::UINT_32_RGBA_8888, format::type::UINT_32_ARGB_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888,                                                                                                                                      format::type::INVALID } };
            Match[(int)format::type::UINT_32_ABGR_8888] = best_match{ { format::type::UINT_32_ABGR_8888, format::type::UINT_32_BGRA_8888, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888,                                                                                                                                      format::type::INVALID } };
            Match[(int)format::type::UINT_32_BGRA_8888] = best_match{ { format::type::UINT_32_BGRA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888,                                                                                                                                      format::type::INVALID } };
            Match[(int)format::type::UINT_32_BGRU_8888] = best_match{ { format::type::UINT_32_BGRU_8888, format::type::UINT_32_RGBU_8888, format::type::UINT_32_BGRA_8888, format::type::UINT_32_ABGR_8888, format::type::UINT_32_ARGB_8888, format::type::UINT_32_RGBA_8888,                                                                    format::type::INVALID } };
            
            return Match;
        }();

        static constexpr auto g_ColorCategories = std::array
        {   unit<std::uint32_t>{ 0x1f77b4ffu }
        ,   unit<std::uint32_t>{ 0xaec7e8ffu }
        ,   unit<std::uint32_t>{ 0xff7f0effu }
        ,   unit<std::uint32_t>{ 0xffbb78ffu }
        ,   unit<std::uint32_t>{ 0x2ca02cffu }
        ,   unit<std::uint32_t>{ 0x98df8affu }
        ,   unit<std::uint32_t>{ 0xd62728ffu }
        ,   unit<std::uint32_t>{ 0xff9896ffu }
        ,   unit<std::uint32_t>{ 0x9467bdffu }
        ,   unit<std::uint32_t>{ 0xc5b0d5ffu }
        ,   unit<std::uint32_t>{ 0x8c564bffu }
        ,   unit<std::uint32_t>{ 0xc49c94ffu }
        ,   unit<std::uint32_t>{ 0xe377c2ffu }
        ,   unit<std::uint32_t>{ 0xf7b6d2ffu }
        ,   unit<std::uint32_t>{ 0x7f7f7fffu }
        ,   unit<std::uint32_t>{ 0xc7c7c7ffu }
        ,   unit<std::uint32_t>{ 0xbcbd22ffu }
        ,   unit<std::uint32_t>{ 0xdbdb8dffu }
        ,   unit<std::uint32_t>{ 0x17becfffu }
        ,   unit<std::uint32_t>{ 0x9edae5ffu }
        };
    }

    //------------------------------------------------------------------------------

    constexpr 
    unit<std::uint32_t> getColorCategory(int Index) noexcept
    {
        return details::g_ColorCategories[Index];
    }

    //------------------------------------------------------------------------------

    const format::descriptor& format::getDescriptor( void ) const noexcept
    {
        return details::g_FormatDesc[(int)m_Value];
    }

    //------------------------------------------------------------------------------

    constexpr
    format format::FindClosestFormat( std::uint32_t FormatMask, format Match ) noexcept
    {
        for (const auto* pMatch = &details::g_Match[(int)Match.m_Value].m_Format[0]; *pMatch != format::type::INVALID; pMatch++ )
        {
            if (FormatMask & (1 << int(*pMatch))) return {*pMatch};
        }

        return {format::type::INVALID};
    }

    //------------------------------------------------------------------------------
    constexpr
    format format::FindFormat(std::uint32_t AMask, std::uint32_t RMask, std::uint32_t GMask, std::uint32_t BMask) noexcept
    {
        for (int i = (int)format::type::INVALID + 1; i < (int)format::type::ENUM_COUNT; i++)
        {
            if (AMask != details::g_FormatDesc[i].m_AMask) continue;
            if (RMask != details::g_FormatDesc[i].m_RMask) continue;
            if (GMask != details::g_FormatDesc[i].m_GMask) continue;
            if (BMask != details::g_FormatDesc[i].m_BMask) continue;
                         
            return {details::g_FormatDesc[i].m_Format};
        }

        return { format::type::INVALID };
    }

    //------------------------------------------------------------------------------
    // Description:
    //      This constuctor builds the color by passing each component
    // Arguments:
    //      R   - is the Red component of the color.
    //      G   - is the Green component of the color.
    //      B   - is the Blue component of the color.
    //      A   - is the Alpha component of the color.
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T>::unit( element R, element G, element B, element A ) noexcept
    {
        parent_t::m_R = R;
        parent_t::m_G = G;
        parent_t::m_B = B;
        parent_t::m_A = A;
    }

    //------------------------------------------------------------------------------

    template< typename T > 
    template< typename J >constexpr
    unit<T>::unit(const unit<J>& C) noexcept
    {
        if constexpr( std::is_same_v<T,J> )
        {
            *this = C;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<std::uint8_t>(C.m_X * 0xff);
            parent_t::m_G = static_cast<std::uint8_t>(C.m_Y * 0xff);
            parent_t::m_B = static_cast<std::uint8_t>(C.m_Z * 0xff);
            parent_t::m_A = static_cast<std::uint8_t>(C.m_A * 0xff);
        }
        else
        {
            parent_t::m_R = static_cast<element>(C.m_X * 1.0f/0xff);
            parent_t::m_G = static_cast<element>(C.m_Y * 1.0f/0xff);
            parent_t::m_B = static_cast<element>(C.m_Z * 1.0f/0xff);
            parent_t::m_A = static_cast<element>(C.m_A * 1.0f/0xff);
        }
    }

    //------------------------------------------------------------------------------
    // Description:
    //      This constuctor builds the color by passing a vector3 which represents the
    //      floating point values of RGB.
    // Arguments:
    //      C   - C represents a vector which contains values ranges from 0 to 1, 
    //             C.X: is the Red component of the color.
    //             C.Y: is the Green component of the color.
    //             C.Z: is the Blue component of the color.
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T>::unit(const vector3d& C) noexcept
    {
        if constexpr ( std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<std::uint8_t>(C.m_X * 0xff);
            parent_t::m_G = static_cast<std::uint8_t>(C.m_Y * 0xff);
            parent_t::m_B = static_cast<std::uint8_t>(C.m_Z * 0xff);
            parent_t::m_A = 0xff;
        }
        else
        {
            parent_t::m_R = C.m_X;
            parent_t::m_G = C.m_Y;
            parent_t::m_B = C.m_Z;
            parent_t::m_A = 1.0f;
        }
    }

    //------------------------------------------------------------------------------
    // Description:
    //      This constuctor builds the color by passing a vector4 which represents the
    //      floating point values of RGBA.
    // <param name="C"> 
    //      C   - C represents a vector which contains values ranges from 0 to 1, 
    //             C.X: is the Red component of the color.
    //             C.Y: is the Green component of the color.
    //             C.Z: is the Blue component of the color.
    //             C.W: is the Alpha component of the color.
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T>::unit(const vector4& C) noexcept
    {
        if constexpr ( std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<std::uint8_t>(C.m_X * 0xff);
            parent_t::m_G = static_cast<std::uint8_t>(C.m_Y * 0xff);
            parent_t::m_B = static_cast<std::uint8_t>(C.m_Z * 0xff);
            parent_t::m_A = static_cast<std::uint8_t>(C.m_W * 0xff);
        }
        else
        {
            parent_t::m_R = C.m_X;
            parent_t::m_G = C.m_Y;
            parent_t::m_B = C.m_Z;
            parent_t::m_A = C.m_W;
        }
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    unit<T>::unit( std::uint32_t RawData, format DataFormat ) noexcept
    {
        const auto& Fmt = details::g_FormatDesc[(int)DataFormat.m_Value];
        assert( Fmt.m_Format == DataFormat.m_Value );

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<std::uint8_t>((Fmt.m_RShift < 0) ? ((RawData & Fmt.m_RMask) << (-Fmt.m_RShift)) : ((RawData & Fmt.m_RMask) >> (Fmt.m_RShift)));
            parent_t::m_G = static_cast<std::uint8_t>((Fmt.m_GShift < 0) ? ((RawData & Fmt.m_GMask) << (-Fmt.m_GShift)) : ((RawData & Fmt.m_GMask) >> (Fmt.m_GShift)));
            parent_t::m_B = static_cast<std::uint8_t>((Fmt.m_BShift < 0) ? ((RawData & Fmt.m_BMask) << (-Fmt.m_BShift)) : ((RawData & Fmt.m_BMask) >> (Fmt.m_BShift)));
        
            // force m_A to 255 if the src format doesn't have alpha
            if( Fmt.m_AMask == 0 ) parent_t::m_A = 255;
            else                   parent_t::m_A = static_cast<std::uint8_t>((Fmt.m_AShift < 0) ? ((RawData & Fmt.m_AMask) << (-Fmt.m_AShift)) : ((RawData & Fmt.m_AMask) >> (Fmt.m_AShift)));
        }
        else
        {
            *this = unit<T>( unit<std::uint32_t>(RawData, DataFormat) );
        }
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    bool unit<T>::operator == ( const unit<T>& C ) const noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            return parent_t::m_Value == C.m_Value;
        }
        else
        {
            if (std::fabs(parent_t::m_R - C.m_R) >= std::numeric_limits<T>::epsilon() ) return false;
            if (std::fabs(parent_t::m_G - C.m_G) >= std::numeric_limits<T>::epsilon()) return false;
            if (std::fabs(parent_t::m_B - C.m_B) >= std::numeric_limits<T>::epsilon()) return false;
            if (std::fabs(parent_t::m_A - C.m_A) >= std::numeric_limits<T>::epsilon()) return false;
            return true;
        }
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    bool unit<T>::operator != (const unit<T>& C) const noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            return parent_t::m_Value != C.m_Value;
        }
        else
        {
            if (std::fabs(parent_t::m_R - C.m_R) >= std::numeric_limits<T>::epsilon()) return true;
            if (std::fabs(parent_t::m_G - C.m_G) >= std::numeric_limits<T>::epsilon()) return true;
            if (std::fabs(parent_t::m_B - C.m_B) >= std::numeric_limits<T>::epsilon()) return true;
            if (std::fabs(parent_t::m_A - C.m_A) >= std::numeric_limits<T>::epsilon()) return true;
            return false;
        }
    }

    //------------------------------------------------------------------------------
    template< typename T >
    typename unit<T>::element& unit<T>::operator[]( int Index ) noexcept
    {
        assert( Index >= 0 );
        assert( Index <= 3 );
        return ((unit<T>::element*)this)[Index];
    }

    //------------------------------------------------------------------------------
    template< typename T >
    typename unit<T>::element unit<T>::operator[](int Index) const noexcept
    {
        assert(Index >= 0);
        assert(Index <= 3);
        return ((unit<T>::element*)this)[Index];
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T>::operator std::uint32_t( void ) const noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            return xcore::endian::BigToSystem(parent_t::m_Value);
        }
        else
        {
            return static_cast<std::uint32_t>(unit<std::uint32_t>(*this));
        }
    }

    //------------------------------------------------------------------------------
    template< typename T >
    const unit<T>& unit<T>::operator += ( const unit<T>& C ) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_A = static_cast<element>(std::min( 0xffu, std::uint32_t(parent_t::m_A) + C.m_A ));
            parent_t::m_R = static_cast<element>(std::min( 0xffu, std::uint32_t(parent_t::m_R) + C.m_R ));
            parent_t::m_G = static_cast<element>(std::min( 0xffu, std::uint32_t(parent_t::m_G) + C.m_G ));
            parent_t::m_B = static_cast<element>(std::min( 0xffu, std::uint32_t(parent_t::m_B) + C.m_B ));
        }
        else
        {
            parent_t::m_A += C.m_A;
            parent_t::m_R += C.m_R;
            parent_t::m_G += C.m_G;
            parent_t::m_B += C.m_B;    
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    template< typename T >
    const unit<T>& unit<T>::operator -= (const unit<T>& C) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_A = static_cast<element>(std::max(0, std::int32_t(parent_t::m_A) - C.m_A));
            parent_t::m_R = static_cast<element>(std::max(0, std::int32_t(parent_t::m_R) - C.m_R));
            parent_t::m_G = static_cast<element>(std::max(0, std::int32_t(parent_t::m_G) - C.m_G));
            parent_t::m_B = static_cast<element>(std::max(0, std::int32_t(parent_t::m_B) - C.m_B));
        }
        else
        {
            parent_t::m_A -= C.m_A;
            parent_t::m_R -= C.m_R;
            parent_t::m_G -= C.m_G;
            parent_t::m_B -= C.m_B;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    template< typename T >
    const unit<T>& unit<T>::operator *= (const unit<T>& C) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_A = static_cast<element>(std::min(0xffu, std::max(0u, std::uint32_t(parent_t::m_A) * C.m_A)));
            parent_t::m_R = static_cast<element>(std::min(0xffu, std::max(0u, std::uint32_t(parent_t::m_R) * C.m_R)));
            parent_t::m_G = static_cast<element>(std::min(0xffu, std::max(0u, std::uint32_t(parent_t::m_G) * C.m_G)));
            parent_t::m_B = static_cast<element>(std::min(0xffu, std::max(0u, std::uint32_t(parent_t::m_B) * C.m_B)));
        }
        else
        {
            parent_t::m_A *= C.m_A;
            parent_t::m_R *= C.m_R;
            parent_t::m_G *= C.m_G;
            parent_t::m_B *= C.m_B;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    // "The YIQ system is the colour primary system adopted by NTSC for colour
    // television  broadcasting. The YIQ color solid is formed by a linear
    // transformation of the RGB cude. Its purpose is to exploit certain
    // characteristics of the human visual system to maximize the use of a fixed
    // bandwidth" (Funds... op cit).
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getYIQ( float& Y, float& I, float& Q ) const noexcept
    {
        float r, g, b;

        if constexpr (std::is_integral_v<T>)
        {
            r = parent_t::m_R * (1.0f / 0xff);
            g = parent_t::m_G * (1.0f / 0xff);
            b = parent_t::m_B * (1.0f / 0xff);
        }
        else
        {
            r = parent_t::m_R;
            g = parent_t::m_G;
            b = parent_t::m_B;
        }

        Y = r * 0.299f + g * 0.587f + b * 0.114f;
        I = r * 0.596f - g * 0.274f - b * 0.322f;
        Q = r * 0.212f - g * 0.523f + b * 0.311f;
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromYIQ( float Y, float I, float Q ) noexcept
    {
        const float r = Y * 1 + I * 0.956f + Q * 0.621f;
        const float g = Y * 1 - I * 0.272f - Q * 0.647f;
        const float b = Y * 1 - I * 1.105f + Q * 1.702f;

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max( 0.0f, std::min(255.0f, r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max( 0.0f, std::min(255.0f, g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max( 0.0f, std::min(255.0f, b * 0xff)));
            parent_t::m_A = 0xffu;

        }
        else
        {
            parent_t::m_R = r;
            parent_t::m_G = g;
            parent_t::m_B = b;
            parent_t::m_A = 1.0f;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    // YUV is like YIQ, except that it is the PAL/European standard. It's only trivial
    // to many poeple in Northen America (i.e. not only the STATES), but since the
    // USENET messages are read all over the world...
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getYUV( float& Y, float& U, float& V) const noexcept
    {
        float r, g, b;

        if constexpr (std::is_integral_v<T>)
        {
            r = parent_t::m_R * (1.0f / 0xff);
            g = parent_t::m_G * (1.0f / 0xff);
            b = parent_t::m_B * (1.0f / 0xff);
        }
        else
        {
            r = parent_t::m_R;
            g = parent_t::m_G;
            b = parent_t::m_B;
        }

        Y =  r * 0.299f + g * 0.587f + b * 0.114f;
        U = -r * 0.147f - g * 0.289f + b * 0.437f;
        V =  r * 0.615f - g * 0.515f - b * 0.100f;
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromYUV(float Y, float U, float V) noexcept
    {
        const float r = Y * 1 + U * 0.000f + V * 1.140f;
        const float g = Y * 1 - U * 0.394f - V * 0.581f;
        const float b = Y * 1 + U * 2.028f + V * 0.000f;

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max(0.0f, std::min(255.0f, r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max(0.0f, std::min(255.0f, g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max(0.0f, std::min(255.0f, b * 0xff)));
            parent_t::m_A = 0xffu;

        }
        else
        {
            parent_t::m_R = r;
            parent_t::m_G = g;
            parent_t::m_B = b;
            parent_t::m_A = 1.0f;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getCIE(float& C, float& I, float& E) const noexcept
    {
        float r, g, b;

        if constexpr (std::is_integral_v<T>)
        {
            r = parent_t::m_R * (1.0f / 0xff);
            g = parent_t::m_G * (1.0f / 0xff);
            b = parent_t::m_B * (1.0f / 0xff);
        }
        else
        {
            r = parent_t::m_R;
            g = parent_t::m_G;
            b = parent_t::m_B;
        }

        C = r * 0.6067f + g * 0.1736f + b * 0.2001f;
        I = r * 0.2988f + g * 0.5868f + b * 0.1143f;
        E = r * 0.0000f + g * 0.0661f + b * 1.1149f;
    }

    //------------------------------------------------------------------------------

    template< typename T >
    unit<T>& unit<T>::setupFromCIE(float C, float I, float E) noexcept
    {
        const float r =  C * 1.9107f - I * 0.5326f - E * 0.2883f;
        const float g = -C * 0.9843f + I * 1.9984f - E * 0.0283f;
        const float b =  C * 0.0583f - I * 0.1185f + E * 0.8986f;

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max(0.0f, std::min(255.0f, r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max(0.0f, std::min(255.0f, g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max(0.0f, std::min(255.0f, b * 0xff)));
            parent_t::m_A = 0xffu;
        }
        else
        {
            parent_t::m_R = r;
            parent_t::m_G = g;
            parent_t::m_B = b;
            parent_t::m_A = 1.0f;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    // color to Cyan, Magenta and Yellow
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getCMY( float& C, float& M, float& Y) const noexcept
    {
        float r, g, b;

        if constexpr (std::is_integral_v<T>)
        {
            r = parent_t::m_R * (1.0f / 0xff);
            g = parent_t::m_G * (1.0f / 0xff);
            b = parent_t::m_B * (1.0f / 0xff);
        }
        else
        {
            r = parent_t::m_R;
            g = parent_t::m_G;
            b = parent_t::m_B;
        }

        C = 1 - r;   // ASSUME C
        M = 1 - g;   // ASSUME M
        Y = 1 - b;   // ASSUME Y
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromCMY(float C, float M, float Y) noexcept
    {
        const float r = 1 - C;
        const float g = 1 - M;
        const float b = 1 - Y;

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max(0.0f, std::min(255.0f, r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max(0.0f, std::min(255.0f, g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max(0.0f, std::min(255.0f, b * 0xff)));
            parent_t::m_A = 0xffu;
        }
        else
        {
            parent_t::m_R = r;
            parent_t::m_G = g;
            parent_t::m_B = b;
            parent_t::m_A = 1.0f;
        }

        return *this;
    }


    //------------------------------------------------------------------------------
    // color to a parametric rgb values
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getRGBA( float& R, float& G, float& B, float& A ) const noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            R = parent_t::m_R * (1.0f / 0xff);
            G = parent_t::m_G * (1.0f / 0xff);
            B = parent_t::m_B * (1.0f / 0xff);
            A = parent_t::m_A * (1.0f / 0xff);
        }
        else
        {
            R = parent_t::m_R;
            G = parent_t::m_G;
            B = parent_t::m_B;
            A = parent_t::m_A;
        }
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromRGBA( float r, float g, float b, float a ) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max(0.0f, std::min(255.0f, r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max(0.0f, std::min(255.0f, g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max(0.0f, std::min(255.0f, b * 0xff)));
            parent_t::m_A = static_cast<element>(std::max(0.0f, std::min(255.0f, a * 0xff)));
        }
        else
        {
            parent_t::m_R = r;
            parent_t::m_G = g;
            parent_t::m_B = b;
            parent_t::m_A = a;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    // The Hue/Saturation/Value system (or also called Hexcone model) was created by
    // Smith in 1978. It was for an aesthetic purpose, accessing color by family,
    // purely and intensity rather than by its component. With that model it becomes
    // easy to produce some kind of hellish brown or some kind of
    // you-know-that-color-in-between-(x) and (y).
    // The H value is a 360 degree value through color families.
    // The S (or Saturation) is the degree of strength of a color. Greater is S, the
    // purest is the color. if S max is 100, then Hue=red and S=100 would produce an
    // intense red (reproduced by RGB (max,0,0)) 
    // Finally, the V, for value, is the darkness/lightness of a color. More V is
    // great, more the color is close to white.
    //------------------------------------------------------------------------------
    template< typename T > constexpr
    void unit<T>::getHSV( float& H, float& S, float& V ) const noexcept
    {
        float r, g, b;

        if constexpr (std::is_integral_v<T>)
        {
            r = parent_t::m_R * (1.0f / 0xff);
            g = parent_t::m_G * (1.0f / 0xff);
            b = parent_t::m_B * (1.0f / 0xff);
        }
        else
        {
            r = parent_t::m_R;
            g = parent_t::m_G;
            b = parent_t::m_B;
        }

        float K = 0.f;
        if (g < b)
        {
            const float tmp = g; g = b; b = tmp;
            K = -1.f;
        }
        if (r < g)
        {
            const float tmp = r; r = g; g = tmp;
            K = -2.f / 6.f - K;
        }

        const float chroma = r - (g < b ? g : b);
        H = std::fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
        S = chroma / (r + 1e-20f);
        V = r;
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromHSV( float h, float s, float v ) noexcept
    {
        float out_r, out_g, out_b; 
        if (s == 0.0f)
        {
            // gray
            out_r = out_g = out_b = v;
        }
        else
        {
            h = std::fmodf(h, 1.0f) / (60.0f/360.0f);
            int   i = (int)h;
            float f = h - (float)i;
            float p = v * (1.0f - s);
            float q = v * (1.0f - s * f);
            float t = v * (1.0f - s * (1.0f - f));

            switch (i)
            {
            case 0: out_r = v; out_g = t; out_b = p; break;
            case 1: out_r = q; out_g = v; out_b = p; break;
            case 2: out_r = p; out_g = v; out_b = t; break;
            case 3: out_r = p; out_g = q; out_b = v; break;
            case 4: out_r = t; out_g = p; out_b = v; break;
            case 5: default: out_r = v; out_g = p; out_b = q; break;
            }
        }

        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max(0.0f, std::min(255.0f, out_r * 0xff)));
            parent_t::m_G = static_cast<element>(std::max(0.0f, std::min(255.0f, out_g * 0xff)));
            parent_t::m_B = static_cast<element>(std::max(0.0f, std::min(255.0f, out_b * 0xff)));
            parent_t::m_A = 0xffu;
        }
        else
        {
            parent_t::m_R = out_r;
            parent_t::m_G = out_g;
            parent_t::m_B = out_b;
            parent_t::m_A = 1.0f;
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    vector3 unit<T>::getHSV( void ) const noexcept
    {
        vector3 R;
        getHSV(R.m_X, R.m_Y, R.m_Z);
        return R;
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    void unit<T>::getRGB(float& R, float& G, float& B) const noexcept
    {
        float A;
        getRGBA(R,G,B,A);
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromRGB(float r, float g, float b) noexcept
    {
        return setupFromRGBA(r,g,b,1.0f);
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromRGBA( const vector4& C ) noexcept
    {
        return setupFromRGBA(C.m_X, C.m_Y, C.m_Z, C.m_W );
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    xcore::vector4 unit<T>::getRGBA(void) const noexcept
    {
        float r,g,b,a;
        getRGBA(r,g,b,a);
        return {r,g,b,a};
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    xcore::vector3 unit<T>::getRGB(void) const noexcept
    {
        float r, g, b;
        getRGB(r, g, b);
        return { r,g,b };
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromRGB( const vector3d& Vector ) noexcept
    {
        return setupFromRGB( Vector.m_X, Vector.m_Y, Vector.m_Z );
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromNormal( const vector3d& Normal ) noexcept
    {
        float r,g,b;
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_R = static_cast<element>(std::max( 0.0f, std::min(255.0f, ((Normal.m_X + 1.0f) * 127.0f) + 0.5f)));
            parent_t::m_G = static_cast<element>(std::max( 0.0f, std::min(255.0f, ((Normal.m_Y + 1.0f) * 127.0f) + 0.5f)));
            parent_t::m_B = static_cast<element>(std::max( 0.0f, std::min(255.0f, ((Normal.m_Z + 1.0f) * 127.0f) + 0.5f)));
            parent_t::m_A = 0xff;
        }
        else
        {
            parent_t::m_R = (Normal.m_X + 1)*0.5f;
            parent_t::m_G = (Normal.m_Y + 1)*0.5f;
            parent_t::m_B = (Normal.m_Z + 1)*0.5f;
            parent_t::m_A = 1.0f;

        }

        return *this;
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    vector3 unit<T>::getNormal( void ) const noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            return vector3((((float)parent_t::m_R) - 127.0f) * (1 / 127.0f),
                           (((float)parent_t::m_G) - 127.0f) * (1 / 127.0f),
                           (((float)parent_t::m_B) - 127.0f) * (1 / 127.0f));

        }
        else
        {
            return vector3( ( parent_t::m_R - 0.5f) * 2.0f
                          , ( parent_t::m_G - 0.5f) * 2.0f
                          , ( parent_t::m_B - 0.5f) * 2.0f );
        }
    }

    //------------------------------------------------------------------------------
    template< typename T >
    unit<T>& unit<T>::setupFromLight( const vector3d& LightDir ) noexcept
    {
        return setupFromNormal( -LightDir );
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    vector3 unit<T>::getLight(void) const noexcept
    {
        return -getNormal();
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    std::uint32_t unit<T>::getDataFromColor( format DataFormat ) const noexcept
    {
        std::uint32_t   Data;
        const auto&     Fmt = details::g_FormatDesc[DataFormat.m_Value];
        assert( Fmt.m_Format == DataFormat );

        Data = ~(Fmt.m_AMask | Fmt.m_RMask | Fmt.m_GMask | Fmt.m_BMask);

        std::uint32_t r,g,b,a;

        if constexpr (std::is_integral_v<T>)
        {
            r = static_cast<std::uint32_t>(parent_t::m_R);
            g = static_cast<std::uint32_t>(parent_t::m_G);
            b = static_cast<std::uint32_t>(parent_t::m_B);
            a = static_cast<std::uint32_t>(parent_t::m_A);
        }
        else
        {
            r = static_cast<std::uint32_t>(std::min( 255.0f, std::max(0.0f, parent_t::m_R * 0xff)));
            g = static_cast<std::uint32_t>(std::min( 255.0f, std::max(0.0f, parent_t::m_G * 0xff)));
            b = static_cast<std::uint32_t>(std::min( 255.0f, std::max(0.0f, parent_t::m_B * 0xff)));
            a = static_cast<std::uint32_t>(std::min( 255.0f, std::max(0.0f, parent_t::m_A * 0xff)));
        }

        Data |= (Fmt.m_AShift < 0) ? ((a >> (-Fmt.m_AShift)) & Fmt.m_AMask) : ((a << Fmt.m_AShift) & Fmt.m_AMask);
        Data |= (Fmt.m_RShift < 0) ? ((r >> (-Fmt.m_RShift)) & Fmt.m_RMask) : ((r << Fmt.m_RShift) & Fmt.m_RMask);
        Data |= (Fmt.m_GShift < 0) ? ((g >> (-Fmt.m_GShift)) & Fmt.m_GMask) : ((g << Fmt.m_GShift) & Fmt.m_GMask);
        Data |= (Fmt.m_BShift < 0) ? ((b >> (-Fmt.m_BShift)) & Fmt.m_BMask) : ((b << Fmt.m_BShift) & Fmt.m_BMask);

        return Data;
    }

    //------------------------------------------------------------------------------

    template< typename T > constexpr
    unit<T> unit<T>::getBlendedColors( const unit<T> Src1, const unit<T> Src2, float t ) const noexcept
    {
        const vector4 S1        = Src1.getRGBA();
        const vector4 newColor  = S1 + t * (Src2.getRGBA() - S1);
        return { newColor };
    }

    //------------------------------------------------------------------------------
    template< typename T >
    void unit<T>::setAlpha( float Alpha ) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            parent_t::m_A = std::min( 0xffu, std::max( 0u, static_cast<std::uint32_t>(Alpha * (1.0f/0xff)) ));
        }
        else
        {
            parent_t::m_A = std::min(1.0f, std::max(0,Alpha));
        }
    }

    //------------------------------------------------------------------------------
    template< typename T > constexpr
    unit<T> unit<T>::PremultiplyAlpha(void) const noexcept
    {
        vector4 Color = getRGBA();
        float   Alpha = Color.m_W;

        // Premultiply the color
        Color *= Alpha;
        Color.m_W = Alpha;

        return unit<T>{}.setupFromRGBA(Color);
    }



}