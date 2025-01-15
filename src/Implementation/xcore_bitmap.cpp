
constexpr std::uint32_t s_DefaultBitmapSize = 256u; 
using t_default_data = std::array<std::byte, s_DefaultBitmapSize*s_DefaultBitmapSize*sizeof(xcore::icolor) + sizeof(int) >;

static const t_default_data Data = []() noexcept
{
    t_default_data Data = {};
    const xcore::icolor C1{ 128, 128, 128, 255 };
    const xcore::icolor C2{ 187, 187, 187, 255 };
    const xcore::icolor CT[] = { xcore::icolor{ 187,  50,  50, 255 }
                               , xcore::icolor{  50, 187,  50, 255 }
                               , xcore::icolor{  50,  50, 187, 255 }
                               };
    const auto          nCheckers    = 16;
    const auto          CheckerSize  = s_DefaultBitmapSize/nCheckers;

    xcore::icolor* pData = reinterpret_cast<xcore::icolor*>(&Data[4]);
        
    // Create basic checker pattern
    for( auto y = 0u; y < s_DefaultBitmapSize; y++ )
    {
        for(auto x = 0u; x < s_DefaultBitmapSize; x++)
        {
            // Create the checker pattern
            pData[ x+ s_DefaultBitmapSize*y ] = ((y&CheckerSize)==CheckerSize)^((x&CheckerSize)==CheckerSize)?C1:C2;
        }
    }

    // Draw a simple arrows at the top left pointing up...
    const int ArrowSize = CheckerSize*2;
    for( auto k=0; k<3; k++ )
    {
        auto yy = 1u;
        for( auto y=1u; y<(ArrowSize -1); ++y )
        {
            for( auto x = yy; x < (ArrowSize -1)-yy; x++)
            {
                if( k&1) pData[ k* ArrowSize + x + s_DefaultBitmapSize*(ArrowSize -y - 1) ] = CT[k];
                else     pData[ k* ArrowSize + x + s_DefaultBitmapSize*(ArrowSize -y - 1) ] = CT[k];
            }
                
            if (y&1) yy++;
        }
    }

    return Data; 
}();

static const xcore::bitmap s_DefaultBitmap{ { const_cast<std::byte*>(Data.data()), Data.size() }, s_DefaultBitmapSize, s_DefaultBitmapSize, false };


//////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////

xcore::bitmap::~bitmap( void ) noexcept
{
    if( m_pData && m_Flags.m_bOwnsMemory ) delete m_pData;
}

//-------------------------------------------------------------------------------

void xcore::bitmap::Kill( void ) noexcept
{
    if (m_pData && m_Flags.m_bOwnsMemory) delete m_pData;

    m_DataSize      = 0;                
    m_FrameSize     = 0;                
    m_Height        = 0;                
    m_Width         = 0;                
    m_Flags.m_Value = 0;                
    m_nMips         = 0;                
}

//-------------------------------------------------------------------------------

xcore::err xcore::bitmap::SerializeLoad( bitmap*& pBitmap, const string::view<const wchar_t> FileName ) noexcept
{
    xcore::serializer::stream Stream;    
    return Stream.Load(FileName, pBitmap);
}

//-------------------------------------------------------------------------------

xcore::err xcore::bitmap::SerializeSave( const string::view<const wchar_t> FileName, bool bSwapEndian) const noexcept
{
    xcore::serializer::stream Stream;
    return Stream.Save(FileName, *this, {}, bSwapEndian );
}

//-------------------------------------------------------------------------------

xcore::err xcore::bitmap::Load( const string::view<const wchar_t> FileName ) noexcept
{
    Kill();

    xcore::file::stream File;

    if( auto Err = File.open(FileName, "rb" ); Err )
        return Err;

    //
    // Read the signature
    //
    {
        std::uint32_t Signature;
        if( auto Err = File.Read( Signature ); Err ) return Err;
        if( Signature != std::uint32_t('XBMP') )
            return xerr_failure_s("Wrong file signature");
    }

    if ( xcore::err Err; false
        || (Err = File.Read(m_DataSize))
        || (Err = File.Read(m_FrameSize))
        || (Err = File.Read(m_Height))
        || (Err = File.Read(m_Width))
        || (Err = File.Read(m_Flags.m_Value))
        || (Err = File.Read(m_nMips))
        || (Err = File.Read(m_Pad))
        ) return Err;

    //
    // Read the big data
    //
    m_pData = reinterpret_cast<mip*>(new std::byte[ m_DataSize ] );
    return File.ReadView( std::span{ reinterpret_cast<std::byte*>(m_pData), m_DataSize });
}

//-------------------------------------------------------------------------------

xcore::err xcore::bitmap::Save( const string::view<const wchar_t> FileName ) const noexcept
{
    xcore::file::stream File;

    if( xcore::err Err; false
        || (Err = File.open(FileName, "wb"))
        || (Err = File.Write(std::uint32_t('XBMP')))
        || (Err = File.Write(m_DataSize))
        || (Err = File.Write(m_FrameSize))
        || (Err = File.Write(m_Height))
        || (Err = File.Write(m_Width))
        || (Err = File.Write(m_Flags.m_Value))
        || (Err = File.Write(m_nMips))
        || (Err = File.Write(m_Pad))
        || (Err = File.WriteView(std::span{ reinterpret_cast<std::byte*>(m_pData), m_DataSize }))
        ) return Err;

    return {};
}

//-------------------------------------------------------------------------------

xcore::err xcore::bitmap::SaveTGA(const string::view<const wchar_t> FileName) const noexcept
{
    std::array< std::byte, 18> Header;

    // The format of this picture must be in color format
    xassert( getFormat() == xcore::bitmap::format::R8G8B8A8 
          || getFormat() == xcore::bitmap::format::B8G8R8A8
          || getFormat() == xcore::bitmap::format::R8G8B8U8
          || getFormat() == xcore::bitmap::format::B8G8R8U8);

    // Build the header information.
    std::memset(Header.data(), 0, Header.size());
    Header[2]  = std::byte{2u};     // Image type.
    Header[12] = static_cast<std::byte>((getWidth() >> 0) & 0xFF);
    Header[13] = static_cast<std::byte>((getWidth() >> 8) & 0xFF);
    Header[14] = static_cast<std::byte>((getHeight() >> 0) & 0xFF);
    Header[15] = static_cast<std::byte>((getHeight() >> 8) & 0xFF);
    Header[16] = std::byte{ 32u };    // Bit depth.
    Header[17] = std::byte{ 32u };    // NOT flipped vertically.

    // Open the file.
    xcore::file::stream File;

    if( auto Err = File.open(FileName, "wb"); Err )
        return Err;

    // Write out the data.
    if( auto Err = File.WriteView( Header ); Err )
        return Err;

    //
    // Convert to what tga expects as a color
    //
    const xcore::icolor* pColor = getMip<xcore::icolor>(0).data();
    const auto           Size   = getWidth() * getHeight();

    if( getFormat() == xcore::bitmap::format::B8G8R8A8 || 
        getFormat() == xcore::bitmap::format::B8G8R8U8 )
    {
        if (auto Err = File.WriteView(std::span{ m_pData, getFrameSize() }); Err)
            return Err;
    }
    else
    {
        auto Convert = std::make_unique<xcore::icolor[]>(Size);
        for( auto i = 0u; i < Size; ++i )
        {
            auto Color = pColor[i];
            std::swap(Color.m_R, Color.m_B);
            Convert[i] = Color;
        }

        if (auto Err = File.WriteView( std::span{Convert.get(), static_cast<std::size_t>(Size) }); Err)
            return Err;
    }

    return {};
}

//-------------------------------------------------------------------------------

void xcore::bitmap::setup
( const std::uint32_t         Width                   
, const std::uint32_t         Height
, const xcore::bitmap::format BitmapFormat            
, const std::uint64_t         FrameSize
, std::span<std::byte>        Data                    
, const bool                  bFreeMemoryOnDestruction
, const int                   nMips                   
, const int                   nFrames               
) noexcept
{
    xassert( Data.size()       >  4 );
    xassert( FrameSize         >  0 );
    xassert( FrameSize         <  Data.size() );    // In fact this should be equal to: DataSize - ((nMips*sizeof(s32)) * nFrames) which means it should be removed
    xassert( FrameSize         <= 0xffffffff );     // Since we are going to pack it into a u32 it can not be larger than that
    xassert( nMips             >  0 );
    xassert( nFrames           >  0 );
    xassert( Width             >  0 );
    xassert( Height            >  0 );
    xassert((int)BitmapFormat  > (int)xcore::bitmap::format::INVALID );
    xassert((int)BitmapFormat  < (int)xcore::bitmap::format::ENUM_COUNT );

    Kill();

    m_pData               = reinterpret_cast<mip*>(Data.data());
    m_Flags.m_bOwnsMemory = bFreeMemoryOnDestruction;

    m_DataSize          = Data.size();    
    xassert( FrameSize == m_DataSize - ((nMips*sizeof(int)) * nFrames) );
    
    m_FrameSize         = static_cast<std::uint32_t>(FrameSize);
    m_Height            = Height;   
    m_Width             = Width;
    
    m_nMips             = nMips;    
    m_Flags.m_Format    = BitmapFormat;

    xassert( nFrames == getFrameCount() );
}

//-------------------------------------------------------------------------------

void xcore::bitmap::setupFromColor
( const std::uint32_t       Width  
, const std::uint32_t       Height 
, std::span<xcore::icolor>  Data   
, const bool                bFreeMemoryOnDestruction        
) noexcept
{
    setup
    (   Width
    ,   Height
    ,   format::XCOLOR
    ,   sizeof(xcore::icolor)*( Data.size() - 1 )
    ,   { reinterpret_cast<std::byte*>(&Data[0]), sizeof(xcore::icolor)*Data.size() }
    ,   bFreeMemoryOnDestruction
    ,   1
    ,   1 
    );
}

//-------------------------------------------------------------------------------

bool xcore::bitmap::hasAlphaChannel( void ) const noexcept
{
    struct entry
    {
        std::uint8_t        m_bSupportsAlpha;
    };

    constexpr static auto SupportAlphaTable = []() constexpr noexcept
    {
        std::array< entry, (int)format::ENUM_COUNT > SupportAlphaTable = {0xff};

        SupportAlphaTable[ (int)format::R4G4B4A4              ] = entry{ true      };
        SupportAlphaTable[ (int)format::R5G6B5                ] = entry{ false     };
        SupportAlphaTable[ (int)format::B5G5R5A1              ] = entry{ true      };
        SupportAlphaTable[ (int)format::R8G8B8                ] = entry{ false     };
        SupportAlphaTable[ (int)format::R8G8B8U8              ] = entry{ false     };
        SupportAlphaTable[ (int)format::R8G8B8A8              ] = entry{ true      };
        SupportAlphaTable[ (int)format::B8G8R8A8              ] = entry{ true      };
        SupportAlphaTable[ (int)format::B8G8R8U8              ] = entry{ false     };
        SupportAlphaTable[ (int)format::A8R8G8B8              ] = entry{ true      };
        SupportAlphaTable[ (int)format::U8R8G8B8              ] = entry{ false     };

        SupportAlphaTable[ (int)format::PAL4_R8G8B8A8         ] = entry{ true      };
        SupportAlphaTable[ (int)format::PAL8_R8G8B8A8         ] = entry{ true      };
                                                                              
        SupportAlphaTable[ (int)format::ETC2_4RGB             ] = entry{ false     };
        SupportAlphaTable[ (int)format::ETC2_4RGBA1           ] = entry{ false     };
        SupportAlphaTable[ (int)format::ETC2_8RGBA            ] = entry{ true      };
                                                                              
        SupportAlphaTable[ (int)format::BC1_4RGB              ] = entry{ false     };
        SupportAlphaTable[ (int)format::BC1_4RGBA1            ] = entry{ true      };
        SupportAlphaTable[ (int)format::BC2_8RGBA             ] = entry{ true      };
        SupportAlphaTable[ (int)format::BC3_8RGBA             ] = entry{ true      };
        SupportAlphaTable[ (int)format::BC4_4R                ] = entry{ false     };
        SupportAlphaTable[ (int)format::BC5_8RG               ] = entry{ false     };
        SupportAlphaTable[ (int)format::BC6H_8RGB_FLOAT       ] = entry{ false     };
        SupportAlphaTable[ (int)format::BC7_8RGBA             ] = entry{ false     };
                                                                              
        SupportAlphaTable[ (int)format::ASTC_4x4_8RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_5x4_6RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_5x5_5RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_6x5_4RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_6x6_4RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_8x5_3RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_8x6_3RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_8x8_2RGB         ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_10x5_3RGB        ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_10x6_2RGB        ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_10x8_2RGB        ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_10x10_1RGB       ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_12x10_1RGB       ] = entry{ false     };
        SupportAlphaTable[ (int)format::ASTC_12x12_1RGB       ] = entry{ false     };
                                                                              
        SupportAlphaTable[ (int)format::PVR1_2RGB             ] = entry{ false     };
        SupportAlphaTable[ (int)format::PVR1_2RGBA            ] = entry{ true      };
        SupportAlphaTable[ (int)format::PVR1_4RGB             ] = entry{ false     };
        SupportAlphaTable[ (int)format::PVR1_4RGBA            ] = entry{ true      };
        SupportAlphaTable[ (int)format::PVR2_2RGBA            ] = entry{ true      };
        SupportAlphaTable[ (int)format::PVR2_4RGBA            ] = entry{ true      };
                                                                              
        SupportAlphaTable[ (int)format::D24S8_FLOAT           ] = entry{ false     };
        SupportAlphaTable[ (int)format::D24S8                 ] = entry{ false     };
        SupportAlphaTable[ (int)format::R8                    ] = entry{ false     };
        SupportAlphaTable[ (int)format::R32                   ] = entry{ false     };
        SupportAlphaTable[ (int)format::R8G8                  ] = entry{ false     };
        SupportAlphaTable[ (int)format::R16G16B16A16          ] = entry{ true      };
        SupportAlphaTable[ (int)format::R16G16B16A16_FLOAT    ] = entry{ true      };
        SupportAlphaTable[ (int)format::A2R10G10B10           ] = entry{ true      };
        SupportAlphaTable[ (int)format::B11G11R11_FLOAT       ] = entry{ false     };

        return SupportAlphaTable;
    }();

    // Must insert this format into the table and assign avalue to it
    xassume( SupportAlphaTable[(int)m_Flags.m_Format].m_bSupportsAlpha == 1 || SupportAlphaTable[(int)m_Flags.m_Format].m_bSupportsAlpha == 0 );
    static_assert( static_cast<int>(true)  == 1, "True should be just one" );
    static_assert( static_cast<int>(false) == 0, "False should be just zero" );
    return !!SupportAlphaTable[(int)m_Flags.m_Format].m_bSupportsAlpha;
}

//-------------------------------------------------------------------------------

bool xcore::bitmap::ComputeHasAlphaInfo( void ) const noexcept
{
    //
    // Check all the pixels to see if they have information other than the default
    //
    
    // We can handle anything with the compress formats and such
    xassert( (int)m_Flags.m_Format < (int)format::XCOLOR_END );
    
    const auto  Format = xcore::color::format{ static_cast<xcore::color::format::type>(m_Flags.m_Format) };
    const auto& Dest   = Format.getDescriptor();

    if( Dest.m_TB == 16 )
    {
        auto            X = 0u;
        xcore::icolor   C1;
        auto            pData = reinterpret_cast<const std::uint16_t*>( getMipPtr(0,0,0) );
        for( auto y=0u; y<m_Height; ++y )
        for( auto x=0u; x<m_Width;  ++x )
        {
            auto Data = pData[ x + y*m_Width ];
            C1 = xcore::icolor( Data, Format );
            
            X |= (C1.m_A == 0xff);
            X |= (C1.m_A == 0x00)<<1;
            if( X == 1 || X == 2 )
                continue;
            
            return true;
        }
    }
    else
    {
        auto            X=0u;
        xcore::icolor   C1;
        auto            pData = reinterpret_cast<const std::uint32_t*>(getMipPtr(0,0,0));
        for( auto y=0u; y<m_Height; ++y )
        for( auto x=0u; x<m_Width;  ++x )
        {
            auto Data = pData[ x + y*m_Width ];
            C1 = xcore::icolor( Data, Format );
            
            X |= (C1.m_A == 0xff);
            X |= (C1.m_A == 0x00)<<1;
            if( X == 1 || X == 2 )
                continue;
            
            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------

void xcore::bitmap::setDefaultTexture( void ) noexcept
{
    Kill();
    std::memcpy( this, &s_DefaultBitmap, sizeof(*this) );
}

//-------------------------------------------------------------------------------

const xcore::bitmap& xcore::bitmap::getDefaultBitmap( void ) noexcept
{
    return s_DefaultBitmap;
}

//-------------------------------------------------------------------------------

void xcore::bitmap::CreateBitmap( const std::uint32_t Width, const std::uint32_t Height ) noexcept
{
    xassert( Width  >= 1 );
    xassert( Height >= 1 );
    
    // Allocate the necesary data
    const auto  Size = 1 + Width * Height;
    auto        Data = std::make_unique<xcore::icolor[]>(Size);
    
    // Initialize the offset table
    Data[0].m_Value = 0;
    
    setupFromColor
    ( Width 
    , Height 
    , { Data.release(), Size }
    , true 
    );
}

//-------------------------------------------------------------------------------
/*
bool xbitmap::SaveTGA( const xstring FileName ) const noexcept
{
    xbyte   Header[18];
    
    // The format of this picture must be in color format
    ASSERT( m_Format == FORMAT_XCOLOR || m_Format == FORMAT_R8G8B8U8 );
    
    // Build the header information.
    x_memset( Header, 0, 18 );
    Header[ 2] = 2;     // Image type.
    Header[12] = (getWidth()  >> 0) & 0xFF;
    Header[13] = (getWidth()  >> 8) & 0xFF;
    Header[14] = (getHeight() >> 0) & 0xFF;
    Header[15] = (getHeight() >> 8) & 0xFF;
    Header[16] = 32;    // Bit depth.
    Header[17] = 32;    // NOT flipped vertically.
    
    // Open the file.
    xfile File;
    
    if( !File.Open( FileName, "wb" ) )
    {
        return FALSE;
    }
    
    // Write out the data.
    File.WriteRaw( Header, 1, 18 );
    
    //
    // Convert to what tga expects as a color
    //
    const xcolor* pColor = (xcolor*)getMip(0);
    const s32     Size   = getWidth() * getHeight();
    
    for( s32 i=0; i<Size; ++i )
    {
        xcolor Color = pColor[i];
        x_Swap( Color.m_R, Color.m_B );
        File.WriteRaw( &Color, 4, 1 );
    }
    
    return true;
}
*/

//-------------------------------------------------------------------------------

void xcore::bitmap::CreateFromMips( std::span<const bitmap> MipList ) noexcept
{
    //
    // Compute the total size
    //
    std::uint64_t TotalSize = 0;
    for( auto i=0u; i<MipList.size(); i++ )
    {
        const auto& Mip = MipList[i];
        
        TotalSize += Mip.m_DataSize;
        
        xassert( Mip.m_nMips   == 1 );
    }
    
    //
    // Allocate data
    //
    auto   BaseData     = std::make_unique<std::byte[]>( TotalSize );
    auto   pOffsetTable = reinterpret_cast<mip*>(BaseData.get());
    auto   pData        = reinterpret_cast<std::byte*>(&pOffsetTable[MipList.size()]);
    
    //
    // Copy the actual data
    //
    {
        std::uint64_t TotalOffset=0;
        for( auto i=0u; i<MipList.size(); ++i )
        {
            const auto&     Mip         = MipList[i];
            auto&           Offset      = pOffsetTable[i];
            const auto      MipDataSize = Mip.m_DataSize - sizeof(mip);
            
            Offset = {static_cast<int>( TotalOffset )};
            
            // Copy Data
            std::memcpy( &pData[ Offset.m_Offset ], &Mip.m_pData[1], MipDataSize );
            
            // Get ready for the next entry
            TotalOffset += MipDataSize;
        }
    }
    
    //
    // OK we are ready to setup the bitmap
    //
    const auto& Mip         = MipList[0];
    setup
    ( Mip.m_Width
    , Mip.m_Height
    , static_cast<bitmap::format>(Mip.getFormat())
    , static_cast<int>( TotalSize - sizeof(mip) * MipList.size() ),
    { BaseData.release(), TotalSize }
    , true
    , static_cast<int>( MipList.size() )
    , 1 
    );
}

//-------------------------------------------------------------------------------

void xcore::bitmap::ComputePremultiplyAlpha( void ) noexcept
{
    if( m_Flags.m_bAlphaPremultiplied )
        return ;
         
    xassert( static_cast<int>(m_Flags.m_Format) == static_cast<int>(xcore::color::format::type::DEFAULT) );

    if (ComputeHasAlphaInfo() == false) return;

    auto Data = getMip<xcore::icolor>(0);
    for( auto& C : Data ) 
    {
        C = C.PremultiplyAlpha();
    }

    // Remember that we did this
    m_Flags.m_bAlphaPremultiplied = true;
}

//-------------------------------------------------------------------------------

void xcore::bitmap::FlipImageInY( void ) noexcept
{
    xassert( isValid() );
    xassert( getFormat() == format::XCOLOR );
    xassert( getMipCount() == 1 );

    auto pData = getMip<xcore::icolor>(0);
    for( auto y = 0u; y < m_Height/2; ++y )
    for( auto x = 0u; x < m_Width;    ++x )
    {
        std::swap( pData[ x + y * m_Width], pData[ x + (m_Height-y-1) * m_Width] );
    }
}

