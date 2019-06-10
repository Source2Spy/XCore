
namespace xcore::textfile::examples
{
    //------------------------------------------------------------------------------
    // Test types
    //------------------------------------------------------------------------------
    static std::array                       g_Floats32      = { -50.0f, 2.1233f, 1.312323123f, 543.2343f, 0.0f };
    static std::array<string::ref<char>,3>  g_String        = { string::constant("String Test").getView(), string::constant("Another test").getView(), string::constant("Yet another").getView() };
    static std::array                       g_Int32         = { 12, 21, 44, 1, 0 };
    static std::array                       g_Floats64      = { -32.323212312312323, 2.2312323233, 1.312323123, 54.2343, 0.0 };
    static std::array                       g_Int64         = { 13452LL, 321LL, 1434LL, 19LL, 0LL };

    //------------------------------------------------------------------------------
    static
    xcore::err Test01( xcore::string::view<char> FileName, bool isRead, xcore::textfile::file_type FileType )
    {
        xcore::textfile::stream     TextFile;
        xcore::err                  Error;

        //
        // Open File
        //
        if( isRead )
        {
            if( TextFile.openForReading( std::string_view{ FileName } ).isError(Error) )        return Error;
        }
        else
        {
            if( TextFile.openForWriting( std::string_view{ FileName }, FileType ).isError(Error) ) return Error;
        }

        //
        // Save/Load record
        //
        if( TextFile.Record( Error, "FirstRecord"
            ,   [&]( int& C, bool isRead )
                {
                    if( isRead )    xassert( C == 12 );
                    else            C = 12;
                }
            ,   [&]( int i )
                {
                        TextFile.Field( "Description",  g_String[i%g_String.size()].getView()     ).isError(Error)
                    ||  TextFile.Field( "Position",     g_Floats32[(i+0)%g_Floats32.size()], 
                                                        g_Floats32[(i+1)%g_Floats32.size()], 
                                                        g_Floats32[(i+2)%g_Floats32.size()]       ).isError(Error)
                    ||  TextFile.Field( "Number",       g_Int32[i%g_String.size()]                ).isError(Error);
                }
            )) return Error;

#if 0
        //
        // Big types
        //
        if( TextFile.WriteRecord( "SecondRecord" ).isError( Error ) )
            return Error;

        if( TextFile.Field( "pepe", g_Int64[0], g_Floats64[0] ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;
    
        //
        // Dynamic Type
        //
        if( TextFile.WriteRecord( "TheirdRecord", 3 ).isError( Error ) )
            return Error;
    
        // 1
        if( TextFile.Field( "type:<?>", string::fixed<char,128>("fff").getView() ).isError( Error ) )
            return Error;

        if( TextFile.Field( "value:<type>", g_Floats32[0], g_Floats32[1], g_Floats32[2] ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;
    
        // 2
        if( TextFile.Field( "type:<?>", string::fixed<char,128>("s").getView() ).isError( Error ) )
            return Error;

        if( TextFile.Field( "value:<type>", g_String[0].getView() ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;

        // 3
        if( TextFile.Field( "type:<?>", string::fixed<char,128>("DF").getView() ).isError( Error ) )
            return Error;

        if( TextFile.Field( "value:<type>", g_Int64[0], g_Floats64[0] ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;

        //
        // Lets test user types
        //
        TextFile.AddUserType( "fff", ".V3",   crc::FromString<32>(".V3"),   0 );
        TextFile.AddUserType( "fff", ".V3D",  crc::FromString<32>(".V3D"),  1 );
        TextFile.AddUserType( "f",   ".F",    crc::FromString<32>(".F"),    2 );
        TextFile.AddUserType( "sf",  ".SS",   crc::FromString<32>(".SS"),   3 );

        if( TextFile.WriteRecord( "UsersTypes", 2 ).isError(Error) )
            return Error;
    
        // 0
        if( TextFile.Field ( "mytype:<?>", string::fixed<char,128>(".V3").getView() ).isError( Error ) )
            return Error;

        if( TextFile.Field ( "myvalue:<mytype>", g_Floats32[0], g_Floats32[1], g_Floats32[2] ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;
    
        // 1
        if( TextFile.Field ( "mytype:<?>", string::fixed<char,128>(".SS").getView() ).isError( Error ) )
            return Error;

        if( TextFile.Field ( "myvalue:<mytype>", g_String[0].getView(), g_String[1].getView() ).isError( Error ) )
            return Error;

        if( TextFile.WriteLine().isError( Error ) )
            return Error;
    
        //
        // Other record
        //
        if(    TextFile.WriteRecord( "UsersTypes2" ).isError( Error ) 
            || TextFile.Field( "One:.F", g_Floats64[0] ).isError( Error )
            || TextFile.Field( "Two:.V3D", g_Floats32[0], g_Floats32[1], g_Floats32[2] ).isError( Error )
            || TextFile.WriteLine().isError( Error ) )
            return Error;
#endif

        TextFile.close();
        return Error;
    }

    //-----------------------------------------------------------------------------------------

    void Test( void )
    {
        constexpr static xcore::string::constant TextFileName( "TextFileTest.la.txt" ); 
        constexpr static xcore::string::constant BinaryFileName( "BinaryFileTest.la.bin" );
        xcore::err Error;

        //
        // Test write and read (Text Style)
        //
        if( Test01( TextFileName, false, xcore::textfile::file_type::TEXT ).isError(Error) )
        {
            xassert(false);
        }

        if( Test01( TextFileName, true, xcore::textfile::file_type::TEXT ).isError(Error) )
        {
            xassert(false);
        }
    
/*
        //
        // Test write and read (Binary Style)
        //
        Test01( TextFileName,  xtextfile::access_type::MASK_BINARY );
        TextFileRead( TextFileName );

        //
        // Transform to binary format
        //
        xtextfile::TransformTo( TextFileName, BinaryFileName, xtextfile::access_type::MASK_BINARY );
        TextFileRead( BinaryFileName );
    
        //
        // Transform to text format
        //
        xtextfile::TransformTo( BinaryFileName, TextFileName, xtextfile::access_type::MASK_ZERO );
        TextFileRead( TextFileName );
*/
    }

}