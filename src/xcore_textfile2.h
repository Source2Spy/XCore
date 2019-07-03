#ifndef _XCORE_TEXTFILE2_H
#define _XCORE_TEXTFILE2_H
#pragma once

namespace xcore::textfile::version_2
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
        struct info_field
        {
            char                                m_AtomicType;           // Atomic type
            int                                 m_IntWidth;             // Integer part 
            int                                 m_Width;                // Width of this field
            int                                 m_iData;                // Index to the data
        };

        //-----------------------------------------------------------------------------------------------------
        struct dynamic_field
        {
            int                                 m_nTypes;               // How many types does this dynamic field has
            crc::units<32>                      m_UserType;             // if 0 then is not valid
            int                                 m_iField;               // Index to the m_FieldInfo from the column 
        };

        //-----------------------------------------------------------------------------------------------------
        struct column
        {
            string::fixed<char,128>             m_Name;                 // Type name
            int                                 m_nTypes;               // -1 is '?' otherwise is fixed set
            crc::units<32>                      m_UserType;             // if 0 then is not valid
            xcore::unique_span<dynamic_field>   m_DynamicFields;        // if this column has dynamic fields here is where the info is
            xcore::unique_span<info_field>      m_FieldInfo;            // All fields for this column
            void clear ( void ) noexcept { m_DynamicFields.clear(); m_FieldInfo.clear(); }
        };

        //-----------------------------------------------------------------------------------------------------
        struct user
        {
            string::fixed<char,128>             m_UserType          {};     // Name of the user type
            string::fixed<char,16>              m_SystemType        {};     // System types such: fff
            crc::units<32>                      m_CRC32             {};     // CRC32 of the user type string
            bool                                m_bSaved            {};     
            std::uint8_t                        m_nSystemTypes      {};     // How many system types we are using, this is basically the length of the m_SystemType string
            std::uint8_t                        m_UID               {};     // Unique ID from the user
        };

        //-----------------------------------------------------------------------------------------------------
        struct record
        {
            string::fixed<char,256>                 m_Name              {};     // Name of the record
            int                                     m_Count             {};     // How many entries in this record
            bool                                    m_bWriteCount       {};     // If we need to write out the count
        };

        //-----------------------------------------------------------------------------------------------------
        // Build Types
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
          GUID                      = xcore::crc::FromString<32>("xcore::textfile").m_Value
        , OK                        = 0
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

        constexpr                       stream              ( void )                                                                    noexcept = default;
                                       ~stream              ( void )                                                                    noexcept;
        void                            close               ( void )                                                                    noexcept;
                        xcore::err      Open                ( bool isRead, std::string_view View, file_type FileType, flags Flags={} )  noexcept;
                        template< std::size_t N, typename... T_ARGS >
        inline          xcore::err      Field               ( const char(&pFieldName)[N], T_ARGS&... Args )                             noexcept;
                        template< std::size_t N, typename TT, typename T >
        inline          bool            Record              ( xcore::err& Error, const char (&Str)[N]
                                                                , TT&& RecordStar, T&& Callback )                                       noexcept;
        constexpr       bool            isReading           ( void )                                                            const   noexcept { return m_File.m_States.m_isReading; }
        constexpr       bool            isEOF               ( void )                                                            const   noexcept { return m_File.m_States.m_isEOF; }
        constexpr       bool            isWriteFloats       ( void )                                                            const   noexcept { return m_File.m_States.m_isSaveFloats; }
        inline          auto&           getRecordName       ( void )                                                            const   noexcept { return m_Record.m_Name;  }
        inline          int             getRecordCount      ( void )                                                            const   noexcept { return m_Record.m_Count; }

    protected:

                        stream&         setup               ( std::FILE& File, details::states States )                                 noexcept;
                        xcore::err      openForReading      ( const std::filesystem::path& FilePath )                                   noexcept;
                        xcore::err      openForWriting      ( const std::filesystem::path& FilePath
                                                                , file_type FileType, flags Flags )                                     noexcept;
                        bool            isValidType         ( int Type )                                                        const   noexcept;
                        std::uint32_t   AddUserType         ( const char* pSystemTypes, const char* pUserType
                                                                , crc::units<32> CRC32UserType, std::uint8_t UID )                      noexcept;
                        template< typename T >
                        xcore::err      Read                ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
                        xcore::err      ReadRecord          ( void )                                                                    noexcept;
                        xcore::err      ReadingErrorCheck   ( void )                                                                    noexcept;
                        xcore::err      ReadWhiteSpace      ( int& c )                                                                  noexcept;
                        xcore::err      ReadLine            ( void )                                                                    noexcept;
                        xcore::err      getC                ( int& c )                                                                  noexcept;
                        xcore::err      ReadFieldC          ( int& iUserRef, const char* pFieldName, arglist::view Args )               noexcept;
                        template< typename T >
                        xcore::err      Write               ( T& Buffer, int Size = sizeof(T), int Count = 1 )                          noexcept;
                        xcore::err      WriteLine           ( void )                                                                    noexcept;
                        xcore::err      WriteStr            ( string::view<const char> Buffer )                                         noexcept;
                        xcore::err      WriteFmtStr         ( const char* pFmt, ... )                                                   noexcept;
                        xcore::err      WriteChar           ( char C, int Count = 1 )                                                   noexcept;

                        xcore::err      HandleDynamicTable  ( int& Count )                                                              noexcept;

    protected:

        details::file                                       m_File                  {}; // File pointer
        details::record                                     m_Record                {}; // This contains information about the current record
        xcore::vector<details::column>                      m_Columns               {};
        xcore::vector<std::byte>                            m_Memory                {};
        xcore::vector<details::user>                        m_UserTypes             {};
        std::unordered_map<crc::units<32>, std::uint32_t>   m_UserTypeMap           {}; // First uint32 is the CRC32 of the err name
                                                                                        // Second uint32 is the index in the UserTypes vector which contains the actual data
        int                                                 m_iLine                 {}; // Which line we are in the current record
        int                                                 m_iField                {}; // Current Field that we are trying to read
        int                                                 m_nTypesLine            {}; // Number of types per line
        int                                                 m_iCurrentOffet         {};
    };
}
#endif
