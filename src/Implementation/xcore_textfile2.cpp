//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// Details
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
namespace xcore::textfile::version_2::details
{
    //-----------------------------------------------------------------------------------------------------
    // convert wstring to UTF-8 string
    //-----------------------------------------------------------------------------------------------------
    inline
    std::string wstring_to_utf8( const std::wstring& str )
    {
        std::array<char, 512> buffer;
    #if defined(_MSC_VER)
        std::size_t ret;
        auto Err = wcstombs_s( &ret, buffer.data(), buffer.size(), str.c_str(), buffer.size() );
        xassert( Err == false );
        return buffer.data();
    #else
        auto ret = wcstombs(buffer.data(), str.c_str(), buffer.size());
        return buffer.data();
    #endif
    }

    //------------------------------------------------------------------------------
    details::file& file::setup( std::FILE& File, details::states States ) noexcept
    {
        close();
        m_pFP    = &File;
        m_States = States; 
        return *this;
    }

    //------------------------------------------------------------------------------
    file::~file( void ) noexcept 
    { 
        close(); 
    }

    //------------------------------------------------------------------------------

    xcore::err file::openForReading( const std::filesystem::path& FilePath, bool isBinary ) noexcept
    {
        xassert(m_pFP == nullptr);
    #if defined(_MSC_VER)
        auto Err = fopen_s( &m_pFP, wstring_to_utf8(FilePath).c_str(), isBinary ? "rb" : "rt" );
        if( Err )
        {
            return xerr_failure_s( "Fail to open a file for reading" );
        }
    #else
        m_pFile->m_pFP = fopen( wstring_to_utf8(FilePath).c_str(), pAttr );
        if( m_pFP )
        {
            return xerr_failure_s( "Fail to open a file");
        }
    #endif

        m_States.m_isBinary  = isBinary;
        m_States.m_isReading = true;
        m_States.m_isView    = false;
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::openForWritting( const std::filesystem::path& FilePath, bool isBinary  ) noexcept
    {
        xassert(m_pFP == nullptr);
    #if defined(_MSC_VER)
        auto Err = fopen_s( &m_pFP, wstring_to_utf8(FilePath).c_str(), isBinary ? "wb" : "wt" );
        if( Err )
        {
            return xerr_failure_s( "Fail to open a file for writting" );
        }
    #else
        m_pFile->m_pFP = fopen( wstring_to_utf8(FilePath).c_str(), pAttr );
        if( m_pFP )
        {
            return xerr_failure_s( "Fail to open a file");
        }
    #endif

        m_States.m_isBinary  = isBinary;
        m_States.m_isReading = false;
        m_States.m_isView    = false;

        return {};
    }

    //------------------------------------------------------------------------------

    void file::close( void ) noexcept
    {
        if(m_pFP && m_States.m_isView == false ) fclose(m_pFP);
        m_pFP = nullptr;
    }

    //------------------------------------------------------------------------------

    xcore::err file::ReadingErrorCheck( void ) noexcept
    {
        if( m_States.m_isEOF || feof( m_pFP ) ) 
        {
            m_States.m_isEOF = true;
            return xerr_code_s( error_state::UNEXPECTED_EOF, "Found the end of the file unexpectedly while reading" );
        }
        return xerr_failure_s( "Fail while reading the file, expected to read more data" );
    }

    //------------------------------------------------------------------------------
    template< typename T >
    xcore::err file::Read( T& Buffer, int Size, int Count ) noexcept
    {
        xassert(m_pFP);
        if( m_States.m_isEOF ) return ReadingErrorCheck();

    #if defined(_MSC_VER)
        if ( Count != fread_s( &Buffer, Size, Size, Count, m_pFP ) )
            return ReadingErrorCheck();
    #else
    #endif
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::getC( int& c ) noexcept
    {
        xassert(m_pFP);
        if( m_States.m_isEOF ) return ReadingErrorCheck();

        c = fgetc(m_pFP);
        if( c == -1 ) return ReadingErrorCheck();
        return {};
    }

    //------------------------------------------------------------------------------

    template< typename T >
    xcore::err file::Write( T& Buffer, int Size, int Count ) noexcept
    {
        xassert(m_pFP);

    #if defined(_MSC_VER)
        if ( Count != fwrite( &Buffer, Size, Count, m_pFP ) )
        {
            return xerr_failure_s( "Fail writting the required data" );
        }
    #else
    #endif
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::WriteStr( const string::view<const char> Buffer ) noexcept
    {
        xassert(m_pFP);

    #if defined(_MSC_VER)
        if( m_States.m_isBinary )
        {
            if ( Buffer.size() != fwrite( Buffer.data(), sizeof(char), Buffer.size(), m_pFP ) )
            {
                return xerr_failure_s( "Fail writing the required data" );
            }
        }
        else
        {
            const std::uint64_t L           = string::Length(Buffer).m_Value;
            const std::uint64_t TotalData   = L>Buffer.size()?Buffer.size():L;
            if( TotalData != std::fwrite( Buffer.data(), 1, TotalData, m_pFP ) )
                return xerr_failure_s( "Fail 'fwrite' writing the required data" );
        }
    #else
    #endif
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::WriteFmtStr( const char* pFmt, ... ) noexcept
    {
        va_list Args;
        va_start( Args, pFmt );
        if( std::vfprintf( m_pFP, pFmt, Args ) < 0 )
            return xerr_failure_s( "Fail 'fprintf' writing the required data" );
        va_end( Args );
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::WriteChar( char C, int Count ) noexcept
    {
        while( Count-- )
        {
            if( C != fputc( C, m_pFP ) )
                return xerr_failure_s( "Fail 'fputc' writing the required data" );
        }
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::WriteData( string::view<const char> Buffer ) noexcept
    {
        xassert( m_States.m_isBinary );

        if( Buffer.size() != std::fwrite( Buffer.data(), 1, Buffer.size(), m_pFP ) )
            return xerr_failure_s( "Fail 'fwrite' binary mode" );

        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err file::ReadWhiteSpace( int& c ) noexcept
    {
        // Read any spaces
        xcore::err  Error;
    
        do 
        {
            if( getC(c).isError(Error) ) return Error;
        } while( std::isspace( c ) );

        //
        // check for comments
        //
        while( c == '/' )
        {
            if( getC(c).isError(Error) ) return Error;
            if( c == '/' )
            {
                // Skip the comment
                do
                {
                    if( getC(c).isError(Error) ) return Error;
                } while( c != '\n' );
            }
            else
            {
                return xerr_failure( Error, "Error reading file, unexpected symbol found [/]" );
            }

            // Skip spaces
            if( ReadWhiteSpace(c).isError(Error) ) return Error;
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err file::HandleDynamicTable( int& Count ) noexcept
    {
        xcore::err  Error;
        auto        LastPosition    = ftell( m_pFP );
        int         c;
                
        Count           = -2;                   // -1. for the current header line, -1 for the types
        
        if( LastPosition == -1 )
            return xerr_failure_s( "Fail to get the cursor position in the file" );

        if( getC(c).isError(Error) )
        {
            Error.clear();
            return xerr_failure_s( "Unexpected end of file while searching the [*] for the dynamic" );
        }
    
        do
        {
            if( c == '\n' )
            {
                Count++;
                if( ReadWhiteSpace(c).isError(Error) ) return Error;
            
                if( c == '[' )
                {
                    break;
                }
            }
            else
            {
                if( getC(c).isError(Error) ) return Error;
            
                // if the end of the file is in a line then we need to count it
                if( c == -1 )
                {
                    Count++;
                    break;
                }
            }
    
        } while( true );

    
        if( Count <= 0  )
            return xerr_failure( Error, "Unexpected end of file while counting rows for the dynamic table" );
    
        // Rewind to the start
        if( fseek( m_pFP, LastPosition, SEEK_SET ) )
            return xerr_failure_s( "Fail to reposition the cursor back to the right place while reading the file" );

        return Error;
    }
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// Class functions
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
namespace xcore::textfile::version_2
{
    //------------------------------------------------------------------------------------------------
    // Some static variables
    //------------------------------------------------------------------------------------------------

    //------------------------------------------------------------------------------------------------
    bool stream::isValidType( int Type ) const noexcept
    {
        switch( Type )
        {
            // Lets verify that the user_types enter a valid atomic err
            case 'f': case 'F':
            case 'd': case 'D':
            case 'c': case 'C':
            case 's': 
            case 'g': case 'G':
            case 'h': case 'H':
            return true;
        }

        // Sorry but I don't know what kind of syntax / err information 
        // the user_types is trying to provide.
        // Make sure that there are not extra character after the err ex:
        // "Pepe:fff " <- this is bad. "Pepe:fff" <- this is good.
        return false;
    }

    //-----------------------------------------------------------------------------------------------------

    xcore::err stream::openForReading( const std::filesystem::path& FilePath ) noexcept
    {
        xcore::err Failure;

        //
        // Check if we have a real file
        //
        {
            std::error_code ec;
            if( std::filesystem::exists( FilePath, ec ) == false )
            {
                return xerr_code( Failure, error_state::FILE_NOT_FOUND, "Fail to find the file in the specify directory" );
            }

            if( ec )
            {
                return xerr_code( Failure, error_state::FILE_NOT_FOUND, "Fail to detect if the file is in the correct directory" );
            }
        }

        //
        // Okay make sure that we say that we are not reading the file
        // this will force the user_types to stick with the writing functions
        //
        xcore::error::scope CleanUp( Failure, [&]{ close(); });


        // Open the file in binary and make sure everything is ok
        if( m_File.openForReading(FilePath, true).isError(Failure) )
            return Failure;

        //
        // Determine signature (check if we are reading a binary file or not)
        //
        {
            std::uint32_t Signature = 0;
            if( m_File.Read(Signature).isError(Failure) && (Failure.getCode().getState<error_state>() != error_state::UNEXPECTED_EOF) )
            {
                return Failure;
            }
            else 
            {
                Failure.clear();
                if( Signature == std::uint32_t('NOIL') || Signature == std::uint32_t('LION') )
                {
                    if( Signature == std::uint32_t('LION') )
                        m_File.m_States.m_isEndianSwap = true;
                }
                else // We are dealing with a text file, if so the reopen it as such
                {
                    m_File.close();
                    if( m_File.openForReading(FilePath, false).isError(Failure) )
                        return Failure;
                }
            }
        }

        //
        // get ready to start reading
        //
        m_Memory.clear();

        // Growing this guy is really slow so we create a decent count from the start
        if( m_Memory.capacity() < 1048 ) m_Memory.appendList( 1048 );

        //
        // Read the first record
        //
        if( ReadRecord().isError( Failure ) ) return Failure;

        return Failure;
    }

    //-----------------------------------------------------------------------------------------------------

    xcore::err stream::openForWriting( const std::filesystem::path& FilePath, file_type FileType, flags Flags ) noexcept
    {
        //
        // Okay make sure that we say that we are not reading the file
        // this will force the user_types to stick with the writing functions
        //
        xcore::err      Failure;
        xcore::error::scope  CleanUp( Failure, [&]{ close(); });

        //
        // Open the file
        //
        if( m_File.openForWritting( FilePath, FileType == file_type::BINARY ).isError( Failure ) ) 
            return Failure;

        //
        // Determine whether we are binary or text base
        //
        if( FileType == file_type::BINARY )
        {
            // Write binary signature
            const std::uint32_t Signature = std::uint32_t('NOIL');
            if( m_File.Write( Signature ).isError( Failure ) )
                return Failure;
        }

        //
        // Handle flags
        //
        m_File.m_States.m_isEndianSwap = Flags.m_isWriteEndianSwap;
        m_File.m_States.m_isSaveFloats = Flags.m_isWriteFloats;

        //
        // Initialize some of the Write variables
        //
        m_Memory.clear();

        // Growing this guy is really slow so we create a decent count from the start
        if( m_Memory.capacity() < 2048 ) m_Memory.appendList( 2048 );

        return Failure;
    }

    //-----------------------------------------------------------------------------------------------------

    void stream::close( void ) noexcept
    {
        m_File.close();
    }

    //------------------------------------------------------------------------------------------------

    xcore::err stream::Open ( bool isRead, std::string_view View, file_type FileType, flags Flags ) noexcept
    {
        xcore::err Error;
        if( isRead )
        {
            if( openForReading( View ).isError(Error) ) return Error;
        }
        else
        {
            if( openForWriting( View, FileType, Flags ).isError(Error) ) return Error;
        }
        return Error;
    }

    //------------------------------------------------------------------------------

    std::uint32_t stream::AddUserType( const user_defined_types& UserType ) noexcept
    {
        if( const auto It = m_UserTypeMap.find(UserType.m_CRC); It != m_UserTypeMap.end() )
        {
            const auto Index = It->second;
            
            // Make sure that this registered err is fully duplicated
            xassert( UserType.m_SystemTypes == m_UserTypes[Index].m_SystemTypes );
        
            // we already have it so we don't need to added again
            return Index;
        }

        //
        // Sanity check the user_types types
        //
        xassert_block_basic()
        {
            for( int i=0; UserType.m_Name[i]; i++ )
            {
                xassert( false == std::isspace(UserType.m_Name[i]) );
            }
    
            for( int i=0; UserType.m_SystemTypes[i]; i++ )
            {
                xassert( false == std::isspace(UserType.m_SystemTypes[i]) );
            }
        }

        //
        // Fill the entry
        //
        std::uint32_t Index  = m_UserTypes.getIndexByEntry<std::uint32_t>( m_UserTypes.append(UserType) );
        m_UserTypeMap.insert( {UserType.m_CRC, Index} );

        return Index;
    }

    //------------------------------------------------------------------------------
    void stream::AddUserTypes( xcore::span<user_defined_types> UserTypes ) noexcept
    {
        for( auto& T : UserTypes )
            AddUserType( T );
    }

    //------------------------------------------------------------------------------
    void stream::AddUserTypes( xcore::span<const user_defined_types> UserTypes ) noexcept
    {
        for( auto& T : UserTypes )
            AddUserType( T );
    }

    //------------------------------------------------------------------------------

    xcore::err stream::WriteRecord( const char* pHeaderName, std::size_t Count ) noexcept
    {
        xcore::err Error;

        xassert( pHeaderName );
        xassert( m_File.m_States.m_isReading == false );

        //
        // Fill the record info
        //
        string::Copy( m_Record.m_Name, pHeaderName );
        if( Count == ~0 ) 
        { 
            m_Record.m_bWriteCount  = false;  
            m_Record.m_Count        = 1; 
        }
        else
        { 
            m_Record.m_bWriteCount  = true; 
            m_Record.m_Count        = static_cast<int>(Count); 
        }

        //
        // Reset the line count
        //
        m_iLine         = 0;
        m_iColumn       = 0;
        m_iMemOffet     = 0;
        m_nColumns      = 0;

        return Error;
    }

    //------------------------------------------------------------------------------
    static 
    bool isCompatibleTypes( char X, arglist::out_types Y ) noexcept 
    {
        return std::visit([&]( auto Value )
        {
            using t = std::decay_t<decltype(Value)>;
                    if constexpr ( std::is_same_v<t,bool*>               ) return X == 'c';
            else    if constexpr ( std::is_same_v<t,std::uint8_t*>       ) return X == 'h';
            else    if constexpr ( std::is_same_v<t,std::uint16_t*>      ) return X == 'H';
            else    if constexpr ( std::is_same_v<t,std::uint32_t*>      ) return X == 'g';
            else    if constexpr ( std::is_same_v<t,std::uint64_t*>      ) return X == 'G';
            else    if constexpr ( std::is_same_v<t,std::int8_t*>        ) return X == 'c';
            else    if constexpr ( std::is_same_v<t,std::int16_t*>       ) return X == 'C';
            else    if constexpr ( std::is_same_v<t,std::int32_t*>       ) return X == 'd';
            else    if constexpr ( std::is_same_v<t,std::int64_t*>       ) return X == 'D';
            else    if constexpr ( std::is_same_v<t,float*>              ) return X == 'f';
            else    if constexpr ( std::is_same_v<t,double*>             ) return X == 'F';
            else    if constexpr ( std::is_same_v<t,string::view<char>*> ) return X == 's';
            else    if constexpr ( std::is_same_v<t,string::ref<char>*>  ) return X == 's';
            else    return false;
        }, Y );
    }

    //------------------------------------------------------------------------------

    xcore::err stream::WriteComment( const string::view<const char> Comment ) noexcept
    {
        xcore::err Error;

        if( m_File.m_States.m_isReading )
            return Error;

        if( m_File.m_States.m_isBinary )
        {
            return Error;
        }
        else
        {
            int iStart = 0;
            int iEnd   = iStart;

            if( m_File.WriteChar( '\n' ).isError( Error ) )
                return Error;

            do 
            {
                if( m_File.WriteStr( "//" ).isError( Error ) )
                    return Error;

                xassert( iEnd < Comment.size() );

                while( Comment[iEnd] ) 
                    if( Comment[iEnd] == '\n' ) { iEnd++; break; }
                    else                          iEnd++;

                if( m_File.WriteStr( Comment.ViewFromTo(iStart, iEnd) ).isError( Error ) )
                    return Error;

                if( Comment[iEnd] == 0 ) break;

                // Get ready for the next line
                iStart = iEnd++;

            } while( true );
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::WriteUserTypes( void ) noexcept
    {
        xcore::err Error;

        if( m_File.m_States.m_isBinary )
        {
            bool bHaveUserTypes = false;
            for( auto& UserType : m_UserTypes )
            {
                if( UserType.m_bAlreadySaved )
                    continue;
            
                UserType.m_bAlreadySaved = true;
            
                // First write the standard symbol for the user_types types
                if( bHaveUserTypes == false )
                {
                    bHaveUserTypes = true;
                    if( m_File.WriteChar( '<' ).isError(Error) )
                        return Error;
                }

                // Write the name 
                if( m_File.WriteStr( { UserType.m_Name.data(), static_cast<string::details::size_t>(UserType.m_NameLength+1) } ).isError(Error) )
                    return Error;

                // Write type/s
                if( m_File.WriteStr( { UserType.m_SystemTypes.data(), static_cast<string::details::size_t>(UserType.m_nSystemTypes+1) } ).isError(Error) )
                    return Error;
            }
        }
        else
        {
            //
            // Dump all the new types if we have some
            //
            bool bNewTypes = false;
            for( auto& UserType : m_UserTypes )
            {
                if( UserType.m_bAlreadySaved )
                    continue;

                if( bNewTypes == false ) 
                {
                    if( m_File.WriteStr( string::constant{"\n// New Types\n< "} ).isError(Error) ) return Error;
                    bNewTypes = true;
                }

                UserType.m_bAlreadySaved = true;
                string::fixed<char,256> temp;

                string::sprintf( temp, "%s:%s ", UserType.m_Name.data(), UserType.m_SystemTypes.data() );
                if( m_File.WriteStr( temp ).isError(Error) ) return Error;
            }

            if( bNewTypes ) if( m_File.WriteStr( string::constant(">\n") ).isError(Error) ) return Error;
        }

        return Error;
    }

    //------------------------------------------------------------------------------
    
    xcore::err stream::WriteColumn( xcore::crc<32> UserType, const char* pColumnName, arglist::view Args ) noexcept
    {
        //
        // Make sure we always have enough memory
        //
        if( (m_iMemOffet + 1024*2) > m_Memory.size() ) 
            m_Memory.appendList( 1024*4 );

        //
        // When we are at the first line we must double check the syntax of the user_types
        //
        if( m_iLine == 0 )
        {
            // Make sure we have enough columns to store our data
            if( m_Columns.size() <= m_nColumns ) 
            {
                m_Columns.append();
            }
            else
            {
                m_Columns[m_nColumns].clear();
            }

            auto& Column = m_Columns[m_nColumns++];
            
            xassert( Args.size() > 0 );
            Column.m_nTypes   = static_cast<int>(Args.size());
            Column.m_UserType = UserType;

            // Copy name of the field
            for( Column.m_NameLength=0; (Column.m_Name[Column.m_NameLength] = pColumnName[Column.m_NameLength]); Column.m_NameLength++ )
            {
                // Handle dynamic types
                if( pColumnName[Column.m_NameLength] == ':' && pColumnName[Column.m_NameLength+1] == '?' )
                {
                    Column.m_nTypes                     = -1;
                    Column.m_UserType.m_Value           = 0;
                    Column.m_Name[Column.m_NameLength]  = 0;
                    break;
                }

                // Make sure the field name falls inside these constraints 
                xassert( (pColumnName[Column.m_NameLength] >= 'a' && pColumnName[Column.m_NameLength] <= 'z')
                     ||  (pColumnName[Column.m_NameLength] >= 'A' && pColumnName[Column.m_NameLength] <= 'Z')
                     ||  (pColumnName[Column.m_NameLength] >= '0' && pColumnName[Column.m_NameLength] <= '9')
                     ||  (pColumnName[Column.m_NameLength] == '_' ) 
                );
            }
        }

        //
        // Get the column we are processing
        //
        auto& Column = m_Columns[ m_iColumn++ ];

        //
        // Validate the user_types type if any
        //
        xassert_block_basic()
        {
            if( UserType.m_Value && ( Column.m_nTypes == -1 || m_iLine==0 ) )
            {
                auto pUserType = getUserType( UserType );
                    
                // Make sure the user_types register this user_types type
                xassert(pUserType);
                xassert(pUserType->m_nSystemTypes == Args.size() );
                for( auto& A  : Args )
                {
                    xassert(isCompatibleTypes( pUserType->m_SystemTypes[Args.getIndexByEntry(A)], A ));
                }
            }
        }

        //
        // Create all the fields for this column
        //
        if( Column.m_nTypes == -1 || m_iLine == 0 )
        {
            //
            // If is a dynamic type we must add the infos
            //
            if( Column.m_nTypes == -1 )
            {
                auto& DynamicFields = Column.m_DynamicFields.append();
                DynamicFields.m_nTypes   = static_cast<int>(Args.size());
                DynamicFields.m_UserType = UserType;
                DynamicFields.m_iField   = static_cast<int>(Column.m_FieldInfo.size());
            }

            //
            // Write each of the fields types
            //
            {
                auto& FieldInfo = (Column.m_nTypes == -1) ? Column.m_DynamicFields.last() : Column; 
                for( auto& A : Args )
                {
                    FieldInfo.m_SystemTypes[Args.getIndexByEntry(A)] = std::visit( [&]( auto Value ) constexpr
                    {
                        using t = std::decay_t<decltype(Value)>;
                                if constexpr ( std::is_same_v<t,bool*>               ) return 'c';
                        else    if constexpr ( std::is_same_v<t,std::uint8_t*>       ) return 'h';
                        else    if constexpr ( std::is_same_v<t,std::uint16_t*>      ) return 'H';
                        else    if constexpr ( std::is_same_v<t,std::uint32_t*>      ) return 'g';
                        else    if constexpr ( std::is_same_v<t,std::uint64_t*>      ) return 'G';
                        else    if constexpr ( std::is_same_v<t,std::int8_t*>        ) return 'c';
                        else    if constexpr ( std::is_same_v<t,std::int16_t*>       ) return 'C';
                        else    if constexpr ( std::is_same_v<t,std::int32_t*>       ) return 'd';
                        else    if constexpr ( std::is_same_v<t,std::int64_t*>       ) return 'D';
                        else    if constexpr ( std::is_same_v<t,float*>              ) return 'f';
                        else    if constexpr ( std::is_same_v<t,double*>             ) return 'F';
                        else    if constexpr ( std::is_same_v<t,string::view<char>*> ) return 's';
                        else    if constexpr ( std::is_same_v<t,string::ref<char>*>  ) return 's';
                        else    { xassume(false); return char{0}; }
                    }, A );
                }

                // Terminate types as a proper string
                FieldInfo.m_SystemTypes[ static_cast<int>(Args.size()) ] = 0;
            }
        }

        //
        // Ready to buffer the actual fields
        //
        if( m_File.m_States.m_isBinary )
        {
            //
            // Write to a buffer the data
            //
            for( auto& A : Args )
            {
                auto& FieldInfo = Column.m_FieldInfo.append(); 

                std::visit( [&]( auto p ) constexpr
                {
                    using T = std::decay_t<decltype(p)>;

                    if constexpr ( std::is_same_v<T,string::ref<char>*> || std::is_same_v<T,string::view<char>*> ) 
                    {
                        FieldInfo.m_iData = m_iMemOffet;
                        m_iMemOffet += 1 + xcore::string::Copy<string::view<char>,string::view<char>>( m_Memory.ViewFrom(FieldInfo.m_iData), *p ).m_Value;
                        FieldInfo.m_Width = m_iMemOffet - FieldInfo.m_iData;
                    }
                    else
                    {
                        if constexpr ( std::is_pointer_v<T> == false 
                            || std::is_same_v<T,void*> 
                            || std::is_same_v<T,const char*> ) 
                        {
                            xassert(false);
                        }
                        else 
                        {
                            static_assert ( std::is_pointer_v<T> );
                            constexpr auto size = sizeof(decltype(*p)); 
                            FieldInfo.m_Width = size;
                            FieldInfo.m_iData = bits::Align( m_iMemOffet, std::alignment_of_v<T> );
                            m_iMemOffet = FieldInfo.m_iData + FieldInfo.m_Width;

                                    if constexpr ( size ==  1 ) 
                                    {
                                        auto& x = reinterpret_cast<std::uint8_t& >(m_Memory[FieldInfo.m_iData]);
                                        if constexpr ( std::is_same_v<bool,T> )
                                        {
                                            x = *p;
                                        }
                                        else
                                        {
                                            x = reinterpret_cast<std::uint8_t&>(*p);
                                        }
                                    }
                            else    if constexpr ( size == 2 ) 
                                    {
                                        auto& x = reinterpret_cast<std::uint16_t& >(m_Memory[FieldInfo.m_iData]);
                                        x = reinterpret_cast<std::uint16_t&>(*p);
                                        if( m_File.m_States.m_isEndianSwap ) x = endian::Convert(x);
                                    }
                            else    if constexpr ( size == 4 ) 
                                    {
                                        auto& x = reinterpret_cast<std::uint32_t& >(m_Memory[FieldInfo.m_iData]);
                                        x = reinterpret_cast<std::uint32_t&>(*p);
                                        if( m_File.m_States.m_isEndianSwap ) x = endian::Convert(x);
                                    }
                            else    if constexpr ( size == 8 ) 
                                    {
                                        auto& x = reinterpret_cast<std::uint64_t& >(m_Memory[FieldInfo.m_iData]);
                                        x = reinterpret_cast<std::uint64_t&>(*p);

                                        if( m_File.m_States.m_isEndianSwap ) 
                                            x = endian::Convert(x);
                                    }
                        }
                    }
                }, A );
            }
        }
        else
        {
            //
            // Lambda Used to write the fields
            //
            auto Numerics = [&]( details::field_info& FieldInfo, auto p, const char* pFmt ) noexcept
            {
                using T = std::decay_t<decltype(p)>;

                FieldInfo.m_iData = m_iMemOffet;
                m_iMemOffet += 1 + string::sprintf( m_Memory.ViewFrom(m_iMemOffet), pFmt, p ).m_Value;
                FieldInfo.m_Width = m_iMemOffet - FieldInfo.m_iData - 1;

                if constexpr ( std::is_integral_v<T> )
                {
                    FieldInfo.m_IntWidth = FieldInfo.m_Width;
                }
                else
                {
                    for( FieldInfo.m_IntWidth = 0; m_Memory[FieldInfo.m_iData + FieldInfo.m_IntWidth] != '.' && m_Memory[FieldInfo.m_iData+FieldInfo.m_IntWidth]; FieldInfo.m_IntWidth++ );
                }
            };

            //
            // Write to a buffer the data
            //
            for( auto& A : Args )
            {
                auto& Field = Column.m_FieldInfo.append(); 

                std::visit( [&]( auto p ) constexpr
                {
                    using t = std::decay_t<decltype(p)>;
                            if constexpr ( std::is_same_v<t,std::uint8_t*>      ) Numerics( Field, *p, "#%X" );
                    else    if constexpr ( std::is_same_v<t,std::uint16_t*>     ) Numerics( Field, *p, "#%X" );
                    else    if constexpr ( std::is_same_v<t,std::uint32_t*>     ) Numerics( Field, *p, "#%X" );
                    else    if constexpr ( std::is_same_v<t,std::uint64_t*>     ) Numerics( Field, *p, "#%LX" );
                    else    if constexpr ( std::is_same_v<t,bool*>              ) Numerics( Field, (int)*p, "%d" );
                    else    if constexpr ( std::is_same_v<t,std::int8_t*>       ) Numerics( Field, *p, "%d" );
                    else    if constexpr ( std::is_same_v<t,std::int16_t*>      ) Numerics( Field, *p, "%d" );
                    else    if constexpr ( std::is_same_v<t,std::int32_t*>      ) Numerics( Field, *p, "%d" );
                    else    if constexpr ( std::is_same_v<t,std::int64_t*>      ) Numerics( Field, *p, "%ld" );
                    else    if constexpr ( std::is_same_v<t,float*>             ) 
                            { 
                                if( m_File.m_States.m_isSaveFloats )    Numerics( Field, static_cast<double>(*p), "%Lg" );
                                else                                    Numerics( Field, reinterpret_cast<std::uint32_t&>(*p),  "#%X" ); 
                            }
                    else    if constexpr ( std::is_same_v<t,double*>            ) 
                            { 
                                if( m_File.m_States.m_isSaveFloats )    Numerics( Field, *p, "%Lg" );
                                else                                    Numerics( Field, reinterpret_cast<std::uint64_t&>(*p),  "#%LX" ); 
                            }
                    else    if constexpr ( std::is_same_v<t,string::view<char>> ) 
                            {
                                Field.m_iData = m_iMemOffet;
                                m_iMemOffet += 1 + string::sprintf( m_Memory.ViewFrom(Field.m_iData), "\"%s\"", p.data() ).m_Value;
                                Field.m_Width = m_iMemOffet - Field.m_iData - 1;
                            }
                    else    if constexpr ( std::is_same_v<t,string::ref<char>*> || std::is_same_v<t,string::view<char>*> ) 
                            {
                                Field.m_iData = m_iMemOffet;
                                m_iMemOffet += 1 + string::sprintf( m_Memory.ViewFrom(Field.m_iData), "\"%s\"", p->data() ).m_Value;
                                Field.m_Width = m_iMemOffet - Field.m_iData - 1;
                            }
                    else    
                    { 
                        xassume(false); 
                    }
                }, A );
            }
        }

        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err stream::WriteLine( void ) noexcept
    {
        xcore::err  Error;

        xassert( m_File.m_States.m_isReading == false );

        // Make sure that the user_types don't try to write more lines than expected
        xassert( m_iLine < m_Record.m_Count );

        //
        // Increment the line count
        // and reset the column index
        //
        m_iLine++;
        m_iColumn   = 0;

        //
        // We will wait writing the line if we can so we can format
        //
        if( (m_iLine < m_Record.m_Count && (m_iLine%m_nLinesBeforeFileWrite) != 0) )
        {
            return Error;
        }


        //
        // Lets handle the binary case first
        //
        if( m_File.m_States.m_isBinary )
        {
            if( m_iLine <= m_nLinesBeforeFileWrite )
            {
                //
                // Write any pending user_types types
                //
                if( WriteUserTypes().isError(Error))
                    return Error;
            
                //
                // Write record header
                //
                if( m_File.WriteChar( '[' ).isError(Error) )
                    return Error;

                if( m_File.WriteStr( { m_Record.m_Name.data(), string::Length( m_Record.m_Name ).m_Value+1 } ).isError( Error ) )
                    return Error;

                if( m_File.Write( m_Record.m_Count ).isError( Error ) )
                    return Error;

                //
                // Write types
                //
                {
                    std::uint8_t nColumns = static_cast<std::uint8_t>(m_nColumns);
                    if( m_File.Write( nColumns ).isError( Error ) ) return Error;
                }

                for( int i=0; i<m_nColumns; ++i )
                {
                    auto& Column = m_Columns[i];

                    if( m_File.WriteStr( string::view{ Column.m_Name.data(), static_cast<string::details::size_t>(Column.m_NameLength) } ).isError( Error ) )
                        return Error;

                    if( Column.m_nTypes == -1 )
                    {
                        if( m_File.WriteChar( '?' ).isError( Error ) )
                            return Error;
                    }
                    else
                    {
                        if( Column.m_UserType.m_Value )
                        {
                            if( m_File.WriteChar( ';' ).isError( Error ) )
                                return Error;

                            auto Index = m_UserTypes.getIndexByEntry<std::uint8_t>( *getUserType(Column.m_UserType) );
                            if( m_File.Write( Index ).isError( Error ) )
                                return Error;
                        }
                        else
                        {
                            if( m_File.WriteChar( ':' ).isError( Error ) )
                                return Error;

                            if( m_File.WriteStr( { Column.m_SystemTypes.data(), static_cast<string::details::size_t>(Column.m_nTypes + 1) } ).isError( Error ) )
                                return Error;
                        }
                    }
                }
            } // End of first line

            //
            // Dump line info
            //
            int L = m_iLine%m_nLinesBeforeFileWrite;
            if( L == 0 ) L = m_nLinesBeforeFileWrite;
            for( int l = 0; l<L; ++l )
            {
                for( int i = 0; i<m_nColumns; ++i )
                {
                    const auto& Column        = m_Columns[i];

                    if( Column.m_nTypes == -1 )
                    {
                        const auto& DynamicFields = Column.m_DynamicFields[l];

                        //
                        // First write the type
                        //
                        if( DynamicFields.m_UserType.m_Value ) 
                        {
                            auto p     = getUserType( DynamicFields.m_UserType );
                            auto Index = m_UserTypes.getIndexByEntry<std::uint8_t>( *p );

                            if( m_File.WriteChar( ';' ).isError(Error) )
                                return Error;

                            if( m_File.Write( Index ).isError(Error) )
                                return Error;
                        }
                        else
                        {
                            if( m_File.WriteChar( ':' ).isError(Error) )
                                return Error;

                            if( m_File.WriteStr( string::view{ DynamicFields.m_SystemTypes.data(), static_cast<string::details::size_t>(DynamicFields.m_nTypes + 1) } ).isError(Error) )
                                return Error;
                        }

                        //
                        // Then write the values
                        //
                        for( int n=0; n<DynamicFields.m_nTypes; ++n )
                        {
                            const auto& FieldInfo   = Column.m_FieldInfo[ DynamicFields.m_iField + n ];
                            if( m_File.WriteData( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                return Error;
                        }
                    }
                    else
                    {
                        for( int n=0; n<Column.m_nTypes; ++n )
                        {
                            const auto  Index       = l*Column.m_nTypes + n;
                            const auto& FieldInfo   = Column.m_FieldInfo[ Index ];
                            if( m_File.WriteData( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                return Error;
                        }
                    }
                }
            }

            //
            // Clear the memory pointer
            //
            goto CLEAR ;
        }

        //
        // Initialize the columns
        //
        if( m_iLine <= m_nLinesBeforeFileWrite )
        {
            for( int i=0; i<m_nColumns; ++i )
            {
                auto& Column            = m_Columns[i];
                
                Column.m_FormatTotalSubColumns = 0;
                Column.m_FormatWidth           = 0;

                if( Column.m_nTypes == -1 )
                {
                    Column.m_SubColumn.clear();
                    Column.m_SubColumn.appendList(2);
                }
                else
                {
                    Column.m_SubColumn.clear();
                    Column.m_SubColumn.appendList(Column.m_nTypes);
                }
            }
        }

        //
        // Compute the width for each column also for each of its types
        //
        for( int i = 0; i<m_nColumns; ++i )
        {
            auto& Column        = m_Columns[i];

            if( Column.m_nTypes == -1 )
            {
                auto& TypeSubColumn   = Column.m_SubColumn[0];
                auto& ValuesSubColumn = Column.m_SubColumn[1];

                //
                // Compute the width with all the fields 
                //
                for( auto& DField : Column.m_DynamicFields )
                {
                    {
                        // Add the spaces between each field
                        DField.m_FormatWidth = (DField.m_nTypes-1) * m_nSpacesBetweenFields;

                        // count all the widths of the types
                        for( int n=0; n<DField.m_nTypes; n++ )
                        {
                            const auto& FieldInfo   = Column.m_FieldInfo[ DField.m_iField + n ];
                            DField.m_FormatWidth += FieldInfo.m_Width;
                        }

                        ValuesSubColumn.m_FormatWidth = std::max( ValuesSubColumn.m_FormatWidth, DField.m_FormatWidth );
                    }

                    // Dynamic types must include the type inside the column
                    if( DField.m_UserType.m_Value )
                    {
                        auto p = getUserType(DField.m_UserType);
                        xassert(p);
                        TypeSubColumn.m_FormatWidth = std::max( TypeSubColumn.m_FormatWidth, p->m_NameLength + 1 ); // includes ";"
                    }
                    else
                    {
                        TypeSubColumn.m_FormatWidth = std::max( TypeSubColumn.m_FormatWidth, DField.m_nTypes + 1 ); // includes ":"
                    }
                }

                //
                // Handle the column name as well 
                //
                Column.m_FormatNameWidth = Column.m_NameLength + 1 + 1; // includes ":?"
                Column.m_FormatWidth           = std::max( Column.m_FormatWidth, ValuesSubColumn.m_FormatWidth + TypeSubColumn.m_FormatWidth + m_nSpacesBetweenFields );
                Column.m_FormatTotalSubColumns = std::max( Column.m_FormatTotalSubColumns, Column.m_FormatWidth );
                Column.m_FormatWidth           = std::max( Column.m_FormatWidth, Column.m_FormatNameWidth );
            }
            else
            {
                //
                // For any sub-columns that are floats we need to compute the max_int first
                //
                for( int it =0; it<Column.m_nTypes; ++it )
                {
                    if( Column.m_SystemTypes[it] != 'f' && Column.m_SystemTypes[it] != 'F' )
                        continue;
                    
                    auto& SubColumn = Column.m_SubColumn[it];
                    for( int iff = it; iff < Column.m_FieldInfo.size(); iff += Column.m_nTypes )
                    {
                        auto& Field = Column.m_FieldInfo[iff];
                        SubColumn.m_FormatIntWidth = std::max( SubColumn.m_FormatIntWidth, Field.m_IntWidth );
                    }
                }

                //
                // Computes the sub-columns sizes
                //
                for( auto& Field : Column.m_FieldInfo )
                {
                    const int iSubColumn = Column.m_FieldInfo.getIndexByEntry(Field)%Column.m_nTypes;
                    auto&     SubColumn  = Column.m_SubColumn[iSubColumn];
                    if( Column.m_SystemTypes[iSubColumn] == 'f' || Column.m_SystemTypes[iSubColumn] == 'F' )
                    {
                        const int Width = (Field.m_Width - Field.m_IntWidth) + SubColumn.m_FormatIntWidth;
                        SubColumn.m_FormatWidth = std::max( SubColumn.m_FormatWidth, Width );
                    }
                    else
                    {
                        SubColumn.m_FormatWidth = std::max( SubColumn.m_FormatWidth, Field.m_Width );
                    }
                }

                //
                // Compute the columns sizes
                //

                // Add all the spaces between fields
                Column.m_FormatWidth = (Column.m_nTypes - 1) * m_nSpacesBetweenFields;

                xassert( Column.m_nTypes == Column.m_SubColumn.size() );
                // Add all the sub-columns widths
                for( auto& Subcolumn : Column.m_SubColumn )
                {
                    Column.m_FormatWidth += Subcolumn.m_FormatWidth;
                }

                //
                // Handle the column name as well 
                //
                Column.m_FormatNameWidth = Column.m_NameLength + 1; // includes ":"
                if( Column.m_UserType.m_Value )
                {
                    auto p = getUserType( Column.m_UserType );
                    xassert(p);
                    Column.m_FormatNameWidth += p->m_NameLength;
                }
                else
                {
                    Column.m_FormatNameWidth += Column.m_nTypes; 
                }

                // Decide the final width for this column
                Column.m_FormatTotalSubColumns = std::max( Column.m_FormatTotalSubColumns, Column.m_FormatWidth );
                Column.m_FormatWidth = std::max( Column.m_FormatWidth, Column.m_FormatNameWidth );
            }
        }

        //
        // Save the record info
        //
        if( m_iLine <= m_nLinesBeforeFileWrite )
        {
            //
            // Write any pending user_types types
            //
            if( WriteUserTypes().isError( Error ) )
                return Error;
        
            //
            // Write header
            //
            if( m_Record.m_bWriteCount )
            {
                if( m_File.WriteFmtStr( "\n[ %s : %d ]\n", m_Record.m_Name.data(), m_Record.m_Count ).isError( Error ) )
                    return Error;
            }
            else
            {
                if( m_File.WriteFmtStr( "\n[ %s ]\n", m_Record.m_Name.data() ).isError(Error) )
                    return Error;
            }

            //
            // Write the types
            //
            {
                if( m_File.WriteStr( "{ " ).isError(Error) )
                    return Error;

                for( int i = 0; i<m_nColumns; ++i )
                {
                    auto& Column        = m_Columns[i];

                    if( Column.m_nTypes == -1 )
                    {
                        if( m_File.WriteFmtStr( "%s:?", Column.m_Name.data() ).isError(Error) )
                            return Error;
                    }
                    else
                    {
                        if( Column.m_UserType.m_Value )
                        {
                            auto p = getUserType(Column.m_UserType);
                            xassert(p);

                            if( m_File.WriteFmtStr( "%s;%s", Column.m_Name.data(), p->m_Name.data() ).isError(Error) )
                                return Error;
                        }
                        else
                        {
                            if( m_File.WriteFmtStr( "%s:%s", Column.m_Name.data(), Column.m_SystemTypes.data() ).isError(Error) )
                                return Error;
                        }
                    }

                    // Write spaces to reach the end of the column
                    if( Column.m_FormatWidth > Column.m_FormatNameWidth )
                    {
                        if( m_File.WriteChar( ' ', Column.m_FormatWidth - Column.m_FormatNameWidth ).isError(Error) )
                            return Error;
                    }

                    if( (i+1) != m_nColumns )
                    {
                        // Write spaces between columns
                        if( m_File.WriteChar( ' ', m_nSpacesBetweenColumns ).isError(Error) ) return Error;
                    }
                }

                if( m_File.WriteFmtStr( string::constant(" }\n") ).isError(Error) )
                    return Error;
            }

            //
            // Write a nice underline for the columns
            //
            {
                if( m_File.WriteStr( string::constant("//") ).isError(Error) )
                    return Error;

                for( int i = 0; i<m_nColumns; ++i )
                {
                    auto& Column  = m_Columns[i];

                    if( m_File.WriteChar( '-', Column.m_FormatWidth ).isError(Error) )
                        return Error;

                    // Get ready for the next type
                    if( (i+1) != m_nColumns) if( m_File.WriteChar( ' ', m_nSpacesBetweenColumns ).isError(Error) ) return Error;
                }

                if( m_File.WriteChar( '\n' ).isError(Error) )
                    return Error;
            }
        }

        //
        // Print all the data
        //
        {
            int L = m_iLine%m_nLinesBeforeFileWrite;
            if( L == 0 ) L = m_nLinesBeforeFileWrite;
            for( int l = 0; l<L; ++l )
            {
                // Prefix with two spaces to align things
                if( m_File.WriteChar( ' ', 2 ).isError(Error) ) return Error;

                for( int i = 0; i<m_nColumns; ++i )
                {
                    const auto& Column        = m_Columns[i];

                    if( Column.m_nTypes == -1 )
                    {
                        const auto& DynamicFields = Column.m_DynamicFields[l];

                        //
                        // First write the type
                        //
                        if( DynamicFields.m_UserType.m_Value ) 
                        {
                            auto p = getUserType( DynamicFields.m_UserType );
                            xassert(p);
                            if( m_File.WriteFmtStr( ";%s", p->m_Name.data() ).isError(Error) )
                                return Error;

                            // Fill spaces to reach the next column
                            if( m_File.WriteChar( ' ', Column.m_SubColumn[0].m_FormatWidth - p->m_NameLength -1 + m_nSpacesBetweenFields ).isError(Error) )
                                return Error;
                        }
                        else
                        {
                            if( m_File.WriteFmtStr( ":%s", DynamicFields.m_SystemTypes.data() ).isError(Error) )
                                return Error;

                            // Fill spaces to reach the next column
                            if( m_File.WriteChar( ' ', Column.m_SubColumn[0].m_FormatWidth - DynamicFields.m_nTypes -1 + m_nSpacesBetweenFields ).isError(Error) )
                                return Error;
                        }

                        //
                        // Then write the values
                        //
                        for( int n=0; n<DynamicFields.m_nTypes; ++n )
                        {
                            const auto& FieldInfo   = Column.m_FieldInfo[ DynamicFields.m_iField + n ];
                    
                            if( m_File.WriteStr( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                return Error;
                                
                            // Get ready for the next type
                            if( (DynamicFields.m_nTypes-1) != n) if( m_File.WriteChar( ' ', m_nSpacesBetweenFields ).isError(Error) )
                                return Error;
                        }

                        // Pad the width to match the columns width
                        if( m_File.WriteChar( ' ', Column.m_FormatWidth 
                            - DynamicFields.m_FormatWidth
                            - Column.m_SubColumn[0].m_FormatWidth 
                            - m_nSpacesBetweenFields  ).isError(Error) )
                            return Error;
                    }
                    else
                    {
                        const auto  Center      = Column.m_FormatWidth - Column.m_FormatTotalSubColumns;

                        if( (Center>>1) > 0 )
                            if( m_File.WriteChar( ' ', Center>>1).isError(Error) ) return Error;

                        for( int n=0; n<Column.m_nTypes; ++n )
                        {
                            const auto  Index       = l*Column.m_nTypes + n;
                            const auto& FieldInfo   = Column.m_FieldInfo[ Index ];
                            const auto& SubColumn   = Column.m_SubColumn[ n ];

                            if( Column.m_SystemTypes[n] == 'f' || Column.m_SystemTypes[n] == 'F' )
                            {
                                // point align Right align
                                if( m_File.WriteChar( ' ', SubColumn.m_FormatIntWidth - FieldInfo.m_IntWidth ).isError(Error) )
                                    return Error;

                                if( m_File.WriteStr( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                    return Error;

                                // Write spaces to reach the next sub-column
                                int nSpaces = SubColumn.m_FormatWidth - ( SubColumn.m_FormatIntWidth + FieldInfo.m_Width - FieldInfo.m_IntWidth );
                                if( m_File.WriteChar( ' ', nSpaces ).isError(Error) ) return Error;
                            }
                            else if( Column.m_SystemTypes[n] == 's' )
                            {
                                // Left align
                                if( m_File.WriteStr( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                    return Error;

                                if( m_File.WriteChar( ' ', SubColumn.m_FormatWidth - FieldInfo.m_Width ).isError(Error) )
                                    return Error;
                            }
                            else
                            {
                                // Right align
                                if( m_File.WriteChar( ' ', SubColumn.m_FormatWidth - FieldInfo.m_Width ).isError(Error) )
                                    return Error;

                                if( m_File.WriteStr( string::view{ &m_Memory[ FieldInfo.m_iData ], static_cast<string::details::size_t>(FieldInfo.m_Width) } ).isError(Error) )
                                    return Error;
                            }

                            // Write spaces to reach the next sub-column
                            if( (n+1) != Column.m_nTypes ) 
                            {
                                if( m_File.WriteChar( ' ', m_nSpacesBetweenFields ).isError(Error) ) return Error;
                            }
                        }

                        // Add spaces to finish this column
                        if( Center > 0 )
                            if( m_File.WriteChar( ' ', Center - (Center>>1) ).isError(Error) ) return Error;
                    }

                    // Write spaces to reach the next column
                    if((i+1) != m_nColumns) if( m_File.WriteChar( ' ', m_nSpacesBetweenColumns ).isError(Error) ) return Error;
                }

                // End the line
                if( m_File.WriteStr( string::constant("\n") ).isError(Error) )
                    return Error;
            }
        }

        //
        // Reset to get ready for the next block of lines
        //
        CLEAR:
        // Clear the columns
        if( m_iLine < m_Record.m_Count )
        {
            for( int i=0; i<m_nColumns; ++i )
            {
                auto& C = m_Columns[i];

                C.m_DynamicFields.clear();
                C.m_FieldInfo.clear();
            }
        }
        // Clear the memory pointer
        m_iMemOffet = 0;

        return Error;
    }

    //------------------------------------------------------------------------------
    inline
    bool stream::ValidateColumnChar( int c ) const noexcept
    {
        return false
            || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c=='_')
            || (c==':')
            || (c=='>')
            || (c=='<')
            || (c=='?')
            || (c >= '0' && c <= '9' )
            ;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::BuildTypeInformation( const char* pFieldName ) noexcept
    {
        xcore::err Error;
        xcore::error::scope Scope( Error, [&]
        {
            XLOG_CHANNEL_ERROR( m_Channel, "%s [%s] of Record[%s]", Error.getCode().m_pString, pFieldName, m_Record.m_Name.data() );
        });

        // Make sure we have enough columns to store our data
        if( m_Columns.size() <= m_nColumns ) 
        {
            m_Columns.append();
        }
        else
        {
            m_Columns[m_nColumns].clear();
        }

        auto& Column = m_Columns[m_nColumns++];
    
        //
        // Copy the column name 
        //
        for( Column.m_NameLength=0; pFieldName[Column.m_NameLength] !=';' && pFieldName[Column.m_NameLength] !=':'; ++Column.m_NameLength )
        {
            if( pFieldName[Column.m_NameLength]==0 || Column.m_NameLength >= static_cast<int>(Column.m_Name.size()) ) 
                return xerr_failure( Error, "Fail to read a column named, either string is too long or it has no types" );

            Column.m_Name[Column.m_NameLength] = pFieldName[Column.m_NameLength];
        }

        // Terminate name 
        Column.m_Name[Column.m_NameLength] = 0;

        if( pFieldName[Column.m_NameLength] ==';' )
        {
            Column.m_UserType = xcore::crc<32>::FromString( &pFieldName[Column.m_NameLength+1] );
            auto p = getUserType( Column.m_UserType );
            if( p == nullptr )
                return xerr_failure( Error, "Fail to find the user type for a column" );

            Column.m_nTypes = p->m_nSystemTypes;
            string::Copy( Column.m_SystemTypes, p->m_SystemTypes );

        }
        else
        {
            xassert(pFieldName[Column.m_NameLength] ==':');

            if( pFieldName[Column.m_NameLength+1] == '?' )
            {
                Column.m_nTypes = -1;
                Column.m_SystemTypes.clear();
            }
            else
            {
                Column.m_nTypes = string::Copy( Column.m_SystemTypes, &pFieldName[Column.m_NameLength+1] ).m_Value;
                if( Column.m_nTypes <= 0 )
                    return xerr_failure( Error, "Fail to read a column, type. not system types specified" );
            }

            Column.m_UserType.m_Value = 0;
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::ReadFieldUserType( const char* pColumnName ) noexcept
    {
        xcore::err Error;

        if( m_iLine == 1 )
        {
            m_DataMapping.append() = -1;
            //xassert( m_iColumn == (m_DataMapping.size()-1) );

            // Find the column name
            bool bFound = false;
            for( int i=0; i<m_nColumns; ++i )
            {
                auto& Column = m_Columns[i];

                if( pColumnName[ Column.m_NameLength ] != 0 )
                {
                    if( pColumnName[ Column.m_NameLength ] != ':' || pColumnName[ Column.m_NameLength+1 ] != '?' )
                        continue;
                }

                // Make sure that we have a match
                {
                    int j;
                    for( j=Column.m_NameLength-1; j >= 0 && (Column.m_Name[j] == pColumnName[j]) ; --j );

                    // The string must not be the same
                    if( j != -1 )
                        continue;

                    m_DataMapping.last() = i;
                    bFound = true;
                    break;
                }
            }

            if( bFound == false )
            {
                XLOG_CHANNEL_WARNING( m_Channel, "Unable to find the field %s", pColumnName );
                return xerr_code( Error, error_state::FIELD_NOT_FOUND, "Unable to find the filed requrested" );
            }
        }

        if( -1 == m_DataMapping[m_iColumn] )
            return xerr_code( Error, error_state::FIELD_NOT_FOUND, "Skipping unkown field" );

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::ReadFieldUserType( xcore::crc<32>& UserType, const char* pColumnName ) noexcept
    {
        xcore::err Error;
        error::scope Scope( Error, [&]
        {
            // If we have any error lets make sure we always move to the next column
             m_iColumn++;
        });

        //
        // Read the new field
        //
        if( ReadFieldUserType(pColumnName).isError(Error) ) return Error; 

        //
        // Get ready to read to column
        //
        auto& Column = m_Columns[m_DataMapping[m_iColumn]];

        //
        // if the type is '?' then check the types every call
        //
        if( Column.m_nTypes == -1 )
        {
            UserType = Column.m_DynamicFields[0].m_UserType;
        }
        else
        {
            UserType = Column.m_UserType;
        }

        //
        // Let the system know that we have moved on
        //
        m_iColumn++;

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::ReadColumn( const xcore::crc<32> iUserRef, const char* pColumnName, arglist::view Args ) noexcept
    {
        xassert( m_iLine > 0 );

        xcore::err      Error;
        error::scope    Scope( Error, [&]
        {
            // If we have any error lets make sure we always move to the next column
             m_iColumn++;
        });

        //
        // Get the user type if any
        //
        if( ReadFieldUserType( pColumnName ).isError(Error) )
            return Error;

        //
        // Create a mapping from user_types order to file order of fields
        //
        if( m_iLine == 1 )
        {
            // If we have a mapping then lets see if we can determine the types
            if( m_DataMapping[m_iColumn] == - 1 )
            {
                auto& Column = m_Columns[m_DataMapping[m_iColumn]];
                if( Column.m_nTypes >= Args.size() )
                {
                    if( Column.m_FieldInfo.size() == Args.size() )
                    {
                        for( int i=0; i<Column.m_FieldInfo.size(); i++ )
                        {
                            if( isCompatibleTypes( Column.m_SystemTypes[i], Args[i] ) == false )
                            {
                                m_DataMapping.last() = -1;
                                XLOG_CHANNEL_WARNING( m_Channel, "Found the coloumn but the types did not match. %s", pColumnName );
                                break;
                            }
                        }
                    }
                    else
                    {
                        // We don't have the same count
                        m_DataMapping.last() = -1;
                        XLOG_CHANNEL_WARNING( m_Channel, "Found the coloumn but the type count did not match. %s", pColumnName );
                    }
                }
            }
        }

        //
        // Get ready to read to column
        //
        auto& Column = m_Columns[m_DataMapping[m_iColumn]];

        //
        // if the type is '?' then check the types every call
        //
        if( Column.m_nTypes == -1 )
        {
            if( Column.m_FieldInfo.size() == Args.size() )
            {
                auto& D = Column.m_DynamicFields[0];
                for( int i=0; i<D.m_nTypes; i++ )
                {
                    if( isCompatibleTypes( D.m_SystemTypes[i], Args[i] ) == false )
                    {
                        XLOG_CHANNEL_WARNING( m_Channel, "Found the coloumn but the types did not match. %s", pColumnName );
                        return xerr_code( Error, error_state::READ_TYPES_DONTMATCH, "Fail to find the correct type" );
                    }
                }
            }
            else
            {
                // We don't have the same count
                XLOG_CHANNEL_WARNING( m_Channel, "Found the coloumn but the type count did not match. %s", pColumnName );
                return xerr_code( Error, error_state::READ_TYPES_DONTMATCH, "Fail to find the correct type" );
            }
        }

        //
        // Read each type
        //
        for( int i=0; i<Args.size(); i++ )
        {
            const auto& E    = Args[i];
            const auto iData = Column.m_FieldInfo[i].m_iData;

            std::visit( [&]( auto p )
            {
                using t = std::decay_t<decltype(p)>;

                if constexpr ( std::is_same_v<t,std::uint8_t*> || std::is_same_v<t,std::int8_t*> || std::is_same_v<t,bool*> )
                {
                    reinterpret_cast<std::uint8_t&>(*p) = reinterpret_cast<const std::uint8_t&>( m_Memory[ iData ]);
                }
                else if constexpr( std::is_same_v<t,std::uint16_t*> || std::is_same_v<t,std::int16_t*> )
                {
                    reinterpret_cast<std::uint16_t&>(*p) = reinterpret_cast<const std::uint16_t&>( m_Memory[ iData ]);
                }
                else if constexpr ( std::is_same_v<t,std::uint32_t*> || std::is_same_v<t,std::int32_t*> || std::is_same_v<t,float*> )
                {
                    reinterpret_cast<std::uint32_t&>(*p) = reinterpret_cast<const std::uint32_t&>( m_Memory[ iData ]);
                }
                else if constexpr ( std::is_same_v<t,std::uint64_t*> || std::is_same_v<t,std::int64_t*> || std::is_same_v<t,double*> )
                {
                    reinterpret_cast<std::uint64_t&>(*p) = reinterpret_cast<const std::uint64_t&>( m_Memory[ iData ]);
                }
                else if constexpr ( std::is_same_v<t,string::view<char>*> || std::is_same_v<t,string::ref<char>*> )
                {
                    string::Copy( *p, &m_Memory[ iData ] );
                }
                else
                {
                    xassume( false );
                }
            }, E );
        }

        //
        // The user_types reference
        //
     //   iUserRef = static_cast<int>(Column.m_UserType.m_Value);

        //
        // Ready to read the next column
        //
        m_iColumn++;

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::ReadLine( void ) noexcept
    {
        int                     c;
        int                     Size=0;
        string::fixed<char,256> Buffer;
        xcore::err              Error;

        // Make sure that the user_types doesn't read more lines than the record has
        xassert( m_iLine <= m_Record.m_Count );

        //
        // If it is the first line we must read the type information before hand
        //
        if( m_iLine == 0 )
        {
            // Reset the user_types field offsets
            m_DataMapping.clear();

            // Solve types
            if( m_File.m_States.m_isBinary )
            {
                // Read the number of columns
                {
                    std::uint8_t nColumns;
                    if( m_File.Read(nColumns).isError(Error) ) 
                        return Error;

                    m_nColumns = nColumns;
                    m_Columns.clear();
                    m_Columns.appendList( m_nColumns );
                }

                //
                // Read all the types
                //
                for( int l=0; l<m_nColumns; l++)
                {
                    auto& Column = m_Columns[l];

                    // Name
                    Column.m_NameLength = 0;
                    do 
                    {
                        if( m_File.getC(c).isError(Error) ) return Error;
                        Column.m_Name[Column.m_NameLength++] = c;
                    } while( c != ':'
                          && c != ';'
                          && c != '?' );

                    Column.m_NameLength--;
                    Column.m_Name[Column.m_NameLength] = 0;

                    // Read type information
                    if( c == ':' )
                    {
                        Column.m_nTypes = 0;
                        do 
                        {
                            if( m_File.getC(c).isError(Error) ) return Error;
                            Column.m_SystemTypes[Column.m_nTypes++] = c;
                        } while(c);
                        Column.m_nTypes--;
                        Column.m_UserType.m_Value = 0;
                    }
                    else if( c == ';' )
                    { 
                        std::uint8_t Index;
                        if( m_File.Read(Index).isError(Error) ) return Error;

                        auto& UserType = m_UserTypes[Index];
                        Column.m_UserType       = UserType.m_CRC;
                        Column.m_nTypes         = UserType.m_nSystemTypes;
                        Column.m_FormatWidth    = Index;
                    }
                    else if( c == '?' )
                    {
                        Column.m_nTypes = -1;
                        Column.m_UserType.m_Value = 0;
                    }
                }
            }
            else
            {
                // Read out all the white space
                if( m_File.ReadWhiteSpace(c).isError( Error ) )
                    return Error;

                //
                // we should have the right character by now
                //
                if( c != '{' ) return xerr_failure( Error, "Unable to find the types" );

                // Get the next token
                if( m_File.ReadWhiteSpace(c).isError( Error ) )
                    return Error;

                do
                {
                    // Read a word
                    Size=0;
                    while( ValidateColumnChar(c) || c == ';' || c == ':' )
                    {
                        Buffer[Size++] = c;                    
                        if( m_File.getC(c).isError(Error) ) return Error;
                    }
            
                    // Terminate the string
                    Buffer[Size++] = 0;

                    // Okay build the type information
                    if( BuildTypeInformation( Buffer ).isError(Error) ) 
                        return Error;

                    // Read any white space
                    if( m_File.ReadWhiteSpace(c).isError( Error ) )
                        return Error;

                } while( c != '}' );
            }
        }

        //
        // Read the actual data
        //
        if( m_File.m_States.m_isBinary )
        {
            auto ReadData = [&]( details::field_info& Info, int SystemType )
            {
                xcore::err Error;

                switch( SystemType )
                {
                    case 'c': 
                    case 'h':
                    {
                        std::uint8_t H;
                        if( m_File.Read(H).isError(Error) ) return Error;
                        Info.m_iData = bits::Align( m_iMemOffet, 1); m_iMemOffet = Info.m_iData + 1; reinterpret_cast<std::uint8_t &>(m_Memory[Info.m_iData]) = static_cast<std::uint8_t>(H);
                        Info.m_Width = 1;
                        break;
                    }
                    case 'C':
                    case 'H':
                    {
                        std::uint16_t H;
                        if( m_File.Read(H).isError(Error) ) return Error;
                        Info.m_iData = bits::Align( m_iMemOffet, 2); m_iMemOffet = Info.m_iData + 2; reinterpret_cast<std::uint16_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint16_t>(H);
                        Info.m_Width = 2;
                        break;
                    }
                    case 'f':
                    case 'd':
                    case 'g':
                    {
                        std::uint32_t H;
                        if( m_File.Read(H).isError(Error) ) return Error;
                        Info.m_iData = bits::Align( m_iMemOffet, 4); m_iMemOffet = Info.m_iData + 4; reinterpret_cast<std::uint32_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint32_t>(H);
                        Info.m_Width = 4;
                        break;
                    }
                    case 'F':
                    case 'G':
                    case 'D':
                    {
                        std::uint64_t H;
                        if( m_File.Read(H).isError(Error) ) return Error;
                        Info.m_iData = bits::Align( m_iMemOffet, 8); m_iMemOffet = Info.m_iData + 8; reinterpret_cast<std::uint64_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint64_t>(H);
                        Info.m_Width = 8;
                        break;
                    }
                    case 's':
                    {
                        int c;
                        Info.m_iData = m_iMemOffet;
                        do
                        {
                            if( m_File.getC(c).isError(Error) ) return Error;
                            m_Memory[m_iMemOffet++] = c;
                        } while(c); 
                        Info.m_Width = m_iMemOffet - Info.m_iData;
                        break;
                    }
                }

                return Error;
            };

            for( m_iColumn=0; m_iColumn<m_nColumns; ++m_iColumn )
            {
                auto& Column = m_Columns[m_iColumn];

                Column.m_FieldInfo.clear();
                if( Column.m_nTypes == -1 )
                {
                    Column.m_DynamicFields.clear();
                    auto& D = Column.m_DynamicFields.append();
                    D.m_iField = 0;

                    // Get the first key code
                    if( m_File.getC(c).isError(Error) ) return Error;

                    // Read type information
                    if( c == ':' )
                    {
                        D.m_nTypes = 0;
                        do 
                        {
                            if( m_File.getC(c).isError(Error) ) return Error;
                            D.m_SystemTypes[D.m_nTypes++] = c;
                        } while(c);
                        D.m_nTypes--;
                        D.m_UserType.m_Value = 0;
                    }
                    else if( c == ';' )
                    { 
                        std::uint8_t Index;
                        if( m_File.Read(Index).isError(Error) ) return Error;

                        auto& UserType = m_UserTypes[Index];
                        D.m_UserType = UserType.m_CRC;
                        D.m_nTypes   = UserType.m_nSystemTypes;
                        for( int i=0; i<D.m_nTypes; ++i) D.m_SystemTypes[i] = UserType.m_SystemTypes[i];
                    }
                    else
                    {
                        xassert(false);
                    }

                    //
                    // Read all the data
                    //
                    for( int i=0; i<D.m_nTypes; i++ )
                    {
                        if( ReadData( Column.m_FieldInfo.append(), D.m_SystemTypes[i] ).isError(Error) ) return Error;
                    }
                }
                else
                {
                    //
                    // Read all the data
                    //
                    if( Column.m_UserType.m_Value )
                    {
                        auto& UserType = m_UserTypes[Column.m_FormatWidth];
                        for( int i=0; i<UserType.m_nSystemTypes; i++ )
                        {
                            if( ReadData( Column.m_FieldInfo.append(), UserType.m_SystemTypes[i] ).isError(Error) ) return Error;
                        }
                    }
                    else
                    {
                        for( int i=0; i<Column.m_nTypes; i++ )
                        {
                            if( ReadData( Column.m_FieldInfo.append(), Column.m_SystemTypes[i] ).isError(Error) ) return Error;
                        }
                    }
                }
            }
        }
        else
        {
            //
            // Okay now we must read a line worth of data
            //    
            auto ReadComponent = [&]( details::field_info& Info, char SystemType ) noexcept
            {
                xcore::err Error;

                if( c == ' ' )
                    if( m_File.ReadWhiteSpace(c).isError( Error ) )
                        return Error;

                Size = 0;
                if ( c == '"' )
                {
                    if( SystemType != 's' )
                        return xerr_failure( Error, "Unexpeced string value expecting something else");

                    Info.m_iData = m_iMemOffet;
                    do 
                    {
                        if( m_File.getC(c).isError(Error) ) return Error;
                        m_Memory[m_iMemOffet++] = c;
                    } while( c != '"' );

                    m_Memory[m_iMemOffet-1] = 0;
                }
                else
                {
                    std::uint64_t H;

                    if( c == '#' )
                    {
                        if( m_File.getC(c).isError(Error) ) return Error;
                        while( string::isCharHex(c) ) 
                        {
                            Buffer[Size++] = c;                    
                            if( m_File.getC(c).isError(Error) ) return Error;
                        }

                        if( Size == 0 )
                            return xerr_failure( Error, "Fail to read a numeric value");

                        Buffer[Size++] = 0;

                        H = strtoull( Buffer, nullptr, 16 );
                    }
                    else
                    {
                        bool isInt = true;

                        if( c == '-' )
                        {
                            Buffer[Size++] = c;                    
                            if( m_File.getC(c).isError(Error) ) return Error;
                        }

                        while( string::isCharDigit(c) ) 
                        {
                            Buffer[Size++] = c;                    
                            if( m_File.getC(c).isError(Error) ) return Error;
                            if( c == '.' )
                            {
                                // Continue reading as a float
                                isInt = false;
                                do 
                                {
                                    Buffer[Size++] = c;                    
                                    if( m_File.getC(c).isError(Error) ) return Error;
                                    
                                } while( string::isCharDigit(c) || c == 'e' || c == 'E' || c == '-' );
                                break;
                            }
                        }

                        if( Size == 0 )
                            return xerr_failure( Error, "Fail to read a numeric value");

                        Buffer[Size++] = 0;


                             if( SystemType == 'F' ) reinterpret_cast<double&>(H) = atof( Buffer );
                        else if( SystemType == 'f' ) reinterpret_cast<float&>(H)  = static_cast<float>(atof( Buffer ));
                        else if( isInt == false )
                                {
                                    return xerr_code( Error, error_state::MISMATCH_TYPES, "I found a floating point number while trying to load an interger value");
                                }
                        else if( Buffer[0] == '-' )
                        {
                            if(    SystemType == 'g' 
                                || SystemType == 'G' 
                                || SystemType == 'h' 
                                || SystemType == 'H' )
                            {
                                XLOG_CHANNEL_WARNING( m_Channel, "Reading a sign interger into a field which is unsign-int form this record [%s](%d)", m_Record.m_Name.data(), m_iLine );
                            }

                            H = static_cast<std::uint64_t>(strtoll( Buffer, nullptr, 10 ));
                        }
                        else
                        {
                            H = static_cast<std::uint64_t>(strtoull( Buffer, nullptr, 10 ));
                            if(    (SystemType == 'c' && H >= static_cast<std::uint64_t>(std::numeric_limits<std::int8_t>::max() ))
                                || (SystemType == 'C' && H >= static_cast<std::uint64_t>(std::numeric_limits<std::int16_t>::max()))
                                || (SystemType == 'd' && H >= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                                || (SystemType == 'D' && H >= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                                )
                            {
                                XLOG_CHANNEL_WARNING( m_Channel, "Reading a sign interger but the value in the file exceeds the allow positive interger portion in record [%s](%d)", m_Record.m_Name.data(), m_iLine );
                            }
                        }
                    }

                    if( c != ' ' && c != '\n' ) 
                        return xerr_failure( Error, "Expecting a space separator but I got a different character" );

                    switch( SystemType )
                    {
                        case 'c': 
                        case 'h':
                                    Info.m_iData = bits::Align( m_iMemOffet, 1); m_iMemOffet = Info.m_iData + 1; reinterpret_cast<std::uint8_t &>(m_Memory[Info.m_iData]) = static_cast<std::uint8_t>(H);
                                    break;
                        case 'C':
                        case 'H':
                                    Info.m_iData = bits::Align( m_iMemOffet, 2); m_iMemOffet = Info.m_iData + 2; reinterpret_cast<std::uint16_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint16_t>(H);
                                    break;
                        case 'f':
                        case 'd':
                        case 'g':
                                    Info.m_iData = bits::Align( m_iMemOffet, 4); m_iMemOffet = Info.m_iData + 4; reinterpret_cast<std::uint32_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint32_t>(H);
                                    break;
                        case 'F':
                        case 'G':
                        case 'D':
                                    Info.m_iData = bits::Align( m_iMemOffet, 8); m_iMemOffet = Info.m_iData + 8; reinterpret_cast<std::uint64_t&>(m_Memory[Info.m_iData]) = static_cast<std::uint64_t>(H);
                                    break;
                    }
                }

                return Error;
            };

            for( m_iColumn=0; m_iColumn<m_nColumns; ++m_iColumn )
            {
                auto& Column = m_Columns[m_iColumn];

                // Read any white space
                if( m_File.ReadWhiteSpace(c).isError( Error ) )
                    return Error;

                Column.m_FieldInfo.clear();
                if( Column.m_nTypes == -1 )
                {
                    if( c != ':'  && c != ';' )
                        return xerr_failure( Error, "Expecting a type definition" );

                    Column.m_DynamicFields.clear();
                    auto& D = Column.m_DynamicFields.append();
                    D.m_iField = 0;

                    Size = 0;
                    {
                        int x;
                        do 
                        {
                            if( m_File.getC(x).isError(Error) ) return Error;
                            Buffer[Size++] = x;
                        } while( string::isCharSpace(x) == false );

                        Buffer[Size-1] = 0;
                    }

                    if( c == ';' )
                    {
                        D.m_UserType = xcore::crc<32>::FromString( Buffer.data() );
                        auto p = getUserType( D.m_UserType );
                        if( p == nullptr )
                            return xerr_failure( Error, "Fail to find the user type for a column" );

                        D.m_nTypes = p->m_nSystemTypes;
                        string::Copy( D.m_SystemTypes, p->m_SystemTypes );
                    }
                    else
                    {
                        xassert(c ==':');
                        D.m_nTypes = string::Copy( D.m_SystemTypes, Buffer ).m_Value;
                        if( D.m_nTypes <= 0 )
                            return xerr_failure( Error, "Fail to read a column, type. not system types specified" );
                    }

                    // Read all the types
                    c = ' ';
                    for( int n=0; n<D.m_nTypes ;n++ )
                    {
                        auto& Field = Column.m_FieldInfo.append();
                        if( ReadComponent( Field, D.m_SystemTypes[n] ).isError(Error) )
                            return Error;
                    }
                }
                else
                {
                    for( int n=0; n<Column.m_nTypes ;n++ )
                    {
                        auto& Field = Column.m_FieldInfo.append();
                        if( ReadComponent( Field, Column.m_SystemTypes[n] ).isError(Error) )
                            return Error;
                    }
                }
            }
        }

        //
        // Increment the line count
        // reset the memory count
        //
        m_iLine++;
        m_iColumn   = 0;
        m_iMemOffet = 0;

        return Error;
    }

//------------------------------------------------------------------------------
    // Description:
    //      The second thing you do after the read the file is to read a record header which is what
    //      this function will do. After reading the header you probably want to switch base on the 
    //      record name. To do that use GetRecordName to get the actual string containing the name. 
    //      The next most common thing to do is to get how many rows to read. This is done by calling
    //      GetRecordCount. After that you will look throw n times reading first a line and then the fields.
    //------------------------------------------------------------------------------
    xcore::err stream::ReadRecord( void ) noexcept
    {
        xcore::err   Failure;
        int          c;

        xassert( m_File.m_States.m_isReading );

        // if not we expect to read something
        if( m_File.m_States.m_isBinary ) 
        {
            // If it is the end of the file we are done
            do 
            {
                if( m_File.getC(c).isError(Failure) ) return Failure;

            } while( c != '[' && c != '<' );
        
            //
            // Lets deal with user_types types
            //
            if( c == '<' ) do
            {
                std::array<char,64> SystemType;
                std::array<char,64> UserType;
                int i;

                // Read the first character of the user type
                if( m_File.getC(c).isError(Failure) ) return Failure;
                if( c == '[' ) break;

                // Read the user_types err
                i=0;
                UserType[i++] = c;
                while( c ) 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    UserType[i++] = c;
                }

                UserType[i] = 0;

                // Read the system err
                i=0;
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( c == 0 ) break;
                    SystemType[i++] = c;
                } while( true );

                SystemType[i] = 0;

                //
                // Add the err
                //
                {
                    user_defined_types Type{ UserType.data(), SystemType.data() };
                    AddUserType( Type );
                }

            } while( true );

            //
            // Deal with a record
            //
            if( c != '[' ) return xerr_failure( Failure, "Unexpected character while reading the binary file." );

            // Read the record name
            {
                std::size_t NameSize=0;
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( NameSize >= static_cast<std::size_t>(m_Record.m_Name.size()) ) return xerr_failure( Failure, "A record name was way too long, fail to read the file." );
                    m_Record.m_Name[NameSize++] = c;
                } while (c);
            }

            // Read the record count
            if( m_File.Read( m_Record.m_Count ).isError(Failure) ) return Failure;
        }
        else
        {
            xcore::error::scope CleanUp( Failure, [&]
            {
                m_Record.m_Name[0u]=0;
            });

            //
            // Skip blank spaces and comments
            //
            if( m_File.ReadWhiteSpace( c ).isError(Failure) ) return Failure;

            //
            // Deal with user_types types
            // We read the err in case the user_types has not register it already.
            // But the user_types should have register something....
            //
            if( c == '<' )
            {
                // Read any white space
                if( m_File.ReadWhiteSpace( c ).isError(Failure) ) return Failure;

                do
                {
                    // Create a user type
                    user_defined_types UserType;

                    UserType.m_NameLength=0;
                    while( c != ':' ) 
                    {
                        UserType.m_Name[UserType.m_NameLength++] = static_cast<char>(c);
                        if( m_File.getC(c).isError(Failure) ) return Failure;
                        if( UserType.m_NameLength >= static_cast<int>(UserType.m_Name.size()) ) return xerr_failure( Failure, "Failed to find the termination character ':' for a user type");
                    }
                    UserType.m_Name[UserType.m_NameLength]=0;
                    UserType.m_CRC = xcore::crc<32>::FromString(UserType.m_Name);

                    UserType.m_nSystemTypes=0;
                    do 
                    {
                        if( m_File.getC(c).isError(Failure) ) return Failure;
                        if( c == '>' || c == ' ' ) break;
                        if( isValidType(c) == false ) return xerr_failure( Failure, "Found a non-atomic type in user type definition" );
                        UserType.m_SystemTypes[UserType.m_nSystemTypes++] = static_cast<char>(c);
                        if( UserType.m_nSystemTypes >= static_cast<int>(UserType.m_SystemTypes.size()) ) return xerr_failure( Failure, "Failed to find the termination character '>' for a user type block");
                    } while( true );
                    UserType.m_SystemTypes[UserType.m_nSystemTypes]=0;

                    //
                    // Add the User err
                    //
                    AddUserType( UserType );

                    // Read any white space
                    if( string::isCharSpace(c) )
                        if( m_File.ReadWhiteSpace( c ).isError(Failure) ) return Failure;

                } while( c != '>' );

                //
                // Skip spaces
                //
                if ( m_File.ReadWhiteSpace( c ).isError(Failure) ) return Failure;
            }
        
            //
            // Make sure that we are dealing with a header now
            //
            if( c != '[' ) 
                return xerr_failure( Failure, "Unable to find the right header symbol '['" );

            // Skip spaces
            if( m_File.ReadWhiteSpace(c).isError(Failure) ) return Failure;

            int                         NameSize = 0;
            do
            {
                m_Record.m_Name[NameSize++] = c;
            
                if( m_File.getC(c).isError(Failure) ) return Failure;

            } while( std::isspace( c ) == false && c != ':' && c != ']' );

            // Terminate the string
            m_Record.m_Name[NameSize] = 0;

            // Skip spaces
            if( std::isspace( c ) )
                if( m_File.ReadWhiteSpace(c).isError(Failure) ) return Failure;
        
            //
            // Read the record count number 
            //
            m_Record.m_Count = 1;

            if( c == ':' )
            {
                // skip spaces and zeros
                do
                {
                    if( m_File.ReadWhiteSpace(c).isError(Failure) ) return Failure;
                
                } while( c == '0' );
            
                //
                // Handle the case of dynamic sizes tables
                //
                if( c == '?' )
                {
                   // TODO: Handle the special reader
                   if( m_File.HandleDynamicTable( m_Record.m_Count ).isError(Failure) ) return Failure;
                
                    // Read next character
                   if( m_File.getC(c).isError(Failure) ) return Failure;
                }
                else
                {
                    m_Record.m_Count = 0;
                    while( c >= '0' && c <= '9' )
                    {
                        m_Record.m_Count = m_Record.m_Count * 10 + (c-'0');
                       if( m_File.getC(c).isError(Failure) ) return Failure;
                    }
                }

                // Skip spaces
                if( std::isspace( c ) )
                    if( m_File.ReadWhiteSpace(c).isError(Failure) ) return Failure;
            }

            //
            // Make sure that we are going to conclude the field correctly
            //
            if( c != ']' )
                return xerr_failure( Failure, "Fail reading the file. Expecting a '[' but didn't find it." );
        }

        //
        // Reset the line count
        //
        m_iLine         = 0;
        m_iMemOffet     = 0;
        m_nColumns      = 0;

        return {};
    }

}
