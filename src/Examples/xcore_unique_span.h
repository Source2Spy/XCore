

namespace xcore::allocator::examples
{

    void Simple( void )
    {
        xcore::unique_span<int> X;

        X.New( 100 ).CheckError();
        for( auto& E : X )
        {
            E = X.getIndexByEntry<int>(E);
        }
    }


    void Test( void )
    {
        Simple();
    }
}