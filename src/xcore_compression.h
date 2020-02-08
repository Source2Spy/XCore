#ifndef _XCORE_COMPRESSION_H
#define _XCORE_COMPRESSION_H
#pragma once

namespace xcore::compression
{
    //-----------------------------------------------------------------------------------------------------
    class compress
    {
    public:
        enum class error_state : std::uint32_t
        {
              GUID = xcore::crc<32>::FromString("xcore::compression::dictionary").m_Value
            , OK   = 0
            , FAILURE
            , NOT_DONE
        };

        enum class level : std::uint32_t
        {
              FAST
            , MEDIUM
            , HIGH 
        };

    public:
                                       ~compress    ( void )                                                                                                        noexcept;
        xcore::err                      Init        ( std::uint32_t BlockSize, const std::span<std::byte> SourceUncompress, level CompressionLevel = level::HIGH )  noexcept;
        xcore::err                      Pack        ( std::uint32_t& CompressedSize, std::span<std::byte> DestinationCompress )                                     noexcept;

    protected:
        void*                       m_pCCTX     { nullptr };
        std::uint64_t               m_Position  { 0 };
        std::span<const std::byte>  m_Src;
        std::uint32_t               m_BlockSize {0};
    };

    //-----------------------------------------------------------------------------------------------------
    class decompress
    {
    public:
                                       ~decompress    (void)                                                                                                                        noexcept;
        xcore::err                      Init          (std::uint32_t BlockSize)                                                                                                      noexcept;
        xcore::err                      Unpack        (std::uint32_t& DecompressSize, std::span<std::byte> DestinationUncompress, const std::span<const std::byte> SourceCompressed) noexcept;

    protected:
        void*                       m_pDCTX{ nullptr };
        std::span<const std::byte>  m_Src;
        std::uint32_t               m_BlockSize{0};
    };
}

#endif
