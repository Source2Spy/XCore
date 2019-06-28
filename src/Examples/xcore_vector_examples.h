

namespace xcore::containers::vector::examples
{
    //--------------------------------------------------------------------------
    void Test01( void )
    {
        xcore::vector<char> X;
        X.append();
    }

    //--------------------------------------------------------------------------
    void Test( void )
    {
        log::channel Channel("xcore::vector");
        Test01();

    
    }
}