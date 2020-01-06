
namespace xcore::guid::examples
{
    //---------------------------------------------------------------------------------------

    void TestTimming( log::channel&  )
    {
    
    }

    //---------------------------------------------------------------------------------------

    void Test( void )
    {
        log::channel Channel("xcore::guid");
        xcore::guid::unit<> Guid1;
        Guid1.Reset();

        XLOG_CHANNEL_INFO( Channel, "%s", Guid1.getStringHex<char>().data() );

        xcore::guid::unit<> Guid2;
        Guid2.setFromStringHex<char>( Guid1.getStringHex<char>() );
        xassert( Guid1 == Guid2 );

        static constexpr xcore::guid::unit<128> Guid4( "Hello World" );
        xcore::guid::unit<128> Guid3( "Hello World" );
        Guid3.Reset();

        XLOG_CHANNEL_INFO( Channel, "%s", Guid3.getStringHex<char>().data() );
    }

}