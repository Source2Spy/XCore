
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
    #include "../dependencies/tracy/public/common/tracy_lz4.cpp"
    #include "../dependencies/tracy/public/common/tracy_lz4hc.cpp"
    #include "../dependencies/tracy/public/client/tracyDXT1.cpp"

    #include "../dependencies/tracy/public/client/tracy_rpmalloc.cpp"
    #include "../dependencies/tracy/public/client/tracySysTime.cpp"
    #include "../dependencies/tracy/public/client/tracyCallstack.cpp"
    #include "../dependencies/tracy/public/client/TracyAlloc.cpp"
    #ifdef min
    #   undef min
    #endif

    #include "../dependencies/tracy/public/client/tracySysTrace.cpp"
    #include "../dependencies/tracy/public/common/tracySystem.cpp"

    //--------------------------------------------------------------------------------
    // Adding memory profiling
    //--------------------------------------------------------------------------------
    #ifdef max
    #   undef max
    #endif
    #include "xcore_profiler.h"

    //--------------------------------------------------------------------------------
    // Include required libraries
    //--------------------------------------------------------------------------------
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Dbghelp.lib")

#endif

