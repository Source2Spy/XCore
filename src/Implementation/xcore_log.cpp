
#include "xcore_target.h"
#if _XCORE_COMPILER_VISUAL_STUDIO   
    #include "windows.h"
    #undef ERROR
#endif

#include "xcore_log.h"
#include "xcore_context.h"
#include <iostream>
#include <stdarg.h>

//-------------------------------------------------------------------------------------------------------------------

void xcore::log::Output( const char* pMsg, ... ) noexcept
{
    thread_local static std::array<char,1024>   Buffer;
    std::size_t                                 nBytesWritten = 0;

    // Format string to the buffer
    {
        va_list args;
        va_start(args, pMsg);
        nBytesWritten = vsnprintf( Buffer.data(), Buffer.size(), pMsg, args );    
        va_end( args );
    }

//    Buffer[nBytesWritten++] = '\n';
//    Buffer[nBytesWritten++] = 0;

    // Output to the standard error
    //perror( Buffer.data() );

    // Output to the console
    printf( Buffer.data() );

    // Output to the debug window
    #if _XCORE_COMPILER_VISUAL_STUDIO
    //std::array<WCHAR,1024> szBuff;
    //for (int i = 0; !!((szBuff[i] = Buffer[i])); i++);
    OutputDebugString( Buffer.data() );
    #endif
}

//-------------------------------------------------------------------------------------------------------------------
static 
void DefaultOutput( const xcore::log::channel& Channel, xcore::log::msg_type Type, const char* pString, int Line, const char* pFile ) noexcept
{
    constexpr std::array pType = { "INFO", "WARNING", "ERROR" };
    xcore::log::Output( "[%s][%s][Top Context/%s] %s (%d): %s\n"
        , pType[static_cast<int>(Type)]
        , Channel.getName().c_str()
        , xcore::context::get() ? xcore::context::get()->getFullName().c_str() : ""
        , pFile
        , Line
        , pString );
}

//-------------------------------------------------------------------------------------------------------------------

xcore::log::logger::logger( void ) noexcept
    : m_pOutput{ DefaultOutput }
    {}
