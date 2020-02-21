#ifndef _XCORE_ASSERT_H
#define _XCORE_ASSERT_H
#pragma once

namespace xcore
{
    enum class assert_level : std::uint8_t
    {
          NONE
        , BASIC
        , MEDIUM
        , MAX
    };
}

constexpr static auto ms_xCoreAssertLevel = xcore::assert_level::MEDIUM;

    //------------------------------------------------------------------------------
    // xassert - Basic Assert 
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #if _XCORE_RELEASE
        #define xassert( EXP )  do{ if( !(EXP) ) XCORE_BREAK; } while(false)
    #else
        #define xassert( EXP )  do{ if(EXP){} else{ assert( false ); } } while(false)
    #endif
#else
    #define xassert( EXP )  ((void)(0))
#endif

    //------------------------------------------------------------------------------
    // xassert_basic, ... - Asserts with levels for debugging, most important a then b then c 
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #define xassert_basic( EXP )   [&]{if constexpr( xcore::assert_level::BASIC  >= ms_xCoreAssertLevel ) { assert(EXP); }}() 
    #define xassert_medium( EXP )  [&]{if constexpr( xcore::assert_level::MEDIUM >= ms_xCoreAssertLevel ) { assert(EXP); }}() 
    #define xassert_max( EXP )     [&]{if constexpr( xcore::assert_level::MAX    >= ms_xCoreAssertLevel ) { assert(EXP); }}()
#else
    #define xassert_basic( EXP )   ((void)(0))
    #define xassert_medium( EXP )  ((void)(0))
    #define xassert_max( EXP )     ((void)(0))
#endif

    //------------------------------------------------------------------------------
    // xassume - Similar to assert but tells the compiler to assume the statement while compiling giving faster performance
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #if  _XCORE_COMPILER_VISUAL_STUDIO
        #define xassume(EXP) do{ xassert(EXP); __assume( EXP ); }while(false)
    #else
        #define xassume(EXP) xassert(EXP)
    #endif
#else
    #if  _XCORE_COMPILER_VISUAL_STUDIO
        #define xassume(EXP) __assume(EXP)
    #else
        #define xassume(EXP) ((void)(0))
    #endif
#endif

    //------------------------------------------------------------------------------
    // xassert_block_basic, ... - Assert block allows to create a section for debugging
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #define xassert_block_basic()  if ( ms_xCoreAssertLevel >= xcore::assert_level::BASIC   )
    #define xassert_block_medium() if ( ms_xCoreAssertLevel >= xcore::assert_level::MEDIUM  )
    #define xassert_block_max()    if ( ms_xCoreAssertLevel >= xcore::assert_level::MAX     )
#else
    #define xassert_block_basic()  if constexpr ( false )
    #define xassert_block_medium() if constexpr ( false )
    #define xassert_block_max()    if constexpr ( false )
#endif

    //------------------------------------------------------------------------------
    // xassert_verify - Assert block allows to create a section for debugging
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #define xassert_verify( EXP ) xassert( EXP )
#else
    #define xassert_verify( EXP ) (EXP)
#endif

    //------------------------------------------------------------------------------
    // XCORE_CMD_ASSERT - Simple command for debug
    //------------------------------------------------------------------------------
#if _XCORE_ASSERTS
    #define XCORE_CMD_ASSERT( EXP ) EXP
#else
    #define XCORE_CMD_ASSERT( EXP ) 
#endif

    //------------------------------------------------------------------------------
    // Multicore regions
    //------------------------------------------------------------------------------
    #define xassert_linear(A)
    #define xassert_quantum(A)
struct x_debug_linear_quantum {};
//------------------------------------------------------------------------------
// END
//------------------------------------------------------------------------------
#endif