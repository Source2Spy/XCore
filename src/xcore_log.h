#ifndef _XCORE_LOG_H
#define _XCORE_LOG_H
#pragma once

namespace xcore::log
{
    class channel;

    //----------------------------------------------------------------------------------------------
    // Types of messages that we can emit
    //----------------------------------------------------------------------------------------------
    enum class msg_type
    {
          L_INFO
        , L_WARNING
        , L_ERROR
    };

    //----------------------------------------------------------------------------------------------
    // Nice generic function to emit messages
    //----------------------------------------------------------------------------------------------
    void Output( const char* pMsg, ... ) noexcept;

    //----------------------------------------------------------------------------------------------
    // All log messages are emitted by the logger. Locker has a lock to handle one output at a time.
    //----------------------------------------------------------------------------------------------
    struct logger
    {
        using fn = void( const xcore::log::channel& Channel, xcore::log::msg_type Type, const char* String, int Line, const char* file ) noexcept;

                                    logger( void ) noexcept;

        fn*                         m_pOutput  { nullptr };
        mutable xcore::lock::spin   m_Lock     {};
    };

    //----------------------------------------------------------------------------------------------
    // Channels allows to manage logs in a cleaner way because it forces a name and allows to be turn off/on
    //----------------------------------------------------------------------------------------------
    class channel
    {
    public:

                                    channel         ( void )                                                        noexcept = delete;
        xforceinline                channel         ( std::string&& Name
                                                    , logger&       Logger = xcore::get().m_MainLogger )            noexcept : m_Logger{Logger}, m_Name{std::move(Name)} {}
        inline      void            TurnOn          ( void )                                                        noexcept { m_bPrint = true;     }
        inline      void            TurnOff         ( void )                                                        noexcept { m_bPrint = false;    }
        constexpr   bool            isPrint         ( void )                                                const   noexcept { return m_bPrint;     }
        constexpr   auto&           getName         ( void )                                                const   noexcept { return m_Name;       }
        constexpr   auto&           getLogger       ( void )                                                const   noexcept { return m_Logger;     }

    protected:

        logger&                 m_Logger;
        std::string             m_Name      {};
        bool                    m_bPrint    {true};
    };

    namespace details
    {
        constexpr static std::size_t log_buffer_size_v = 512;
    }
}

#ifndef _XCORE_COMPILER_VISUAL_STUDIO
    #define sprintf_s( A, B, ... ) assert_verify( B == sprintf( Buff, __VA_ARGS__ ) );
#endif

//----------------------------------------------------------------------------------------------
// Macros to be user_types for logging
//----------------------------------------------------------------------------------------------
#define XLOG_CHANNEL_ERROR( CHANNEL, FMTSTR, ... )                                                  \
{ auto& __Channel = CHANNEL;                                                                        \
    if( __Channel.isPrint() ) {                                                                     \
        std::array<char,xcore::log::details::log_buffer_size_v> __LogChannelTempBuff;               \
        sprintf_s( __LogChannelTempBuff.data(), __LogChannelTempBuff.size(), FMTSTR, __VA_ARGS__ ); \
        auto& __Logger = __Channel.getLogger();                                                     \
        xcore::lock::scope<xcore::lock::spin> __Lock( __Logger.m_Lock );                            \
        __Logger.m_pOutput( __Channel, xcore::log::msg_type::L_ERROR, __LogChannelTempBuff.data(), __LINE__, __FILE__ ); \
    }                                                                                               \
}

#define XLOG_CHANNEL_WARNING( CHANNEL, FMTSTR, ... )                                                \
{ auto& __Channel = CHANNEL;                                                                        \
    if( __Channel.isPrint() ) {                                                                     \
        std::array<char,xcore::log::details::log_buffer_size_v> __LogChannelTempBuff;               \
        sprintf_s( __LogChannelTempBuff.data(), __LogChannelTempBuff.size(), FMTSTR, __VA_ARGS__ ); \
        auto& __Logger = __Channel.getLogger();                                                     \
        xcore::lock::scope __Lock( __Logger.m_Lock );                                               \
        __Logger.m_pOutput( __Channel, xcore::log::msg_type::L_WARNING, __LogChannelTempBuff.data(), __LINE__, __FILE__ ); \
    }                                                                                               \
}

#define XLOG_CHANNEL_INFO( CHANNEL, FMTSTR, ... )                                                   \
{ auto& __Channel = CHANNEL;                                                                        \
    if( __Channel.isPrint() ) {                                                                     \
        std::array<char,xcore::log::details::log_buffer_size_v> __LogChannelTempBuff;               \
        sprintf_s( __LogChannelTempBuff.data(), __LogChannelTempBuff.size(), FMTSTR, __VA_ARGS__ ); \
        auto& __Logger = __Channel.getLogger();                                                     \
        xcore::lock::scope __Lock( __Logger.m_Lock );                                               \
        __Logger.m_pOutput( __Channel, xcore::log::msg_type::L_INFO, __LogChannelTempBuff.data(), __LINE__, __FILE__ ); \
    }                                                                                               \
}

#endif