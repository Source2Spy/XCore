
namespace xcore::guid::examples
{

    void Test( void )
    {
        log::channel Channel("xcore::guid");
        xcore::guid::unit<> Guid1;
        Guid1.Reset();

        XLOG_CHANNEL_INFO( Channel, "%s", Guid1.getStringHex<char>().data() );

        xcore::guid::unit<> Guid2;
        Guid2.setFromStringHex<char>( Guid1.getStringHex<char>() );
        xassert( Guid1 == Guid2 );

        

    }

}