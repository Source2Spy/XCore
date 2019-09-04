#include "xcore.h"

#if _XCORE_COMPILER_VISUAL_STUDIO   
    #include "windows.h"
    #undef ERROR
    #undef max
#endif

#include <iostream>
#include <stdarg.h>
#include <algorithm>
#include <iterator>

#include "Implementation/xcore_global.cpp"
#include "Implementation/xcore_log.cpp"
//#include "Implementation/xcore_textfile.cpp"
#include "Implementation/xcore_textfile2.cpp"
#include "Implementation/xcore_sprintf.cpp"
#include "Implementation/xcore_scheduler.cpp"

#include "Implementation/xcore_cmdline.cpp"

#include "Implementation/xcore_file.cpp"

//--------------
#define ZSTD_STATIC_LINKING_ONLY
#include "../../dependencies/zstd/lib/zstd.h"

namespace xcore::compression::dictionary
{
    //-------------------------------------------------------------------------------------------------------

    xcore::err compress::Init( std::uint32_t BlockSize, const std::span<std::byte> SourceUncompress ) noexcept
    {
        xassert(!m_pCCTX);
        xassert(!m_pDCTX);
        xcore::err Error;
        m_pCCTX     = (void*)ZSTD_createCCtx();
        if(!m_pCCTX)  return xerr_failure(Error, "Error ZSTD_createCCtx");

        m_pDCTX     = (void*)ZSTD_createDCtx();
        if (!m_pCCTX)  return xerr_failure(Error, "Error ZSTD_createDCtx");

        m_Src       = SourceUncompress;
        m_BlockSize = BlockSize;
        xassert((m_Src.size() % BlockSize) == 0 );
        
        return Error;
    }

    //-------------------------------------------------------------------------------------------------------

    xcore::err  compress::Pack( std::uint32_t& CompressedSize, std::span<std::byte> Destination ) noexcept
    {
        xassert(((m_Position + m_BlockSize) <= m_Src.size()));

        xcore::err              Error;
        const ZSTD_EndDirective end = ((m_Position + m_BlockSize) == m_Src.size()) ? ZSTD_e_end : ZSTD_e_continue;
        ZSTD_inBuffer           in  = { m_Src.data(), m_Src.size(), m_Position };
        ZSTD_outBuffer          out = { Destination.data(), Destination.size(), 0 };
        std::size_t             rc  = ZSTD_compressStream2( (ZSTD_CCtx*)m_pCCTX, &out, &in, end);
        if (ZSTD_isError(rc))
            return xerr_failure(Error, "Error while compresing");

        CompressedSize = static_cast<std::uint32_t>(rc);
        if (end != ZSTD_e_end) return xerr_code(Error, error_state::NOT_DONE, "Not done");
        
        return Error;
    }
}

#include "../../dependencies/zstd/lib/common/zstd_common.c"


