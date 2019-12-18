#ifndef _XCORE_TARGET_H
#define _XCORE_TARGET_H
#pragma once

//------------------------------------------------------------------------------
// Determine the compiler err set it in _XCORE_COMPILER_ ...
//------------------------------------------------------------------------------
#if defined(__clang__)
    #define _XCORE_COMPILER_CLANG true
#elif defined(__GNUC__) || defined(__GNUG__)
    #define _XCORE_COMPILER_GCC true
#elif defined(_MSC_VER)
        #define _XCORE_COMPILER_VISUAL_STUDIO true
    #if _MSC_VER >= 1920
        #define _XCORE_COMPILER_VS_2019 true
    #else
        #define _XCORE_COMPILER_VS_2017 true
    #endif
#endif

//------------------------------------------------------------------------------
// Determine the platform err set it in _XCORE_PLATFORM_ ...
//------------------------------------------------------------------------------
#ifdef _WIN32
    #define _XCORE_PLATFORM_WINDOWS true
#elif __APPLE__
    #if TARGET_IPHONE_SIMULATOR || TARGET_IPHONE
        #define _XCORE_PLATFORM_IOS true
    #elif TARGET_MAC
        #define _XCORE_PLATFORM_MAC true
    #else 
        #error "Unkown Apple platform. Please make sure you define in your C++ compiler TARGET_IPHONE_SIMULATOR or TARGET_IPHONE"
    #endif
#elif defined __linux__ || defined __unix__
    #define _XCORE_PLATFORM_MAC_LINUX true
#else 
    // TODO: Handle the android platform
    #error "Unkown platform!"
#endif

//------------------------------------------------------------------------------
// Main definitions
//------------------------------------------------------------------------------
namespace xcore::target
{
    enum class compiler : unsigned char
    {
          VS_2017           // for #ifs use: _XCORE_COMPILER_VISUAL_STUDIO
        , VS_2019           // for #ifs use: _XCORE_COMPILER_VISUAL_STUDIO
        , CLANG             // for #ifs use: _XCORE_COMPILER_CLANG even if it has LLVM backend
        , GCC               // for #ifs use: _XCORE_COMPILER_GCC
    };

    enum class platform : unsigned char
    {
          FIRST
        , WINDOWS = FIRST   // for #ifs use: _XCORE_TARGET_WINDOWS
        , MAC               // for #ifs use: _XCORE_TARGET_MAC
        , IOS               // for #ifs use: _XCORE_TARGET_IOS
        , LINUX             // for #ifs use: _XCORE_TARGET_LINUS
        , ANDROID           // for #ifs use: _XCORE_TARGET_ANDROID
        , COUNT 
    };

    enum class chipset : unsigned char
    {
          INTEL             // for #ifs use: _XCORE_TARGET_INTEL
        , ARM               // for #ifs use: _XCORE_TARGET_ARM
    };
    
    enum class endian : unsigned char
    {
          LITTLE            // for #ifs use: _XCORE_ENDIAN_LITTLE
        , BIG               // for #ifs use: _XCORE_ENDIAN_BIG
    };
    
    // X_TARGET_BITS - will be assigned at compile time one of the following
    enum class bits : unsigned char
    {
          b64 = 64          // for #ifs use: _XCORE_TARGET_64BITS
        , b32 = 32          // for #ifs use: _XCORE_TARGET_32BITS
    };
    
    // Constant expressions functions to find out detail of the about the target been build
    constexpr compiler      getCompiler         ( void )                    noexcept;
    constexpr unsigned      getCacheLineSize    ( void )                    noexcept 
    { 
        #if _XCORE_PLATFORM_WINDOWS || _XCORE_PLATFORM_MAC_LINUX
            return 128;
        #else
            return 64; 
        #endif
    }
    constexpr platform      getPlatform         ( void )                    noexcept;
    constexpr chipset       getChipset          ( void )                    noexcept;
    constexpr endian        getEndian           ( void )                    noexcept;
    constexpr bits          getBits             ( void )                    noexcept;
    constexpr auto          getPlatformCount    ( void )                    noexcept { return static_cast<int>(platform::COUNT); }
    constexpr bool          isDebug             ( void )                    noexcept;

    namespace details
    {
        constexpr static std::array<const char*,static_cast<int>(platform::COUNT)> Strings =
        { 
              "WINDOWS" 
            , "MAC" 
            , "IOS"
            , "LINUX" 
            , "ANDROID" 
        };
    }

    constexpr const char*   getPlatformString   ( const platform Platform ) noexcept
    {
        return details::Strings[ static_cast<int>(Platform) ];
    }
}

//------------------------------------------------------------------------------
// Determine the compiler
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_CLANG
constexpr xcore::target::compiler xcore::target::getCompiler( void ) noexcept { return xcore::target::compiler::CLANG; }
#elif _XCORE_COMPILER_GCC
constexpr xcore::target::compiler xcore::target::getCompiler( void ) noexcept { return xcore::target::compiler::GCC; }
#elif _XCORE_COMPILER_VS_2017
constexpr xcore::target::compiler xcore::target::getCompiler( void ) noexcept { return xcore::target::compiler::VS_2017; }
#elif _XCORE_COMPILER_VS_2019
constexpr xcore::target::compiler   xcore::target::getCompiler      ( void ) noexcept { return xcore::target::compiler::VS_2019; }
#else
    #error "Unkown compiler type"
#endif 

//------------------------------------------------------------------------------
// Determine the platform details
//------------------------------------------------------------------------------
#if _XCORE_PLATFORM_WINDOWS
    constexpr xcore::target::platform   xcore::target::getPlatform      ( void ) noexcept { return xcore::target::platform::WINDOWS; }
    constexpr xcore::target::chipset    xcore::target::getChipset       ( void ) noexcept { return xcore::target::chipset::INTEL;    }
    constexpr xcore::target::endian     xcore::target::getEndian        ( void ) noexcept { return xcore::target::endian::LITTLE;    }
    #define _XCORE_ENDIAN_LITTLE     true
    #define _XCORE_CHIPSET_INTEL     true
    #define _XCORE_SSE4              true
#elif _XCORE_PLATFORM_IOS
    constexpr xcore::target::platform   xcore::target::getPlatform      ( void ) noexcept { return xcore::target::platform::IOS;     }
    constexpr xcore::target::chipset    xcore::target::getChipset       ( void ) noexcept { return xcore::target::chipset::ARM;      }
    constexpr xcore::target::endian     xcore::target::getEndian        ( void ) noexcept { return xcore::target::endian::LITTLE;    }
    #define _XCORE_ENDIAN_LITTLE     true
    #define _XCORE_CHIPSET_ARM       true
#elif _XCORE_PLATFORM_MAC
    constexpr xcore::target::platform   xcore::target::getPlatform      ( void ) noexcept { return xcore::target::platform::MAC;     }
    constexpr xcore::target::chipset    xcore::target::getChipset       ( void ) noexcept { return xcore::target::chipset::INTEL;    }
    constexpr xcore::target::endian     xcore::target::getEndian        ( void ) noexcept { return xcore::target::endian::LITTLE;    }
    #define _XCORE_ENDIAN_LITTLE     true
    #define _XCORE_CHIPSET_INTEL     true
    #define _XCORE_SSE4              true
#elif _XCORE_PLATFORM_LINUX
    constexpr xcore::target::platform   xcore::target::getPlatform      ( void ) noexcept { return xcore::target::platform::LINUX;   }
    constexpr xcore::target::chipset    xcore::target::getChipset       ( void ) noexcept { return xcore::target::chipset::INTEL;    }
    constexpr xcore::target::endian     xcore::target::getEndian        ( void ) noexcept { return xcore::target::endian::LITTLE;    }
    #define _XCORE_ENDIAN_LITTLE     true
    #define _XCORE_CHIPSET_INTEL     true
    #define _XCORE_SSE4              true
#elif _XCORE_PLATFORM_ANDROID
    constexpr xcore::target::platform   xcore::target::getPlatform      ( void ) noexcept { return xcore::target::platform::ANDROID; }
    constexpr xcore::target::chipset    xcore::target::getChipset       ( void ) noexcept { return xcore::target::chipset::INTEL;    }
    constexpr xcore::target::endian     xcore::target::getEndian        ( void ) noexcept { return xcore::target::endian::LITTLE;    }
    #define _XCORE_ENDIAN_LITTLE     true
    #define _XCORE_CHIPSET_INTEL     true
#else 
    #error "Unkown platform type"
#endif

//------------------------------------------------------------------------------
// Determine if we are compiling for 64bits or 32bits
//------------------------------------------------------------------------------
#if defined(_WIN64) || defined(__x86_64__)
    constexpr xcore::target::bits xcore::target::getBits( void ) noexcept { return xcore::target::bits::b64; }
    #define _XCORE_TARGET_64BITS            true
#else
    constexpr xcore::target::bits xcore::target::getBits( void ) noexcept { return xcore::target::bits::b32; }
    #define _XCORE_TARGET_32BITS            true
#endif

//------------------------------------------------------------------------------
// Determine configuration (debug/release)
//------------------------------------------------------------------------------
#if defined(_DEBUG) || defined(DEBUG)
    #define _XCORE_DEBUG             true
    constexpr bool xcore::target::isDebug( void ) noexcept { return true; }
#else
    #define _XCORE_RELEASE           true
    constexpr bool xcore::target::isDebug( void ) noexcept { return false; }
#endif

//------------------------------------------------------------------------------
// Include the user settings
//------------------------------------------------------------------------------
#include "xcore_user_settings.h"

//------------------------------------------------------------------------------
// Enable the profiler if the user request it
//------------------------------------------------------------------------------
#ifdef XCORE_USER_SETTINGS_PROFILE
    #define _XCORE_PROFILE
#endif

//------------------------------------------------------------------------------
// Determine if we need to turn on or off asserts
//------------------------------------------------------------------------------
#if _XCORE_DEBUG || XCORE_USER_SETTINGS_ASSERTS
    #define _XCORE_ASSERTS          true
#else
    #define _XCORE_ASSERTS          false
#endif

//------------------------------------------------------------------------------
// Additional includes for each platform
//------------------------------------------------------------------------------
#if _XCORE_SSE4
    #include <smmintrin.h>     //SSE4

    namespace xcore
    {
        using floatx4     = __m128;

        namespace sse
        {
            template<unsigned i> constexpr
            float getElementByIndex( floatx4 V ) noexcept
            {
                static_assert( i>=0 && i<=4, "Index out of range" );
                #if defined __SSE4_1__ || defined __SSE4_2__ 
                    return _mm_extract_epi32(V, i);
                #else
                    // shuffle V so that the element that you want is moved to the least-
                    // significant element of the vector (V[0])
                    // return the value in V[0]
                    return _mm_cvtss_f32( _mm_shuffle_ps(V, V, _MM_SHUFFLE(i, i, i, i)) );
                #endif
            }
        }
    }

#else
    namespace sse
    {
        union floatx4
        {
            struct { float x, y, z, w; };
            float m[4];
        };

        namespace sse
        {
            template<unsigned i> constexpr
            float getElementByIndex( floatx4 V ) noexcept
            {
                static_assert( i>=0 && i<=4, "Index out of range" );
                return V.m[i];
            }
        }
    }
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #pragma warning( disable : 4201) // nonstandard extension used : nameless struct / union
#elif _XCORE_COMPILER_CLANG
#else  
#endif

//------------------------------------------------------------------------------
// The macro BREAK will cause a debugger breakpoint if possible on any given 
// platform.  If a breakpoint cannot be caused, then a divide by zero will be 
// forced.  Note that the BREAK macro is highly platform specific.  The 
// implementation of BREAK on some platforms prevents it from being used 
// syntactically as an expression.  It can only be used as a statement.
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #define XCORE_BREAK         { __debugbreak(); }
#elif _XCORE_COMPILER_CLANG
    #if _XCORE_PLATFORM_IOS
        // http://iphone.m20.nl/wp/2010/10/xcode-iphone-debugger-halt-assertions/
        #define XCORE_BREAK     { __builtin_trap(); }
    #else
        #define XCORE_BREAK abort();//__asm__("int $3")
    #endif 
#else  
    // Generic BREAK to be used if no proper version can be created.
    extern volatile s32 g_xcoreDDBZ;   // Debug Divide By Zero
    #define X_BREAK      {g_xcoreDDBZ=0;g_xcoreDDBZ=1/g_xcoreDDBZ;}
#endif

//------------------------------------------------------------------------------
// Sets a NOP operation in the code assembly. This is commonly used for 
// debugging. By adding nops allows to see the assembly clearly in code.
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #if _XCORE_TARGET_64BITS
        #define XCORE_NOP        __nop() 
    #else
        #define XCORE_NOP       { __asm nop   } //{ __emit(0x00000000); }
    #endif
#else 
    #define XCORE_NOP           { __asm nop   }
#endif

//------------------------------------------------------------------------------
// Simple command for debug
//------------------------------------------------------------------------------
#if _XCORE_DEBUG
    #define XCORE_CMD_DEBUG(EXP) EXP
#else
    #define XCORE_CMD_DEBUG(EXP)
#endif

//------------------------------------------------------------------------------
// Compilers micro optimizations hints for conditional branching 
// http://stackoverflow.com/questions/1440570/likely-unlikely-equivalent-for-msvc
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_GCC
    #define xif_likely(expr)     if( __builtin_expect(!(expr), 0 ) )
    #define xif_unlikely(expr)   if( __builtin_expect((expr),  0 ) )
#else
    #define xif_unlikely(expr)   if (!(expr)); else 
    #define xif_likely(expr)     if (expr)
#endif

//------------------------------------------------------------------------------
// xforceinline - Tells the compiler to just inline the function don't think about it
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #define xforceinline       __forceinline
    #if _XCORE_RELEASE
        #pragma inline_recursion( on )
        #pragma inline_depth( 255 )
    #endif
#elif _XCORE_COMPILER_CLANG
    #define xforceinline       __attribute__((always_inline))
#elif _XCORE_COMPILER_GCC
    #define xforceinline       __attribute__((forceinline))
#else
    #error "Missing compiler"
#endif

//------------------------------------------------------------------------------
// xnoinline - Please don't inline this function
//------------------------------------------------------------------------------
#if _XCORE_COMPILER_VISUAL_STUDIO
    #define xnoinline          __declspec(noinline)
#elif _XCORE_COMPILER_CLANG
    #define xnoinline          __attribute__((__noinline__))
#elif _XCORE_COMPILER_GCC
    #define xnoinline          __attribute__((noinline))
#else
    #error "Missing compiler"
#endif

//------------------------------------------------------------------------------
// Some messages
//------------------------------------------------------------------------------
#define __X_STRINGIFY( X ) #X
#define X_STRINGIFY( X ) __X_STRINGIFY( X )

#if _XCORE_COMPILER_VISUAL_STUDIO
    // More information: https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019 
    #ifdef _CPPRTTI
 //       #pragma message( __FILE__ "[" X_STRINGIFY(__LINE__) "]:" "xCore Warning: C++ RTTI is enable, may generate slower code and take more memory than required")
    #endif
    #ifdef _CPPUNWIND
 //       #pragma message( __FILE__ "[" X_STRINGIFY(__LINE__) "]:" "xCore Warning: C++ Exceptions are enable, may generate slower and take more memory than required")
    #endif
#endif

//------------------------------------------------------------------------------
// PROFILE
//------------------------------------------------------------------------------
#ifdef _XCORE_PROFILE
    #define TRACY_ENABLE
#endif


// Provide a generic tag that allows constructor to generate a default
namespace xcore
{
    struct not_null_t {};
    constexpr static not_null_t not_null {};
}

//------------------------------------------------------------------------------
// END
//------------------------------------------------------------------------------
#endif
