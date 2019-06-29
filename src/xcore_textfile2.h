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
        };

        //-----------------------------------------------------------------------------------------------------
        struct user_type
        {
            string::fixed<char,256>             m_UserType          {};     // Name of the user type
            string::fixed<char,256>             m_SystemType        {};     // System types such: fff
            crc::units<32>                      m_CRC32             {};     // CRC32 of the user type string
            bool                                m_bSaved            {};     
            std::uint8_t                        m_nSystemTypes      {};     // How many system types we are using, this is basically the length of the m_SystemType string
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
    class stream
    {
    public:
                        stream              ( void )                                                                    noexcept = default;
                       ~stream              ( void )                                                                    noexcept;

    protected:

        stream&         setup               ( std::FILE& File, details::states States )                                 noexcept;
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

    protected:

        details::file                                       m_File                  {}; // File pointer
        details::record_info                                m_Record                {}; // This contains information about the current record
        xcore::unique_span<details::column>                 m_Columns               {};
        xcore::unique_span<std::byte>                       m_Memory                {};
        xcore::unique_span<user_type>                       m_UserTypes             {};
        std::unordered_map<crc::units<32>, std::uint32_t>   m_UserTypeMap           {}; // First uint32 is the CRC32 of the err name
                                                                                        // Second uint32 is the index in the UserTypes vector which contains the actual data
    };
}
#endif
