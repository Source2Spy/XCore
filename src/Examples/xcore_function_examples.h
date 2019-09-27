
namespace xcore::function::examples
{
    //---------------------------------------------------------------------------------------------------------
    // Test class members functions
    //---------------------------------------------------------------------------------------------------------
    struct test
    {
        void Example01( void )                              { }
        void Example02( void ) noexcept                     { }

        void Example03( void ) const                        { }
        void Example04( void ) const noexcept               { }

        float Example06( int, float )                       { return 0;}
        float Example07( int, float ) noexcept              { return 0;}

        float Example08( int, float ) const                 { return 0;}
        float Example09( int, float ) const noexcept        { return 0;}
    };

    static constexpr auto Example01_Args = xcore::function::traits<decltype(&test::Example01)>::arg_count_v;
    static constexpr auto Example02_Args = xcore::function::traits<decltype(&test::Example02)>::arg_count_v;
    static constexpr auto Example03_Args = xcore::function::traits<decltype(&test::Example03)>::arg_count_v;
    static constexpr auto Example04_Args = xcore::function::traits<decltype(&test::Example04)>::arg_count_v;
    static_assert( Example01_Args == 0, "" ); 
    static_assert( Example02_Args == 0, "" );
    static_assert( Example03_Args == 0, "" );
    static_assert( Example04_Args == 0, "" );

    static constexpr auto Example06_Args = xcore::function::traits<decltype(&test::Example06)>::arg_count_v;
    static constexpr auto Example07_Args = xcore::function::traits<decltype(&test::Example07)>::arg_count_v;
    static constexpr auto Example08_Args = xcore::function::traits<decltype(&test::Example08)>::arg_count_v;
    static constexpr auto Example09_Args = xcore::function::traits<decltype(&test::Example09)>::arg_count_v;
    static_assert( Example06_Args == 2, "" ); 
    static_assert( Example07_Args == 2, "" );
    static_assert( Example08_Args == 2, "" );
    static_assert( Example09_Args == 2, "" );

    //---------------------------------------------------------------------------------------------------------
    // Test regular functions
    //---------------------------------------------------------------------------------------------------------
    void    Example10   ( void )                            { }
    void    Example11   ( void ) noexcept                   { }
    float   Example12   ( int, float )                      { return 0; }
    float   Example13   ( int, float ) noexcept             { return 0; }

    static constexpr auto Example10_Args  = xcore::function::traits<decltype(&Example10)>::arg_count_v;
    static constexpr auto Example11_Args  = xcore::function::traits<decltype(&Example11)>::arg_count_v;
    static constexpr auto Example12_Args  = xcore::function::traits<decltype(&Example12)>::arg_count_v;
    static constexpr auto Example13_Args  = xcore::function::traits<decltype(&Example13)>::arg_count_v;
    static constexpr auto Example10b_Args = xcore::function::traits<decltype(Example10)>::arg_count_v;
    static constexpr auto Example11b_Args = xcore::function::traits<decltype(Example11)>::arg_count_v;
    static constexpr auto Example12b_Args = xcore::function::traits<decltype(Example12)>::arg_count_v;
    static constexpr auto Example13b_Args = xcore::function::traits<decltype(Example13)>::arg_count_v;
    static_assert( Example10_Args == 0, "" );
    static_assert( Example11_Args == 0, "" );
    static_assert( Example12_Args == 2, "" );
    static_assert( Example13_Args == 2, "" );
    static_assert( Example10b_Args == 0, "" );
    static_assert( Example11b_Args == 0, "" );
    static_assert( Example12b_Args == 2, "" );
    static_assert( Example13b_Args == 2, "" );

    //---------------------------------------------------------------------------------------------------------
    // Test lambdas
    //---------------------------------------------------------------------------------------------------------
    static constexpr auto Example14 = [](){};
    static constexpr auto Example15 = []() noexcept {};

    static constexpr auto Example14_Args  = xcore::function::traits<decltype(Example14)>::arg_count_v;
    static constexpr auto Example15_Args  = xcore::function::traits<decltype(Example15)>::arg_count_v;
    static constexpr auto Example14b_Args = xcore::function::traits<decltype(&Example14)>::arg_count_v;
    static constexpr auto Example15b_Args = xcore::function::traits<decltype(&Example15)>::arg_count_v;
    static_assert( Example14_Args  == 0, "" );
    static_assert( Example15_Args  == 0, "" );
    static_assert( Example14b_Args == 0, "" );
    static_assert( Example15b_Args == 0, "" );

    //---------------------------------------------------------------------------------------------------------
    // Test function pointers and function pointers to lambdas
    //---------------------------------------------------------------------------------------------------------
    static constexpr void(* Example16 )(void) = +[](){};
    static constexpr auto Example17 = +[]() noexcept {};

    static constexpr auto Example16_Args   = xcore::function::traits<decltype(*Example16)>::arg_count_v;
    static constexpr auto Example17_Args   = xcore::function::traits<decltype(*Example17)>::arg_count_v;
    static_assert( Example14_Args  == 0, "" );
    static_assert( Example15_Args  == 0, "" );

    //---------------------------------------------------------------------------------------------------------
    // Run tests
    //---------------------------------------------------------------------------------------------------------
    void Tests( void )
    {
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );
        xcore::log::Output( "Testing: xcore::function\n" );
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );
        
    }
}
