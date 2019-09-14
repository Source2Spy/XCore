
namespace xcore::error::examples
{
    enum class my_error
    {
          GUID = xcore::crc<32>::FromString("xcore::err_unitests::examples").m_Value // GUID for this err err this is better been generated with a constexpr string to CRC32 or something like that
        , OK   = 0                                                                   // Default OK required by the system
        , FAILURE                                                                    // Default FAILURE required by the system (generic failure)
        , SOMETHING                                                                  // Example of a unique error
    };

    //------------------------------------------------------------------------------------
    // Simple function with errors
    //------------------------------------------------------------------------------------
    xcore::err ExampleFunction00()
    {
        int a = 2;
        if (a == 1) return xerr_failure_s( "I have failed");
        if (a == 2) return xerr_code_s( my_error::SOMETHING, "I have created a new code just for fun");
        return {};
    }

    //------------------------------------------------------------------------------------
    // Simple function with a clean up function notice that we use a variable to keep track 
    // when an error is really emitted and so when we have to clean up.
    //------------------------------------------------------------------------------------
    xcore::err ExampleFunction01()
    {
        int a = 2;

        xcore::err Failure;
        xcore::error::scope General(Failure, [&]()
        {
            // Clean up here
        });

        if( a == 1 ) return xerr_failure( Failure, "Some Evil error");
        if( a == 2 ) return xerr_code( Failure, my_error::SOMETHING, "I have created a new code just for fun");

        return Failure;
    }

    //------------------------------------------------------------------------------------
    // Function calling to another function both with clean up
    //------------------------------------------------------------------------------------
    xcore::err ExampleFunction02()
    {
        xcore::err Failure;
        xcore::error::scope General(Failure, [&]()
        {
            // Clean up here
        });

        // Here having a floating Failure also helps to clean the code
        // not need to if( auto Err = ... ; Err.isError() ) ...
        if( ExampleFunction01().isError(Failure) ) 
            return Failure;

        // We can also group several calls into a single if statement
        if(    ExampleFunction01().isError(Failure) 
            || ExampleFunction01().isError(Failure)
            || ExampleFunction01().isError(Failure)
          ) return Failure;

        return Failure;
    }

    //------------------------------------------------------------------------------------
    // Function calling to another function both with clean up
    //------------------------------------------------------------------------------------
    xcore::err_optional<int> ExampleFunction03()
    {
        int a = 2;

        xcore::err Failure;
        xcore::error::scope General(Failure, [&]()
        {
            // Clean up here
        });

        if( a == 1 ) return xerr_failure( Failure, "Some Evil error");
        if( a == 2 ) return xerr_code( Failure, my_error::SOMETHING, "I have created a new code just for fun");

        return a;
    }

    //------------------------------------------------------------------------------------
    // Function calling to another function both with clean up
    //------------------------------------------------------------------------------------
    xcore::err_optional<int> ExampleFunction05()
    {
        xcore::err Failure;
        xcore::error::scope General(Failure, [&]()
        {
            // Clean up here
        });

        // Here having a floating Failure also helps to clean the code
        // not need to if( auto Err = ... ; Err.isError() ) ...
        if( auto v = ExampleFunction03(); v.isError(Failure) ) 
            return Failure;
        else 
            return std::move(v);
    }

    //------------------------------------------------------------------------------------
    // Tests
    //------------------------------------------------------------------------------------
    void Tests( void )
    {
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );
        xcore::log::Output( "Testing: xcore::error\n" );
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );

        // You can also have it scoped to the `if` if you like.
        if( xcore::err Failure; ExampleFunction02().isError(Failure) )
        {
            auto& Code = Failure.getCode();
            printf( "%d %s - Query with the correct type\n", Code.getState<my_error>(), Code.m_pString );
            printf( "%d %s - Query with the incorrect type\n", Code.getState(), Code.m_pString );

            // You are always responsible to clean the error at the end
            // All errors must be clean it forces people to deal with errors
            Failure.clear();
        }

        int a=22;
    }
}
