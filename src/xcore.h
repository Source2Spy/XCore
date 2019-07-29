#ifndef _X_CORE_H
#define _X_CORE_H
#pragma once

//---------------------------------------------------------------------------
// System headers
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

//---------------------------------------------------------------------------
// Predefinitions
//---------------------------------------------------------------------------
namespace xcore
{
    namespace log       { struct logger; }
    namespace context   { struct base;   }
}

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
#include "xcore_target.h"
#include "xcore_assert.h"
#include "xcore_error.h"
#include "xcore_span.h"
#include "xcore_array.h"
#include "xcore_unique_span.h"
#include "xcore_vector.h"
#include "xcore_units.h"
#include "xcore_lock.h"
#include "xcore_global.h"
#include "xcore_types.h"
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
namespace xcore::textfile { namespace version_1{} namespace version_2{} using namespace version_2; }
#include "xcore_textfile.h"
#include "xcore_textfile2.h"
#include "xcore_rtti.h"

//---------------------------------------------------------------------------
// including properties
//---------------------------------------------------------------------------
#include "../dependencies/Properties/src/Properties.h" 
namespace xcore::property{ using namespace ::property; }

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------
#include "Implementation/xcore_string_inline.h"
#include "Implementation/xcore_vector_inline.h"
#include "Implementation/xcore_textfile2_inline.h"

#endif
