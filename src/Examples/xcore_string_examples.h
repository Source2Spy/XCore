
namespace xcore::string::examples
{
    //---------------------------------------------------------------------------------------
    // Determine all the different ways to call length
    //---------------------------------------------------------------------------------------
    void CallLength( void )
    {
        // Low level type of parameters
        {
            const char  x[32] = {'h','l','l','o',0};
            const char* p = x;
            const char* pp = p;

            xcore::string::Length( "asdsadasd" );
            xcore::string::Length( x );
            xcore::string::Length( p );
            xcore::string::Length( pp );
        }

        // Complex classes
        {
            const char  x[32] = { 'h','l','l','o',0 };
            xcore::string::view         V{ x, 32 };
            string::fixed<char,256>     F;
            string::constant            C("asdasd");
            string::ref                 R("asd");

            xcore::string::Length( F );
            xcore::string::Length( C );
            xcore::string::Length( R );
            xcore::string::Length( V );

            xcore::string::Length( xcore::string::view{ "dsfdsf", 32 } );
            xcore::string::Length( string::constant("asdsd") );
            xcore::string::Length( string::ref("asdsd") );
        }

        // Complex classes with const
        {
            const char x[32] = {'h','l','l','o',0};

            const xcore::string::view         V{ x, 32 };
            const string::fixed<char,256>     F{};
            const string::constant            C("asdasd");
            const string::ref                 R("asd");

            xcore::string::Length( F );
            xcore::string::Length( C );
            xcore::string::Length( R );
            xcore::string::Length( V );
        }
    }

    //---------------------------------------------------------------------------------------
    // Determine all the different ways to call Copy
    //---------------------------------------------------------------------------------------
    void CallCopy( void )
    {
        char        x[32] = {'h','l','l','o',0};
        char*       p = x;
        const char* pp = p;

        xcore::string::view<char>   V{ x, 32 };
        string::fixed<char,256>     F;
        string::constant            C("asdasd");
        string::ref                 R("asd");

        xcore::string::Copy( V, "Hello Boss" );
        xcore::string::Copy( V, x );
        xcore::string::Copy( V, p );
        xcore::string::Copy( V, pp );

        xcore::string::Copy( V, V );
        xcore::string::Copy( V, F );
        xcore::string::Copy( V, C );
        xcore::string::Copy( V, R );

        xcore::string::Copy( V, V );
        xcore::string::Copy( F, F );
     //   xcore::string::Copy( C, C );      // This should fail
        xcore::string::Copy( R, R );

        xcore::string::Copy( V, xcore::string::view{ "dsfdsf", 32 } );
        xcore::string::Copy( V, string::constant("asdsd") );
        xcore::string::Copy( V, string::ref("asdsd") );

        xcore::string::Copy( F, xcore::string::view{ "dsfdsf", 32 } );
        xcore::string::Copy( F, string::constant("asdsd") );
        xcore::string::Copy( F, string::ref("asdsd") );

        xcore::string::Copy( R, xcore::string::view{ "dsfdsf", 32 } );
        xcore::string::Copy( R, string::constant("asdsd") );
        xcore::string::Copy( R, string::ref("asdsd") );

        xcore::string::Copy( xcore::string::view{ x, 32 }, xcore::string::view{ "dsfdsf", 32 } );
        xcore::string::Copy( xcore::string::view{ x, 32 }, string::constant("asdsd") );
        xcore::string::Copy( xcore::string::view{ x, 32 }, string::ref("asdsd") );

      //  xcore::string::Copy( C, xcore::string::view{ "dsfdsf", 32 } );
      //  xcore::string::Copy( C, string::constant("asdsd") );
      //  xcore::string::Copy( C, string::ref("asdsd") );

    }

    //---------------------------------------------------------------------------------------
    // Proper way to pass a read only string to a function
    //---------------------------------------------------------------------------------------
    void PassingToConstantFunction( const xcore::string::view<const char> V )
    {
        xcore::string::Length(V);
        static_assert( xcore::string::Length("Hello") == xcore::string::units<char>{5} );
    }

    //---------------------------------------------------------------------------------------
    // Proper way to pass a inside the bound string
    //---------------------------------------------------------------------------------------
    void PassingToMutableFunction( xcore::string::view<char> V )
    {
        xcore::string::CopyN( V, xcore::string::constant("This is a test"), units<char>{5} );
        xcore::string::CopyN( V, "This is a test", units<char>{5} );
    }

    //---------------------------------------------------------------------------------------
    // Proper way to pass a string which will be created new
    //---------------------------------------------------------------------------------------
    void PassingToCreationFunction( xcore::string::ref<char>& V )
    {
        char  x[32];
        char* p = x;
        V.MakeUnique( units<char>{255} );

        xcore::string::Copy( V, "Hello Boss" );
     //   xcore::string::Copy( xcore::string::view<char>{ x, 32 }, "asds" );
        xcore::string::Copy( V, p );

        xcore::string::Copy( V, "Hello Boss" );
        xcore::string::Copy( xcore::string::view<char>{ x, 32 }, "asds" );
        xcore::string::Copy( V, p );

    }

    //---------------------------------------------------------------------------------------
    // Testing the different types
    //---------------------------------------------------------------------------------------
    void TypesOfStrings( void )
    {
        // You can make a fixed string which is very similar to say std::array<CHAR,SIZE>
        xcore::string::fixed<char,256>  F("Hello Darling");

        // This is where you don't know the size
        xcore::string::ref              R("What you got");

        // This creates some constant string
        xcore::string::constant         C("This is some string");
    
        //----------------------------------------------------------------------
        // Passing constant strings to a function
        PassingToConstantFunction( F );
    //    PassingToConstantFunction( R );    // will assert because it gets it as a data()
        PassingToConstantFunction( C );

        //----------------------------------------------------------------------
        // Passing mutable strings to a function
        PassingToMutableFunction( F );
    //    PassingToMutableFunction( R );	// will assert because it gets it as a data()	
        // PassingToMutableFunction( C );   // This won't work because is a read only data

        //----------------------------------------------------------------------
        // Passing to a function which will create a string from zero
        // PassingToCreationFunction(F);    // This won't work because has nothing to do with a fixed string
        PassingToCreationFunction(R);
        // PassingToCreationFunction(C);    // This won't work because has nothing to do with a constant string
    }

    //---------------------------------------------------------------------------------------
    // Main tests
    //---------------------------------------------------------------------------------------
    void Tests( void )
    {
        TypesOfStrings();
    }
}