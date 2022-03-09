// Description:
//     This class serves as a container for a texture and a bitmap. The class can
//     handle cubemaps as well as multiple frame animations. Note that there is not
//     information about the playback of the animation. The class may also contain
//     mip levels for any texture.
//
//<CODE>
//
// Note that the Offsets is from after the offset table (begging of the actual data)
//
//                                     Memory Layout
// m_pMip ---------------------------> +-------------------+
//                                     | mip offset array  |
//          mip offset array           +-------------------+ <-- offset base (nMips * sizeof(bitmap::mip))
// m_pMip -> +------------+            | Face 0  (Frame 0) |  ^  <-- If it is a cube map will have 6 faces other wise 1
//           | s32 Offset |  ------->  | Mip0 Data         |  |
//           +------------+            |                   |  |
//           | s32 Offset |  ------->  | Mip1 Data         |  |
//           +------------+            |                   |  | 
//           | s32 Offset |  ------->  | Mip2 Data         |  |
//           +------------+            |                   |  | FrameSize
//           | s32 Offset |  ------->  | Mip3 Data         |  |
//           +------------+            |                   |  |
//           | s32 Offset |  ------->  | Mip4 Data         |  |
//           +------------+            |                   |  |
//                                     | ----------------- |  |         
//                                     | Face 2 starts...  |  |
//                                     | Mip0 Data         |  |
//                                     | ...               |  v
//                                     +-------------------+ <-- next Frame 1
//     *  Note that previous offsets   | Face 0            |
//        Still work in new faces      | Mip 0 ..          |
//        as well as frames            | ...               |
//                                     |                   |
//     *  If the bitmap will have a    +-------------------+
//        palette it will be part of   +-------------------+
//        the mip data as well,        +-------------------+
//        at the top.
//
// There are a few compressed texture types supported. Here is a quick over view of some of them:
//
//      * PVR1 also known as PVRTC - This is Imagination’s version one of its widely used PowerVR texture compression.
//        It supports a 2/4bpp versions and RGBA/RGB as well. It is not block base rather it has 2 low rest texture
//        that are combine with a larger gray scale texture. More info:
//        http://blog.imgtec.com/powervr/pvrtc-the-most-efficient-texture-compression-standard-for-the-mobile-graphics-world
//
//      * PVR2 also know as PVRTC2 - This is Imagination's newly updated compression formats. When possible use this over version one.
//        http://blog.imgtec.com/powervr/pvrtc2-taking-texture-compression-to-a-new-dimension
//
//      * ETCn - ETC1 (Ericsson texture compression) and ETC2/EAC (backwards compatible with ETC1 and mandatory in the OpenGL ES 3.0 graphics standard)
//        http://en.wikipedia.org/wiki/Ericsson_Texture_Compression
//
//      * ASTC - (Adaptive scalable texture compression), an upcoming optional extension for both OpenGL and OpenGL ES
//        http://en.wikipedia.org/wiki/Adaptive_Scalable_Texture_Compression
//
//      * ATITC - (ATI texture compression)
//
//      * S3TC also know as DXTn - (S3 texture compression), also called DXTn, DXTC or BCn
//        http://en.wikipedia.org/wiki/S3_Texture_Compression
//
//      * Vulkan reference: https://www.khronos.org/registry/dataformat/specs/1.1/dataformat.1.1.pdf 
//                            https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html
//      * NVidia Overview on texture formats: https://developer.nvidia.com/astc-texture-compression-for-game-assets
//
//</CODE>
//
// TODO:
//     The class in unfinish
//
// See Also:
//     xcore::color
//==============================================================================
namespace xcore
{
    class bitmap 
    {
    public:

        // Bit wise formatting for the enumeration.
        // FORMAT_(LOW BITS elements first then moving to HIGH BITS)
        // byte order       0    1    2    3    
        // bit order:    0    8    16   24   32
        //               | R8 | G8 | B8 | A8 |
        enum class format : std::uint8_t
        {   INVALID

            //
            // Uncompressed formats
            //
        ,   B8G8R8A8     = xcore::color::format::type::UINT_32_BGRA_8888
        ,   B8G8R8U8     = xcore::color::format::type::UINT_32_BGRU_8888
        ,   A8R8G8B8     = xcore::color::format::type::UINT_32_ARGB_8888
        ,   U8R8G8B8     = xcore::color::format::type::UINT_32_URGB_8888
        ,   R8G8B8U8     = xcore::color::format::type::UINT_32_RGBU_8888
        ,   R8G8B8A8     = xcore::color::format::type::UINT_32_RGBA_8888     // PRIMARY FORMAT (xcolor)
        ,   XCOLOR       = R8G8B8A8
        ,   R8G8B8       = xcore::color::format::type::UINT_24_RGB_888
        ,   R4G4B4A4     = xcore::color::format::type::UINT_16_RGBA_4444
        ,   R5G6B5       = xcore::color::format::type::UINT_16_RGB_565
        ,   B5G5R5A1     = xcore::color::format::type::UINT_16_BGRA_5551
           
        ,   XCOLOR_END                                      // end of the range of xcolor

            // S3TC Compressed Texture Image Formats 
        ,   BC1_4RGB                                        // DXT1_RGB 
        ,   BC1_4RGBA1                                      // DXT1_RGBA
        ,   BC2_8RGBA                                       // DXT3_RGBA
        ,   BC3_8RGBA                                       // DXT5_RGBA

            // RGTC Compressed Texture Image Formats
        ,   BC4_4R                                          // High quality R  (good for suplemental alpha)
        ,   BC5_8RG                                         // High Quality RG (good for normal maps)

            // BPTC Compressed Texture Image Formats 
        ,   BC6H_8RGB_FLOAT                                 // Floating point compression    (good for normal maps)
        ,   BC7_8RGBA                                       // High quality RGBA compression (good for normal maps) 

            // Ericsson Texture Compression (ETC)
        ,   ETC2_4RGB                                       
        ,   ETC2_4RGBA1                                   
        ,   ETC2_8RGBA                                     
        
            // ASTC stands for Adaptive Scalable Texture Compression
        ,   ASTC_4x4_8RGB                                   // 8.00bpp
        ,   ASTC_5x4_6RGB                                   // 6.40bpp
        ,   ASTC_5x5_5RGB                                   // 5.12bpp (good for normal maps)
        ,   ASTC_6x5_4RGB                                   // 4.27bpp
        ,   ASTC_6x6_4RGB                                   // 3.56bpp
        ,   ASTC_8x5_3RGB                                   // 3.20bpp
        ,   ASTC_8x6_3RGB                                   // 2.67bpp
        ,   ASTC_8x8_2RGB                                   // 2.00bpp
        ,   ASTC_10x5_3RGB                                  // 2.56bpp
        ,   ASTC_10x6_2RGB                                  // 2.13bpp
        ,   ASTC_10x8_2RGB                                  // 1.60bpp
        ,   ASTC_10x10_1RGB                                 // 1.28bpp
        ,   ASTC_12x10_1RGB                                 // 1.07bpp
        ,   ASTC_12x12_1RGB                                 // 0.89bpp

            //
            // Compression formats
            //
        ,   PAL4_R8G8B8A8                                  // 4 bpp Index + 16  RGBA8888 palette
        ,   PAL8_R8G8B8A8                                  // 8 bpp Index + 256 RGBA8888 palette

            // PVR compression modes
        ,   PVR1_2RGB                                       
        ,   PVR1_2RGBA
        ,   PVR1_4RGB
        ,   PVR1_4RGBA
        ,   PVR2_2RGBA
        ,   PVR2_4RGBA

            //
            // Extra Frame buffer Formats
            //
        ,   D24S8_FLOAT                                     // Floating point depth and 8bit stencil
        ,   D24S8                                           // Depth 24 bits and 8 bit Stencil    
        ,   R8
        ,   R32                                             
        ,   R8G8                                            
        ,   R16G16B16A16                                    
        ,   R16G16B16A16_FLOAT                                  
        ,   A2R10G10B10                                     
        ,   B11G11R11_FLOAT                                       

            //
            // End
            //
        ,   ENUM_COUNT
        };

        enum class color_space : std::uint8_t
        {   SRGB
        ,   LINEAR
        };

    public:

       constexpr                                bitmap                  ( void 
                                                                        ) noexcept = default;
        inline                                 ~bitmap                  ( void
                                                                        ) noexcept;
        inline                                  bitmap                  ( std::span<std::byte>  Data
                                                                        , std::uint32_t         Width
                                                                        , std::uint32_t         Height
                                                                        , bool                  bReleaseWhenDone 
                                                                        ) noexcept;
        static      xcore::err                  SerializeLoad           ( bitmap*&                                  pBitmap
                                                                        , const xcore::string::view<const wchar_t>  FileName
                                                                        ) noexcept;
                    xcore::err                  SerializeSave           ( const xcore::string::view<const wchar_t>  FileName
                                                                        , bool                                      bSwapEndian = false
                                                                        ) const noexcept;
                    xcore::err                  Load                    ( const string::view<const wchar_t> FileName
                                                                        ) noexcept;
                    xcore::err                  Save                    ( const string::view<const wchar_t> FileName
                                                                        ) const noexcept;
                    xcore::err                  SaveTGA                 ( const string::view<const wchar_t> FileName
                                                                        ) const noexcept;
                    void                        Kill                    ( void 
                                                                        ) noexcept;
    //            bool                            SaveTGA             ( const xstring FileName ) const;
    //            void                            SerializeIO         ( xserialfile& SerialFile ) const;
        
        inline      void                        setOwnMemory            ( bool bOwnMemory 
                                                                        ) noexcept;
        inline      void                        setHasAlphaInfo         ( bool bAlphaInfo 
                                                                        ) noexcept;
                    void                        ComputeHasAlphaInfo     ( void 
                                                                        ) noexcept;
                    void                        ComputePremultiplyAlpha ( void 
                                                                        ) noexcept;

        constexpr   bool                        hasAlphaInfo            ( void 
                                                                        ) const noexcept;
                    bool                        hasAlphaChannel         ( void 
                                                                        ) const noexcept;
        
        inline      bool                        isValid                 ( void 
                                                                        ) const noexcept;
        constexpr   bool                        isSquare                ( void 
                                                                        ) const noexcept;
        constexpr   bool                        isPowerOfTwo            ( void 
                                                                        ) const noexcept;
        constexpr   bool                        isLinearSpace           ( void 
                                                                        ) const noexcept;
        constexpr   bool                        isSigned                ( void
                                                                        ) const noexcept;
        constexpr   bool                        isCubemap               ( void 
                                                                        ) const noexcept;
        inline      void                        setCubemap              ( bool isCubeMap
                                                                        ) noexcept;
        constexpr   std::uint32_t               getWidth                ( void 
                                                                        ) const noexcept;
        constexpr   std::uint32_t               getHeight               ( void 
                                                                        ) const noexcept;
        constexpr   format                      getFormat               ( void 
                                                                        ) const noexcept;
        inline      void                        setFormat               ( const format Format 
                                                                        ) noexcept;
        inline      void                        setColorSpace           ( const color_space ColorSpace 
                                                                        ) noexcept;
        constexpr   std::uint64_t               getFrameSize            ( void 
                                                                        ) const noexcept;
        constexpr   int                         getFrameCount           ( void
                                                                        ) const noexcept;
        constexpr   int                         getFaceCount            ( void
                                                                        ) const noexcept;
        constexpr   std::uint64_t               getFaceSize             ( void 
                                                                        ) const noexcept;
        constexpr   float                       getAspectRatio          ( void
                                                                        ) const noexcept;
        inline      void                        Copy                    ( const bitmap& Src 
                                                                        ) noexcept;

        constexpr   std::uint64_t               getDataSize             ( void 
                                                                        ) const noexcept;
        constexpr   int                         getMipCount             ( void 
                                                                        ) const noexcept;
                    void                        FlipImageInY            ( void 
                                                                        ) noexcept;
        template< typename T >
        inline      std::span<T>                getMip                  ( int iMip
                                                                        , int iFace  = 0
                                                                        , int iFrame = 0 
                                                                        ) noexcept;
        template< typename T >
        inline      std::span<const T>          getMip                  ( int iMip
                                                                        , int iFace  = 0
                                                                        , int iFrame = 0 
                                                                        ) const noexcept;
        inline      std::uint64_t               getMipSize              ( int Mip 
                                                                        ) const noexcept;
        inline      int                         getFullMipChainCount    ( void 
                                                                        ) const noexcept;
        
                    void                        CreateResizedBitmap     ( bitmap&           Dest         
                                                                        , std::uint32_t     FinalWidth
                                                                        , std::uint32_t     FinalHeight 
                                                                        ) const noexcept;
        
                    void                        setDefaultTexture       ( void 
                                                                        ) noexcept;
        static      const bitmap&               getDefaultBitmap        ( void 
                                                                        ) noexcept;
        
                    void                        CreateBitmap            ( std::uint32_t Width
                                                                        , std::uint32_t Height 
                                                                        ) noexcept;
        
                    void                        CreateFromMips          ( std::span<const bitmap> MipList
                                                                        ) noexcept;
        
                    void                        setupFromColor          ( std::uint32_t                 Width     
                                                                        , std::uint32_t                 Height    
                                                                        , std::span<icolor>             Data  
                                                                        , bool                          bFreeMemoryOnDestruction = true 
                                                                        ) noexcept;
        
                    void                        setup                   ( std::uint32_t                 Width                         
                                                                        , std::uint32_t                 Height                        
                                                                        , bitmap::format                BitmapFormat
                                                                        , std::uint64_t                 FrameSize                     
                                                                        , std::span<std::byte>          Data                          
                                                                        , bool                          bFreeMemoryOnDestruction      
                                                                        , int                           nMips                         
                                                                        , int                           nFrames                       
                                                                        ) noexcept;

    /*
        void                    ConvertBitmap       ( s32 Bpp, xcolor::format Format );
        void                    ConvertBitmap       ( bitmap& Bitmap, s32 Bpp, xcolor::format Format ) const;    

        std::uint32_t                     GetPixel            ( s32 X, s32 Y, s32 Mip = 0 ) const;
        void                    SetPixel            ( s32 X, s32 Y, std::uint32_t Pixel, s32 Mip = 0 );
        xcolor                  GetPixelColor       ( s32 X, s32 Y, s32 Mip = 0 ) const;
        xcolor                  GetBilinearColor    ( f32 ParamU, f32 ParamV, bool Clamp=FALSE, s32 Mip = 0 ) const;
        void                    SetPixelColor       ( xcolor Color, s32 X, s32 Y, s32 Mip = 0 );
    */

        struct mip
        {
            std::int32_t            m_Offset;                           // Offset in bitmap::m_pData for a mip's data
        };

    protected:

        union bit_pack_fields
        {
            std::uint16_t           m_Value{};
            struct
            {
                std::uint8_t        m_bCubeMap              : 1     // Tells if this bitmap is a cubemap 
                ,                   m_bOwnsMemory           : 1     // if the bitmap is allowed to free the memory
                ,                   m_bHasAlphaInfo         : 1     // it tells that the picture has alpha information
                ,                   m_bAlphaPremultiplied   : 1     // Tells it the alpha has already been premultiplied
                ,                   m_bLinearSpace          : 1     // What is the color space for the bitmap
                ,                   m_bWrapU                : 1     // Tells if it can wrap around U
                ,                   m_bWarpV                : 1     // Tells if it can wrap around V
                ,                   m_bSignedPixels         : 1     // Tells if the pixels in the bitmap are suppose to be signed bits
                ;
                format              m_Format;                       // Format of the data
            };

            static constexpr auto zero_mask_v                 = std::uint16_t{0};
            static constexpr auto cubemap_mask_v              = std::uint16_t{1 << 0};
            static constexpr auto owns_memory_mask_v          = std::uint16_t{1 << 1};
            static constexpr auto has_alpha_info_mask_v       = std::uint16_t{1 << 2};
            static constexpr auto alpha_premultiplied_mask_v  = std::uint16_t{1 << 3};
            static constexpr auto line_space_mask_v           = std::uint16_t{1 << 4};
            static constexpr auto wap_u_mask_v                = std::uint16_t{1 << 5};
            static constexpr auto wap_v_mask_v                = std::uint16_t{1 << 6};
            static constexpr auto signed_pixels_mask_v        = std::uint16_t{1 << 7};
            static constexpr auto offset_to_format_v          = std::uint16_t{8};
        };
        static_assert(sizeof(bit_pack_fields) == 2);

        inline      const void*         getMipPtr( const int iMip, const int iFace, const int iFrame  ) const noexcept;
        inline      void*               getMipPtr( const int iMip, const int iFace, const int iFrame  )       noexcept;

    public:

        mip*                            m_pData         {};             // +8 pointer to the mip data
        std::uint64_t                   m_DataSize      { 0 };          // +8 total data size in bytes
        std::uint32_t                   m_FrameSize     { 0 };          // +4 Size of one frame of data.(one bitmap)
        std::uint32_t                   m_Height        { 0 };          // +4 height in pixels
        std::uint32_t                   m_Width         { 0 };          // +4 width in pixels
        bit_pack_fields                 m_Flags         {};             // +2 all flags including the format of the bitmap
        std::uint8_t                    m_nMips         { 0 };          // +1 Number of mips
        std::uint8_t                    m_Pad           { 0 };          // +1 Padding
                                                                        // 32 bytes total
    };
    static_assert( sizeof(bitmap) == 32, "The bitmap structure should always be 32bytes long" );
}
