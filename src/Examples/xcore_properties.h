
#include "../../dependencies/Properties/src/Examples/Examples.h"

namespace xcore::property::examples
{
    //-------------------------------------------------------------------------------

    struct entry
    {
        xcore::string::ref<char>    m_Name;
        xcore::property::data       m_Data;

        entry()=default;
        entry( const char* p, xcore::property::data&& D ) : m_Name{ string::view<const char>{ p, string::Length(p).m_Value } }, m_Data{std::move(D) } {}
    };

    //-------------------------------------------------------------------------------

    constexpr static xcore::array Types
    {
            xcore::textfile::user_defined_types{    "S32",   "d"  }
        ,   xcore::textfile::user_defined_types{   "BOOL",   "c"  }
        ,   xcore::textfile::user_defined_types{    "F32",   "f"  }
        ,   xcore::textfile::user_defined_types{ "STRING",   "s"  }
        ,   xcore::textfile::user_defined_types{   "OOBB",  "ff"  }
    };

    //-------------------------------------------------------------------------------

    xcore::err HandleOperations( xcore::textfile::stream& TextFile, xcore::crc<32> Type, property::data& Data ) noexcept
    {
        return std::visit( [&]( auto&& V )
        {
            using T = std::decay_t<decltype( V )>;
            if constexpr ( std::is_same_v<T, oobb> ) return TextFile.Field( Type, "Value:?", V.m_Min, V.m_Max );
            else                                     return TextFile.Field( Type, "Value:?", V );
        }, Data );
    }

    //-------------------------------------------------------------------------------

    void ConstructDataFromType( xcore::crc<32> Type, property::data& Data ) noexcept
    {
        switch(Type.m_Value)
        {
            case xcore::crc<32>::FromString("OOBB").m_Value:   Data.emplace<oobb>();     break;
            case xcore::crc<32>::FromString("S32").m_Value:    Data.emplace<int>();      break;
            case xcore::crc<32>::FromString("F32").m_Value:    Data.emplace<float>();    break;
            case xcore::crc<32>::FromString("STRING").m_Value: Data.emplace<string_t>(); break;
            case xcore::crc<32>::FromString("BOOL").m_Value:   Data.emplace<bool>();     break;
            default: xassert(false);
        }
    }

    //-------------------------------------------------------------------------------

    template< typename T_EXAMPLE >
    xcore::err Test01( xcore::string::view<char> FileName )
    {
        xcore::err  Error;
        T_EXAMPLE   A;

        //
        // Print out the name of the example
        //
        printf( "------------------------------------------------------------------------------\n" ); 
        printf( "[Test01 - String Properties. Saving and Loading to a file and to a std::vector]\n" );              
        printf( "------------------------------------------------------------------------------\n" ); 
        std::cout.flush();  

        //
        // Check the basic stuff first
        //
        A.DefaultValues();
        A.SanityCheck();

        //
        // Collect all properties into a list as an example, here we could save them too
        //
        xcore::vector<entry> List;
        property::SerializeEnum( A, [&]( std::string_view PropertyName, property::data&& Data, const property::table&, std::size_t, property::flags::type Flags )
        {
            // If we are dealing with a scope that is not an array someone may have change the SerializeEnum to a DisplayEnum they only show up there.
            assert( Flags.m_isScope == false || PropertyName.back() == ']' );
            List.append( entry{ PropertyName.data(), std::move(Data) } );
        });

        //
        // Save and Load properties to a file and from a file
        //
        bool isRead = false;    // First we are going to write the data
        for( int k=0; k<2; k++ )
        {
            xcore::textfile::stream         TextFile;

            // Open file for writing/reading
            if( TextFile.Open( isRead, std::string_view{ FileName }, textfile::file_type::TEXT, {textfile::flags::WRITE_FLOATS} ).isError(Error) ) return Error;

            if( isRead == false )
            {
                auto pTableName = property::getTableByType<T_EXAMPLE>().m_pName;

                if( TextFile.WriteComment( string::constant("--------------------------------------------------------------------------------") ).isError(Error) ) return Error;
                if( TextFile.WriteComment( string::view<const char>{ pTableName, string::Length(pTableName).m_Value + 1 } ).isError(Error) ) return Error;
                if( TextFile.WriteComment( string::constant("--------------------------------------------------------------------------------\n") ).isError(Error) ) return Error;

                // Add the property types
                TextFile.AddUserTypes( Types );
            }

            //
            // Save/Load a record
            //
            if( TextFile.Record( Error, "Properties"
                ,   [&]( std::size_t& C, xcore::err& Error )
                    {
                        if( isRead ) { List.clear(); List.appendList(C); }
                        else         C = List.size();
                    }
                ,   [&]( std::size_t i, xcore::err& Error )
                    {
                        auto& Var = List[i];

                        // Save the name of the property
                        if( TextFile.Field( "Name", Var.m_Name ).isError(Error) ) return;

                        // Determine the data type
                        xcore::crc<32> Type;
                        if( isRead )
                        {
                            if( TextFile.ReadFieldUserType( Type, "Value:?" ).isError(Error) ) return;
                            ConstructDataFromType( Type, Var.m_Data );
                        }
                        else
                        {
                            // Property_to_type
                            Type = Types[ Var.m_Data.index() ].m_CRC;
                        }

                        // Actually save/load the data
                        if( HandleOperations( TextFile, Type, Var.m_Data ).isError(Error) ) return;
                    }
                )) return Error;

            isRead = !isRead;
        }

        //
        // Copy values to B and print them for the user to read
        //
        T_EXAMPLE B;
        int i=0;
        for ( const auto&[ Name, Data ] : List )
        {
            // Copy to B
            property::set( B, Name.data(), Data );

            //
            // Print out the property
            //
            printf( "[%3d] -> ", i++ );                 
            std::cout.flush();

            printf( "%-70s", Name.data() );            
            std::cout.flush();

            std::visit( [&]( auto&& Value )
            {
                using T = std::decay_t<decltype( Value )>;

                if constexpr ( std::is_same_v<T, int> )
                {
                    printf( " int    (%d)", Value );    
                }
                else if constexpr ( std::is_same_v<T, float> )
                {
                    printf( " float  (%f)", Value );
                }
                else if constexpr ( std::is_same_v<T, bool> )
                {
                    printf( " bool   (%s)", Value ? "true" : "false" );
                }
                else if constexpr ( std::is_same_v<T, string_t> )
                {
                    printf( " string (%s)", Value.data() );
                }
                else if constexpr ( std::is_same_v<T, oobb> )
                {
                    printf( " oobb   (%f, %f)", Value.m_Min, Value.m_Max );
                }
                else static_assert( always_false<T>::value, "We are not covering all the cases!" );
            }
            , Data );

            printf( "\n" ); 
            std::cout.flush();
        }

        // Confirm that all values were copied
        B.SanityCheck();
        printf( "[SUCCESSFUL] Copied all properties to new class\n" );


        return Error;
    }

    //-------------------------------------------------------------------------------

    template< typename T_EXAMPLE >
    xcore::err Test02( xcore::string::view<char> FileName )
    {
        xcore::err  Error;
        T_EXAMPLE   A;

        //
        // Print out the name of the example
        //
        printf( "------------------------------------------------------------------------------\n" );
        printf( "[Test02 - Optimized properties - Saving and Loading to a property::pack]\n" );
        printf( "------------------------------------------------------------------------------\n" );

        //
        // Check the basic stuff first
        //
        A.DefaultValues();
        A.SanityCheck();

        //
        // Collect all properties into a list as an example, here we could save them too
        //
        property::pack Pack;
        property::Pack( A, Pack );

        //
        // Save and Load properties to a file and from a file
        //
        bool isRead = false;    // First we are going to write the data
        for( int k=0; k<2; k++ )
        {
            xcore::textfile::stream         TextFile;

            // Open file for writing/reading      textfile::file_type::TEXT
            if( TextFile.Open( isRead, std::string_view{ FileName }, textfile::file_type::BINARY ).isError(Error) ) return Error;

            if( isRead == false )
            {
                auto pTableName = property::getTableByType<T_EXAMPLE>().m_pName;

                if( TextFile.WriteComment( string::constant("--------------------------------------------------------------------------------") ).isError(Error) ) return Error;
                if( TextFile.WriteComment( string::view<const char>{ pTableName, string::Length(pTableName).m_Value + 1 } ).isError(Error) ) return Error;
                if( TextFile.WriteComment( string::constant("--------------------------------------------------------------------------------\n") ).isError(Error) ) return Error;

                // Add the property types
                TextFile.AddUserTypes( Types );
            }

            //
            // Save/Load Paths
            //
            if( TextFile.Record( Error, "Property-Paths"
                ,   [&]( std::size_t& C, xcore::err& Error )
                    {
                        if( isRead ) 
                        { 
                            Pack.m_lPath.clear(); 
                            Pack.m_lPath.resize(C); 
                        }
                        else
                        {
                            C = Pack.m_lPath.size();
                        }
                    }
                ,   [&]( std::size_t i, xcore::err& Error )
                    {
                        auto& Var = Pack.m_lPath[i];
                        if( TextFile.Field( "Index", Var.m_Index ).isError(Error) ) return;
                        if( TextFile.Field( "Key",   Var.m_Key   ).isError(Error) ) return;
                    }
                )) return Error;


            //
            // Save/Load Data
            //
            if( TextFile.Record( Error, "Property-Data"
                ,   [&]( std::size_t& C, xcore::err& Error )
                    {
                        if( isRead ) 
                        { 
                            Pack.m_lEntry.clear(); 
                            Pack.m_lEntry.resize(C); 
                        }
                        else
                        {
                            C = Pack.m_lEntry.size();
                        }
                    }
                ,   [&]( std::size_t i, xcore::err& Error )
                    {
                        auto& Var = Pack.m_lEntry[i];

                        // 
                        if( TextFile.Field( "isArrayCount", Var.m_isArrayCount  ).isError(Error) ) return;
                        if( TextFile.Field( "nPaths",       Var.m_nPaths        ).isError(Error) ) return;
                        if( TextFile.Field( "nPopPaths",    Var.m_nPopPaths     ).isError(Error) ) return;

                        // Determine the data type
                        xcore::crc<32> Type;
                        if( isRead )
                        {
                            if( TextFile.ReadFieldUserType( Type, "Value:?" ).isError(Error) ) return;
                            ConstructDataFromType( Type, Var.m_Data );
                        }
                        else
                        {
                            // Property_to_type
                            Type = Types[ Var.m_Data.index() ].m_CRC;
                        }

                        // Actually save/load the data
                        if( HandleOperations( TextFile, Type, Var.m_Data ).isError(Error) ) return;
                    }
                )) return Error;

            isRead = !isRead;
        }


        //
        // Debug display the pack
        //
        float LookUps = -1;         // The very first one is actually not a lookup
        {
            int iPath = 0;
            for( const auto& E : Pack.m_lEntry )
            {
                printf( "[%3d] -> ", static_cast<int>(&E - &Pack.m_lEntry[0]) );

                int c = 0;
                // Mark how many pops we are making
                for( int i=0; i<E.m_nPopPaths; ++i )
                    c += printf( "/.." );

                for( int i=0; i<E.m_nPaths; ++i, ++iPath )
                {
                    // Are we dealing with an array type?
                    if( Pack.m_lPath[ iPath ].m_Index != property::lists_iterator_ends_v )
                    {
                        // Arrays/Lists we get them for free
                        c += printf( "/[%" PRIu64 "]", Pack.m_lPath[ iPath ].m_Index );
                        LookUps -= 1.0f;
                    }
                    else
                    {
                        c += printf( "/%u", Pack.m_lPath[ iPath ].m_Key );
                        if ( i == (E.m_nPaths - 1) && E.m_isArrayCount ) c += printf( "[]" );
                    }
                }

                // Keep track of lookups
                LookUps += E.m_nPaths;

                // Make sure everything is align
                while( (c++)!=70 ) printf(" ");

                // print type and data
                std::visit( [&]( auto&& Value )
                {
                    using T = std::decay_t<decltype( Value )>;

                    if constexpr ( std::is_same_v<T, int> )
                    {
                        printf( " int    (%d)", Value );
                    }
                    else if constexpr ( std::is_same_v<T, float> )
                    {
                        printf( " float  (%f)", Value );
                    }
                    else if constexpr ( std::is_same_v<T, bool> )
                    {
                        printf( " bool   (%s)", Value ? "true" : "false" );
                    }
                    else if constexpr ( std::is_same_v<T, string_t> )
                    {
                        printf( " string (%s)", Value.data() );
                    }
                    else if constexpr ( std::is_same_v<T, oobb> )
                    {
                        printf( " oobb   (%f, %f)", Value.m_Min, Value.m_Max );
                    }
                    else static_assert( always_false<T>::value, "We are not covering all the cases!" );
                }
                , E.m_Data );

                // next line
                printf( "\n" );
            }
        }

        //
        // Copy to B
        //
        T_EXAMPLE B;
        property::set( B, Pack );

        // Confirm that all values were copied
        B.SanityCheck();
        printf( "[SUCCESSFUL] Copied all properties to new class. Lookups per property (%.2f)\n", LookUps/Pack.m_lEntry.size() );

        return Error;
    }

    //-------------------------------------------------------------------------------

    void Test( void ) noexcept
    {
        constexpr static xcore::string::constant TextFileName( "PropertyText.la.txt" ); 
        constexpr static xcore::string::constant BinFileName( "PropertyBin.la.bin" ); 

        xcore::err  Error;

        if( 1 && ( Test01<example3_custom_lists>(TextFileName).isError(Error)
                || Test01<example2_custom_lists>(TextFileName).isError(Error)
                || Test01<example1_custom_lists>(TextFileName).isError(Error)
                || Test01<example0_custom_lists>(TextFileName).isError(Error)
                || Test01<example10>(TextFileName).isError(Error)
                || Test01<example9>(TextFileName).isError(Error)
                || Test01<example8>(TextFileName).isError(Error)
                || Test01<example7>(TextFileName).isError(Error)
                || Test01<example6>(TextFileName).isError(Error)
                || Test01<example5>(TextFileName).isError(Error)
                || Test01<example4>(TextFileName).isError(Error)
                || Test01<example3>(TextFileName).isError(Error)
                || Test01<example2>(TextFileName).isError(Error)
                || Test01<example1>(TextFileName).isError(Error)
                || Test01<example0>(TextFileName).isError(Error)) )
        {
        }

        if( 1 && ( Test02<example3_custom_lists>(BinFileName).isError(Error)
                || Test02<example2_custom_lists>(BinFileName).isError(Error)
                || Test02<example1_custom_lists>(BinFileName).isError(Error)
                || Test02<example0_custom_lists>(BinFileName).isError(Error)
                || Test02<example10>(BinFileName).isError(Error)
                || Test02<example9>(BinFileName).isError(Error)
                || Test02<example8>(BinFileName).isError(Error)
                || Test02<example7>(BinFileName).isError(Error)
                || Test02<example6>(BinFileName).isError(Error)
                || Test02<example5>(BinFileName).isError(Error)
                || Test02<example4>(BinFileName).isError(Error)
                || Test02<example3>(BinFileName).isError(Error)
                || Test02<example2>(BinFileName).isError(Error)
                || Test02<example1>(BinFileName).isError(Error)
                || Test02<example0>(BinFileName).isError(Error)) )
        {
        }

    }
}
