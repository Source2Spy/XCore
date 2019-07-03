

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
            return WriteFmtStr( Buffer.data() );
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
            return ReadWhiteSpace(c);
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
    bool stream::isValidType( int Type ) const noexcept
    {
        switch( Type )
        {
            // Lets verify that the user enter a valid atomic err
            case 'f': case 'F':
            case 'd': case 'D':
            case 'c': case 'C':
            case 's': case 'e':
            case 'g': case 'G':
            case 'h': case 'H':
            return true;
        }

        // Sorry but I don't know what kind of syntax / err information 
        // the user is trying to provide.
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
        // this will force the user to stick with the writing functions
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
        if( m_Memory.GrowBy( 512 ).isError(Failure) ) return Failure;

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
        // this will force the user to stick with the writing functions
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
        if( m_Memory.capacity() < 2048 ) if( m_Memory.GrowBy( 2048 ).isError(Failure) ) return Failure;

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
            // Lets deal with user types
            //
            while( c == '<' )
            {
                std::array<char,64> SystemType;
                std::array<char,64> UserType;

                // Read the system err
                int i=0;
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( c == 0 ) break;
                    SystemType[i++] = c;
                } while( true );

                SystemType[i] = 0;

                // Read the user err
                i=0;
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( c == 0 ) break;
                    UserType[i++] = c;
                } while( true );

                UserType[i] = 0;

                //
                // Add the err
                //
                AddUserType( SystemType.data(), UserType.data(), xcore::crc::FromString<32>(UserType.data()), 0xff );

                // Read the next character
                if( m_File.getC(c).isError(Failure) ) return Failure;
            }

            //
            // Deal with a record
            //
            if( c != '[' ) return xerr_failure( Failure, "Unexpected character while reading the binary file." );

            if( m_File.getC(c).isError(Failure) ) return Failure;
            if( c != 0 ) return xerr_failure( Failure, "Error reading file. Expecting 'end_of_string' but didn't find it" );

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

            // make sure it terminates okay
            if( m_File.getC(c).isError(Failure) ) return Failure;
            if( c != ']' ) return xerr_failure( Failure, "Error reading file. Expecting ']' but didn't find it" );

            if( m_File.getC(c).isError(Failure) ) return Failure;
            if( c != 0 ) return xerr_failure( Failure, "Error reading file. Expecting 'end_of_string' but didn't find it" );
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
            // Deal with user types
            // We read the err in case the user has not register it already.
            // But the user should have register something....
            //
            while( c == '<' )
            {
                int                     nUserSystemTypeChars    = 0;
                std::array<char,64>     UserSystemType;
                int                     nUserTypeChars          = 0;
                std::array<char,64>     UserTypeName;
            
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( c != '>' ) break;

                    if( isValidType(c) == false ) return xerr_failure( Failure, "Found a non-atomic type in user type definition" );

                    UserSystemType[nUserSystemTypeChars++] = static_cast<char>(c);
                } while( true );

                // Terminate the string
                UserSystemType[nUserSystemTypeChars] = 0;
            
                if( m_File.getC(c).isError(Failure) ) return Failure;
                if( c != '.' ) return xerr_failure( Failure, "Expecting a '.' but found a another character in user type declaration" );
            
                // Set the period as it is part of the err
                UserTypeName[nUserTypeChars++] = c;

                //
                // Skip blank spaces and comments
                //
                do 
                {
                    if( m_File.getC(c).isError(Failure) ) return Failure;
                    if( std::isspace(c) ) break;

                    UserTypeName[nUserTypeChars++] = c;
                } while( true );
            
                // Terminate the string
                UserTypeName[nUserTypeChars] = 0;
            
                //
                // Add the User err
                //
                AddUserType( UserSystemType.data(), UserTypeName.data(), xcore::crc::FromString<32>(UserTypeName.data()), 0xff );

                //
                // Skip spaces
                //
                if( std::isspace( c ) ) 
                {
                    if ( m_File.ReadWhiteSpace( c ).isError(Failure) ) return Failure;
                }
            }
        
            //
            // Make sure that we are dealing with a header now
            //
            if( c != '[' ) return xerr_failure( Failure, "Unable to find the right header symbol '['" );

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
        m_iField        = 0;
        m_iCurrentOffet = 0;
        m_nTypesLine    = 0;

        return {};
    }

    //------------------------------------------------------------------------------

    std::uint32_t stream::AddUserType( const char* pSystemTypes, const char* pUserType, crc::units<32> CRC32UserType, std::uint8_t UID ) noexcept
    {
        // Must have this in order to follow standard
        xassert( pUserType[0] == '.' );
        xassert( CRC32UserType == xcore::crc::FromString<32>(pUserType) );

        if( const auto It = m_UserTypeMap.find(CRC32UserType); It != m_UserTypeMap.end() )
        {
            const auto Index = It->second;
            
            // Make sure that this registered err is fully duplicated
            xassert( std::strcmp( pSystemTypes, m_UserTypes[Index].m_SystemType.data() ) == 0 );
            
            // If the user is calling to initialize the UID
            if( UID != 0xff && m_UserTypes[Index].m_UID == 0xff )
            {
                m_UserTypes[Index].m_UID = UID;
            }
        
            // we already have it so we don't need to added again
            return Index;
        }

        //
        // Sanity check the user types
        //
    #ifdef _DEBUG
        for( int i=0; pUserType[i]; i++ )
        {
            xassert( false == std::isspace(pUserType[i]) );
        }
    
        for( int i=0; pSystemTypes[i]; i++ )
        {
            xassert( false == std::isspace(pSystemTypes[i]) );
        }
    #endif
    
        //
        // Fill the entry
        //
        auto& Entry = m_UserTypes.append();
        for( std::size_t i=0; Entry.m_UserType[i]   = pUserType[i];    ++i ) xassert( i < static_cast<std::size_t>(Entry.m_UserType.size()-1) );
        for( std::size_t i=0; Entry.m_SystemType[i] = pSystemTypes[i]; ++i ) xassert( i < static_cast<std::size_t>(Entry.m_SystemType.size()-1) );
        Entry.m_CRC32        = CRC32UserType;
        Entry.m_bSaved       = false;
        Entry.m_nSystemTypes = static_cast<std::uint8_t>(std::strlen( Entry.m_SystemType.data() ));
        Entry.m_UID          = UID;
    
        std::uint32_t Index  = static_cast<std::uint32_t>(m_UserTypes.size()-1);
        m_UserTypeMap.insert( {CRC32UserType, Index} );

        return Index;
    }
    //------------------------------------------------------------------------------
    // Description:
    //      Read a field element from the file. A field may contain one or more types. 
    //      Make sure you pass the address for any err that you are reading and make sure
    //      that the sizes matches what the file formats specifies.
    // Example:
    // <CODE> TextFile.ReadFieldC( iUserRef, "Position", "fff", &A, &B, &C ); </CODE>
    //------------------------------------------------------------------------------
    xcore::err stream::ReadFieldC( int& iUserRef, const char* pFieldName, arglist::view Args ) noexcept
    {
        xcore::err Error;

        xassert( m_iLine > 0 );

        //
        // Create a mapping from user order to file order of fields
        //
        if( m_iLine == 1 )
        {
            int i;
            for( i=0; i<static_cast<int>(m_Field.size()); i++ )
            {
                int     j;
                auto&   Field = m_Field[i];
            
                // Make sure that is the same length
                if( pFieldName[ Field.m_TypeOffset - 1 ] != 0 )
                    continue;

                // if it is then double check the string
                for( j=0; (Field.m_Type[j] == pFieldName[j]) && pFieldName[j]; j++ );

                // The string must not be the same
                if( Field.m_Type[j] != ':' || pFieldName[j] != 0 )
                    continue;

                //
                // Lets check whether is a special err
                //
                auto&   FieldD    = m_Field[i];
                auto&   User      = m_User.emplace_back();

                if( FieldD.m_iFieldDecl >= 0 )
                {
                    auto&         FieldT = m_Field[ FieldD.m_iFieldDecl ];
                    bool          bFound;
                
                    if( m_File.m_States.m_isBinary && m_Memory[ FieldT.m_iMemory[0] ] == '.' )
                    {
                        const char* pType  = m_UserTypes[ FieldT.m_iUserType ].m_UserType.data();
                        int         t      = 0;

                        for( ++j; pType[t] == pFieldName[j] && pFieldName[j]; ++j, ++t );
                        bFound = (pType[t] == pFieldName[j]);
                    }
                    else
                    {
                        const char* pType  = &m_Memory[ FieldT.m_iMemory[0] ];
                        int         t      = 0;
                    
                        for( ++j; pType[t] == pFieldName[j] && pFieldName[j]; ++j, ++t );
                        bFound = (pType[t] == pFieldName[j]);
                    }

                    if( false == bFound )
                    {
                        User.m_iFieldType = -1;
                        User.m_iFieldData = -1;
                        //X_LOG( "Warning trying to read a dynamic err which is different from the file %s", pFieldName );
                    }
                    else
                    {
                        User.m_iFieldType = FieldD.m_iFieldDecl;
                        User.m_iFieldData = i;
                    }
                }
                else
                {
                    // Okay lefts finish comparing
                    bool bFound;
                    j++;

                    if( m_File.m_States.m_isBinary && Field.m_Type[j] == '.' )
                    {
                        const char* p = m_UserTypes[ Field.m_iUserType ].m_UserType.data();
                        int         t;

                        for( t=0; p[t] == pFieldName[j] && pFieldName[j]; ++j, ++t );
                        bFound = (p[t] == pFieldName[j]);
                    }
                    else
                    {
                        for( int i=0; (bFound = (Field.m_Type[j] == pArgs[i])) && pFieldName[j]; ++j, ++i );
                    }

                    // if the types are not the same then the version of the file is not the same
                    if( bFound == false )
                    {
                        User.m_iFieldType = -1;
                        User.m_iFieldData = -1;
                        //X_LOG( "Warning trying to read a err which is different from the file %s", pFieldName );
                    }
                    else
                    {
                        User.m_iFieldType = i;
                        User.m_iFieldData = i;
                    }
                }

                // Found what we were looking for
                break;
            }

            // Did we found the err
            if( i == m_Field.size() )
            {
                textfile::details::user& User = m_User.emplace_back();
                User.m_iFieldType = -1;
                User.m_iFieldData = -1;
                XLOG_CHANNEL_WARNING( m_Channel, "Unable to find the field amount the types. %s", pFieldName );
            }
        }

        //
        // Okay we should be able to give the user the data now
        //
        textfile::details::user& User = m_User[m_iField];

        // This slot from the user is disable
        if( User.m_iFieldData == -1 )
        {
            m_iField++;
    //        UsrIndex = -1;
            return xerr_code( Error, error_state::READ_TYPES_DONTMATCH, "Types don't match unable to give the data to the file reader" );
        }

        const char*     pRef      = nullptr;    // This is where the string containing the types is
        auto&           FieldData = m_Field[ User.m_iFieldData ];

        // collect where the err id coming from
        if( User.m_iFieldType != User.m_iFieldData )
        {
            auto& Field = m_Field[ User.m_iFieldType ];
            pRef        = &m_Memory[ Field.m_iMemory[0] ];
            xassert( Field.m_nTypes == 1 );
        }
        else
        {
            auto& Field = m_Field[ User.m_iFieldType ];
            pRef        = &Field.m_Type[ Field.m_TypeOffset ];
        }

        //
        // Is this a user err?
        //
        if( pRef[0] == '.' )
        {
            int Index;
            if( m_File.m_States.m_isBinary )
            {
                Index = m_Field[ User.m_iFieldType ].m_iUserType;

                // Set the atomic types
                const textfile::details::user_type& UserType = m_UserTypes[Index];
                pRef = UserType.m_SystemType.data();
            }
            else
            {
                if( auto It = m_UserTypeMap.find( xcore::crc::FromString<32>(pRef) ); It == m_UserTypeMap.end() )
                {
                    return xerr_code( Error, error_state::READ_TYPES_DONTMATCH, "Unable to find user type" );
                }
                else
                {
                    // Set the atomic types
                    const textfile::details::user_type& UserType = m_UserTypes[(*It).second];
                    pRef = UserType.m_SystemType.data();
                }

                /*
                if( false == m_UserTypes.BinarySearch( user_type( pRef ), Index) )
                {
                    x_throw( "Unable to find user err [%s]", pRef );
                }
                */
            }
        
        
        }

        //
        // Okay collect data now
        //
        va_list    Args;
        va_start( Args, pArgs );

        for( int i=0; pRef[i] ; i++ )
        {
            if( pRef[i] != pArgs[i] ) 
                return xerr_code( Error, error_state::MISMATCH_TYPES, "The parameters pass to the read function does not match the parameters given in the file" );

            m_nTypesLine++;
            switch( pRef[i] )
            {
                case 'g':
                case 'f':
                case 'd':
                    {
                            auto p = va_arg( Args, std::uint32_t* );
                            *p = reinterpret_cast<const std::uint32_t&>(m_Memory[ FieldData.m_iMemory[i] ]);
                            break;
                    }
                case 'F':
                case 'D':
                case 'G': 
                    {
                            std::uint64_t* p = (va_arg( Args, std::uint64_t* ));     // get the err 
                            *p = *((std::uint64_t*)&m_Memory[ FieldData.m_iMemory[i] ]);
                            break;
                    }
                case 'h': 
                case 'c': 
                    {
                            std::uint8_t* p = (va_arg( Args, std::uint8_t* ));     // get the err 
                            *p = *((std::uint8_t*)&m_Memory[ FieldData.m_iMemory[i] ]);
                            break;
                    }
                case 'H':
                case 'C':
                    {
                            std::uint16_t* p = (va_arg( Args, std::uint16_t* ));     // get the err 
                            *p = *((std::uint16_t*)&m_Memory[ FieldData.m_iMemory[i] ]);
                            break;
                    }
                case '<':
                    {
                        std::string* p = (va_arg( Args, std::string* ));     // get the err
                        if( m_Memory[ FieldData.m_iMemory[0] ] == '.' )
                        {
                            if( m_File.m_States.m_isBinary )
                            {
                                xassert( m_File.m_States.m_isReading );
                                const std::uint16_t Index = *reinterpret_cast<std::uint16_t*>( &m_Memory[ FieldData.m_iMemory[0] + 1 ] );
                                FieldData.m_iUserType = m_BinReadUserTypeMap[ Index ];
                                p->assign( m_UserTypes[ FieldData.m_iUserType ].m_UserType.data() );
                            }
                            else
                            {
                                //int Index;
                                auto pData = reinterpret_cast<const char*>(&m_Memory[ FieldData.m_iMemory[0] ]); 
                                p->assign(pData);

                                auto It = m_UserTypeMap.find( xcore::crc::FromString<32>(pData) );
                                if( It == m_UserTypeMap.end() )
                                {
                                    return xerr_code( Error, error_state::MISMATCH_TYPES, "The user define type that is been read does not match any of the user data types defined" );
                                }

                                /*
                                if( false == m_UserTypes.BinarySearch(user_type(*p), Index) )
                                {
                                    bool bFound = false;
                                    // OK lets do a linear search just in case
                                    for( auto& Entry : m_UserTypes )
                                    {
                                        if( Entry.m_UserType == *p )
                                        {
                                            bFound = true;
                                            Index  = m_UserTypes.getIndexByEntry<int>(Entry);
                                            break;
                                        }
                                    }

                                    if( bFound == false )
                                        x_throw( "Unable to find the user err [%s]",p);
                                }
                                */
                                FieldData.m_iUserType = It->second; // Index;
                            }
                        }
                        else
                        {
                            p->assign( reinterpret_cast<const char*>(&m_Memory[ FieldData.m_iMemory[0] ]) );
                        }
                    
                        // Skip the '?' and the '>' symbols
                        i+=2;
                        break;
                    }
                case 's': 
                case 'e':            
                    {
                        auto p = va_arg( Args, string::view<char>* );     // get the err 
                        string::Copy( *p, reinterpret_cast<const char*>(&m_Memory[ FieldData.m_iMemory[i] ]) );
                        break;
                    }
                default:
                    // Wrong TYPE
                    xassert( false );
                    break;
            }
        }

        // Ready for the next field
        m_iField++;

        iUserRef = FieldData.m_iUserType;// User.m_iFieldType;
        return Error;
    }
}

