
#define ZSTD_STATIC_LINKING_ONLY
#include "../../dependencies/zstd/lib/zstd.h"

//-------------------------------------------------------------------------------------------------------
// Add libz libraries
//-------------------------------------------------------------------------------------------------------
#if _XCORE_DEBUG
#   pragma comment(lib, "dependencies/zstd/build/VS2010/libzstd/bin/x64_Debug/libzstd_static.lib")
#else
#   pragma comment(lib, "dependencies/zstd/build/VS2010/libzstd/bin/x64_Release/libzstd_static.lib")
#endif


namespace xcore::compression
{
    //-------------------------------------------------------------------------------------------------------

    xcore::err compress::Init(bool bBlockSizeIsOutputSize, std::uint32_t BlockSize, const std::span<std::byte> SourceUncompress, level CompressionLevel) noexcept
    {
        xassert(!m_pCCTX);
        xcore::err Error;
        
        auto pCCTX = ZSTD_createCCtx();
        if (!pCCTX)  return xerr_failure(Error, "Error ZSTD_createCCtx");

        ZSTD_CCtx_setParameter(pCCTX, ZSTD_c_targetCBlockSize, BlockSize);
        switch (CompressionLevel)
        {
        case level::FAST:           ZSTD_CCtx_setParameter(pCCTX, ZSTD_c_compressionLevel, 1 );                     break;
        case level::MEDIUM:         ZSTD_CCtx_setParameter(pCCTX, ZSTD_c_compressionLevel, ZSTD_CLEVEL_DEFAULT );   break;
        case level::HIGH:           ZSTD_CCtx_setParameter(pCCTX, ZSTD_c_compressionLevel, ZSTD_maxCLevel() );      break;
        }
        ZSTD_CCtx_setParameter(pCCTX, ZSTD_c_srcSizeHint,      (int)SourceUncompress.size());
        
        m_pCCTX                  = (void*)pCCTX;
        m_Src                    = SourceUncompress;
        m_BlockSize              = BlockSize;
        m_bBlockSizeIsOutputSize = bBlockSizeIsOutputSize;

        return Error;
    }

    //-------------------------------------------------------------------------------------------------------
    compress::~compress(void) noexcept
    {
        if(m_pCCTX) ZSTD_freeCCtx((ZSTD_CCtx*)m_pCCTX);
    }

    //-------------------------------------------------------------------------------------------------------

    xcore::err compress::Pack(std::uint32_t& CompressedSize, std::span<std::byte> Destination) noexcept
    {
        xassert(m_Position <= m_Src.size());
        xassert(auto V = ZSTD_compressBound(m_BlockSize); Destination.size() >= V );

        xcore::err              Error;
        
        //
        // Flushing extra data if we have to 
        //
        if (m_Position == m_Src.size())
        {
            ZSTD_outBuffer          out = { Destination.data(), Destination.size(), 0 };
            ZSTD_inBuffer           in  = { nullptr, 0,             0 };
            std::size_t             rc  = ZSTD_compressStream2((ZSTD_CCtx*)m_pCCTX, &out, &in, ZSTD_e_flush);
            if (ZSTD_isError(rc))
                return xerr_failure(Error, "Error while compressing");
            CompressedSize = static_cast<std::uint32_t>(out.pos);
            if (rc != 0) return xerr_code(Error, error_state::NOT_DONE, "Waiting To flush stuff");
            return Error;
        }

        //
        // Compress data
        //
        do
        {
            const auto              Left    = m_Src.size() - m_Position;
            const auto              InSize  = Left > m_BlockSize ? m_BlockSize : Left;
            const ZSTD_EndDirective end     = m_bBlockSizeIsOutputSize ? ZSTD_e_end : (Left <= m_BlockSize) ? ZSTD_e_end : ZSTD_e_continue;
            ZSTD_inBuffer           in      = { &m_Src[m_Position], InSize,             0 };
            ZSTD_outBuffer          out     = { Destination.data(), Destination.size(), 0 };
            std::size_t             rc      = ZSTD_compressStream2((ZSTD_CCtx*)m_pCCTX, &out, &in, end);

            if (ZSTD_isError(rc))
                return xerr_failure(Error, "Error while compressing");

            m_Position += in.pos;

            CompressedSize = static_cast<std::uint32_t>(out.pos);
            if (m_bBlockSizeIsOutputSize)
            {
                if (m_Position == m_Src.size())
                {
                    if (rc != 0) return xerr_code(Error, error_state::NOT_DONE, "Waiting To flush stuff");
                    return Error;
                }
                break;
            }

            if (end == ZSTD_e_end)
            {
                if(rc != 0) return xerr_code(Error, error_state::NOT_DONE, "Waiting To flush stuff");
                return Error;
            }

        } while (CompressedSize == 0);

        return xerr_code(Error, error_state::NOT_DONE, "Not done");
    }

    //-------------------------------------------------------------------------------------------------------

    xcore::err decompress::Init(bool bBlockIsOutputSize, std::uint32_t BlockSize, bool bDontNeedWorkingBuffer) noexcept
    {
        // I haven't implemented the case where the block size is the inputput size
        xassert(bBlockIsOutputSize);
        xassert(!m_pDCTX);
        xcore::err Error;

        auto pDCTX = ZSTD_createDCtx();
        if (!pDCTX)  return xerr_failure(Error, "Error ZSTD_createDCtx");

        // Create working buffer for the decompression
        if (bDontNeedWorkingBuffer == false ) m_WorkingBuffer.Alloc(BlockSize*2).CheckError();
        ZSTD_DCtx_setParameter(pDCTX, ZSTD_d_windowLogMax, bits::Log2IntRoundUp(BlockSize) );

        m_pDCTX     = (void*)pDCTX;
        m_BlockSize = BlockSize;

        return Error;
    }

    //-------------------------------------------------------------------------------------------------------
    decompress::~decompress(void) noexcept
    {
        if (m_pDCTX) ZSTD_freeDCtx((ZSTD_DCtx*)m_pDCTX);
    }

    //-------------------------------------------------------------------------------------------------------

    xcore::err decompress::Unpack( std::uint32_t& DecompressSize, std::span<std::byte> DestinationUncompress, const std::span<const std::byte> SourceCompressed ) noexcept
    {
        xcore::err Error;
        
        ZSTD_inBuffer  rtIn     = { SourceCompressed.data(),       SourceCompressed.size(),        0 };
        ZSTD_outBuffer rtOut    = m_WorkingBuffer.size() ? ZSTD_outBuffer{ m_WorkingBuffer.data(), m_WorkingBuffer.size(), 0 } : ZSTD_outBuffer{ DestinationUncompress.data(), DestinationUncompress.size(), 0 };

        std::size_t    rc = ZSTD_decompressStream((ZSTD_DCtx*)m_pDCTX, &rtOut, &rtIn);
        if (ZSTD_isError(rc))
        {
            auto p = ZSTD_getErrorName(rc);
            return xerr_failure(Error, "Error while compressing");
        }

        // Copy the results into the final buffer
        if (m_WorkingBuffer.size()) std::memcpy(DestinationUncompress.data(), m_WorkingBuffer.data(), std::min(rtOut.pos, DestinationUncompress.size()) );

        DecompressSize = static_cast<std::uint32_t>(rtOut.pos);
        return Error;
    }
}
