

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

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// Details
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
namespace xcore::textfile::version_1
{
    //------------------------------------------------------------------------------
    details::file& details::file::setup( std::FILE& File, details::states States ) noexcept
    {
        close();
        m_pFP    = &File;
        m_States = States; 
        return *this;
    }

    //------------------------------------------------------------------------------
    details::file::~file( void ) noexcept 
    { 
        close(); 
    }

    //------------------------------------------------------------------------------

    xcore::err details::file::openForReading( const std::filesystem::path& FilePath, bool isBinary ) noexcept
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

    xcore::err details::file::openForWritting( const std::filesystem::path& FilePath, bool isBinary  ) noexcept
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

    void details::file::close( void ) noexcept
    {
        if(m_pFP && m_States.m_isView == false ) fclose(m_pFP);
        m_pFP = nullptr;
    }

    //------------------------------------------------------------------------------

    xcore::err details::file::ReadingErrorCheck( void ) noexcept
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
    xcore::err details::file::Read( T& Buffer, int Size, int Count ) noexcept
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

    xcore::err details::file::getC( int& c ) noexcept
    {
        xassert(m_pFP);
        if( m_States.m_isEOF ) return ReadingErrorCheck();

        c = fgetc(m_pFP);
        if( c == -1 ) return ReadingErrorCheck();
        return {};
    }

    //------------------------------------------------------------------------------

    template< typename T >
    xcore::err details::file::Write( T& Buffer, int Size, int Count ) noexcept
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

    xcore::err details::file::WriteStr( const string::view<const char> Buffer ) noexcept
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

    xcore::err details::file::WriteFmtStr( const char* pFmt, ... ) noexcept
    {
        va_list Args;
        va_start( Args, pFmt );
        if( std::vfprintf( m_pFP, pFmt, Args ) < 0 )
            return xerr_failure_s( "Fail 'fprintf' writing the required data" );
        va_end( Args );
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err details::file::WriteChar( char C, int Count ) noexcept
    {
        while( Count-- )
        {
            if( C != fputc( C, m_pFP ) )
                return xerr_failure_s( "Fail 'fputc' writing the required data" );
        }
        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err details::file::ReadWhiteSpace( int& c ) noexcept
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

    xcore::err details::file::HandleDynamicTable( int& Count ) noexcept
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool xcore::textfile::stream::isValidType( int Type ) const noexcept
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

xcore::err xcore::textfile::stream::openForReading( const std::filesystem::path& FilePath ) noexcept
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
    m_Field.clear();
    m_Memory.clear();

    // Growing this guy is really slow so we create a decent count from the start
    m_Field.resize( 64 );
    m_Memory.resize( 512 );

    //
    // Read the first record
    //
    if( ReadRecord().isError( Failure ) ) return Failure;

    return Failure;
}

//-----------------------------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::openForWriting( const std::filesystem::path& FilePath, file_type FileType, flags Flags ) noexcept
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
    m_Field.clear();
    m_Memory.clear();

    // Growing this guy is really slow so we create a decent count from the start
    if(m_Field.capacity()<128) m_Field.resize( 128 );
    if(m_Memory.capacity()<2048) m_Memory.resize( 2048 );

    return Failure;
}

//-----------------------------------------------------------------------------------------------------

void xcore::textfile::stream::close( void ) noexcept
{
}

//-----------------------------------------------------------------------------------------------------

void xcore::textfile::stream::ReadFromFile( std::FILE& File, file_type FileType ) noexcept
{
    textfile::details::states States;
    States.m_isReading    = true;
    States.m_isBinary     = FileType == file_type::BINARY;
    States.m_isView       = true;

    m_File.setup( File, States );

    // Growing this guy is really slow so we create a decent count from the start
    m_Field.reserve( 64 );
    m_Memory.reserve( 512 );
}

//-----------------------------------------------------------------------------------------------------

void xcore::textfile::stream::WriteToFile( std::FILE& File, file_type FileType ) noexcept
{
    textfile::details::states States;
    States.m_isReading    = false;
    States.m_isBinary     = FileType == file_type::BINARY;
    States.m_isView       = true;

    m_File.setup( File, States );

    // Growing this guy is really slow so we create a decent count from the start
    m_Field.reserve( 64 );
    m_Memory.reserve( 512 );
}

//------------------------------------------------------------------------------

std::uint32_t xcore::textfile::stream::AddUserType( const char* pSystemTypes, const char* pUserType, crc::units<32> CRC32UserType, std::uint8_t UID ) noexcept
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
    textfile::details::user_type Entry;
    for( std::size_t i=0; Entry.m_UserType[i]   = pUserType[i];    ++i ) xassert( i < static_cast<std::size_t>(Entry.m_UserType.size()-1) );
    for( std::size_t i=0; Entry.m_SystemType[i] = pSystemTypes[i]; ++i ) xassert( i < static_cast<std::size_t>(Entry.m_SystemType.size()-1) );
    Entry.m_CRC32        = CRC32UserType;
    Entry.m_bSaved       = false;
    Entry.m_nSystemTypes = static_cast<std::uint8_t>(std::strlen( Entry.m_SystemType.data() ));
    Entry.m_UID          = UID;
    
    m_UserTypes.push_back( Entry );
    std::uint32_t Index = static_cast<std::uint32_t>(m_UserTypes.size()-1);

    m_UserTypeMap.insert( {CRC32UserType, Index} );

    return Index;
}

//------------------------------------------------------------------------------
// Description:
//      The second thing you do after the read the file is to read a record header which is what
//      this function will do. After reading the header you probably want to switch base on the 
//      record name. To do that use GetRecordName to get the actual string containing the name. 
//      The next most common thing to do is to get how many rows to read. This is done by calling
//      GetRecordCount. After that you will look throw n times reading first a line and then the fields.
//------------------------------------------------------------------------------
xcore::err xcore::textfile::stream::ReadRecord( void ) noexcept
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

            /*
            // Make sure that we set the right mapping for it
            auto it         = std::lower_bound( m_BinReadUserTypeMap.begin(), m_BinReadUserTypeMap.end(), Index );
            auto iSubIndex  = std::distance( m_BinReadUserTypeMap.begin(), it );
            m_BinReadUserTypeMap.insert( iSubIndex, Index );
            */
            /*
            m_BinReadUserTypeMap.BinarySearch( Index, iSubIndex );
            m_BinReadUserTypeMap.Insert( iSubIndex ) = Index;
            */

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
    m_iMemOffet = 0;
    m_nTypesLine    = 0;

    //
    // Kill all the lists
    //
    m_Field.clear();

    return {};
}

//------------------------------------------------------------------------------
// Description:
//      Read a field element from the file. A field may contain one or more types. 
//      Make sure you pass the address for any err that you are reading and make sure
//      that the sizes matches what the file formats specifies.
// Example:
// <CODE> TextFile.ReadFieldC( iUserRef, "Position", "fff", &A, &B, &C ); </CODE>
//------------------------------------------------------------------------------
xcore::err xcore::textfile::stream::ReadFieldC( int& iUserRef, const char* pFieldName, const char* const pArgs, ... ) noexcept
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

//------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::WriteRecord( const char* pHeaderName, std::size_t Count ) noexcept
{
    xcore::err Error;

    xassert( pHeaderName );
    xassert( m_File.m_States.m_isReading == false );

    //
    // Fill the record info
    //
    string::Copy( m_Record.m_Name, pHeaderName );
    m_Record.m_bWriteCount  = Count != ~0;
    m_Record.m_Count        = m_Record.m_bWriteCount?static_cast<int>(Count):1;

    //
    // Reset the line count
    //
    m_iLine         = 0;
    m_iField        = 0;
    m_iMemOffet = 0;
    m_nTypesLine    = 0;

    //
    // Kill all the lists
    //
    m_Field.clear();    

    return Error;
}

//------------------------------------------------------------------------------

void xcore::textfile::stream::WriteFieldC( const char* pFieldName, const char* pArgs, ... ) noexcept
{
    va_list Args1, Args2;
    va_start( Args1, pArgs );
    va_start( Args2, pArgs );
    WriteFieldC( pFieldName, pArgs, Args1, Args2, false );
    va_end(Args1);
    va_end(Args2);
}

//------------------------------------------------------------------------------

void xcore::textfile::stream::WriteFieldCIndirect( const char* pFieldName, const char* pArgs, ... ) noexcept
{
    va_list Args1, Args2;
    va_start( Args1, pArgs );
    va_start( Args2, pArgs );
    WriteFieldC( pFieldName, pArgs, Args1, Args2,  true );
    va_end(Args1);
    va_end(Args2);
}

//------------------------------------------------------------------------------

void xcore::textfile::stream::WriteFieldC( const char* pFieldName, const char* pArgsTypes, va_list& Args1, va_list& Args2, bool bIndirect ) noexcept
{   
    xassert( m_File.m_States.m_isReading == false );

    //
    // When we are at the first line we must double check the syntax of the user
    //
    if( m_iLine == 0 )
    {
        BuildTypeInformation( pFieldName, pArgsTypes );

        //
        // Initialize all the width for formating the fields
        //
        auto& Field  = m_Field[ m_Field.size()-1 ];
        Field.m_Width = Field.m_iFieldDecl >= 0 ? string::Length(pFieldName).m_Value : string::Length(Field.m_Type).m_Value;
        
        for( std::uint32_t i=0; i<Field.m_TypeWidth.size(); i++ )
        {
            Field.m_TypeWidth[i] = 0;
            Field.m_FracWidth[i] = 0;
        }
    }

    //
    // Okay ready to write the data
    //
    auto& Field = m_Field[m_iField];

    switch( Field.m_iFieldDecl )
    {
    case -2:
        {
            /*
            const string::view<char> p = (va_arg( Args2, string::view<char> ));     // get the type

            // Are we writing with a user type?
            if( p[0] == '.' )
            {
                int Index;
                
                 if( m_File.m_States.m_isReading )
                 {
                     Index = Field.m_iUserType;
                 }
                else
                {
                    // We must be able to find the user type
                    auto R = m_UserTypeMap.find( xcore::crc::FromString<32>(p) );
                    xassert_verify( m_UserTypeMap.end() != R );// .BinarySearch( user_type( p ), Index ) );
                    Index = R->second; 
                }
                const auto& Entry = m_UserTypes[Index];
                
                // Now we can set the number of types
                Field.m_DynamicTypeCount = Entry.m_nSystemTypes;
                Field.m_iUserType        = static_cast<std::uint16_t>(Index);
            }
            else
            {
                // Make sure that the user is fallowing the syntax of the format.
                // types should be contain like this: "fff"
                Field.m_DynamicTypeCount = 0;
                / *
                for( int i=0; p[i]; i++ )
                {
                    xassert( isValidType( p[i] ));
                    Field.m_DynamicTypeCount++;
                }
                * /
            }


            // Write type information in the file
            if( m_File.m_States.m_isBinary ) Field.m_iMemory[0] = m_iMemOffet;
            else                             Field.m_iMemory[0] = m_iMemOffet + 4;
            
            WriteComponent( m_iField, 0, '?', Args1, bIndirect );
            */

            // Write type information in the file
            if( m_File.m_States.m_isBinary ) Field.m_iMemory[0] = m_iMemOffet;
            else                             Field.m_iMemory[0] = (m_iMemOffet += 4);

            const auto indexStr = m_iMemOffet;
            Field.m_DynamicTypeCount = 0;
            m_Memory[ m_iMemOffet++ ] = ':';
            for( int i=0; pArgsTypes[i]; i++ )
            {
                xassert( isValidType( pArgsTypes[i] ));
                Field.m_DynamicTypeCount++;
                m_Memory[ m_iMemOffet++ ] = pArgsTypes[i];
            }
            m_Memory[ m_iMemOffet++ ] = 0;
            Field.m_TypeWidth[0] = std::max( Field.m_TypeWidth[0], static_cast<std::uint8_t>(m_iMemOffet - indexStr) );

            {
                //char c;
                for( int i=0; char c = pArgsTypes[i]; i++ )
                {
                    Field.m_iMemory[1+i] = m_iMemOffet + 4;
                    WriteComponent( m_iField, 1+i, c, Args1, bIndirect );
                }
            }


            // Done here
            break;
        }
    case -1:
        {
            auto& FieldName = Field.m_Type;
            
            // Are we writing user types or atomic types
            if( FieldName[ Field.m_TypeOffset ] == '.' )
            {
                const auto& Entry = m_UserTypes[ Field.m_iUserType ];
                char        c;
                
                // Now we can set the number of types
                Field.m_nTypes = Entry.m_nSystemTypes;
                
                // Dump all the types
                for( std::uint32_t i=0; (c = Entry.m_SystemType[ i ]); i++ )
                {
                    Field.m_iMemory[i] = m_iMemOffet + 4;
                    WriteComponent( m_iField, i, c, Args1, bIndirect );
                }
            }
            else
            {
                char c;
                for( int i=0; (c = FieldName[ Field.m_TypeOffset + i ]); i++ )
                {
                    Field.m_iMemory[i] = m_iMemOffet + 4;
                    WriteComponent( m_iField, i, c, Args1, bIndirect );
                }
            }
            break;
        }
    default:
        {
            auto&       Decl    = m_Field[ Field.m_iFieldDecl ];
            const char* p       = &m_Memory[ Decl.m_iMemory[0] ];
            
            if( p[0] == '.' )
            {
                const auto&     Entry = m_UserTypes[ Decl.m_iUserType ];
                char            c;
                std::uint32_t   nTypes = 0;
                
                for( ; (c = Entry.m_SystemType[ nTypes ]); nTypes++ )
                {
                    Field.m_iMemory[nTypes] = m_iMemOffet + 4;
                    WriteComponent( m_iField, nTypes, c, Args1, bIndirect );
                }
                
                xassert( nTypes == Decl.m_DynamicTypeCount );
            }
            else
            {
                xassert( p[0] == '<' );
                
                int nTypes = 0;
                for( int i=1; p[ i ]!='>'; i++ )
                {
                    nTypes++;

                    Field.m_iMemory[i-1] = m_iMemOffet + 4;
                    WriteComponent( m_iField, i-1, p[ i ], Args1, bIndirect );
                }
                
                xassert( nTypes == Decl.m_DynamicTypeCount );
            }
            break;
        }
    }

    //
    // Okay we are done advance the field count
    //
    m_iField++;
}

//------------------------------------------------------------------------------

void xcore::textfile::stream::WriteComponent( int iField, int iType, int c, va_list& Args, bool bIndirect ) noexcept
{   
    xassert( m_File.m_States.m_isReading == false );

    //
    // Standard function for the interger types
    //
    auto Numerics = [&]( auto p, const char* pFmt ) noexcept
    {
        using T = std::decay_t<decltype(p)>;
        if constexpr ( std::is_integral_v<T> )
        {
            if constexpr ( std::is_signed_v<T> )
            {
                p = bIndirect ? (*va_arg( Args, T* )):static_cast<T>(va_arg( Args, types::byte_size_int_t< sizeof(T) < 4 ? 4:8 > ));
            }
            else
            {
                if( p && bIndirect == false )
                {
                    // Special case when saving a floating point number as hex
                    float x = static_cast<float>(va_arg( Args, double ));
                    p       = reinterpret_cast<T&>(x);
                }
                else p = bIndirect ? (*va_arg( Args, T* )):static_cast<T>(va_arg( Args, types::byte_size_uint_t< sizeof(T) < 4 ? 4:8 > ));
            }

            if( m_File.m_States.m_isBinary )
            {
                if( m_File.m_States.m_isEndianSwap ) p = endian::Convert(p);
                ptr::MemCopy( { reinterpret_cast<T*>(&m_Memory[m_iMemOffet]), sizeof(T) }, std::span<T>{ &p, sizeof(T) } );
                m_iMemOffet += sizeof(T);
            }
            else
            {
                auto& Field = m_Field[iField];

                // We reserve the first byte so that we can add additional spaces to this floating point number
                m_Memory[m_iMemOffet++] = iField;
                m_Memory[m_iMemOffet++] = iType;
                reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet])   = 1 + p<0 ? 1:0;
                m_iMemOffet+=2;
                auto iInt = m_iMemOffet;

                // Okay lets write the floating point number
                m_iMemOffet += 1 + string::sprintf( { &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, pFmt, p ).m_Value;

                // Set the width for this type
                reinterpret_cast<std::uint16_t&>(m_Memory[iInt-2])    = m_iMemOffet - iInt;
                Field.m_TypeWidth[iType] = std::max( Field.m_TypeWidth[iType], static_cast<std::uint8_t>(m_iMemOffet - iInt) );
            }
        }
        else
        {
            auto p = bIndirect?(*va_arg( Args, T* )):static_cast<T>(va_arg( Args, double ));     // get the type
        
            if( m_File.m_States.m_isBinary )
            {
                auto n = static_cast<T>(p);
                if( m_File.m_States.m_isEndianSwap ) n = xcore::endian::Convert(n);
                ptr::MemCopy( { reinterpret_cast<T*>(&m_Memory[m_iMemOffet]),sizeof(T)}, std::span<T>{&n, sizeof(T)} );
                m_iMemOffet += sizeof(T);
            }
            else
            {
                auto& Field = m_Field[iField];

                // We reserve the first byte so that we can add additional spaces to this floating point number
                m_Memory[m_iMemOffet++] = iField;
                m_Memory[m_iMemOffet++] = iType;
                reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet])   = 1;
                m_iMemOffet+=2;
                const auto iFloat = m_iMemOffet;

                // Okay lets write the floating point number
                m_iMemOffet += 1+string::sprintf( { &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, pFmt, p ).m_Value;

                reinterpret_cast<std::uint16_t&>(m_Memory[iFloat-2]) = m_iMemOffet - iFloat;

                // Lets determine how many character does it have before the decimal point
                int i;
                for( i=0; m_Memory[iFloat+i]!='.' && m_Memory[iFloat+i]; i++ );

                Field.m_FracWidth[iType] = std::max( Field.m_FracWidth[iType], static_cast<std::uint8_t>(i) );

                const int Width   = m_iMemOffet - iFloat
                                    - i                              // We get the floating point part
                                    + Field.m_FracWidth[iType];      // We add the worse case integral part

                Field.m_TypeWidth[iType] = std::max( Field.m_TypeWidth[iType], static_cast<std::uint8_t>(Width)  );
            }
        }
    };

    //
    // Write data
    //
    switch( c )
    {
    case 'F': if( m_File.m_States.m_isSaveFloats )  Numerics( double(0),         "%Lg" );
              else                                  Numerics( std::uint64_t(0),  "#%LX" );
              break;
    case 'f': if( m_File.m_States.m_isSaveFloats ) Numerics( float(0),          "%g"  );
              else                                 Numerics( std::uint32_t(1),  "#%X" );
              break; 
    case 'c': Numerics( std::int8_t(0),    "%d"  ); break;
    case 'C': Numerics( std::int16_t(0),   "%d"  ); break;
    case 'd': Numerics( std::int32_t(0),   "%d"  ); break;
    case 'D': Numerics( std::int64_t(0),   "%ld" ); break;
    case 'G': Numerics( std::uint64_t(0),  "#%LX" ); break;
    case 'g': Numerics( std::uint32_t(0),  "#%X"  ); break;
    case 'H': Numerics( std::uint16_t(0),  "#%X"  ); break;
    case 'h': Numerics( std::uint8_t(0),   "#%X"  ); break;
    case 'E': 
    case 'S': xassert( false ); break;
    case 'e':
    case 's':
            {
                // Strings are always indirect
                auto p = va_arg( Args, const string::view<char> );
                if( m_File.m_States.m_isBinary )
                {
                    for( int m=0; !!(m_Memory[m_iMemOffet]=p[m]); m++, m_iMemOffet++ );
                    m_iMemOffet++;
                }
                else
                {
                    auto& Field = m_Field[iField];

                    // We reserve the first byte so that we can add additional spaces to this floating point number
                    m_Memory[m_iMemOffet++] = iField;
                    m_Memory[m_iMemOffet++] = iType;
                    reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet])   = 1;
                    m_iMemOffet+=2;
                    int indexStr = m_iMemOffet;

                    if(c == 'e')
                        m_iMemOffet += 1+string::sprintf( string::view<char>{ &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, "%s", p.data()).m_Value;
                    else
                        m_iMemOffet += 1+string::sprintf( string::view<char>{ &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, "\"%s\"", p.data()).m_Value;

                    reinterpret_cast<std::uint16_t&>(m_Memory[indexStr-2])         = m_iMemOffet - indexStr;
                    Field.m_TypeWidth[iType] = std::max( Field.m_TypeWidth[iType], static_cast<std::uint8_t>(m_iMemOffet - indexStr) );
                }
                break;
            }
    case '?':
        {
            // strings are always indirect
            //string::view<char> p = va_arg( Args, string::view<char> );       // get the type   
            if( m_File.m_States.m_isBinary )
            {
                /*
                // Determine if it is a user type if so make sure we follow the right syntax
                if( p[0] == '.' )
                {
                    auto&  Field = m_Field[iField];
                    
                    // For user types we provide the actual index to the type been read
                    // this is faster to load and smaller in memory
                    std::uint16_t Index = Field.m_iUserType;
                    
                    // Write to the file the fact that we are a user type
                    m_Memory[m_iMemOffet++] = '.';
                    
                    // write the index
                    Index = ( m_File.m_States.m_isEndianSwap ) ? xcore::endian::Convert(Index) : Index;
                    reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet]) = Index;
                    m_iMemOffet += 2;
                }
                else
                {
                    m_Memory[m_iMemOffet++]='<';
                    for( int i=0; !!(m_Memory[m_iMemOffet]=p[i]); m_iMemOffet++, i++);
                    m_Memory[m_iMemOffet++]='>';
                    m_Memory[m_iMemOffet++]=0;
                }
                */
            }
            else
            {
                m_Memory[m_iMemOffet++] = iField;
                m_Memory[m_iMemOffet++] = iType;

                reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet])   = 1;
                m_iMemOffet+=2;
                int indexStr = m_iMemOffet;

                auto&  Field = m_Field[iField];
                const char* p = &Field.m_Type[ Field.m_TypeOffset];
                m_iMemOffet += 1+string::sprintf( string::view<char>{ &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, ":%s", p ).m_Value;

                reinterpret_cast<std::uint16_t&>(m_Memory[indexStr-2])         = m_iMemOffet - indexStr;
                Field.m_TypeWidth[iType] = std::max( Field.m_TypeWidth[iType], static_cast<std::uint8_t>(m_iMemOffet - indexStr) );

                //////////// 6/22/2019 old from <asd>
                /*
                reinterpret_cast<std::uint16_t&>(m_Memory[m_iMemOffet]) = 1;
                m_iMemOffet+=2;
                int iStr = m_iMemOffet;

                // Determine if it is a user type if so make sure we follow the right syntax
                if( p[0] == '.' )
                {
                    m_iMemOffet += 1+string::sprintf( string::view<char>{ &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, "%s", p.data() ).m_Value;
                }
                else
                {
                    m_iMemOffet += 1+string::sprintf( string::view<char>{ &m_Memory[m_iMemOffet], static_cast<int>(m_Memory.size()-m_iMemOffet) }, "<%s>", p.data() ).m_Value;
                }

                // Set the width for this type
                // Set the width for this type (we may need to have 2 bytes here if this assert is a problem)
                xassert( (m_iMemOffet - iStr) <= 255 );
                reinterpret_cast<std::uint16_t&>(m_Memory[iStr-2])         = m_iMemOffet - iStr;
                Field.m_TypeWidth[iType] = std::max( Field.m_TypeWidth[iType], static_cast<std::uint8_t>(m_iMemOffet - iStr) );
                */
            }
            break;
        }
    default:
        // Unknown type
        xassert( false );
    } 

    // Increment the number of types written
    m_nTypesLine++;

    //
    // Make sure that there are always plenty of memory left
    //
    if( (m_Memory.size()-m_iMemOffet) < 512 ) 
        m_Memory.resize( m_Memory.size() + 1024*4 );
}


//------------------------------------------------------------------------------

void xcore::textfile::stream::BuildTypeInformation( const char* pFieldName, const char* pTypes ) noexcept
{
    string::fixed< char, 256 > Str;
    string::sprintf( Str, "%s:%s", pFieldName, pTypes );
    BuildTypeInformation( Str );
}

//------------------------------------------------------------------------------

void xcore::textfile::stream::BuildTypeInformation( const char* pFieldName ) noexcept
{
    int i;

    xassert( pFieldName );

    //
    // Set the current field
    //
    auto& Field = m_Field.emplace_back();
    
    string::Copy( Field.m_Type, pFieldName );
    Field.m_nTypes = 0;

    //
    // First lest find where in the string the types begin
    //
    for( i=0; pFieldName[i]; i++ )
    {
        if( pFieldName[i] == ':' ) break;

        // Now spaces are allow. It should be like this "Vector3:fff"
        xassert(pFieldName[i] != ' ' );
    }

    // We must have some name indicating the type. It found ':' way too early
    // either that or the string was blank
    xassert( i > 0 );

    // We didn't find the ':' character which indicates where the types begging
    xassert( pFieldName[i] != 0 );

    // Make sure that the only reason we have stop was because we found the correct token
    xassert( pFieldName[i] == ':' );

    i++;
    Field.m_TypeOffset = i;

    //
    // Okay now lets deal with the types
    //

    // is user is indicating that he want to deal with a complex type
    if( pFieldName[i] == '?' )
    {
        // So lets advance to the next character
        i++;
//        xassert( pFieldName[i] == ':' );

        Field.m_Type[i] = 0;
        Field.m_TypeOffset = i+1;

        // Try to find how many times we are talking about
        i++;
        for( Field.m_nTypes=0; pFieldName[i]; i++, Field.m_nTypes++ )
        {
            xassert( isValidType( pFieldName[i] ) );
        }

        Field.m_nTypes++;
        Field.m_iFieldDecl = -2;

        /*
        // Now is the user declaring or using a complex type?
        if( pFieldName[i] == '?' )
        {
            // okay here we only we have one type
            Field.m_nTypes = 1;
            
            // Now lets verify that the user knew how to end the complex type
            i++;
            xassert( pFieldName[i] == '>' );
            Field.m_Type[i+1] = 0;

            // Mark that we are a declaration of a type
            Field.m_iFieldDecl = -2;
        }
        else
        {
            string::fixed<char,256> TypeName;
            std::uint32_t           j,k;

            // Here we set it to zero because this is going to be dependent
            Field.m_nTypes = 0;

            // Now lets read the type name:
            for( j=0; !!(TypeName[j] = pFieldName[i]); j++, i++ )
            {
                // This is the terminator for a complex type
                if( TypeName[j] == '>' ) break;
            }

            // Lets make sure that we are okay this far.
            // If the use did add a terminator we will be mess up, the complex type need
            // to be use in the fallowing way "Jack:<pepe>" this is the syntax
            xassert( TypeName[j] == '>' );
            Field.m_Type[i+1] = 0;

            // Okay now lets terminate the type properly
            TypeName[j] = 0;

            //
            // Okay now lets search for the declaration
            //
            k=0;
            for( j=0; j<m_Field.size(); j++ )
            {
                auto&        Decl  = m_Field[j];

                // Skip this field if it is not a declaration of a type
                if( Decl.m_iFieldDecl != -2 )
                    continue;

                // get the string
                auto&  FieldName = Decl.m_Type;
                
                // Do a string compare
                for( k=0; TypeName[k] && TypeName[k] == FieldName[k]; k++ );

                // we found the declaration
                if( TypeName[k] == 0 && FieldName[k] == ':'  )
                    break;                        
            }

            // check to make sure that we have found a type declaration for this
            // complex type. If we haven't the user need to make sure that he enters 
            // a complex type declaration. The type declaration should read like this:
            // "jack:<pepe>"
            xassert( j != m_Field.size() );
            xassert( TypeName[k] == 0 );

            //
            // Okay lets write the info in our structures
            //
            Field.m_iFieldDecl = j;
        }
        */
    }
    else if( pFieldName[i] == '.')
    {
        // If it is a user type lets mark it down as a regular type
        // we will decode it later
        Field.m_iFieldDecl = -1;
        
        if( m_File.m_States.m_isBinary && m_File.m_States.m_isReading )
        {
            Field.m_iUserType = reinterpret_cast<const std::uint16_t&>(pFieldName[i+1]);
            Field.m_iUserType = m_BinReadUserTypeMap[Field.m_iUserType];
            Field.m_nTypes    = m_UserTypes[ Field.m_iUserType ].m_nSystemTypes;
        }
        else
        {
            string::fixed<char,128> Type;
            std::uint32_t j;
            for( j = 0; (Type[j] = pFieldName[i+j]) != ':' && Type[j]; j++ );
            xassert(j && Type[j] == ':');
            Type[j] = 0;

            auto I = m_UserTypeMap.find( crc::FromString<32>(Type) );
            xassert_verify( I != m_UserTypeMap.end() );//m_UserTypes.BinarySearch( user_type( &pFieldName[i] ), Index ) );
            int Index = I->second;
            Field.m_Type[i+j] = 0;
            Field.m_iUserType = Index;
            Field.m_nTypes    = m_UserTypes[ Field.m_iUserType ].m_nSystemTypes;

            xassert_block_basic()
            {
                std::uint32_t index = 0;
                for( int k = i+j+1; pFieldName[k]; k++ )
                {
                    xassert( pFieldName[k] == m_UserTypes[Index].m_SystemType[index++] );
                }
            }
        }
    }
    else
    {
        xassert( isValidType( pFieldName[i] ) );

        // Mark that we are a generic type
        Field.m_iFieldDecl = -1;

        // Try to find how many times we are talking about
        for( Field.m_nTypes=0; pFieldName[i]; i++, Field.m_nTypes++ )
        {
            xassert( isValidType( pFieldName[i] ) );
        }
    }
}


//------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::WriteLine( void ) noexcept
{
    int         j;
    xcore::err  Error;

    xassert( m_File.m_States.m_isReading == false );

    // Make sure that the user don't try to write more lines than expected
    xassert( m_iLine < m_Record.m_Count );

    //
    // Lets handle the binary case first
    //
    if( m_File.m_States.m_isBinary )
    {
        if( m_iLine == 0 )
        {
            //
            // Write any pending user types
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

            if( m_File.WriteChar( ']' ).isError( Error ) )
                return Error;
            
            //
            // Print types
            //
            if( m_File.WriteChar( '{' ).isError( Error ) )
                return Error;
            
            for( const auto& Field : m_Field )
            {
                if( Field.m_Type[ Field.m_TypeOffset ] == '.' )
                {
                    if( m_File.WriteStr( { Field.m_Type.data(), static_cast<int>(Field.m_TypeOffset + 1) } ).isError( Error ) )
                        return Error;

                    if( m_File.Write( Field.m_iUserType ).isError( Error ) )
                        return Error;
                }
                else
                {
                    if( m_File.WriteStr( { Field.m_Type, string::Length( Field.m_Type ).m_Value+1 } ).isError( Error ) )
                        return Error;
                }
            }
            
            if( m_File.WriteChar( '}' ).isError( Error ) )
                return Error;
        }

        //
        // Dump line info
        //
        if( m_File.WriteStr( { m_Memory.data(), static_cast<int>(m_iMemOffet) } ).isError( Error ) )
            return Error;

        //
        // Increment the line count
        // and reset the field count
        // reset the memory count
        //
        m_iLine++;
        m_iField        = 0;
        m_iMemOffet = 0;
        m_nTypesLine    = 0;

        return Error;
    }

    //
    // Increment the line count
    // and reset the field count
    // reset the memory count
    //
    m_iLine++;
    m_iField        = 0;
    m_nTypesLine    = 0;

    //
    // Handle writing text format
    //
    static constexpr int MaxLines = 64;

    //
    // We will wait writing the line if we can so we can format
    //
    if( (m_iLine < m_Record.m_Count && (m_iLine%MaxLines) != 0) )
    {
        return Error;
    }

    //
    // Compute the width for each of the types
    //
    for( auto& Field : m_Field )
    {           
        int     TypeWitdh   = 0;

        // Count the total space used for this type
        for( int j=0; Field.m_TypeWidth[j]; j++ )
            TypeWitdh += Field.m_TypeWidth[j];

        // We dont count the last type to add spaces
        TypeWitdh -= 1;

        // okay compute the width of the type
        Field.m_Width = std::max( Field.m_Width, static_cast<std::uint32_t>(TypeWitdh) );

        //
        // Now lets center all sub columns into the main one 
        //
        const auto ExtraSpace = Field.m_Width - TypeWitdh;

        Field.m_TypesPostWidth  = ExtraSpace / 2;
        Field.m_TypesPreWidth   = Field.m_Width - ( Field.m_TypesPostWidth + TypeWitdh );

        xassert( Field.m_TypesPreWidth  >= 0 );
        xassert( Field.m_TypesPostWidth >= 0 );
    }

    //
    // Write the record header
    //
    if( m_iLine <= MaxLines )
    {
        //
        // Write any pending user types
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
        if( m_File.WriteChar( '{' ).isError(Error) )
            return Error;

        for( auto& Field : m_Field ) 
        {               
            if( m_File.WriteFmtStr( " %s ", Field.m_Type.data() ).isError(Error) )
                return Error;

            const auto Count = string::Length( Field.m_Type ).m_Value;
            if( static_cast<std::uint32_t>(Count) < Field.m_Width )
            {
                if( m_File.WriteChar( ' ', Field.m_Width-Count ).isError(Error) )
                    return Error;
            }
        }

        if( m_File.WriteFmtStr( string::constant("}\n") ).isError(Error) )
            return Error;

        //
        // Write the under line
        //
        if( m_File.WriteFmtStr( string::constant("//") ).isError(Error) )
            return Error;

        for( auto& Field : m_Field )
        { 
            if( m_File.WriteChar( '-', Field.m_Width ).isError(Error) )
                return Error;

            // Get ready for the next type
            if( m_File.WriteChar( ' ', 2 ).isError(Error) )
                return Error;
        }

        if( m_File.WriteChar( '\n' ).isError(Error) )
            return Error;
    }

    //
    // Write the commented version of the types
    //
    if( (m_iLine%MaxLines) == 0 && (m_iLine>MaxLines) )
    {
        //
        // Write the under line
        //
        if( m_File.WriteFmtStr( string::constant("//") ).isError(Error) )
            return Error;

        for( auto& Field : m_Field )
        { 
            if( m_File.WriteChar( '-', Field.m_Width ).isError(Error) )
                return Error;

            // Get ready for the next type
            if( m_File.WriteStr( string::constant("  ") ).isError(Error) )
                return Error;
        }

        if( m_File.WriteChar( '\n' ).isError(Error) )
            return Error;

        //
        // Write the types
        //
        if( m_File.WriteStr( string::constant("//") ).isError(Error) )
            return Error;

        for( auto& Field : m_Field )
        {   
            if( m_File.WriteStr( Field.m_Type ).isError(Error) ) 
                return Error;

            const auto Count = string::Length(Field.m_Type).m_Value;
            if( static_cast<std::uint32_t>(Count) < Field.m_Width )
            {
                if( m_File.WriteChar( ' ', Field.m_Width - Count ).isError(Error) )
                    return Error;
            }

            // Get ready for the next type
            if( m_File.WriteStr( string::constant("  ") ).isError(Error) )
                return Error;
        }

        if( m_File.WriteChar( '\n' ).isError(Error) )
            return Error;

        //
        // Write the under line
        //
        if( m_File.WriteStr( string::constant("//") ).isError(Error) )
            return Error;

        for( auto& Field : m_Field )
        { 
            if( m_File.WriteChar( '-', Field.m_Width ).isError(Error) )
                return Error;

            // Get ready for the next type
            if( m_File.WriteStr( string::constant("  ") ).isError(Error) )
                return Error;
        }
        if( m_File.WriteChar( '\n' ).isError(Error) )
            return Error;
    }

    //
    // Dump lines
    //

    // Write each member
    for( std::uint32_t i=0; i<m_iMemOffet; i++ )
    {
        int     iField = m_Memory[i++];
        int     iType  = m_Memory[i++];
        int     Len    = reinterpret_cast<std::uint16_t&>(m_Memory[i]);
        
        i+=2;
        
        auto&  Field  = m_Field[ iField ];
        auto   Type   = Field.m_Type[ Field.m_TypeOffset + iType ];

        // Force the type if it is a dynamic type
        if( Field.m_iFieldDecl >= 0 )
        {
            Type   = '<';
        }
        
        //
        // Add any spaces that we may need to center our columns
        //
        if( iType == 0 )
        {
            // account for the comment
            if( iField == 0 )
            {
                if( m_File.WriteChar( ' ', 2 ).isError(Error) )
                    return Error;
            }

            if( Field.m_TypesPreWidth > 0 ) 
            {
                if( m_File.WriteChar( ' ', Field.m_TypesPreWidth ).isError(Error) )
                    return Error;
            }
        }

        //
        // Okay handle the printing here
        //
        switch( Type )
        {                    
        case 'f':       // Floating point alignment
        case 'F':
            if( m_File.m_States.m_isSaveFloats )
            {
                // lets find again where the '.' is
                for( j=0; m_Memory[j+i]!='.' && m_Memory[j+i]; j++ );

                // Pad int spaces
                if( Field.m_FracWidth[iType] > j ) 
                {
                    if( m_File.WriteChar( ' ', Field.m_FracWidth[iType] - j ).isError(Error) )
                        return Error;
                }

                // Now we should be able to write the number
                if( m_File.WriteFmtStr( "%s", &m_Memory[i] ).isError(Error) )
                    return Error;

                // Now lets padd the fractional part
                j = Len + Field.m_FracWidth[iType] - j;
                if( j < Field.m_TypeWidth[iType] )
                {
                    if( m_File.WriteChar( ' ', Field.m_TypeWidth[iType] - j ).isError(Error) )
                        return Error;
                }
                break;
            }
            // Fall throw                
        case 'd':       // Right aligment
        case 'D':
        case 'C':
        case 'c':
        case 'h':
        case 'H':
        case 'G':
        case 'g':
            {
                // Now lets padd the fractional part
                if( Len < Field.m_TypeWidth[iType] )
                {
                    if( m_File.WriteChar( ' ', Field.m_TypeWidth[iType] - Len ).isError(Error) )
                        return Error;
                }

                // Now we should be able to write the number
                if( m_File.WriteFmtStr( "%s", &m_Memory[i] ).isError(Error) )
                    return Error;
            }
            break;
        case '<':
            {
                if( Field.m_iFieldDecl == -2 )
                {
                    //This is a declaration, retrieve information
                    xassert(Field.m_iFieldDecl == -2);
                    
                    //Look for '>'
                    int iRightBracket = 0;
                    if( m_Memory[i] == '<' )
                    {
                        const char* p = &m_Memory[i];
                        
                        while( p[iRightBracket] != '>')
                        {
                            iRightBracket++;
                        }
                        
                        iRightBracket--;
                    }
                    else if( m_Memory[i] == '.' )
                    {
                        const char* p = &m_Memory[i];
                        
                        auto I = m_UserTypeMap.find( xcore::crc::FromString<32>(p) );
                        if( I == m_UserTypeMap.end() )
                            return xerr_failure( Error, "Failed to find the user type" );

                        //xassert_verify( m_UserTypes.BinarySearch( user_type(p), Index ) );
                        
                        auto Index = I->second;
                        const auto& Type = m_UserTypes[ Index ];
                        iRightBracket = Type.m_nSystemTypes;
                    }
                    else
                    {
                        xassert( false );
                    }

                    //For now, I just added this barrier because i don't think there will be a value which contains more than 32 elements. If there is, let me know
                    xassert(iRightBracket<32);

                    // This is just temporary as it will be overwritten again and again
                    Field.m_DynamicTypeCount = iRightBracket;
                }
                else
                {
                    // This is a temporary variable to store how many types we have
                    Field.m_nTypes = m_Field[ Field.m_iFieldDecl ].m_DynamicTypeCount;
                }
            }
                
                //>>>>>>>>>>>> FALL THOUGHT <<<<<<<<<<<<<<<
                
        default:        // Left aligment
            {
                // Now we should be able to write the number
                if( m_File.WriteFmtStr( "%s", &m_Memory[i] ).isError(Error) )
                    return Error;

                if( Len < Field.m_TypeWidth[iType] )
                {
                    if( m_File.WriteChar( ' ', Field.m_TypeWidth[iType] - Len ).isError(Error) )
                        return Error;
                }
            }
            break;
        }

        // Set the next i
        i += Len -1;
        xassert( m_Memory[i] == 0 );

        if( iType == Field.m_nTypes-1 )
        {
            if (iField == m_Field.size()-1 ) 
            {
                if( m_File.WriteChar( '\n' ).isError(Error) )
                    return Error;
            }
            else
            {
                // add any spaces needed to end the column properly
                if( Field.m_TypesPostWidth > 0 ) 
                {
                    if( m_File.WriteChar( ' ', Field.m_TypesPostWidth ).isError(Error) ) 
                        return Error;
                }

                // ready for the next type
                if( m_File.WriteStr( string::constant("  ") ).isError(Error))
                    return Error;
            }
        }
        else
        {
            // separate any sub fields
            if( m_File.WriteStr( string::constant(" ") ).isError(Error)) 
                return Error;
        }
    }

    //
    // Make sure to reset the current offset since we are done with the memory
    //
    m_iMemOffet = 0;

    return Error;
}

//------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::WriteUserTypes( void ) noexcept
{
    xcore::err Error;

    if( m_File.m_States.m_isBinary )
    {
        for( auto& UserType : m_UserTypes )
        {
            if( UserType.m_bSaved )
                continue;
            
            UserType.m_bSaved = true;
            
            // First write the standard symbol for the user types
            if( m_File.WriteChar( '<' ).isError(Error) )
                return Error;
            
            // Now dump the data for the user type
            if( m_File.WriteStr( { UserType.m_SystemType.data(), UserType.m_nSystemTypes+1 } ).isError(Error) )
                return Error;
            
            if( m_File.WriteStr( { UserType.m_UserType.data(), string::Length(UserType.m_UserType).m_Value+1 } ).isError(Error) )
                return Error;
        }
    }
    else
    {
        //
        // Collect any new types
        //
        string::ref<char> Types( string::units<char>(1024) );
        for( auto& UserType : m_UserTypes )
        {
            if( UserType.m_bSaved )
                continue;
            
            UserType.m_bSaved = true;
            
            string::fixed<char,256> temp;
            string::sprintf( temp, "<%s>%s ", UserType.m_SystemType.data(),
                                              UserType.m_UserType.data() );
            string::Append( Types, temp );
        }

        //
        // Dump all the new types if we have some
        //
        if( Types.empty() == false )
        {
            auto L = string::Length(Types).m_Value;
            
            if( m_File.WriteStr( string::constant{"\n// New Types\n"} ).isError(Error) )
                return Error;
            
            if( m_File.WriteStr( string::constant{"//"} ).isError(Error) )
                return Error;

            L = (L-1)>>1;
            
            for( int i=0; i<L; i++ ) 
            {
                if( m_File.WriteStr( string::constant{"--"} ).isError(Error) )
                    return Error;
            }
            
            if( m_File.WriteFmtStr( "\n%s\n", Types.data() ).isError(Error) )
                return Error;
        }
    }

    return Error;
}


//------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::WriteComment( const string::view<char> Comment ) noexcept
{
    xcore::err Error;
    xassert( m_File.m_States.m_isReading == false );

    if( m_File.m_States.m_isBinary )
    {
        // No comments supported for binary files
        //    m_pFile->Write( Comment(), 1, Comment.GetLength()+1 );
    }
    else
    {
        if( m_File.WriteStr( { Comment.data(), string::Length( Comment ).m_Value } ).isError( Error ) )
            return Error;
    }

    return Error;
}

//------------------------------------------------------------------------------
namespace xcore::textfile
{
    static
    bool isFloat( int c ) noexcept
    {
        constexpr static const char* pF = "+-Ee.#QNABIF";
        int i;
    
        if( (c >= '0' && c <= '9') ) return true;
    
        for( i=0; (c != pF[i]) && pF[i]; i++);
    
        if( pF[i] == 0 ) return false;
        return true;
    }
}

//------------------------------------------------------------------------------

xcore::err xcore::textfile::stream::ReadComponent( std::uint32_t& iStartOffset, int Type ) noexcept
{
    xcore::err      Error;

    xcore::error::scope General( Error, [&]()
    {
        XLOG_CHANNEL_ERROR( m_Channel, "ReadComponent fail to while reading .%c (%s)", Type, Error.getCode().m_pString );
    });

    auto Handle = [&]( auto T ) -> xcore::err
    {
        xcore::err  Error;
        using t = decltype(T);

        // Align the Offset to be happy
        iStartOffset = m_iMemOffet = xcore::bits::Align( m_iMemOffet, sizeof(t) );
        if( m_File.m_States.m_isBinary )
        {
            if( m_File.Read( reinterpret_cast<t&>(m_Memory[ m_iMemOffet ] ) ).isError(Error) )
                return Error;
        }
        else
        {
            int                         iBuffer=0;
            string::fixed<char,256>     Buffer;
            int                         c;

            // Read out all the white space
            if( m_File.ReadWhiteSpace(c).isError(Error) )
                return Error;

            if( c == '#' )
            {
                if( m_File.getC(c).isError( Error ) ) return Error;
                while( (c>='0'&&c<='9')||(c>='A'&&c<='F') )
                {
                    Buffer[iBuffer++]=c;
                    if( m_File.getC(c).isError( Error ) )
                        return Error;
                }
                Buffer[iBuffer] = 0;
                        if constexpr ( sizeof(t) == 1   ) reinterpret_cast<std::uint8_t& >(m_Memory[ m_iMemOffet ]) = static_cast<std::uint8_t >(std::strtoul ( &Buffer[0], nullptr, 16 ));
                else    if constexpr ( sizeof(t) == 2   ) reinterpret_cast<std::uint16_t&>(m_Memory[ m_iMemOffet ]) = static_cast<std::uint16_t>(std::strtoul ( &Buffer[0], nullptr, 16 ));
                else    if constexpr ( sizeof(t) == 4   ) reinterpret_cast<std::uint32_t&>(m_Memory[ m_iMemOffet ]) = static_cast<std::uint32_t>(std::strtoul ( &Buffer[0], nullptr, 16 ));
                else    if constexpr ( sizeof(t) == 8   ) reinterpret_cast<std::uint64_t&>(m_Memory[ m_iMemOffet ]) = static_cast<std::uint64_t>(std::strtoull( &Buffer[0], nullptr, 16 ));
                else
                {
                    xassume(false);
                }
            }
            else
            {
                // Copy all the characters 
                if constexpr ( std::is_same_v<t,float> || std::is_same_v<t,double> )
                {
                    while( isFloat(c) )
                    {
                        Buffer[iBuffer++]=c;
                        if( m_File.getC(c).isError( Error ) )
                            return Error;
                    }

                    Buffer[iBuffer] = 0;
                    reinterpret_cast<t&>(m_Memory[ m_iMemOffet ]) = static_cast<t>(std::atof( &Buffer[0] ));
                }
                else if constexpr( std::is_integral_v<t> )
                {
                    while( (c>='0'&&c<='9')||(c=='-')||(c=='+') )
                    {
                        Buffer[iBuffer++]=c;
                        if( m_File.getC(c).isError( Error ) )
                            return Error;
                    }

                    Buffer[iBuffer] = 0;
                            if constexpr( std::is_same_v<t,std::int8_t>     ) reinterpret_cast<t&>(m_Memory[ m_iMemOffet ]) = static_cast<t>(std::atoi    ( &Buffer[0] ));
                    else    if constexpr( std::is_same_v<t,std::int16_t>    ) reinterpret_cast<t&>(m_Memory[ m_iMemOffet ]) = static_cast<t>(std::atoi    ( &Buffer[0] ));
                    else    if constexpr( std::is_same_v<t,std::int32_t>    ) reinterpret_cast<t&>(m_Memory[ m_iMemOffet ]) = static_cast<t>(std::atol    ( &Buffer[0] ));
                    else    if constexpr( std::is_same_v<t,std::int64_t>    ) reinterpret_cast<t&>(m_Memory[ m_iMemOffet ]) = static_cast<t>(std::atoll   ( &Buffer[0] ));
                    else
                    {
                        xassume(false);
                    }
                }
            }

            // Sanity check
            xassert( c == ' ' || c == '\n' || c == 10 );
        }
        m_iMemOffet += sizeof(t);

        return Error;
    };

    switch( Type )
    {
    default:
        return xerr_failure( Error, "Error reading the file" );
        break;

    case 'f': if( Handle( float(0)          ).isError(Error)) return Error; break;
    case 'F': if( Handle( double(0)         ).isError(Error)) return Error; break;
    case 'c': if( Handle( std::int8_t(0)    ).isError(Error)) return Error; break;
    case 'C': if( Handle( std::int16_t(0)   ).isError(Error)) return Error; break;
    case 'd': if( Handle( std::int32_t(0)   ).isError(Error)) return Error; break;
    case 'D': if( Handle( std::int64_t(0)   ).isError(Error)) return Error; break;
    case 'h': if( Handle( std::uint8_t(0)   ).isError(Error)) return Error; break;
    case 'H': if( Handle( std::uint16_t(0)  ).isError(Error)) return Error; break;
    case 'g': if( Handle( std::uint32_t(0)  ).isError(Error)) return Error; break;
    case 'G': if( Handle( std::uint64_t(0)  ).isError(Error)) return Error; break;
    case 's':
    case 'e':
         {
             iStartOffset = m_iMemOffet;
             int c;
 
             if( m_File.m_States.m_isBinary )
             {
                 // Read all the characters
                 do 
                 {
                     if( m_File.getC( c ).isError(Error) )
                         return Error;
                     m_Memory[ m_iMemOffet++ ] = c;

                 } while( c );
             }
             else
             {
                 // Read out all the white space
                 if( m_File.ReadWhiteSpace(c).isError(Error))
                     return Error;
                 
                 // Read all the characters
                 if( c != '"' )
                 {
                     do
                     {
                         m_Memory[ m_iMemOffet++ ] = c;
                         if( m_File.getC( c ).isError(Error) )
                             return Error;
                     } while( c >= '!' && c <= '~' );
                     if( c != ' ' && c != '\n' && c != 10 ) return xerr_failure( Error, "Unexpected character in an enumerated value" );
                 }
                 else
                 {
                     if( m_File.getC( c ).isError(Error) )
                         return Error;
                 
                     do
                     {
                         m_Memory[ m_iMemOffet++ ] = c;
                         if( m_File.getC( c ).isError(Error) )
                             return Error;
                     } while( c != '"' );
                 }
 
                 // terminate the string correctly
                 m_Memory[ m_iMemOffet++ ] = 0;
             }
             break;
         }
    case '?':
        {
            iStartOffset = m_iMemOffet;
            int c;

            if( m_File.m_States.m_isBinary )
            {
                if( m_File.getC( c ).isError(Error) )
                    return Error;

                if( c == '.' )
                {
                    // Align the Offset to be happy
                    iStartOffset = m_iMemOffet = xcore::bits::Align( m_iMemOffet+1, 2 ) - 1;
                    
                    // Leave the period to indicate that is a user type
                    m_Memory[m_iMemOffet++] = '.';
                    
                    // Read the imapping for the user type
                    std::uint16_t iMapping;
                    if( m_File.Read( iMapping ).isError(Error) )
                        return Error;
                    /*
                    if( auto Err = m_pFile->Read( iMapping ); Err.isError() )
                    {
                        X_LOG("ERROR: xtextfile::ReadComponent ?.(%s)", Err.getStringAndClear() );
                        return -1;
                    }
                    
                    // Write the final remapped type
                    *((u16*)&m_Memory[ m_iMemOffet ]) = m_BinReadUserTypeMap[ iMapping ];
                    */
                   
                    reinterpret_cast<std::uint16_t&>(m_Memory[ m_iMemOffet ]) = m_UserTypes[iMapping].m_UID;
                    m_iMemOffet += 2;
                }
                else if( c == '<' )
                {
                    // Read all the types
                    do {
                        if( m_File.getC( c ).isError(Error) )
                            return Error;
                        m_Memory[ m_iMemOffet++ ] = c;
                    } while(c);

                    // Make sure that we are doing okay
                    if( m_Memory[m_iMemOffet-1] != '>' ) return xerr_failure( Error, "Expecting a '>' Token but we didn't find it" );

                    // Terminate the string
                    m_Memory[ m_iMemOffet-1 ] = 0;
                }
                else
                {
                    // Expecting '<' or '.' but we didn't find it
                    xassert( false );
                }
            }
            else
            {
                // Read out all the white space
                if( m_File.ReadWhiteSpace(c).isError(Error) )
                    return Error;

                //
                // Read in the user type
                //
                if( c == '.' )
                {
                    // Save the period as it is part of the user type name
                    m_Memory[ m_iMemOffet++ ] = c;
                    
                    // Read the name of the type
                    do 
                    {
                        if( m_File.getC(c).isError(Error) )
                            return Error;
                        if( string::isCharSpace( c ) ) break;
                        m_Memory[ m_iMemOffet++ ] = c;

                    } while (c);
                    
                    // If this happens this means that we didn't read any types so we found '<>'
                    // we must have types!
                    xassert( iStartOffset != m_iMemOffet );
                    
                    // Terminate the string
                    m_Memory[ m_iMemOffet++ ] = 0;
                }
                else
                {
                    //
                    // we should have the right character by now
                    //
                    if( c != '<' ) return xerr_failure( Error, "Expecting '<' but we didn't find it" );

                    // Read the types
                    if( m_File.getC(c).isError(Error) )
                        return Error;

                    while( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
                    {
                        m_Memory[ m_iMemOffet++ ] = c;                    
                        if( m_File.getC(c).isError(Error) )
                            return Error;
                    }
                        
                    // Make sure that we are doing okay
                    if( c != '>' ) return xerr_failure( Error, "Expecting a '>' Token but we didn't find it" );
                
                    // If this happens this means that we didn't read any types so we found '<>'
                    // we must have types!
                    xassert( iStartOffset != m_iMemOffet );

                    // Terminate the string
                    m_Memory[m_iMemOffet++] = 0;
                    
                    //
                    // Sanity check
                    //
                    for( int i=0; m_Memory[iStartOffset+i]; i++ )
                    {
                        // Make sure that the user enter a valid type
                        if( isValidType( m_Memory[iStartOffset+i] ) == false )
                            return xerr_failure( Error, "We have read an invalid type" );
                    }
                }
            }

            break;
        }
    }

    // Increment the number of types written
    // m_nTypesLine++;

    //
    // Make sure that there are always plenty of memory left
    //
    if( (m_Memory.size()-m_iMemOffet) < 512 ) 
        m_Memory.resize( m_Memory.size() + 512 );

    return Error;
}

//------------------------------------------------------------------------------
// Description:
//      When ever reading you first need to call this function before any fields can be read.
//      The reason for it is that the class handles one line/row at a time worth of data.
//      And the reasons for that is that the user can read as many fields from that line as he wants and skip the rest.
//      This makes it very handy for handling multiple versions of the data.
//------------------------------------------------------------------------------
xcore::err xcore::textfile::stream::ReadLine( void ) noexcept
{
    int                     c;
    int                     Size=0;
    string::fixed<char,256> Buffer;
    xcore::err              Error;

    // Make sure that the user doesn't read more lines than the record has
    xassert( m_iLine <= m_Record.m_Count );

    //
    // If it is the first line we must read the type information before hand
    //
    if( m_iLine == 0 )
    {
        // Reset the user field offsets
        m_User.clear();

        // Solve types
        if( m_File.m_States.m_isBinary )
        {
            // make sure it starts okay
            if( m_File.getC(c).isError(Error) || c != '{' ) 
                if( Error )   return Error;
                else          return xerr_failure( Error, "Error reading file. Expecting '{' but didn't find it" );

            if( m_File.getC(c).isError(Error) || c != 0 ) 
                if( Error )   return Error;
                else          return xerr_failure( Error, "Error reading file. Expecting 'end_of_string' but didn't find it" );

            // Read all the types
            do
            {
                // Read type information
                Size = 0;
                if( m_File.getC(c).isError(Error) ) return Error;
                while( c != -1 )
                {
                    Buffer[Size++] = c;
                    if( c == 0 ) break;
                    if( c == '.' )
                    {
                        if( m_File.Read( reinterpret_cast<std::uint16_t&>(Buffer[Size])).isError( Error ) )
                            return Error;
                        xassert_block_basic()
                        {
                            auto Index = reinterpret_cast<std::uint16_t&>(Buffer[Size]);
                            xassert( Index <= m_UserTypes.size() );
                        }
                        
                        Size += 2;
                        break;
                    }
                    if( m_File.getC(c).isError(Error) ) return Error;
                }

                /*
                if( c == -1 )
                {
                    if( m_pFile->isEOF()  ) x_throw( "Found an expected end of file");
                    else x_throw( "Found an error while reading the types");
                }
                */

                if( Buffer[0] != '}' )
                {
                    // Okay build the type information
                    BuildTypeInformation( Buffer );
                }

            } while( Buffer[0] != '}' );
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
                while( (c >= 'a' && c <= 'z') || 
                       (c >= 'A' && c <= 'Z') || 
                       (c=='_') ||
                       (c==':') || 
                       (c=='>') || 
                       (c=='<') ||
                       (c=='?') ||
                       (c >= '0' && c <= '9' ) ||
                       (c=='.') )
                {
                    Buffer[Size++] = c;                    
                    if( m_File.getC(c).isError(Error) ) return Error;
                }
            
                // Terminate the string
                Buffer[Size++] = 0;

                // Okay build the type information
                BuildTypeInformation( Buffer );

                // Read any white space
                if( m_File.ReadWhiteSpace(c).isError( Error ) )
                    return Error;

            } while( c != '}' );
        }
    }

    //
    // Okay now we must read a line worth of data
    //    
    for( auto& Field : m_Field )
    {
        switch( Field.m_iFieldDecl )
        {
        case -2:
            {
                if( ReadComponent( Field.m_iMemory[0], '?' ).isError(Error) )
                    return Error;
                
                //
                // For binary lets move the index of the user type into the right place
                //
                if( m_File.m_States.m_isBinary )
                {
                    if( Field.m_iMemory[0] == '.' )
                    {
                        Field.m_iUserType = reinterpret_cast<std::uint16_t&>(Field.m_iMemory[1]);
                        Field.m_iUserType = m_UserTypes[ Field.m_iUserType ].m_UID;
                    }
                }
                
                // Done here
                break;
            }
        case -1:
            {
                auto& FieldType = Field.m_Type;

                //
                // First deal with user types
                //
                if( FieldType[ Field.m_TypeOffset ] == '.' )
                {
                    const auto& UserType = m_UserTypes[ Field.m_iUserType ];
                    
                    for( int i=0; i<UserType.m_nSystemTypes; i++ )
                    {
                        if( ReadComponent( Field.m_iMemory[i], UserType.m_SystemType[i] ).isError(Error) )
                            return Error;
                    }
                }
                else
                {
                    for( int i=0; FieldType[ Field.m_TypeOffset + i]; i++ )
                    {
                        if( ReadComponent( Field.m_iMemory[i], FieldType[ Field.m_TypeOffset + i] ).isError(Error) )
                            return Error;
                    }
                }
                break;
            }
        default:
            {
                auto&         FieldDecl = m_Field[Field.m_iFieldDecl];
                const char*   pType     = &m_Memory[ FieldDecl.m_iMemory[0] ];
                
                //
                // Read based on user type
                //
                if( pType[0] == '.' )
                {
                    int Index;
                    
                    // Deal with fast mapping for binary reads
                    if( m_File.m_States.m_isBinary )
                    {
                        Index = reinterpret_cast<const std::uint16_t&>(pType[1]);
                        // Not need to remap as it already has been remapped
                    }
                    else
                    {
                        /*
                        if( m_UserTypes.BinarySearch( user_type( pType ), Index ) == false )
                        {
                            x_throw( "Unable to find user type");
                        }
                        */
                        xassert(false);
                    }
                    
                    const auto& UserType = m_UserTypes[Index];
                    
                    Field.m_nTypes      = UserType.m_nSystemTypes;
                    Field.m_iUserType   = Index;
                    
                    for( std::uint32_t i=0; i<Field.m_nTypes; i++ )
                    {
                        if( ReadComponent( Field.m_iMemory[i], UserType.m_SystemType[i] ).isError(Error) )
                            return Error;
                    }
                }
                else
                {
                    for( Field.m_nTypes=0; pType[ Field.m_nTypes ]; Field.m_nTypes++ )
                    {
                        if( ReadComponent( Field.m_iMemory[Field.m_nTypes], pType[ Field.m_nTypes ] ).isError(Error) )
                            return Error;
                    }
                }
                break;
            }
        }
    }

    //
    // Increment the line count
    // and reset the field count
    // reset the memory count
    //
    m_iLine++;
    m_iField        = 0;
    m_iMemOffet = 0;
    m_nTypesLine    = 0;

    return Error;
}
