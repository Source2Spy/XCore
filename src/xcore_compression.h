#ifndef _XCORE_COMPRESSION_H
#define _XCORE_COMPRESSION_H
#pragma once

namespace xcore::compression
{
    std::uint32_t   CompressMem   ( std::span<std::byte> DestinationCompress,   const std::span<std::byte> SourceUncompress ) noexcept;
    void            DecompressMem ( std::span<std::byte> DestinationUncompress, const std::span<std::byte> SourceCompressed ) noexcept;
}

#endif
