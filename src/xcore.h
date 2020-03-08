#ifndef _XCORE_H
#define _XCORE_H
#pragma once

//--------------------------------------------------------------------------------------------
// xCore can be linked in two modes. One is a normal library which means that there is only one common global state
// The other mode is linked like a duplication. Which means that there are two distinct xcore running.
// This is useful for C++ scripting. Where the entire game can be unloaded and reloaded.
//--------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
// C++ Standard headers
//---------------------------------------------------------------------------
#include <thread>
#include <vector>
#include <assert.h>
#include <variant>
#include <string>
#include <array>
#include <tuple>
#include <memory>
#include <atomic>
#include <filesystem>
#include <stdlib.h>
#include <unordered_map>
#include <stdarg.h>
#include <mutex>

#ifdef _MSC_VER 
    #include <malloc.h>         // align_alloc
    #include <intrin.h>
#else
    #include <cstdlib>
#endif

//---------------------------------------------------------------------------
// Predefinitions
//---------------------------------------------------------------------------
#include "xcore_target.h"

namespace xcore
{
    namespace log       { struct logger; }
    namespace context   { struct base;   }
    namespace scheduler { struct local_storage_data { std::uint32_t m_ThreadID; constexpr local_storage_data(std::nullptr_t) : m_ThreadID{~0u}{} }; }
    namespace system    { class registration; }
    namespace file      { class device_i;  void InitSystem( std::vector<std::unique_ptr<device_i>>& ) noexcept; }
}


//---------------------------------------------------------------------------
// including the profiler 
//---------------------------------------------------------------------------
#include "xcore_profiler.h"

//---------------------------------------------------------------------------
// including span 
//---------------------------------------------------------------------------
#define TCB_SPAN_NAMESPACE_NAME std
#include "../dependencies/span/include/tcb/span.hpp"

//---------------------------------------------------------------------------
// including meow hash
//---------------------------------------------------------------------------
#include "../dependencies/meow_hash_cpp/meow_hash.hpp"

//---------------------------------------------------------------------------
// xCore headers
//---------------------------------------------------------------------------
#include "xcore_assert.h"
#include "xcore_error.h"
#include "xcore_span.h"
#include "xcore_array.h"
#include "xcore_unique_span.h"
#include "xcore_vector.h"
#include "xcore_units.h"
#include "xcore_lock.h"
#include "xcore_types.h"
#include "xcore_global.h"
#include "xcore_bits.h"
#include "xcore_context.h"
#include "xcore_crc.h"
#include "xcore_ptr.h"
#include "xcore_endian.h"
#include "xcore_function.h"
#include "xcore_event.h"
#include "xcore_string.h"
#include "xcore_arglist.h"
#include "xcore_guid.h"
#include "xcore_log.h"
#include "xcore_memory.h"

// Text file
namespace xcore::textfile { namespace version_1{} namespace version_2{} using namespace version_2; }
#include "xcore_textfile.h"
#include "xcore_textfile2.h"

// RTTI
#include "xcore_rtti.h"

// Lockless
#include "xcore_lockless_queues.h"
#include "xcore_lockless_pool.h"

// including properties
#include "../dependencies/Properties/src/Properties.h" 
namespace xcore::property{ using namespace ::property; }

// System Registration
#include "xcore_system_reg.h"

// Scheduler
#include "xcore_scheduler_job.h"
#include "xcore_scheduler_trigger.h"
#include "xcore_scheduler_channel.h"
#include "xcore_scheduler_system.h"

// Math
#include "xcore_math.h"
#include "xcore_math_radian3.h"
#include "xcore_math_vector2.h"
#include "xcore_math_vector3.h"
#include "xcore_math_vector4.h"
#include "xcore_math_matrix4.h"
#include "xcore_math_quaternion.h"

#include "xcore_math_transform.h"
#include "xcore_math_shapes.h"
namespace xcore { using namespace xcore::math; }

// Random
#include "xcore_random.h"

// Cmdline
#include "xcore_cmdline.h"

// file
#include "xcore_file.h"

// Compression
#include "xcore_compression.h"

// Serializer
#include "xcore_serializer.h"

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------
#include "Implementation/xcore_string_inline.h"
#include "Implementation/xcore_vector_inline.h"
#include "Implementation/xcore_unqiue_span_inline.h"
#include "Implementation/xcore_textfile2_inline.h"
#include "Implementation/xcore_scheduler_inline.h"

// Math
#include "Implementation/xcore_math_inline.h"
#include "Implementation/xcore_math_radian3_inline.h"
#include "Implementation/xcore_math_vector2_inline.h"
#include "Implementation/xcore_math_vector3_inline.h"
#include "Implementation/xcore_math_vector3d_inline.h"
#include "Implementation/xcore_math_vector4_inline.h"
#include "Implementation/xcore_math_quaternion_inline.h"
#include "Implementation/xcore_math_matrix4_inline.h"

#include "Implementation/xcore_math_rect_inline.h"
#include "Implementation/xcore_math_bbox_inline.h"
#include "Implementation/xcore_math_plane_inline.h"
#include "Implementation/xcore_math_irect_inline.h"

#include "Implementation/xcore_random_inline.h"

#include "Implementation/xcore_cmdline_inline.h"
#include "Implementation/xcore_file_inline.h"

#include "Implementation/xcore_serializer_inline.h"

//---------------------------------------------------------------------------
// END
//---------------------------------------------------------------------------
#endif
