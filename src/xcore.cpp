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



//--------------------------------------------------------------------------------
// Adding tracy 
//--------------------------------------------------------------------------------
#ifdef TRACY_ENABLE

    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Dbghelp.lib")

    // "old" unaligned overloads
    void* operator new(std::size_t size) 
    {
        auto ptr = malloc(size);
        TracyAllocS( ptr, size, 10 );
        return ptr ? ptr : throw std::bad_alloc{};
    }

    void operator delete(void* ptr, std::size_t size) 
    {
        free(ptr);
        TracyFreeS( ptr, 10 );
    }

    void operator delete(void* ptr) 
    {
        free(ptr);
        TracyFreeS( ptr, 10 );
    }

    // "new" over-aligned overloads
    void* operator new(std::size_t size, std::align_val_t align) 
    {
        auto ptr = _aligned_malloc(size, static_cast<std::size_t>(align));
        TracyAllocS( ptr, size, 10 );
        return ptr ? ptr : throw std::bad_alloc{};
    }

    void operator delete(void* ptr, std::size_t size, std::align_val_t align) 
    {
        _aligned_free(ptr);
        TracyFreeS( ptr, 10 );
    }

    void operator delete(void* ptr, std::align_val_t align) 
    {
        _aligned_free(ptr);
        TracyFreeS( ptr, 10 );
    }
#endif