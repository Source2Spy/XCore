#ifndef _XCORE_TEXTFILE_H
#define _XCORE_TEXTFILE_H
#pragma once

//-----------------------------------------------------------------------------------------------------
//
// Please remember that text file are lossy files due to the floats don't survive text-to-binary conversion
//
//     Type    Description
//     ------  ----------------------------------------------------------------------------------------
//      f      32 bit float
//      F      64 bit double
//      d      32 bit integer
//      g      32 bit unsigned integer
//      D      64 bit integer
//      G      64 bit unsigned integer
//      c       8 bit integer
//      h       8 bit unsigned integer
//      C      16 bit integer
//      H      16 bit unsigned integer
//      s      this is a std::string
//      S      undefined (DONT USE)
//      e      Enum this is a std::string but without spaces
//-----------------------------------------------------------------------------------------------------
namespace xcore::textfile::version_1
{
    //-----------------------------------------------------------------------------------------------------
    // private interface
    //-----------------------------------------------------------------------------------------------------
    namespace details
    {
        //-----------------------------------------------------------------------------------------------------
        union states
        {
            std::uint32_t       m_Value{ 0 };
            struct
            {
                  bool            m_isView        : 1       // means we don't own the pointer
                                , m_isEOF         : 1       // We have reach end of file so no io operations make sense after this
                                , m_isBinary      : 1       // Tells if we are dealing with a binary file or text file
                                , m_isEndianSwap  : 1       // Tells if when reading we should swap endians
                                , m_isReading     : 1       // Tells the system whether we are reading or writing
                                , m_isSaveFloats  : 1;      // Save floats as hex
            };
        };

        //-----------------------------------------------------------------------------------------------------
        struct file
        {
            std::FILE*      m_pFP{ nullptr };
            states          m_States;

                            file                ( void )                                                                    noexcept = default;
                           ~file                ( void )                                                                    noexcept;

            file&           setup               ( std::FILE& File, states States )                                          noexcept;
            xcore::err      openForReading      ( const std::filesystem::path& FilePath, bool isBinary )                    noexcept;
            xcore::err      openForWritting     ( const std::filesystem::path& FilePath, bool isBinary )                    noexcept;
            void            close               ( void )                                                                    noexcept;
            xcore::err      ReadingErrorCheck   ( void )                                                                    noexcept;
            template< typename T >
            xcore::err      Read                ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
            xcore::err      getC                ( int& c )                                                                  noexcept;
            xcore::err      WriteStr            ( string::view<const char> Buffer )                                         noexcept;
            xcore::err      WriteFmtStr         ( const char* pFmt, ... )                                                   noexcept;
            template< typename T >
            xcore::err      Write               ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
            xcore::err      WriteChar           ( char C, int Count = 1 )                                                   noexcept;
            xcore::err      ReadWhiteSpace      ( int& c )                                                                  noexcept;
            xcore::err      HandleDynamicTable  ( int& Count )                                                              noexcept;
        };

        //-----------------------------------------------------------------------------------------------------
        struct field
        {
            constexpr static auto MAX_TYPES_PER_FIELD = 32;

            string::ref<char>                               m_Type              {};         // Field name such "jack:ff"
            std::uint32_t                                   m_nTypes            = ~0u;      // Number of types for this field
            std::uint32_t                                   m_TypeOffset        = ~0u;      // This is the offset to the string which gives the err ex:"fff"
            std::array<std::uint32_t,MAX_TYPES_PER_FIELD>   m_iMemory           {};         // The location in the memory buffer for each of these fields    
            int                                             m_iFieldDecl        = -999;     //  * When this is -1 means this is a normal field type
                                                                                            //  * When this is -2 this means is a declaration
                                                                                            //  * When is positive then it is a "pepe:<test>" and
                                                                                            //  this has an index to the field which contain the err declaration
            std::uint32_t                                   m_Width             = ~0u;      // What is the width in character for this field. This is only used when writing.
            std::uint16_t                                   m_TypesPreWidth     = ~0;
            std::uint16_t                                   m_TypesPostWidth    = ~0;
            std::array<std::uint8_t,MAX_TYPES_PER_FIELD>    m_TypeWidth         {};         // This is the width that each column has per err
            std::array<std::uint8_t,MAX_TYPES_PER_FIELD>    m_FracWidth         {};         // This is the additional offset for floats/ints so that the integer part can align
            int                                             m_DynamicTypeCount  = -999;     // This is just used for debugging but its intent is a err count for dynamic types
            std::uint16_t                                   m_iUserType         = ~0;       // A place to put the user_types err
        };

        //-----------------------------------------------------------------------------------------------------
        struct record_info
        {
            string::fixed<char,256>                 m_Name              {};     // Name of the record
            int                                     m_Count             {};     // How many entries in this record
            bool                                    m_bWriteCount       {};     // If we need to write out the count
        };

        //-----------------------------------------------------------------------------------------------------
        struct user_types
        {
            int                                     m_iFieldType        {};
            int                                     m_iFieldData        {};
        };
    
        //-----------------------------------------------------------------------------------------------------
        struct user_type
        {
            string::fixed<char,256>                 m_UserType          {};     // Name of the user_types type
            string::fixed<char,256>                 m_SystemType        {};     // System types such: fff
            crc<32>                                 m_CRC32             {};     // CRC32 of the user_types type string
            bool                                    m_bSaved            {};     
            std::uint8_t                            m_nSystemTypes      {};     // How many system types we are using, this is basically the length of the m_SystemType string
            std::uint8_t                            m_UID               {};     // UID from the user_types. He may want to remap things.
        };

        //-----------------------------------------------------------------------------------------------------
        template< typename T >
        struct type_filter
        {
            using type = T&;
        };

        template<>
        struct type_filter<typename string::ref<char>>
        {
            using type = string::view<char>;
        };

        template<std::size_t N>
        struct type_filter<typename string::view<char,N>>
        {
            using type = string::view<char,N>&;
        };

        //-----------------------------------------------------------------------------------------------------
        template< typename T, typename... T_ARGS >
        constexpr void BuildTypesRecursive( char* p ) noexcept
        {
            char c {};
            using t = std::decay_t<T>;

                 if constexpr ( std::is_same_v< t, std::int32_t >       ) c = 'd';
            else if constexpr ( std::is_same_v< t, std::int64_t >       ) c = 'D';
            else if constexpr ( std::is_same_v< t, std::int8_t >        ) c = 'c';
            else if constexpr ( std::is_same_v< t, std::int16_t >       ) c = 'C';
            else if constexpr ( std::is_same_v< t, float >              ) c = 'f';
            else if constexpr ( std::is_same_v< t, double >             ) c = 'F';
            else if constexpr ( std::is_same_v< t, string::view<char> > ) c = 's';
            else if constexpr ( std::is_same_v< t, string::ref<char> >  ) c = 's';
            else if constexpr ( std::is_same_v< t, std::uint32_t >      ) c = 'g';
            else if constexpr ( std::is_same_v< t, std::uint64_t >      ) c = 'G';
            else if constexpr ( std::is_same_v< t, std::uint8_t >       ) c = 'h';
            else if constexpr ( std::is_same_v< t, std::uint16_t >      ) c = 'H';
            else static_assert( xcore::types::always_false_v<T>, "xcore::textfile, Unknown type. Please make sure you enter a supported type" );

            *p = c;
            p++;
            if constexpr ( sizeof...(T_ARGS) == 0 ) *p = 0;
            else                                     BuildTypesRecursive<T_ARGS...>(p);
        }

        //-----------------------------------------------------------------------------------------------------
        template< std::size_t N, typename... T_ARGS >
        constexpr string::fixed<char,N + sizeof...(T_ARGS) + 2> BuildTypes( const char (&pFieldName)[N] ) noexcept
        {
            string::fixed<char,N + sizeof...(T_ARGS)+2> Data {};
            for( int i=0; Data[i] = pFieldName[i]; i++ );
            Data[N] = ':';
            BuildTypesRecursive<T_ARGS...>( Data.data() + (1+N) );
            return Data;
        }

        //-----------------------------------------------------------------------------------------------------
        template<  typename... T_ARGS >
        constexpr string::fixed<char,sizeof...(T_ARGS) + 1> BuildTypes( void ) noexcept
        {
            string::fixed<char,sizeof...(T_ARGS)+1> Data {};
            BuildTypesRecursive<T_ARGS...>( Data.data() );
            return Data;
        }
    }

    //-----------------------------------------------------------------------------------------------------
    // Error and such
    //-----------------------------------------------------------------------------------------------------
    enum class error_state : std::uint32_t
    {
          GUID      = xcore::crc<32>::FromString("xcore::textfile").m_Value
        , OK        = 0
        , FAILURE
        , FILE_NOT_FOUND
        , UNEXPECTED_EOF
        , READ_TYPES_DONTMATCH
        , MISMATCH_TYPES
    };

    enum class file_type
    {
          TEXT
        , BINARY
    };

    union flags
    {
        std::uint32_t   m_Value{0};

        constexpr static std::uint32_t WRITE_FLOATS       = (1<<0);
        constexpr static std::uint32_t WRITE_SWAP_ENDIAN  = (1<<1);

        struct 
        {
            bool        m_isWriteFloats:1               // Writes floating point numbers as floating point rather than hex
            ,           m_isWriteEndianSwap:1;          // Swaps endian before writing (Only useful when writing binary)
        };
    };

    //-----------------------------------------------------------------------------------------------------
    // public interface
    //-----------------------------------------------------------------------------------------------------
    class stream
    {
    public:
                            xcore::err                  openForReading      ( const std::filesystem::path& FilePath )                                                       noexcept;
                            xcore::err                  openForWriting      ( const std::filesystem::path& FilePath, file_type Type = file_type::TEXT, flags Flags = {} )   noexcept;
                            void                        close               ( void )                                                                                        noexcept;
                            void                        ReadFromFile        ( std::FILE& File, file_type Type )                                                             noexcept;
                            void                        WriteToFile         ( std::FILE& File, file_type Type )                                                             noexcept;

                            xcore::err                  ReadRecord          ( void )                                                                                        noexcept;
                            int                         ReadField           ( const char* FieldName, ... )                                                                  noexcept;
                            xcore::err                  ReadLine            ( void )                                                                                        noexcept;
                            xcore::err                  ReadNextRecord      ( void )                                                                                        noexcept;
        inline              int                         getRecordCount      ( void )                                                                                        noexcept { return m_Record.m_Count; }
        inline              const string::view<char>    getRecordName       ( void )                                                                                        noexcept { return m_Record.m_Name;  }
                            std::uint32_t               AddUserType         ( const char* pSystemTypes, const char* pUserType, xcore::crc<32> CRC32UserType, std::uint8_t UID )  noexcept;
        static              bool                        isValidType         ( int Type )                                                                                    noexcept;
        constexpr           bool                        isReading           ( void )    const                                                                               noexcept { return m_File.m_States.m_isReading; }
        constexpr           bool                        isEOF               ( void )    const                                                                               noexcept { return m_File.m_States.m_isEOF; }
        constexpr           bool                        isWriteFloats       ( void )    const                                                                               noexcept { return m_File.m_States.m_isSaveFloats; }
        inline              xcore::err                  Open                ( bool isRead, std::string_view View, file_type FileType, flags Flags = {} )                    noexcept
                            {
                                xcore::err Error;
                                if( isRead )
                                {
                                    if( openForReading( View ).isError(Error) )        return Error;
                                }
                                else
                                {
                                    if( openForWriting( View, FileType, Flags ).isError(Error) ) return Error;
                                }
                                return Error;
                            }
                            template< std::size_t N, typename... T_ARGS >
        inline              xcore::err_optional<int> Field          ( const char(&pFieldName)[N], T_ARGS&... Args )                                                     noexcept
                            {
                                static_assert( 0 != (sizeof...(T_ARGS)) );
                                constexpr static string::fixed<char, sizeof...(T_ARGS)+1> Types{ details::BuildTypes<T_ARGS...>() };
                                if( isReading() ) 
                                {
                                    int         iUserRef;
                                    xcore::err  Error;
                                    static_assert( (std::is_const_v<T_ARGS> || ... ) == false );
                                    if( ReadFieldC( iUserRef, pFieldName, Types.data(), &static_cast<typename details::type_filter<T_ARGS>::type>(Args)... ).isError(Error) ) return Error;
                                    return iUserRef;
                                }
                                else 
                                {
                                    WriteColumn( pFieldName, Types.data(), static_cast<typename details::type_filter<T_ARGS>::type>( Args )... );
                                }
                                return {0};
                            }

                            xcore::err                  WriteRecord         ( const char* pHeaderName, std::size_t Count = ~0 )                                         noexcept;
                            xcore::err                  WriteComment        ( const string::view<char> Comment )                                                        noexcept;
                            xcore::err                  WriteLine           ( void )                                                                                    noexcept;


                            template< std::size_t N, typename TT, typename T >
        inline              bool                        Record              ( xcore::err& Error, const char (&Str)[N], TT&& RecordStar, T&& Callback )                  noexcept
                            {
                                if( m_File.m_States.m_isReading )
                                {
                                    if( getRecordName() != Str )
                                    { 
                                        (void)xerr_failure( Error, "Unexpected record" ); 
                                        return true; 
                                    }
                                    std::size_t Count = getRecordCount();
                                    RecordStar(Count,Error);
                                    for( std::remove_const_t<decltype(Count)> i=0; i<Count; i++ )
                                    {
                                        if( ReadLine().isError(Error) ) return true;
                                        Callback(i,Error);
                                        if( Error ) return true;
                                    }
                                    // Read the next record
                                    if( ReadRecord().isError( Error ) ) 
                                    {
                                        if( Error.getCode().getState<error_state>() == error_state::UNEXPECTED_EOF ) Error.clear();
                                        else return true;
                                    }
                                }
                                else 
                                {
                                    std::size_t Count;
                                    RecordStar(Count,Error);
                                    if( WriteRecord( Str, Count ).isError( Error ) ) return true;

                                    for( std::remove_const_t<decltype(Count)> i=0; i<Count; i++ )
                                    {
                                        Callback(i,Error);
                                        if( Error ) return true;
                                        if( WriteLine().isError(Error) ) return true;
                                    }
                                }
                                return false;
                            }

    protected:

                            xcore::err                  ReadFieldC          ( int& iUserRef, const char* pFieldName, const char* pArgsTypes, ... )                      noexcept;
                            void                        WriteColumn         ( const char* pFieldName, const char* pArgsTypes, ... )                                     noexcept;
                            void                        WriteFieldCIndirect ( const char* pFieldName, const char* pArgsTypes, ... )                                     noexcept;
                            void                        WriteColumn         ( const char* pFieldName, const char* pArgsTypes, va_list& Args1, va_list& Args2, bool bIndirect ) noexcept;
                            void                        WriteComponent      ( int iField, int iType, int c, va_list& Args, bool bIndirect )                             noexcept;
                            xcore::err                  WriteUserTypes      ( void )                                                                                    noexcept;

                            void                        BuildTypeInformation( const char* pFullFieldName )                                                              noexcept;
                            void                        BuildTypeInformation( const char* pFieldName, const char* pArgsTypes )                                          noexcept;

                            xcore::err                  ReadComponent       ( std::uint32_t& iStartOffset, int Type )                                                   noexcept;

    protected:
        
        xcore::log::channel                                 m_Channel               { "xcore::textfile::stream" };

        details::file                                       m_File                  {}; // File pointer
        details::record_info                                m_Record                {}; // This contains information about the current record
        int                                                 m_iLine                 {}; // Which line we are in the current record
        int                                                 m_iField                {}; // Current Field that we are trying to read
        int                                                 m_nTypesLine            {}; // Number of types per line

        std::vector<details::field>                         m_Field                 {}; // Contain a list of field names and their types
        std::uint32_t                                       m_iMemOffet         {}; // Index to the next location ready to be allocated
        std::vector<char>                                   m_Memory                {}; // Buffer where the fields are store (Only one line worth)

        std::vector<std::uint16_t>                          m_BinReadUserTypeMap    {}; // This is an optimization done when loading binary files using usertypes
                                                                                        // this tables have index to types that were loaded from the file rather
                                                                                        // than added by the user_types. This table is always shorted as well.

        std::vector<details::user_types>                          m_User                  {}; // This is used only when reading
        std::vector<details::user_type>                     m_UserTypes             {}; // List of types created by the user_types
        std::unordered_map<xcore::crc<32>, std::uint32_t>   m_UserTypeMap           {}; // First uint32 is the CRC32 of the err name
                                                                                        // Second uint32 is the index in the UserTypes vector which contains the actual data
    };
}
#endif


