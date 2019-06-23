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
// 3RDParty
//---------------------------------------------------------------------------

// Src from: https://github.com/tcbrindle/span
#define TCB_SPAN_NAMESPACE_NAME std
#include "Implementation/3rdparty/span.hpp"


//---------------------------------------------------------------------------
// Predefinitions
//---------------------------------------------------------------------------
namespace xcore
{
    namespace log       { struct logger; }
    namespace context   { struct base;   }
}

//---------------------------------------------------------------------------
// xCore headers
//---------------------------------------------------------------------------
#include "xcore_target.h"
#include "xcore_span.h"
#include "xcore_assert.h"
#include "xcore_array.h"
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
#include "xcore_error.h"
#include "xcore_function.h"
#include "xcore_event.h"
#include "xcore_string.h"
#include "xcore_arglist.h"
#include "Implementation/meow_hash.hpp"
#include "xcore_guid.h"
#include "xcore_log.h"
namespace xcore::textfile { namespace version_1{} namespace version_2{} using namespace version_1; }
#include "xcore_textfile.h"
#include "xcore_textfile2.h"
#include "xcore_rtti.h"

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------
#include "Implementation/xcore_string_inline.h"


#endif
