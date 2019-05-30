
#include "../xcore.h"
#include "xcore_error_examples.h"
#include "xcore_function_examples.h"
#include "xcore_log_examples.h"
#include "xcore_string_examples.h"
#include "xcore_textfile_examples.h"
#include "xcore_guid_examples.h"


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
        }
    
        xcore::guid::Test();

        xcore::textfile::examples::Test();
    }
}