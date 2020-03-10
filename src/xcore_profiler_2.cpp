//-------------------------------------------------------------------------------
// Disable some warnings from Visual Studio
//-------------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS

//-------------------------------------------------------------------------------
// xBase basics
//-------------------------------------------------------------------------------
#include <array>
#include "xcore_target.h"

#ifdef _XCORE_PROFILE
    //-------------------------------------------------------------------------------
    // Tracy files
    //-------------------------------------------------------------------------------
    #ifdef _WIN32
    #  include <winsock2.h>
    #  include <ws2tcpip.h>
    #endif

    #include "../dependencies/tracy/common/tracySocket.cpp"
    #include "../dependencies/tracy/client/tracyProfiler.cpp"

#endif

