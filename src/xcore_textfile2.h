#ifndef _XCORE_TEXTFILE2_H
#define _XCORE_TEXTFILE2_H
#pragma once

//-----------------------------------------------------------------------------------------------------
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
//      s      this is a xcore::string
//-----------------------------------------------------------------------------------------------------
namespace xcore::textfile::version_2
{
    //-----------------------------------------------------------------------------------------------------
    // Error and such
    //-----------------------------------------------------------------------------------------------------
    enum class error_state : std::uint32_t
    {
          GUID                      = xcore::crc<32>::FromString("xcore::textfile").m_Value
        , OK                        = 0
        , FAILURE
        , FILE_NOT_FOUND
        , UNEXPECTED_EOF
        , READ_TYPES_DONTMATCH
        , MISMATCH_TYPES
        , FIELD_NOT_FOUND
    };

    enum class file_type
    {
          TEXT
        , BINARY
    };

    union flags
    {
        std::uint32_t   m_Value{0};

        constexpr static std::uint32_t DEFAULTS           = 0;
        constexpr static std::uint32_t WRITE_FLOATS       = (1<<0);
        constexpr static std::uint32_t WRITE_SWAP_ENDIAN  = (1<<1);

        struct 
        {
            bool        m_isWriteFloats:1               // Writes floating point numbers as floating point rather than hex
            ,           m_isWriteEndianSwap:1;          // Swaps endian before writing (Only useful when writing binary)
        };
    };

    struct user_defined_types
    {
        constexpr user_defined_types() noexcept = default;
        constexpr user_defined_types( const char* pName, const char* pTypes ) noexcept 
            : m_Name                    {pName}
            , m_SystemTypes             {pTypes}
            , m_CRC                     { xcore::crc<32>::FromString(pName) }
            , m_NameLength              { []( const char* pName )  constexpr { int i=0; while( pName[++i] ); return i;}(pName) }
            , m_nSystemTypes            { []( const char* pTypes ) constexpr { int i=0; while( pTypes[++i]); return i;}(pTypes) } 
            {}
        string::fixed<char,32>  m_Name;
        string::fixed<char,32>  m_SystemTypes;
        xcore::crc<32>          m_CRC;
        int                     m_NameLength{};
        int                     m_nSystemTypes{};     // How many system types we are using, this is basically the length of the m_SystemType string
    };

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
            xcore::err      WriteData           ( string::view<const char> Buffer )                                         noexcept;
            xcore::err      ReadWhiteSpace      ( int& c )                                                                  noexcept;
            xcore::err      HandleDynamicTable  ( int& Count )                                                              noexcept;
        };

        //-----------------------------------------------------------------------------------------------------
        struct field_info
        {
            int                                 m_IntWidth;             // Integer part 
            int                                 m_Width;                // Width of this field
            int                                 m_iData;                // Index to the data
        };

        //-----------------------------------------------------------------------------------------------------
        struct field_type
        {
            int                                 m_nTypes;               // How many types does this dynamic field has
            xcore::crc<32>                      m_UserType;             // if 0 then is not valid
            string::fixed<char,16>              m_SystemTypes;          // System types
            int                                 m_iField;               // Index to the m_FieldInfo from the column 

            int                                 m_FormatWidth;          // Width of the column
        };

        //-----------------------------------------------------------------------------------------------------
        struct sub_column
        {
            int                                 m_FormatWidth      {0};
            int                                 m_FormatIntWidth   {0};
        };

        //-----------------------------------------------------------------------------------------------------
        struct column : field_type
        {
            string::fixed<char,128>             m_Name;                 // Type name
            int                                 m_NameLength;           // string Length for Name
            xcore::vector<field_type>           m_DynamicFields;        // if this column has dynamic fields here is where the info is
            xcore::vector<field_info>           m_FieldInfo;            // All fields for this column
            xcore::vector<sub_column>           m_SubColumn;            // Each of the types inside of a column is a sub_column.

            int                                 m_FormatNameWidth;      // Text Formatting name width 
            int                                 m_FormatTotalSubColumns;// Total width taken by the subcolumns

            void clear ( void ) noexcept { m_DynamicFields.clear(); m_FieldInfo.clear(); m_Name.clear(); }
        };

        //-----------------------------------------------------------------------------------------------------
        struct user_types : user_defined_types
        {
            bool                                m_bAlreadySaved     {false};    
        };

        //-----------------------------------------------------------------------------------------------------
        struct record
        {
            string::fixed<char,256>                 m_Name              {};     // Name of the record
            int                                     m_Count             {};     // How many entries in this record
            bool                                    m_bWriteCount       {};     // If we need to write out the count
        };
    }

    //-----------------------------------------------------------------------------------------------------
    // public interface
    //-----------------------------------------------------------------------------------------------------
    class stream
    {
    public:

        constexpr                       stream              ( void )                                                                    noexcept = default;
        void                            close               ( void )                                                                    noexcept;
                        xcore::err      Open                ( bool isRead, std::string_view View, file_type FileType, flags Flags={} )  noexcept;

                        template< std::size_t N, typename... T_ARGS >
        inline          xcore::err      Field               ( xcore::crc<32> UserType, const char(&pFieldName)[N], T_ARGS&... Args )    noexcept;

                        xcore::err      ReadFieldUserType   ( xcore::crc<32>& UserType, const char* pFieldName )                        noexcept;

                        template< std::size_t N, typename... T_ARGS >
        inline          xcore::err      Field               ( const char(&pFieldName)[N], T_ARGS&... Args )                             noexcept;

        inline          const auto*     getUserType         ( xcore::crc<32> UserType )                                         const   noexcept { if( auto I = m_UserTypeMap.find(UserType); I == m_UserTypeMap.end() ) return (details::user_types*)nullptr; else return &m_UserTypes[I->second]; }

                        template< std::size_t N, typename TT, typename T >
        inline          bool            Record              ( xcore::err& Error, const char (&Str)[N]
                                                                , TT&& RecordStar, T&& Callback )                                       noexcept;

                        template< std::size_t N, typename TT, typename T >
        inline          xcore::err      Record              ( const char (&Str)[N]
                                                                , TT&& RecordStar, T&& Callback )                                       noexcept;

                        template< std::size_t N, typename T >
        inline          bool            Record              ( xcore::err& Error, const char (&Str)[N]
                                                                , T&& Callback )                                                        noexcept;
                        xcore::err      WriteComment        ( const string::view<const char> Comment )                                  noexcept;


        constexpr       bool            isReading           ( void )                                                            const   noexcept { return m_File.m_States.m_isReading; }
        constexpr       bool            isEOF               ( void )                                                            const   noexcept { return m_File.m_States.m_isEOF; }
        constexpr       bool            isWriteFloats       ( void )                                                            const   noexcept { return m_File.m_States.m_isSaveFloats; }
        inline          auto&           getRecordName       ( void )                                                            const   noexcept { return m_Record.m_Name;  }
        inline          int             getRecordCount      ( void )                                                            const   noexcept { return m_Record.m_Count; }
                        std::uint32_t   AddUserType         ( const user_defined_types& UserType )                                      noexcept;
                        void            AddUserTypes        ( xcore::span<user_defined_types> UserTypes )                               noexcept;
                        void            AddUserTypes        ( xcore::span<const user_defined_types> UserTypes )                         noexcept;

    protected:

                        stream&         setup               ( std::FILE& File, details::states States )                                 noexcept;
                        xcore::err      openForReading      ( const std::filesystem::path& FilePath )                                   noexcept;
                        xcore::err      openForWriting      ( const std::filesystem::path& FilePath
                                                                , file_type FileType, flags Flags )                                     noexcept;
                        bool            isValidType         ( int Type )                                                        const   noexcept;
                        template< typename T >
                        xcore::err      Read                ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
                        xcore::err      ReadRecord          ( void )                                                                    noexcept;
                        xcore::err      ReadingErrorCheck   ( void )                                                                    noexcept;
                        xcore::err      ReadWhiteSpace      ( int& c )                                                                  noexcept;
                        xcore::err      ReadLine            ( void )                                                                    noexcept;
                        xcore::err      getC                ( int& c )                                                                  noexcept;
                        xcore::err      ReadColumn          ( xcore::crc<32> UserType, const char* pFieldName, arglist::view Args )     noexcept;
                        xcore::err      ReadFieldUserType   ( const char* pFieldName )                                                  noexcept;

                        template< typename T >
                        xcore::err      Write               ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
                        xcore::err      WriteLine           ( void )                                                                    noexcept;
                        xcore::err      WriteStr            ( string::view<const char> Buffer )                                         noexcept;
                        xcore::err      WriteFmtStr         ( const char* pFmt, ... )                                                   noexcept;
                        xcore::err      WriteChar           ( char C, int Count = 1 )                                                   noexcept;
                        xcore::err      WriteColumn         ( xcore::crc<32> UserType, const char* pFieldName, arglist::view Args )     noexcept;
                        xcore::err      WriteUserTypes      ( void )                                                                    noexcept;

                        xcore::err      HandleDynamicTable  ( int& Count )                                                              noexcept;

                        xcore::err      WriteRecord         ( const char* pHeaderName, std::size_t Count )                              noexcept;

        inline          bool            ValidateColumnChar  ( int c )                                                           const   noexcept;
                        xcore::err      BuildTypeInformation( const char* pFieldName )                                                  noexcept;


    protected:

        xcore::log::channel                                 m_Channel               { "xcore::textfile::stream" };

        details::file                                       m_File                  {};     // File pointer
        details::record                                     m_Record                {};     // This contains information about the current record
        xcore::vector<details::column>                      m_Columns               {};
        xcore::vector<char>                                 m_Memory                {};
        xcore::vector<details::user_types>                  m_UserTypes             {};
        xcore::vector<int>                                  m_DataMapping           {};
        std::unordered_map<xcore::crc<32>, std::uint32_t>   m_UserTypeMap           {};     // First uint32 is the CRC32 of the err name
                                                                                            // Second uint32 is the index in the UserTypes vector which contains the actual data
        int                                                 m_nColumns              {};
        int                                                 m_iLine                 {};     // Which line we are in the current record
        int                                                 m_iMemOffet             {};
        int                                                 m_iColumn               {};

        constexpr static int                                m_nSpacesBetweenFields  { 1 };
        constexpr static int                                m_nSpacesBetweenColumns { 2 };
        constexpr static int                                m_nLinesBeforeFileWrite { 64 };
    };
}
#endif
