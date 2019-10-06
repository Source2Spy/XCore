#ifndef _XCORE_FILE_H
#define _XCORE_FILE_H
#pragma once

namespace xcore::file
{
    #ifdef MAX_PATH
        #undef MAX_PATH
    #endif

    namespace max_length
    {
        constexpr static int drive_v        = 32;         // How long a drive name can be
        constexpr static int directory_v    = 256;        // How long a path or directory
        constexpr static int file_name_v    = 256;        // File name
        constexpr static int extension_v    = 256;        // Extension
        constexpr static int path_v         = drive_v + directory_v + file_name_v + extension_v;
    };

    enum class error : std::uint32_t
    {
          GUID              = xcore::crc<32>::FromString("xcore::file").m_Value // GUID for this err err this is better been generated with a constexpr string to CRC32 or something like that
        , OK                = 0                                                 // Default OK required by the system
        , FAILURE           = 1                                                 // Default FAILURE required by the system
        , DEVICE_FAILURE
        , CREATING_FILE
        , OPENING_FILE
        , UNEXPECTED_EOF
        , INCOMPLETE
    };

    union access_types
    {
        std::uint32_t   m_Value{ 0 };
        struct
        {
            bool m_bCreate       : 1;  // If not create file then we are accessing an existing file
            bool m_bRead         : 1;  // Has read permissions or not
            bool m_bWrite        : 1;  // Has write permissions or not
            bool m_bASync        : 1;  // Async enable?
            bool m_bCompress     : 1;  // Do compress files (been compress)
            bool m_bText         : 1;  // This is a text file. Note that this is handle at the top layer.
            bool m_bForceFlush   : 1;  // Forces to flush constantly (good for debugging). Note that this is handle at the top layer.
        };
    };

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    static const wchar_t*   getTempPath     ( void )                                             noexcept;
    static bool             getFileTime     ( const char* pFileName, std::uint64_t& FileTime )   noexcept;

    //------------------------------------------------------------------------------
    // Description:
    //     This class is the most low level class for the file system. This class deals
    //     with the most low level platform specific API to do its job. For most part
    //     users will never deal with this class directly. It is intended to be used
    //     by more expert users and low level people. This class defines a device to be 
    //     access by the stream class.
    //------------------------------------------------------------------------------
    class device_i
    {
    public:

        enum seek_mode
        {
            SKM_ORIGIN,
            SKM_CURENT,
            SKM_END
        };

    public:
                                            device_i        (void)                                                                  noexcept = delete;
        constexpr                           device_i        ( string::constant<char> DeviceName)                                    noexcept : m_DeviceName{ DeviceName } {}
        virtual                            ~device_i        (void)                                                                  noexcept {}
        virtual         void*               open            (const string::view<const wchar_t> FileName, access_types Flags)        noexcept = 0;
        virtual         void                close           (void* pFile)                                                           noexcept = 0;
        virtual         xcore::err          Read            (void* pFile, std::span<std::byte> View )                               noexcept = 0;
        virtual         xcore::err          Write           (void* pFile, const std::span<const std::byte> View )                   noexcept = 0;
        virtual         xcore::err          Seek            (void* pFile, seek_mode Mode, units::bytes Pos)                         noexcept = 0;
        virtual         xcore::err          Tell            (void* pFile, units::bytes& Pos )                                       noexcept = 0;
        virtual         void                Flush           (void* pFile)                                                           noexcept = 0;
        virtual         xcore::err          Length          (void* pFile, units::bytes& L )                                         noexcept = 0;
        virtual         bool                isEOF           (void* pFile)                                                           noexcept = 0;
        virtual         xcore::err          Synchronize     (void* pFile, bool bBlock)                                              noexcept = 0;
        virtual         void                AsyncAbort      (void* pFile)                                                           noexcept = 0;
        virtual         void                Init            (const void*)                                                           noexcept {}
        virtual         void                Kill            (void)                                                                  noexcept {}
        constexpr       auto&               getName         (void)                                                          const   noexcept { return m_DeviceName; }

    public:

        device_i*                       m_pNext         { nullptr };
        const string::constant<char>    m_DeviceName    {};
    };

    //------------------------------------------------------------------------------
    // Description:
    //      The stream class is design to be a direct replacement to the fopen. The class
    //      supports most features of the fopen standard plus a few more. The Open function
    //      has been change significantly in order to accommodate the new options. The access
    //      modes are similar BUT not identical to the fopen. Some of the functionality like
    //      the append has drastically change from the standard. Here are the access modes.
    //
    //<TABLE>
    //     Access Mode         Description
    //     =================  ----------------------------------------------------------------------------------------
    //         "r"            Read only                 - the file must exits. Useful when accessing DVDs or other read only media
    //         "r+"           Reading and Writing       - the file must exits
    //         "w" or "w+"    Reading and Writing       - the file will be created.  
    //         "a" or "a+"    Reading and Writing       - the file must exists, and it will do an automatic SeekEnd(0). 
    //  
    //         "@"            Asynchronous Mode         - Allows you to use async features.
    //         "c"            Enable File Compression   - (No Supported) This must be use with 'w'. It will compress at file close.
    //  
    //         "b"            Binary files Mode         - This is the default so you don't need to put it really. 
    //         "t"            Text file Mode            - For writing or Reading text files. If you don't add this assumes you are doing binary files.             
    //                                                      This command basically writes an additional character '\\r' whenever it finds a '\\n' and       
    //                                                      when reading it removes it when it finds '\\r\\n' 
    //</TABLE>
    //
    //      To illustrate different possible combinations and their meaning we put together a table 
    //      to show a few examples.
    //<TABLE>
    //     Examples of Modes  Description
    //     =================  ----------------------------------------------------------------------------------------
    //          "r"           Good old fashion read a file. Note that you never need to do "rc" as the file system detects that automatically.
    //          "wc"          Means that we want to write a compress file       
    //          "wc@"         Means to create a compress file and we are going to access it asynchronous
    //          "r+@"         Means that we are going to read and write to an already exiting file asynchronously.
    //</TABLE>
    //
    //     Other key change added was the ability to have more type of devices beyond the standard set.
    //     Some of the devices such the net device can be use in conjunction with other devices.
    //     The default device is of course the hard-disk in the PC.
    //<TABLE>
    //     Known devices      Description
    //     =================  ----------------------------------------------------------------------------------------
    //          ram:\          (No Supported) To use ram as a file device
    //          dvd:\          (No Supported) To use the dvd system of the console
    //          net:\          (No Supported) To access across the network
    //          temp:\         To the temporary folder/drive for the machine
    //          memcard:\      (No Supported) To access memory card file system
    //          localhd:\      (No Supported) To access local hard-drives such the ones found in the XBOX
    //          buffer:\       (No Supported) User provided buffer data
    //          c:\ d:\ e:\    ...etc local devices such PC drives
    //</TABLE>
    //
    //     Example of open strings:
    //<CODE>
    //     Open( X_WSTR("net:\\\\c:\\test.txt"),        "r" );      // Reads a file across the network
    //     Open( X_WSTR("ram:\\\\name doesnt matter"),  "w" );      // Creates a ram file which you can read/write
    //     Open( X_WSTR("c:\\dumpfile.bin"),            "wc" );     // Creates a compress file in you c: drive
    //     Open( X_WSTR("UseDefaultPath.txt"),          "r@" );     // No device specify so it reads the file from the default path
    //</CODE>
    //
    //------------------------------------------------------------------------------

    class stream
    {
    public:
        constexpr                               stream          ( void )                                                            noexcept = default;
        inline                                 ~stream          ( void )                                                            noexcept { close(); }
                        xcore::err              open            ( const string::view<const wchar_t> FileName, const char* pMode)    noexcept;
                        void                    close           ( void )                                                            noexcept;
        inline          xcore::err              ToFile          ( stream& File )                                                    noexcept;
        inline          xcore::err              ToMemory        ( std::span<std::byte> View )                                       noexcept;
        xforceinline    xcore::err              Synchronize     ( bool bBlock )                                                     noexcept;
        xforceinline    void                    AsyncAbort      ( void )                                                            noexcept;
        xforceinline    void                    setForceFlush   ( bool bOnOff)                                                      noexcept;
        xforceinline    void                    Flush           ( void)                                                             noexcept;
        xforceinline    xcore::err              SeekOrigin      ( units::bytes Offset )                                             noexcept;
        xforceinline    xcore::err              SeekEnd         ( units::bytes Offset )                                             noexcept;
        xforceinline    xcore::err              SeekCurrent     ( units::bytes Offset )                                             noexcept;
        xforceinline    xcore::err              Tell            ( units::bytes& Bytes )                                             noexcept;
        xforceinline    bool                    isEOF           ( void )                                                            noexcept;
        xforceinline    bool                    isOpen          ( void )                                                    const   noexcept { return !!m_pFile; }
        xforceinline    xcore::err              getC            ( int& C )                                                          noexcept;
        inline          xcore::err              putC            ( int C, int Count = 1, bool bUpdatePos = true)                     noexcept;
        inline          xcore::err              AlignPutC       ( int C, int Count = 0, int Aligment = 4, bool bUpdatePos = true)   noexcept;
        xforceinline    xcore::err              getFileLength   ( units::bytes& Length )                                            noexcept;
  
        template<class T>
        inline          xcore::err              ReadString      ( string::view<T> Val )                                             noexcept;
        template<typename T>
        inline          xcore::err              Write           ( const T&& Val )                                                   noexcept;

        template<typename T>
        inline          xcore::err              Write           ( const T&  Val )                                                   noexcept;

        template<typename T>
        inline          xcore::err              WriteView       ( const T&& A )                                                     noexcept;

        template<typename T>
        inline          xcore::err              WriteView       ( const T& A )                                                      noexcept;

        template<typename T>
        inline          xcore::err              WriteString     ( const string::view<T> View )                                      noexcept;

        template<class T>
        inline          xcore::err              Read            ( T&& Val )                                                         noexcept;

        template<class T>
        inline          xcore::err              Read            ( T& Val )                                                          noexcept;

        template<class T>
        inline          xcore::err              ReadView        ( T&& View )                                                        noexcept;

        template<class T>
        inline          xcore::err              ReadView        ( T& View )                                                         noexcept;

        xforceinline    bool                    isBinaryMode    ( void )                                                    const   noexcept;
        xforceinline    bool                    isReadMode      ( void )                                                    const   noexcept;
        xforceinline    bool                    isWriteMode     ( void )                                                    const   noexcept;

        template<typename... T_ARGS>
        inline          xcore::err              Printf          ( const char* pFormatStr, const T_ARGS& ... Args )                  noexcept;

    protected:

        void                    Clear           ( void )                                                                    noexcept;
        xcore::err              ReadRaw         (std::span<std::byte> View)                                                 noexcept;
        xcore::err              WriteRaw        (std::span<std::byte> View)                                                 noexcept;

    protected:

        void*                   m_pFile         { nullptr };
        device_i*               m_pDevice       { nullptr };
        access_types            m_AccessType    {};
        string::ref<wchar_t>    m_FileName      {};

        friend bool             setCompressDevice( stream& File, bool bCompressAndText );
        friend class            device_i;
    };
}
#endif