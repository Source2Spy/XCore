
#include "../../dependencies/Properties/src/Examples/Examples.h"

namespace xcore::property::examples
{
    //-------------------------------------------------------------------------------

    struct entry
    {
        xcore::string::fixed<char, 256> m_Name;
        xcore::property::data           m_Data;
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

    template< typename T_EXAMPLE >
    void Test01( xcore::string::view<char> FileName )
    {
        xcore::err  Error;
        T_EXAMPLE   A;

        //
        // Print out the name of the example
        //
        printf( "------------------------------------------------------------------------------\n" ); 
        printf( "[Test01 - String Properties. Saving and Loading to a std::vector]\n" );              
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
            List.append( property::entry{ PropertyName, Data } );
        });


        //
        // Open File
        //
        bool                            isRead = false;
        xcore::textfile::stream         TextFile;

        if( TextFile.Open( isRead, std::string_view{ FileName }, file_type::TEXT, {flags::WRITE_FLOATS} ).isError(Error) ) return Error;

        // Add the property types
        TextFile.AddUserTypes( Types );

        //
        // Save/Load a record
        //
        if( TextFile.Record( Error, "Properties"
            ,   [&]( std::size_t& C, xcore::err& Error )
                {
                    if( isRead ) xassert( C == 3 );
                    else         C = List.size();
                }
            ,   [&]( std::size_t i, xcore::err& Error )
                {
                    // Just save some integers to show that we can still safe normal fields at any point
                    if( TextFile.Field( "Name", List[i].m_Name ).isError(Error) ) return;

                    // Handle the data
                    crc::units<32> Type;
                    if( isRead )
                    {
                        if( TextFile.ReadFieldUserType( Type, "Value:?" ).isError(Error) ) return;
                    }
                    else
                    {
                        // Property_to_type
                        Type = Types[i].m_CRC;
                    }

                    switch(Type.m_Value)
                    {
                    case crc::FromString<32>("V3").m_Value:     if( TextFile.Field( Type, "Value:?", Position[0], Position[1], Position[2]   ).isError(Error)    ) return; break;
                    case crc::FromString<32>("BOOL").m_Value:   if( TextFile.Field( Type, "Value:?", isValid                                 ).isError(Error)    ) return; break;
                    case crc::FromString<32>("STRING").m_Value: if( TextFile.Field( Type, "Value:?", types::lvalue(Name.getView())           ).isError(Error)    ) return; break;
                    }
                }
            )) return Error;

        //
        // Copy values to B
        //
        T_EXAMPLE B;
        int i=0;
        for ( const auto&[ Name, Data ] : List )
        {
            // Copy to B
            property::set( B, Name.c_str(), Data );

            //
            // Print out the property
            //
            printf( "[%3d] -> ", i++ );                 
            std::cout.flush();

            printf( "%-70s", Name.c_str() );            
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
                else if constexpr ( std::is_same_v<T, std::string> )
                {
                    printf( " string (%s)", Value.c_str() );
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
    }

    //-------------------------------------------------------------------------------

    void Test( void ) noexcept
    {
        constexpr static xcore::string::constant TextFileName( "PropertyTest.la.txt" ); 

        Test01<example3_custom_lists>(TextFileName);
        Test01<example2_custom_lists>(TextFileName);
        Test01<example1_custom_lists>(TextFileName);
        Test01<example0_custom_lists>(TextFileName);
        Test01<example10>(TextFileName);
        Test01<example9>(TextFileName);
        Test01<example8>(TextFileName);
        Test01<example7>(TextFileName);
        Test01<example6>(TextFileName);
        Test01<example5>(TextFileName);
        Test01<example4>(TextFileName);
        Test01<example3>(TextFileName);
        Test01<example2>(TextFileName);
        Test01<example1>(TextFileName);
        Test01<example0>(TextFileName);
    }
}
