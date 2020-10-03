#ifndef _XCORE_GLOBAL_H
#define _XCORE_GLOBAL_H
#pragma once

namespace xcore::scheduler { class system; } 

//------------------------------------------------------------------------------------------
// Global state of xcore
//------------------------------------------------------------------------------------------
namespace xcore::global
{
    //------------------------------------------------------------------------------------------
    // Class used to create local storage per thread
    //------------------------------------------------------------------------------------------
    struct local_storage
    {
                                                local_storage   ( void )             noexcept;
                                               ~local_storage   ( void )             noexcept;
                void                            setRaw          ( void* pPtr )       noexcept;
                void*                           getRaw          ( void )             noexcept;

        std::uint32_t   m_dwThreadIndex {};
    };

    //------------------------------------------------------------------------------------------
    // Details
    //------------------------------------------------------------------------------------------
    namespace details
    {
        using local_storage_ptrs = std::variant< context::base*, scheduler::local_storage_data >;
        using local_storage_data = std::array< local_storage_ptrs, std::variant_size_v<local_storage_ptrs> >;

        template< typename T >
        struct local_storage_creator;

        template< typename... T_ARGS >
        struct local_storage_creator< std::variant< T_ARGS...> >
        {
            static auto Make( void ) noexcept { return new std::array< local_storage_ptrs, std::variant_size_v<local_storage_ptrs> > { T_ARGS{nullptr}... }; }
        };
    }

    //------------------------------------------------------------------------------------------
    // Global state for xcore
    //------------------------------------------------------------------------------------------
    struct state final
    {
                                        state           ( const char* pAppName )            noexcept;
                                       ~state           ( void )                            noexcept;
        static  std::uint64_t           getProcessID    ( void )                            noexcept;

        template< typename T > constexpr
        T&                             getLocalStorage ( void )    noexcept 
        { 
            auto p = reinterpret_cast<details::local_storage_data*>( m_LocalStorage.getRaw() );
            if( p == nullptr ) 
            {
                p = details::local_storage_creator<details::local_storage_ptrs>::Make();
                m_LocalStorage.setRaw( p );
                xcore::lock::scope Clean(m_LocalCleanup);
                m_LocalCleanup.get().emplace_back(p);
            }
            static_assert( xcore::types::template variant_t2i_v<T,details::local_storage_ptrs> < std::variant_size_v<details::local_storage_ptrs>, "" );
            return std::get<T>(p->at(xcore::types::template variant_t2i_v<T,details::local_storage_ptrs>));
        }

        using state_ptr_type = std::variant< state*, std::unique_ptr<state> >;
        using local_cleanup  = xcore::lock::object<std::vector<std::unique_ptr<details::local_storage_data>>,        xcore::lock::spin >;
        using system_reg     = xcore::lock::object< std::unordered_map<std::uint64_t, xcore::system::registration*>, xcore::lock::spin >;
        
        system_reg                                          m_SystemReg         {};
        local_cleanup                                       m_LocalCleanup      {};
        local_storage                                       m_LocalStorage      {};
        log::logger&                                        m_MainLogger;
        scheduler::system&                                  m_Scheduler;
        std::vector<std::unique_ptr<xcore::file::device_i>> m_lFileDevice       {};

        const char* const               m_pAppName          { nullptr };
        inline static state_ptr_type    ms_Instance         { nullptr };
    };
}

//------------------------------------------------------------------------------------------
// Main functions to initialize xcore
//------------------------------------------------------------------------------------------
namespace xcore
{
                void            Init            ( global::state& G )                        noexcept;
                void            Init            ( const char* pAppName, int nWorkers = -1 ) noexcept;
                void            Kill            ( void )                                    noexcept;
    constexpr   global::state&  get             ( void )                                    noexcept { return (global::state::ms_Instance.index()) ? *std::get<1>(global::state::ms_Instance) : *std::get<0>(global::state::ms_Instance); }
}

#endif