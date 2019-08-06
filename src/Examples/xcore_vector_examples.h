

namespace xcore::containers::vector::examples
{
    struct test
    {
        const char* m_pName    {"Hello world"};
        int         m_Number   { 22 };
    };

    //--------------------------------------------------------------------------
    void Test02( void )
    {
        xcore::vector<test> X;
        X.append();
        auto Y = X.View();

        xassert( Y[0].m_Number == 22 );
    }

    //--------------------------------------------------------------------------
    void Test01( void )
    {
        xcore::vector<int> X;
        for( int i=0; i< 100000; i++ )
            X.append(i);
    }

    //--------------------------------------------------------------------------
    void Test( void )
    {
        log::channel Channel("xcore::vector");
        Test01();
        Test02();

    
    }
}