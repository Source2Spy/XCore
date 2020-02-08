#ifndef _XCORE_ERROR_H
#define _XCORE_ERROR_H
#pragma once

namespace xcore::error
{
    //-------------------------------------------------------------------------------------------------------
    // temporary class that hold a lambda for any clean up code in case of failure
    //-------------------------------------------------------------------------------------------------------
    template< typename T_CLASS, typename T >
    struct scope
    {
        constexpr   scope (T_CLASS& Error, T&& Callback)    noexcept : m_Error{ Error }, m_Callback{ Callback } {}
        inline     ~scope (void)                            noexcept { if ( m_Error ) m_Callback(); }

        T           m_Callback;
        T_CLASS&    m_Error;
    };

    //-------------------------------------------------------------------------------------------------------
    // Contains information about a particular error
    //-------------------------------------------------------------------------------------------------------
    struct code
    {
        //-------------------------------------------------------------------------------------------------------
        template< typename T_STATE >
        constexpr code ( T_STATE State, const char* pMsg, const char* pCpp, long Line ) noexcept 
            : m_RawState            { static_cast<uint32_t>( State ) }
            , m_pString             { pMsg }
            , m_ClassTypeGuid       { static_cast<uint32_t>( T_STATE::GUID ) }
            , m_pCppFile            { pCpp }
            , m_LineNumber          { Line } 
            { 
                xassert( sizeof(T_STATE) <= sizeof(std::uint32_t) ); 

                // When the error state is zero it is assume to be OK!
                static_assert( static_cast<uint32_t>(T_STATE::GUID)      != 0, "You must have a guid for your error type. This guid is used to identify the error type/class" );
                static_assert( static_cast<uint32_t>(T_STATE::OK)        == 0, "Your Enum must have a state call OK with numeric value 0" );
                static_assert( static_cast<uint32_t>(T_STATE::FAILURE )  == 1, "Your Enum must have a state call FAILURE with numeric value 1 - This is used for generic type of failures" );
            }

        //-------------------------------------------------------------------------------------------------------
        template< typename T >
        constexpr bool isSameClass( void ) const noexcept
        {
            return static_cast<std::uint32_t>(T::GUID) == m_ClassTypeGuid;
        }

        //-------------------------------------------------------------------------------------------------------
        template< typename T = xcore::err::states >
        constexpr T getState( void ) const noexcept
        { 
            // Can only return the specific error state if the user_types can provide with the right err
            // Other wise we will default to given the generic code
            return ( isSameClass<T>() ) ? static_cast<T>(m_RawState) : T::FAILURE;
        }

        //-------------------------------------------------------------------------------------------------------
        const char*                     m_pString;              // String for the error
        const char*                     m_pCppFile;             // Which Cpp file did the error came from
        long                            m_LineNumber;           // Which line of the code was the error generated from
        std::uint32_t                   m_ClassTypeGuid;        // GUID for the err of these error codes
        std::uint32_t                   m_RawState;             // State for this error code, raw value provided here, Use the getValue function to give your the right value
    };

    //-------------------------------------------------------------------------------------------------------
    // create new err of errors
    //-------------------------------------------------------------------------------------------------------
    class err [[nodiscard]]
    {
    public:

        //-------------------------------------------------------------------------------------------------------
        // The default error states
        //-------------------------------------------------------------------------------------------------------
        enum class states : std::uint32_t
        {
              GUID = 0xbadbadcd                                                 // GUID for this err err this is better been generated with a constexpr string to CRC32 or something like that
            , OK   = 0                                                          // Default OK required by the system
            , FAILURE                                                           // Default FAILURE required by the system
        };

        //-------------------------------------------------------------------------------------------------------
        // Used to break when an error of this err is generated
        //-------------------------------------------------------------------------------------------------------
        template< typename T_ERROR_ENUM_STATE > inline
        static auto& getBreakOnError( void ) noexcept
        {
            static bool s_bBreakOnError = false;
            return s_bBreakOnError;
        }

        //-------------------------------------------------------------------------------------------------------
        // will break on any error in the system when the error is generated
        //-------------------------------------------------------------------------------------------------------
        inline static bool g_bBreakOnError = false;

    public:

        constexpr                       err                 ( void )                                noexcept = default;
        constexpr                       err                 ( const code& Code )                    noexcept : m_pCode{ &Code } {}
        constexpr                       err                 ( const err&  Error )                   noexcept = delete;
        constexpr                       err                 (       err&& Error )                   noexcept : m_pCode{ Error.m_pCode } {}
        inline                         ~err                 ( void )                                noexcept {}
        inline       err&              operator =           ( err&& Error )                         noexcept { xassert(m_pCode==nullptr); m_pCode = Error.m_pCode; return *this; }
        inline       err&              operator =           ( const code&& Code )                   noexcept { xassert(m_pCode==nullptr); m_pCode = &Code; return *this; }

        constexpr   const auto&         getCode             ( void )                    const       noexcept { xassert(m_pCode); return *m_pCode; }
        inline      const auto&         getCodeAndClear     ( void )                                noexcept { auto& Code = getCode(); clear(); return Code; }
        constexpr                       operator bool       ( void )                    const       noexcept { return m_pCode; }
        inline      bool                isError             ( err& F )                              noexcept { xassert(F.m_pCode==nullptr); F.m_pCode = m_pCode; return  F.m_pCode; }
        inline      bool                isOK                ( err& F )                              noexcept { xassert(F.m_pCode==nullptr); F.m_pCode = m_pCode; return !F.m_pCode; }
        inline      void                clear               ( void )                                noexcept { m_pCode = nullptr; }
        inline      void                CheckError          ( void )                                noexcept { xassert(m_pCode==nullptr); m_pCode = nullptr; }

    protected:

        const code* m_pCode  { nullptr };
    };

    //-------------------------------------------------------------------------------------------------------
    // Optional types of errors
    //-------------------------------------------------------------------------------------------------------
    template< typename T >
    class optional [[nodiscard]] : public err
    {
    public:

        constexpr               optional    ( T Value )                                         noexcept : err{}, m_OptValue{ Value } {}
        constexpr               optional    ( err&& Err )                                       noexcept : err{ std::move(Err) }, m_OptValue{} {}
        constexpr               optional    ( optional&& Op )                                   noexcept : err{ std::move(Op) },  m_OptValue{std::move(Op.m_OptValue)} {}
        constexpr    auto&      getValue    ( void )                                            noexcept { xassert( m_pCode == nullptr ); return m_OptValue; }

    protected:

        T m_OptValue;
    };
}

//-------------------------------------------------------------------------------------------------------
// Handy shortcuts
//-------------------------------------------------------------------------------------------------------
namespace xcore
{
                            using err           = error::err;
    template< typename T >  using err_optional  = error::optional<T>;
}

//-------------------------------------------------------------------------------------------------------
// Macros to detail with error codes and such
//-------------------------------------------------------------------------------------------------------
#define xerr_code_s(CODE,STRING)  []() noexcept -> auto& { using t = decltype(CODE); constexpr static xcore::error::code Code{ CODE, STRING, __FILE__, 0 }; xassert(false == (xcore::err::g_bBreakOnError && xcore::err::getBreakOnError<t>())); return Code; }()
#define xerr_failure_s(STRING) xerr_code_s( xcore::err::states::FAILURE, STRING )

#define xerr_code(ERROR_VAR,CODE,STRING)  std::move(ERROR_VAR = []() noexcept -> auto& { using t = decltype(CODE); constexpr static xcore::error::code Code{ CODE, STRING, __FILE__, 0 }; xassert(false == (xcore::err::g_bBreakOnError && xcore::err::getBreakOnError<t>())); return Code; }())
#define xerr_failure(ERROR_VAR,STRING) xerr_code( ERROR_VAR, xcore::err::states::FAILURE, STRING )

#endif