
#include "../xcore.h"
#include "xcore_error_examples.h"
#include "xcore_function_examples.h"
#include "xcore_log_examples.h"
#include "xcore_string_examples.h"
#include "xcore_textfile_examples.h"
#include "xcore_guid_examples.h"
#include "xcore_rtti_examples.h"
#include "xcore_vector_examples.h"
#include "xcore_properties.h"
#include "xcore_lockless_queue.h"
#include "xcore_unique_span.h"
#include "xcore_scheduler_examples.h"
#include "xcore_math_vector3d_examples.h"
#include "xcore_cmdline_example.h"
#include "xcore_file_example.h"

namespace xcore::examples
{
    void Test( void )
    {
        if(0)
        {
            xcore::string::examples::Tests();
            xcore::error::examples::Tests();
            xcore::log::examples::Tests();
            xcore::function::examples::Tests();
            xcore::arglist::out Arg( 123, 1.5, "hello" );
            xcore::rtti::examples::Test();
            xcore::guid::examples::Test();
            xcore::containers::vector::examples::Test();
            xcore::textfile::examples::Test();
            xcore::property::examples::Test();
            xcore::allocator::examples::Test();
            xcore::containers::vector::examples::Test();
            xcore::lockless::queues::examples::Test();
            xcore::math::examples::vector3s::Test();
            xcore::scheduler::examples::Test();
            xcore::cmdline::examples::Test();
        }

        xcore::file::examples::Tests();
    }
}