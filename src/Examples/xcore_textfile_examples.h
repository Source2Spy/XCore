
namespace xcore::textfile::examples
{
    //------------------------------------------------------------------------------
    inline
    xcore::err Test01( xcore::string::view<char> FileName, bool isRead, xcore::textfile::file_type FileType ) noexcept
    {
        constexpr std::array                            Floats32_ABack          = { -50.0f,  2.1233f,  1.312323123f };
        constexpr std::array                            Floats32_BBack          = {  50.0f, -2.1233f, -1.312323123f };
        constexpr std::array                            Floats32_CBack          = {  -5.0f,  0.1233f,  131.2323123f };
        constexpr std::array<string::constant<char>,3>  StringBack              = { "String Test", "Another test", "Yet another" };
        constexpr std::array                            Int32Back               = { 12, 21, 44 };
        constexpr std::array                            Floats64_ABack          = { -32.323212312312323,  2.2312323233,  1.312323123 };
        constexpr std::array                            Floats64_BBack          = { -3.323212312312323,   0.2312323233,  0.312323123 };
        constexpr std::array                            Floats64_CBack          = { -2.323212312312323,  -1.2312323233, -0.312323123 };
        constexpr std::array                            Int64Back               = { 13452LL, 321LL, 1434LL };

        auto                                            Floats32_A              = Floats32_ABack;
        auto                                            Floats32_B              = Floats32_BBack;
        auto                                            Floats32_C              = Floats32_CBack;
        std::array<string::ref<char>,3>                 String                  = { StringBack[0].getView(), StringBack[1].getView(), StringBack[2].getView() };
        auto                                            Int32                   = Int32Back     ;
        auto                                            Floats64_A              = Floats64_ABack;
        auto                                            Floats64_B              = Floats64_BBack;
        auto                                            Floats64_C              = Floats64_CBack;
        auto                                            Int64                   = Int64Back     ;

        xcore::textfile::stream                         TextFile;
        xcore::err                                      Error;

        //
        // Open File
        //
        if( TextFile.Open( isRead, std::string_view{ FileName }, FileType ).isError(Error) ) return Error;

        //
        // Save/Load record
        //
        if( TextFile.Record( Error, "FirstRecord"
            ,   [&]( int& C, xcore::err& Error )
                {
                    if( isRead )    xassert( C == 3 );
                    else            C = 3;
                }
            ,   [&]( int i, xcore::err& Error )
                {
                    0
                    ||  TextFile.Field( "Description",  String[i]     ).isError(Error)
                    ||  TextFile.Field( "Position",     Floats32_A[i]
                                                  ,     Floats32_B[i]
                                                  ,     Floats32_C[i] ).isError(Error)
                    ||  TextFile.Field( "Number",       Int32[i]      ).isError(Error);
                }
            )) return Error;

        //
        // Sanity check
        //
        if( isRead ) for( int i=0; i<3; ++i )
        {
            auto Convert = []( auto V, const char* Fmt ) -> string::view<char>
            {
                static string::fixed<char, 256> Buffer;
                string::sprintf( Buffer, Fmt, V );
                return Buffer;
            };

            float a = static_cast<float>(std::atof(Convert(Floats32_ABack[i],"%g")));
            float b = static_cast<float>(std::atof(Convert(Floats32_BBack[i],"%g")));
            float c = static_cast<float>(std::atof(Convert(Floats32_CBack[i],"%g")));

            xassert( StringBack[i]  == String[i] );
            xassert( a              == Floats32_A[i] );
            xassert( b              == Floats32_B[i] );
            xassert( c              == Floats32_C[i] );
            xassert( Int32Back[i]   == Int32[i] );
        }

        //
        // When we are reading and we are done dealing with records we can finish like this
        //
        if( isRead && TextFile.isEOF() )
            return {};

        return Error;
    }

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

        TextFile.close();
        return Error;
    }
#endif

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