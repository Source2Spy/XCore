namespace xcore
{
    //-----------------------------------------------------------------------------------

    void bitmap::setOwnMemory(bool bOwnMemory) noexcept
    {
        m_Flags.m_bOwnsMemory = bOwnMemory;
    }

    //-----------------------------------------------------------------------------------

    void bitmap::setWrapMode(wrap_mode WrapMode) noexcept
    {
        m_Flags.m_WrapMode = static_cast<std::uint8_t>(WrapMode);
    }

    //-----------------------------------------------------------------------------------

    bool bitmap::isValid(void) const noexcept
    {
        return !!m_pData;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    bool bitmap::isLinearSpace(void) const noexcept
    {
        return m_Flags.m_bLinearSpace;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    bool bitmap::isSigned(void) const noexcept
    {
        return m_Flags.m_bSignedPixels;
    }

    //-----------------------------------------------------------------------------------
    constexpr
    std::uint32_t bitmap::getWidth(void) const noexcept
    {
        return m_Width;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    std::uint32_t bitmap::getHeight(void) const noexcept
    {
        return m_Height;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    bitmap::format bitmap::getFormat(void) const noexcept
    {
        return m_Flags.m_Format;
    }

    //-----------------------------------------------------------------------------------

    void bitmap::setFormat(const bitmap::format Format) noexcept
    {
        m_Flags.m_Format = Format;
    }

    //-----------------------------------------------------------------------------------

    void bitmap::setColorSpace(const bitmap::color_space ColorSpace) noexcept
    {
        m_Flags.m_bLinearSpace = std::uint8_t(ColorSpace);
    }

    //-----------------------------------------------------------------------------------
    constexpr
    bitmap::color_space bitmap::getColorSpace( void ) const noexcept
    {
        return static_cast<bitmap::color_space>(m_Flags.m_bLinearSpace);
    }

    //-----------------------------------------------------------------------------------

    constexpr
    std::uint64_t bitmap::getFrameSize(void) const noexcept
    {
        return m_FrameSize;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    int bitmap::getFrameCount(void) const noexcept
    {
        const auto DataSize = getDataSize() - getMipCount()*sizeof(mip);
        return static_cast<int>(DataSize / getFrameSize());
    }

    //-----------------------------------------------------------------------------------
    constexpr
    bool bitmap::isCubemap( void ) const noexcept
    {
        return m_Flags.m_bCubeMap;
    }

    //-----------------------------------------------------------------------------------

    void bitmap::setCubemap( bool isCubeMap ) noexcept
    {
        m_Flags.m_bCubeMap = isCubeMap;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    int bitmap::getFaceCount(void) const noexcept
    {
        return m_Flags.m_bCubeMap ? 6 : 1;
    }

    //-----------------------------------------------------------------------------------
    constexpr
    std::uint64_t bitmap::getFaceSize( void ) const noexcept
    {
        xassert( (getFrameSize() / getFaceCount()) * getFaceCount() == getFrameSize() );
        return getFrameSize() / getFaceCount();
    }

    //-----------------------------------------------------------------------------------
    constexpr   
    float bitmap::getAspectRatio( void ) const noexcept
    {
        return m_Width / static_cast<float>(m_Height);
    }

    //-----------------------------------------------------------------------------------
    constexpr
    bitmap::wrap_mode bitmap::getWrapMode(void) const noexcept
    {
        return static_cast<wrap_mode>(m_Flags.m_WrapMode);
    }

    //-----------------------------------------------------------------------------------

    void bitmap::Copy(const bitmap& Src) noexcept
    {
        CreateFromMips({ &Src, 1u });
    }

    //-----------------------------------------------------------------------------------

    constexpr
    std::uint64_t bitmap::getDataSize(void) const noexcept
    {
        return m_DataSize;
    }

    //-----------------------------------------------------------------------------------

    constexpr
    int bitmap::getMipCount(void) const noexcept
    {
        return m_nMips;
    }

    //-----------------------------------------------------------------------------------
    
    bitmap::bitmap( std::span<std::byte> Data, std::uint32_t Width, std::uint32_t Height, bool bReleaseWhenDone ) noexcept
    :   m_pData                 { reinterpret_cast<mip*>( Data.data() )         }
    ,   m_DataSize              { Data.size()                                   }
    ,   m_FrameSize             { static_cast<std::uint32_t>(Data.size() -4)    }
    ,   m_Height                { Height                                        }   
    ,   m_Width                 { Width                                         }
    ,   m_Flags                 { static_cast<std::uint16_t>( (bReleaseWhenDone ? bit_pack_fields::owns_memory_mask_v : bit_pack_fields::zero_mask_v)
                                                              | ( static_cast<std::uint16_t>(bitmap::format::XCOLOR) << bit_pack_fields::offset_to_format_v )
                                                                               )}
    ,   m_nMips                 { 1u                                            }
    {
        xassert( m_FrameSize == (m_DataSize - m_nMips*sizeof(mip)) );
        xassert( ( static_cast<std::uint64_t>(m_Width) * m_Height * sizeof(icolor) + sizeof(int) ) == Data.size() );
    }

    //-------------------------------------------------------------------------------
    
    std::uint64_t bitmap::getMipSize( int iMip ) const noexcept
    {
        xassert( iMip >= 0 );
        xassert( iMip < m_nMips );

        const auto NextOffset = ((iMip + 1) == m_nMips ) ? m_FrameSize : static_cast<std::size_t>(m_pData[iMip+1].m_Offset);
        return NextOffset - m_pData[iMip].m_Offset;
    }

    //-------------------------------------------------------------------------------
    
    int bitmap::getFullMipChainCount( void ) const noexcept
    {
        const auto SmallerDimension  = std::min( m_Height, m_Width );
        const auto nMips             = xcore::bits::Log2IntRoundUp( SmallerDimension ) + 1;
        return nMips;
    }

    //-------------------------------------------------------------------------------
    
    const void* bitmap::getMipPtr(const int iMip, const int iFace, const int iFrame ) const noexcept
    {
        xassume(m_Width > 0);
        xassume(m_Height > 0);
        xassume(m_pData);
        xassume(iMip < m_nMips);
        xassume(iMip >= 0);
        xassume(iFrame >= 0);
        xassume(iFrame < getFrameCount());
        xassume(iFace >= 0);
        xassume(iFace < getFaceCount());

        auto FinalOffest = m_pData[iMip].m_Offset + iFrame * m_FrameSize + iFace * getFaceSize();
        return &reinterpret_cast<const std::byte*>(&m_pData[m_nMips])[FinalOffest];
    }

    //-------------------------------------------------------------------------------
    
    void* bitmap::getMipPtr( const int iMip, const int iFace, const int iFrame ) noexcept
    {
        xassume( m_Width  > 0 );
        xassume( m_Height > 0 );
        xassume( m_pData );
        xassume( iMip < m_nMips );
        xassume( iMip >= 0 );
        xassume( iFrame >= 0 );
        xassume( iFrame < getFrameCount() );
        xassume( iFace >= 0 );
        xassume( iFace < getFaceCount() );

        auto FinalOffest = m_pData[iMip].m_Offset + iFrame * m_FrameSize + iFace * getFaceSize();
        return &reinterpret_cast<std::byte*>(&m_pData[m_nMips])[FinalOffest];
    }

    //-------------------------------------------------------------------------------

    template< typename T >
    std::span<T> bitmap::getMip( const int iMip, const int iFace, const int iFrame ) noexcept
    {
        return { reinterpret_cast<T*>(getMipPtr(iMip, iFace, iFrame)), getMipSize(iMip) / sizeof(T) };
    }

    //-------------------------------------------------------------------------------

    template< typename T >
    std::span< const T> bitmap::getMip(const int iMip, const int iFace, const int iFrame) const noexcept
    {
        return { reinterpret_cast<const T*>(getMipPtr(iMip, iFace, iFrame)), getMipSize(iMip) / sizeof(T) };
    }

    //-------------------------------------------------------------------------------

    constexpr
    bool bitmap::isSquare( void ) const noexcept
    {
        xassume(m_Width > 0);
        xassume(m_Height > 0);
        //xassume(m_pData);

        return m_Width == m_Height;
    }

    //-------------------------------------------------------------------------------
    constexpr
    bool bitmap::isPowerOfTwo( void ) const noexcept
    {
        xassume( m_Width > 0 );
        xassume( m_Height > 0 );
        //xassume( m_pData );
        return xcore::bits::isPowTwo( m_Width ) && xcore::bits::isPowTwo( m_Height );
    }
}

//-------------------------------------------------------------------------------
// Serializer functions
//-------------------------------------------------------------------------------
namespace xcore::serializer::io_functions
{
    template<> inline
    xcore::err SerializeIO<bitmap>( xcore::serializer::stream& Stream, const bitmap& Bitmap ) noexcept
    {
        xcore::err Err;

        false
        || (Err = Stream.Serialize(reinterpret_cast<const std::byte* const&>(Bitmap.m_pData), Bitmap.m_DataSize, mem_type::Flags(mem_type::flags::UNIQUE)))
        || (Err = Stream.Serialize(Bitmap.m_DataSize))
        || (Err = Stream.Serialize(Bitmap.m_FrameSize))
        || (Err = Stream.Serialize(Bitmap.m_Height))
        || (Err = Stream.Serialize(Bitmap.m_Width))
        || (Err = Stream.Serialize(Bitmap.m_Flags.m_Value))
        || (Err = Stream.Serialize(Bitmap.m_nMips))
        || (Err = Stream.Serialize(Bitmap.m_Pad))
        ;

        return Err;
    }
}