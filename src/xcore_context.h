#ifndef _XCORE_CONTEXT_H
#define _XCORE_CONTEXT_H
#pragma once

namespace xcore::context
{
    struct base;

    //----------------------------------------------------------------------------------------------
    // Get the global context;
    //----------------------------------------------------------------------------------------------
    constexpr auto& get( void ) noexcept { return xcore::get().getLocalStorage<base*>(); }

    //----------------------------------------------------------------------------------------------
    // base
    //----------------------------------------------------------------------------------------------
    struct base
    {
        using data = std::variant< const char*, std::string >;

        constexpr           base                ( std::string&& Name )              noexcept : m_Name{ std::move(Name) }{ auto& p = context::get(); m_pNext = p; p = this; }
        constexpr           base                ( const char* pName )               noexcept : m_Name{ pName }          { auto& p = context::get(); m_pNext = p; p = this; }
        constexpr   auto    getName             ( void )                    const   noexcept { return m_Name.index() ? std::get<1>(m_Name).c_str() : std::get<0>(m_Name); }
        inline      auto    getFullName         ( void )                    const   noexcept { std::string Str; Str.append( getName() ); for( auto p = m_pNext; p; p = p->m_pNext ) { Str.append("/"); Str.append( p->getName() ); } return Str; }

        data        m_Name;
        base*       m_pNext{};
    };

    //----------------------------------------------------------------------------------------------
    // scope
    //----------------------------------------------------------------------------------------------
    struct scope : base
    {
        using base::base;    
                            ~scope                ( void )                            noexcept { context::get() = m_pNext; }
    };
}

#endif