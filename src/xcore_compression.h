#ifndef _XCORE_COMPRESSION_H
#define _XCORE_COMPRESSION_H
#pragma once

namespace xcore::compression
{
    namespace dictionary
    {
        //-----------------------------------------------------------------------------------------------------
        // Error and such
        //-----------------------------------------------------------------------------------------------------
        enum class error_state : std::uint32_t
        {
              GUID = xcore::crc::FromString<32>("xcore::compression::dictionary").m_Value
            , OK   = 0
            , FAILURE
            , NOT_DONE
        };

        struct compress
        {
            xcore::err                      Init          ( std::uint32_t BlockSize, const std::span<std::byte> SourceUncompress ) noexcept;
            xcore::err                      Pack          ( std::uint32_t& CompressedSize, std::span<std::byte> DestinationCompress ) noexcept;

            void*                       m_pCCTX{ nullptr };
            std::uint64_t               m_Position{ 0 };
            std::span<const std::byte>  m_Src;
            std::uint32_t               m_BlockSize;
        };

        struct decompression
        {
            xcore::err                      Init          (std::uint32_t BlockSize) noexcept;
            xcore::err                      Unpack        (std::uint32_t& DecompressSize, std::span<std::byte> DestinationUncompress, const std::span<const std::byte> SourceCompressed) noexcept;
            void*                       m_pDCTX{ nullptr };
            std::span<const std::byte>  m_Src;
            std::uint32_t               m_BlockSize;
        };
    }
}

#endif
