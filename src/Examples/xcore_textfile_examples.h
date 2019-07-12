
namespace xcore::textfile::examples
{
    constexpr static std::array<string::constant<char>,3>  StringBack              = { "String Test",              "Another test",         "Yet another"           };

    constexpr static std::array                            Floats32Back            = { -50.0f,                     2.1233f,                1.312323123f            };
    constexpr static std::array                            Floats64Back            = { -32.323212312312323,        2.2312323233,           1.312323123             };

    constexpr static std::array                            Int64Back               = { std::int64_t{13452},        std::int64_t{-321},     std::int64_t{1434}      };
    constexpr static std::array                            Int32Back               = { std::int32_t{0},            std::int32_t{21231},    std::int32_t{4344}      };
    constexpr static std::array                            Int16Back               = { std::int16_t{-2312},        std::int16_t{211},      std::int16_t{344}       };
    constexpr static std::array                            Int8Back                = { std::int8_t {16},           std::int8_t {21},       std::int8_t {-44}       };

    constexpr static std::array                            UInt64Back              = { std::uint64_t{13452323212}, std::uint64_t{321},     std::uint64_t{1434}     };
    constexpr static std::array                            UInt32Back              = { std::uint32_t{33313452},    std::uint32_t{222321},  std::uint32_t{111434}   };
    constexpr static std::array                            UInt16Back              = { std::uint16_t{31352},       std::uint16_t{2221},    std::uint16_t{11434}    };
    constexpr static std::array                            UInt8Back               = { std::uint8_t {33},          std::uint8_t {22},      std::uint8_t {44}       };

    //------------------------------------------------------------------------------
    // Test different types
    //------------------------------------------------------------------------------
    inline
    xcore::err Test01( xcore::string::view<char> FileName, bool isRead, xcore::textfile::file_type FileType, xcore::textfile::flags Flags = {} ) noexcept
    {
        std::array<string::ref<char>,3>                 String                  = { StringBack[0].getView(),    StringBack[1].getView(), StringBack[2].getView() };

        auto                                            Floats32                = Floats32Back;
        auto                                            Floats64                = Floats64Back;

        auto                                            Int64                   = Int64Back;
        auto                                            Int32                   = Int32Back;
        auto                                            Int16                   = Int16Back;
        auto                                            Int8                    = Int8Back ;

        auto                                            UInt64                  = UInt64Back;
        auto                                            UInt32                  = UInt32Back;
        auto                                            UInt16                  = UInt16Back;
        auto                                            UInt8                   = UInt8Back ;

        xcore::textfile::stream                         TextFile;
        xcore::err                                      Error;

        //
        // Open File
        //
        if( TextFile.Open( isRead, std::string_view{ FileName }, FileType, Flags ).isError(Error) ) return Error;

        //
        // Save/Load a record
        //
        if( TextFile.Record( Error, "TestTypes"
            ,   [&]( std::size_t& C, xcore::err& Error )
                {
                    if( isRead )    xassert( C == StringBack.size() );
                    else            C = StringBack.size();
                }
            ,   [&]( std::size_t i, xcore::err& Error )
                {
                    0
                    ||  TextFile.Field( "String",   String[i]       ).isError(Error)
                    ||  TextFile.Field( "Floats",   Floats64[i]
                                                ,   Floats32[i]     ).isError(Error)
                    ||  TextFile.Field( "Ints"  ,   Int64[i]
                                                ,   Int32[i]
                                                ,   Int16[i]
                                                ,   Int8[i]         ).isError(Error)
                    ||  TextFile.Field( "UInts" ,   UInt64[i]
                                                ,   UInt32[i] 
                                                ,   UInt16[i] 
                                                ,   UInt8[i]        ).isError(Error)
                    ;
                }
            )) return Error;
            
        //
        // Sanity check
        //
        if( isRead ) for( int i=0; i<StringBack.size(); ++i )
        {
            auto Convert = []( auto V, const char* Fmt ) -> string::view<char>
            {
                static string::fixed<char, 256> Buffer;
                string::sprintf( Buffer, Fmt, V );
                return Buffer;
            };

            //
            // If we tell the file system to write floats as floats then we will leak precision
            // so we need to take that into account when checking.
            //
            if( Flags.m_isWriteFloats )
            {
                double a = std::atof(Convert(Floats64Back[i],"%Lg"));
                float  b = static_cast<float>(std::atof(Convert(Floats32Back[i],"%g")));
                xassert( a == Floats64[i] );
                xassert( b == Floats32[i] );
            }
            else
            {
                xassert( Floats64Back[i]    == Floats64[i] );
                xassert( Floats32Back[i]    == Floats32[i] );
            }

            xassert( StringBack[i]  == String[i] );
            xassert( Int64Back[i]   == Int64[i] );
            xassert( Int32Back[i]   == Int32[i] );
            xassert( Int16Back[i]   == Int16[i] );
            xassert( Int8Back [i]   == Int8[i]  );
            xassert( UInt64Back[i]  == UInt64[i] );
            xassert( UInt32Back[i]  == UInt32[i] );
            xassert( UInt16Back[i]  == UInt16[i] );
            xassert( UInt8Back [i]  == UInt8[i]  );
        }

        //
        // When we are reading and we are done dealing with records we can check if we are done reading a file like this
        //
        if( isRead && TextFile.isEOF() )
            return {};

        return Error;
    }

    //------------------------------------------------------------------------------
    // Description:
    //      Converts an rvalue to an lvalue 
    //------------------------------------------------------------------------------
    template <typename T>
    constexpr T& lvalue(T&& r) noexcept { return r; }

    //------------------------------------------------------------------------------
    // Test different types
    //------------------------------------------------------------------------------
    inline
    xcore::err Test02( xcore::string::view<char> FileName, bool isRead, xcore::textfile::file_type FileType, xcore::textfile::flags Flags = {} ) noexcept
    {
        std::array<string::ref<char>,3>                 String                  = { StringBack[0].getView(),    StringBack[1].getView(), StringBack[2].getView() };

        auto                                            Floats32                = Floats32Back;
        auto                                            Floats64                = Floats64Back;

        auto                                            Int64                   = Int64Back;
        auto                                            Int32                   = Int32Back;
        auto                                            Int16                   = Int16Back;
        auto                                            Int8                    = Int8Back ;

        auto                                            UInt64                  = UInt64Back;
        auto                                            UInt32                  = UInt32Back;
        auto                                            UInt16                  = UInt16Back;
        auto                                            UInt8                   = UInt8Back ;

        xcore::textfile::stream                         TextFile;
        xcore::err                                      Error;

        //
        // Open File
        //
        if( TextFile.Open( isRead, std::string_view{ FileName }, FileType, Flags ).isError(Error) ) return Error;

        //
        // Save/Load record
        //
        if( TextFile.Record( Error, "TestTypes"
            ,   [&]( std::size_t& C, xcore::err& Error )
                {
                    if( isRead ) xassert( C == StringBack.size() );
                    else         C = StringBack.size();
                }
            ,   [&]( std::size_t i, xcore::err& Error )
                {
                    string::ref<char> ReadTypes;

                    // For user_types data
                    // if( TextFile.Field( UserInfo, "Ints", Int32[i] ).isError(Error) ) return;

                    // Just save some integers to show that we can still safe normal fields at any point
                    if( TextFile.Field( "Ints", Int32[i] ).isError(Error) ) return;

                    // Here we are going to save different types and give meaning to 'type' from above
                    // so in a way here we are saving two columns first the "type:<?>" then the "value<type>"
                    // the (i&1) shows that we can choose whatever types we want
                    if( i&1 ) if( TextFile.Field( "value:?", Floats64[i], Floats32[i] ).isError( Error ) ) return; else;
                    else      if( TextFile.Field( "value:?", Int64[i]                 ).isError( Error ) ) return;
                }
            )) return Error;

        //
        // Close
        //
        TextFile.close();
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
        // Lets test user_types types
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
        if( true ) if( 0
            || Test01( TextFileName, false, file_type::TEXT, {flags::WRITE_FLOATS} ).isError(Error) 
            || Test01( TextFileName, true,  file_type::TEXT ).isError(Error)
            || Test01( TextFileName, false, file_type::TEXT, {xcore::textfile::flags::WRITE_FLOATS} ).isError(Error)
            || Test01( TextFileName, true,  file_type::TEXT, {xcore::textfile::flags::WRITE_FLOATS} ).isError(Error) )
        {
            xassert(false);
        }

        //
        // Test write and read (Text Style)
        //
        if( true ) if( 0
            || Test02( TextFileName, false, xcore::textfile::file_type::TEXT ).isError(Error) 
            || Test02( TextFileName, true,  xcore::textfile::file_type::TEXT ).isError(Error) )
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