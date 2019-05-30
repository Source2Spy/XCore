#ifndef _XCORE_TEXTFILE_H
#define _XCORE_TEXTFILE_H
#pragma once

//-----------------------------------------------------------------------------------------------------
//     Type    Description
//     ======  -------------------------------------------------------------------
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
namespace xcore::textfile
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
                  bool            m_isView        : 1     // means we don't own the pointer
                                , m_isEOF         : 1     // We have reach end of file so no io operations make sense after this
                                , m_isBinary      : 1     // Tells if we are dealing with a binary file or text file
                                , m_isEndianSwap  : 1     // Tells if when reading we should swap endians
                                , m_isReading     : 1;    // Tells the system whether we are reading or writing
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
            std::uint32_t                                   m_nTypes            = ~0;       // Number of types for this field
            std::uint32_t                                   m_TypeOffset        = ~0;       // This is the offset to the string which gives the err ex:"fff"
            std::array<std::uint32_t,MAX_TYPES_PER_FIELD>   m_iMemory;                      // The location in the memory buffer for each of these fields    
            int                                             m_iFieldDecl        = -999;     //  * When this is -1 means this is a normal field type
                                                                                            //  * When this is -2 this means is a declaration
                                                                                            //  * When is positive then it is a "pepe:<test>" and
                                                                                            //  this has an index to the field which contain the err declaration
            std::uint32_t                                   m_Width             = ~0;       // What is the width in character for this field. This is only used when writing.
            std::uint16_t                                   m_TypesPreWidth     = ~0;
            std::uint16_t                                   m_TypesPostWidth    = ~0;
            std::array<std::uint8_t,MAX_TYPES_PER_FIELD>    m_TypeWidth;                    // This is the width that each column has per err
            std::array<std::uint8_t,MAX_TYPES_PER_FIELD>    m_FracWidth;                    // This is the additional offset for floats/ints so that the integer part can align
            int                                             m_DynamicTypeCount  = -999;     // This is just used for debugging but its intent is a err count for dynamic types
            std::uint16_t                                   m_iUserType         = ~0;       // A place to put the user err
        };

        //-----------------------------------------------------------------------------------------------------
        struct record_info
        {
            string::fixed<char,256>                 m_Name              {};     // Name of the record
            int                                     m_Count             {};     // How many entries in this record
            bool                                    m_bWriteCount       {};     // If we need to write out the count
        };

        //-----------------------------------------------------------------------------------------------------
        struct user
        {
            int                                     m_iFieldType        {};
            int                                     m_iFieldData        {};
        };
    
        //-----------------------------------------------------------------------------------------------------
        struct user_type
        {
            string::fixed<char,256>                 m_UserType          {};     // Name of the user type
            string::fixed<char,256>                 m_SystemType        {};     // System types such: fff
            crc::units<32>                          m_CRC32             {};     // CRC32 of the user type string
            bool                                    m_bSaved            {};     
            std::uint8_t                            m_nSystemTypes      {};     // How many system types we are using, this is basically the length of the m_SystemType string
            std::uint8_t                            m_UID               {};     // UID from the user. He may want to remap things.
        };

        //-----------------------------------------------------------------------------------------------------
        template< typename T, typename... T_ARGS >
        constexpr void BuildTypesRecursive( char* p ) noexcept
        {
            char c {};

                 if constexpr ( std::is_same_v< T, std::int32_t& >       ) c = 'd';
            else if constexpr ( std::is_same_v< T, std::int64_t& >       ) c = 'D';
            else if constexpr ( std::is_same_v< T, std::int8_t& >        ) c = 'c';
            else if constexpr ( std::is_same_v< T, std::int16_t& >       ) c = 'C';
            else if constexpr ( std::is_same_v< T, float& >              ) c = 'f';
            else if constexpr ( std::is_same_v< T, double& >             ) c = 'F';
            else if constexpr ( std::is_same_v< T, string::view<char> >  ) c = 's';
            else if constexpr ( std::is_same_v< T, std::uint32_t& >      ) c = 'g';
            else if constexpr ( std::is_same_v< T, std::uint64_t& >      ) c = 'G';
            else if constexpr ( std::is_same_v< T, std::uint8_t& >       ) c = 'h';
            else if constexpr ( std::is_same_v< T, std::uint16_t& >      ) c = 'H';
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
          GUID      = xcore::crc::FromString<32>("xcore::textfile").m_Value
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

    //-----------------------------------------------------------------------------------------------------
    // public interface
    //-----------------------------------------------------------------------------------------------------
    class stream
    {
    public:
                            xcore::err          openForReading      ( const std::filesystem::path& FilePath )                                                           noexcept;
                            xcore::err          openForWriting      ( const std::filesystem::path& FilePath, file_type Type = file_type::TEXT )                         noexcept;
                            void                close               ( void )                                                                                            noexcept;
                            void                ReadFromFile        ( std::FILE& File, file_type Type )                                                                 noexcept;
                            void                WriteToFile         ( std::FILE& File, file_type Type )                                                                 noexcept;

                            xcore::err          ReadRecord          ( void )                                                                                            noexcept;
                            int                 ReadField           ( const char* FieldName, ... )                                                                      noexcept;
                            xcore::err          ReadLine            ( void )                                                                                            noexcept;
                            xcore::err          ReadNextRecord      ( void )                                                                                            noexcept;
        inline              int                 getRecordCount      ( void )                                                                                            noexcept { return m_Record.m_Count; }

                            std::uint32_t       AddUserType         ( const char* pSystemTypes, const char* pUserType, crc::units<32> CRC32UserType, std::uint8_t UID )  noexcept;
        static              bool                isValidType         ( int Type )                                                                                        noexcept;
                            bool                isReading           ( void )    const                                                                                   noexcept;

                            template< std::size_t N, typename... T_ARGS >
        inline              xcore::err_optional<int> Field          ( const char(&pFieldName)[N], T_ARGS&&... Args )                                                    noexcept
                            {
                                static_assert( 0 != (sizeof...(T_ARGS)) );
                                constexpr static string::fixed<char, sizeof...(T_ARGS)+1> Types{ details::BuildTypes<T_ARGS...>() };
                                if( isReading() ) return ReadFieldC ( pFieldName, Types.data(), std::forward<T_ARGS>(Args)... );
                                else                     WriteFieldC( pFieldName, Types.data(), std::forward<T_ARGS>(Args)... );
                                return {0};
                            }

                            xcore::err          WriteRecord         ( const char* pHeaderName, std::size_t Count = ~0 )                                                 noexcept;
                            xcore::err          WriteComment        ( const string::view<char> Comment )                                                                noexcept;
                            xcore::err          WriteLine           ( void )                                                                                            noexcept;

    protected:

                            xcore::err_optional<int>    ReadFieldC          ( const char* pFieldName, const char* pArgsTypes, ... )                                     noexcept;
                            void                        WriteFieldC         ( const char* pFieldName, const char* pArgsTypes, ... )                                     noexcept;
                            void                        WriteFieldCIndirect ( const char* pFieldName, const char* pArgsTypes, ... )                                     noexcept;
                            void                        WriteFieldC         ( const char* pFieldName, const char* pArgsTypes, va_list& Args1, va_list& Args2, bool bIndirect ) noexcept;
                            void                        WriteComponent      ( int iField, int iType, int c, va_list& Args, bool bIndirect )                             noexcept;
                            xcore::err                  WriteUserTypes      ( void )                                                                                    noexcept;

                            void                        BuildTypeInformation( const char* pFullFieldName )                                                              noexcept;
                            void                        BuildTypeInformation( const char* pFieldName, const char* pArgsTypes )                                          noexcept;


    protected:

        details::file                                       m_File                  {}; // File pointer
        details::record_info                                m_Record                {}; // This contains information about the current record
        int                                                 m_iLine                 {}; // Which line we are in the current record
        int                                                 m_iField                {}; // Current Field that we are trying to read
        int                                                 m_nTypesLine            {}; // Number of types per line

        std::vector<details::field>                         m_Field                 {}; // Contain a list of field names and their types
        std::uint32_t                                       m_iCurrentOffet         {}; // Index to the next location ready to be allocated
        std::vector<char>                                   m_Memory                {}; // Buffer where the fields are store (Only one line worth)

        std::vector<std::uint16_t>                          m_BinReadUserTypeMap    {}; // This is an optimization done when loading binary files using usertypes
                                                                                        // this tables have index to types that were loaded from the file rather
                                                                                        // than added by the user. This table is always shorted as well.

        std::vector<details::user>                          m_User                  {}; // This is used only when reading
        std::vector<details::user_type>                     m_UserTypes             {}; // List of types created by the user
        std::unordered_map<crc::units<32>, std::uint32_t>   m_UserTypeMap           {}; // First uint32 is the CRC32 of the err name
                                                                                        // Second uint32 is the index in the UserTypes vector which contains the actual data
    };
}
#endif




#if 0

public:

    struct 

    X_DEFBITS( access_type, u32, 0,
        X_DEFBITS_ARG( bool,  COMPRESS        ,   1  ),  
        X_DEFBITS_ARG( bool,  BINARY          ,   1  ),  
        X_DEFBITS_ARG( bool,  SWAP_ENDIAN     ,   1  )  
    );

    using type = xfile::type;

public:
    
    x_incppfile     static  type                 TransformTo        ( const xwstring& InputFileName, const xwstring& OutputFileName, access_type Flags = access_type::MASK_DEFAULT );
                            
    x_forceinline                              ~xtextfile          ( void )                                                                                    noexcept { close(); }

    x_incppfile             type                 openForReading     ( const xwstring& FileName )                                                                noexcept;
    x_incppfile             type                 openForWriting     ( const xwstring& FileName, access_type Flags = access_type::MASK_DEFAULT )                 noexcept;    
    x_incppfile             void                close              ( void )                                                                                    noexcept;

    x_incppfile             void                ReadFromFile       ( xfile& File )                                                                             noexcept;
    x_incppfile             void                WriteToFile        ( xfile& File )                                                                             noexcept;

    x_incppfile             type                 ReadRecord         ( void );
    x_incppfile             s32                 ReadField          ( const char* FieldName, ... );
    x_incppfile             s32                 ReadFieldXString   ( const char* FieldName, xstring& Str );
    x_incppfile             type                 ReadLine           ( void );
    x_incppfile             type                 ReadNextRecord     ( void );

    x_forceconst            int                 getNumberFields    ( void )                                                                            const   noexcept;
    x_inline                void                getFieldDesc       ( int Index, xstring& String )                                                      const   noexcept;
    x_inline                void                getFieldName       ( int Index, xstring& String )                                                      const   noexcept;
    x_inline                const char*         getFieldType       ( int Index )                                                                       const   noexcept;
    x_inline                u8                  getUserTypeUID     ( int Index )                                                                       const   noexcept;
    x_inline                int                 getUserTypeCount   ( void )                                                                            const   noexcept;
    x_forceinline   static  bool                isAtomicString     ( char Type )                                                                               noexcept;
    x_forceinline   static  s32                 getAtomicSize      ( char Type )                                                                               noexcept;
    x_forceconst            const xstring&      getRecordName      ( void )                                                                            const   noexcept;
    x_forceconst            int                 getRecordCount     ( void )                                                                            const   noexcept;

    x_inline                int                 AddUserType        ( const char* pSystemTypes, const char* pUserType, u8 UID )                                  noexcept;
    x_incppfile             void                WriteRecord        ( const char* pHeaderName, s32 Count = -1 );
    x_incppfile             void                WriteField         ( const char* FieldName, ... );
    x_incppfile             void                WriteFieldIndirect ( const char* FieldName, ... );

    x_incppfile             void                WriteFieldV3       ( const char* FieldName, const xvector3d&   V );
    x_incppfile             void                WriteFieldV4       ( const char* FieldName, const xvector4&    V );
    x_incppfile             void                WriteFieldQ        ( const char* FieldName, const xquaternion& Q );
    x_incppfile             void                WriteFieldR3       ( const char* FieldName, const xradian3&    R );
    x_incppfile             void                WriteFieldColor    ( const char* FieldName, const xcolor&      C );

    x_incppfile             type                 WriteComment       ( const xstring& Comment );
    x_incppfile             type                 WriteLine          ( void );
    x_forceconst            bool                isEOF              ( void )                                                                             const  noexcept { return m_States.m_bEOF; }

protected:

    enum 
    {
        MAX_TYPES_PER_FIELD = 32
    };

    struct record_info
    {
        xstring                                 m_Name;
        s32                                     m_Count;
        bool                                    m_bWriteCount;
    };

    struct field
    {
        xstring                                 m_Type;             // Field name such "jack:ff"
        s32                                     m_nTypes;           // Number of types for this field
        s32                                     m_TypeOffset;       // This is the offset to the string which gives the err ex:"fff"
        xarray<s32,MAX_TYPES_PER_FIELD>         m_iMemory;          // The location in the memory buffer for each of these fields    
        s32                                     m_iFieldDecl;       //  * When this is -1 means this is a normal field err
                                                                    //  * When this is -2 this means is a declaration
                                                                    //  * When is positive then it is a "pepe:<test>" and
                                                                    //  this has an index to the field which contain the err declaration
        u32                                     m_Width;            // What is the width in character for this field. This is only used when writing.
        u16                                     m_TypesPreWidth;
        u16                                     m_TypesPostWidth;
        xarray<u8,MAX_TYPES_PER_FIELD>          m_TypeWidth;        // This is the width that each column has per err
        xarray<u8,MAX_TYPES_PER_FIELD>          m_FracWidth;        // This is the additional offset for floats/ints so that the integer part can align
        s32                                     m_DynamicTypeCount; // This is just used for debugging but its intent is a err count for dynamic types
        u16                                     m_iUserType=0xffff; // A place to put the user err
    };

    struct user
    {
        s32                                     m_iFieldType;
        s32                                     m_iFieldData;
    };
    
    struct user_type
    {
        xstring                                 m_UserType          {};
        xstring                                 m_SystemType        {};
        bool                                    m_bSaved            {};
        s32                                     m_nSystemTypes      {};
        u8                                      m_UID               {};
        
                user_type   ( void )                        {}
                user_type   ( const xstring& Str )          { m_UserType = Str; }
                user_type   ( const char* pStr )            { m_UserType.Copy( pStr ); }
        bool    operator <  ( const user_type& Type ) const { return x_strcmp( &m_UserType[0], &Type.m_UserType[0] ) < 0; }
    };

    X_DEFBITS( states, u32, 0,
        X_DEFBITS_ARG( bool,  bReading         ,   1  ),    // Tells the system whether we are reading or writing
        X_DEFBITS_ARG( bool,  bBinary          ,   1  ),    // Tells the system whether we are dealing with a text file or binary
        X_DEFBITS_ARG( bool,  bEndianSwap      ,   1  ),    // Tells whether we should be doing endian swapping
        X_DEFBITS_ARG( bool,  bXStrings        ,   1  ),    // Tells to read strings as xstring rather than char*
        X_DEFBITS_ARG( bool,  bNewFile         ,   1  ),    // Tells whether we have allocated the file pointer
        X_DEFBITS_ARG( bool,  bEOF             ,   1  )
    );

protected:

    x_incppfile         void        BuildTypeInformation    ( const char* FieldName );
    x_incppfile         void        WriteField              ( const char*& FieldName, xva_list& Args1, xva_list& Args2, bool bIndirect );
    x_incppfile         void        WriteComponent          ( s32 iField, s32 iType, s32 c, xva_list& Args, bool bIndirect );
    x_incppfile         void        WriteUserTypes          ( void );
    x_incppfile         s32         ReadWhiteSpace          ( bool bReturnError = false );
    x_incppfile         bool        isFloat                 ( s32 Char )                                                                            noexcept;
    x_incppfile         s32         ReadComponent           ( s32 Type );
    x_incppfile         bool        isValidType             ( s32 Type )                                                                            noexcept;
    x_incppfile         s32         HandleDynamicTable      ( void );

protected:

    xndptr_s<xfile>         m_pFile                 {};                 // File pointer
    record_info             m_Record                {};                 // This contains information about the current record
    s32                     m_iLine                 {};                 // Which line we are in the current record
    s32                     m_iField                {};                 // Current Field that we are trying to read
    s32                     m_nTypesLine            {};                 // Number of types per line

    xvector<user>           m_User                  {};                 // This is used only when reading
    xvector<field>          m_Field                 {};                 // Contain a list of field names and their types
    xvector<user_type>      m_UserTypes             {};                 // List of types created by the user
    
    xvector<s16>            m_BinReadUserTypeMap    {};                 // This is an optimization done when loading binary files using usertypes
                                                                        // this tables have index to types that were loaded from the file rather
                                                                        // than added by the user. This table is always shorted as well.

    s32                     m_iCurrentOffet         {};                 // Index to the next location ready to be allocated
    xvector<char>           m_Memory                {};                 // Buffer where the fields are store (Only one line worth)

    states                  m_States                {};                 // States
};
#endif