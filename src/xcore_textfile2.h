#ifndef _XCORE_TEXTFILE2_H
#define _XCORE_TEXTFILE2_H
#pragma once

namespace xcore::textfile::version_2
{
    //-----------------------------------------------------------------------------------------------------
    // private interface for the file member
    //-----------------------------------------------------------------------------------------------------
    namespace details::file
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
        struct stream
        {
            std::FILE*      m_pFP{ nullptr };
            states          m_States;

                            stream              ( void )                                                                    noexcept = default;
                           ~stream              ( void )                                                                    noexcept;

            stream&         setup               ( std::FILE& File, states States )                                          noexcept;
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
    }

    //-----------------------------------------------------------------------------------------------------
    // private interface for the fields 
    //-----------------------------------------------------------------------------------------------------
    namespace details
    {
        /*
        struct formatting
        {
        };
        struct field
        {
            int Width;
        };

        struct coloumn 
        {
            int                 m_Width;            // With of the coloumn before writing in a text file
            std::span<field>    m_Fields;
            std::array<field>   m_Fields;           //
        };
        */
    }
}
#endif
