
namespace xcore::log::examples
{
    //------------------------------------------------------------------------------------------

    void FunctionCall( xcore::log::channel& Channel )
    {
        xcore::context::scope MyContext("Some Context");
        XLOG_CHANNEL_INFO( Channel, "Context are handy ways to add meaning to different systems" );
    }

    //------------------------------------------------------------------------------------------

    void Tests( void )
    {
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );
        xcore::log::Output( "Testing: xcore::log\n" );
        xcore::log::Output( "---------------------------------------------------------------------------------------\n" );

        xcore::log::Output( "This is a Simple Message, " );
        xcore::log::Output( "Messages can also have parameters %s.", "LIKE ME" );
        xcore::log::Output( " But please note that Messages are responsible to start a new line\n" );

        xcore::log::channel Channel( "Test Channel" );

        XLOG_CHANNEL_ERROR( Channel, "Example of logging an error (%s)", "You can add parameters as well" );

        XLOG_CHANNEL_WARNING( Channel, "Example of logging an warning, not all messages have to have parameters" );

        XLOG_CHANNEL_INFO( Channel, "Infos just give general information to the user..." );

        FunctionCall( Channel );
    }
}

