
//-------------------------------------------------------------------------------
// Disable some warnings from Visual Studio
//-------------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS

//-------------------------------------------------------------------------------
// xBase basics
//-------------------------------------------------------------------------------
#include <array>
#include "xcore_target.h"

//-------------------------------------------------------------------------------
// Tracy files
//-------------------------------------------------------------------------------
#include "../dependencies/tracy/common/tracy_lz4.cpp"
#include "../dependencies/tracy/common/tracy_lz4hc.cpp"
#include "../dependencies/tracy/client/tracyDXT1.cpp"

#include "../dependencies/tracy/client/tracy_rpmalloc.cpp"
#include "../dependencies/tracy/client/tracySysTime.cpp"
#include "../dependencies/tracy/client/tracyCallstack.cpp"
#include "../dependencies/tracy/common/tracySystem.cpp"

//--------------------------------------------------------------------------------
// Adding memory profiling
//--------------------------------------------------------------------------------
#ifdef max
    #undef max
#endif
#include "xcore_profiler.h"

#ifdef _XCORE_PROFILE

    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Dbghelp.lib")

    // "old" unaligned overloads
    void* operator new(std::size_t size) 
    {
        auto ptr = malloc(size);
        XCORE_PROF_ALLOC_S( ptr, size, 10 );
        return ptr ? ptr : throw std::bad_alloc{};
    }

    void operator delete(void* ptr, std::size_t size) 
    {
        free(ptr);
        XCORE_PROF_FREE_S( ptr, 10 );
    }

    void operator delete(void* ptr) 
    {
        free(ptr);
        XCORE_PROF_FREE_S( ptr, 10 );
    }

    // "new" over-aligned overloads
    void* operator new(std::size_t size, std::align_val_t align) 
    {
        auto ptr = _aligned_malloc(size, static_cast<std::size_t>(align));
        XCORE_PROF_ALLOC_S( ptr, size, 10 );
        return ptr ? ptr : throw std::bad_alloc{};
    }

    void operator delete(void* ptr, std::size_t size, std::align_val_t align) 
    {
        _aligned_free(ptr);
        XCORE_PROF_FREE_S( ptr, 10 );
    }

    void operator delete(void* ptr, std::align_val_t align) 
    {
        _aligned_free(ptr);
        XCORE_PROF_FREE_S( ptr, 10 );
    }
#endif