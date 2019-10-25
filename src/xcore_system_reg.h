#ifndef _XCORE_SYSTEM_REG_H
#define _XCORE_SYSTEM_REG_H
#pragma once

namespace xcore::system
{
    class registration
    {
    public:

        using guid = guid::unit<64,registration*>;

    public:
                        registration ( void ) = delete;
        constexpr       registration ( std::nullptr_t, xcore::property::base& Props, const string::const_universal VirtualName ) noexcept
            : m_Props       { Props }
            , m_Name        { VirtualName } 
            , m_Guid        { xcore::crc<guid::bit_size_v>::FromString( VirtualName.m_Str.m_pValue ).m_Value }
        {}

        inline          registration ( xcore::property::base& Props, const string::const_universal VirtualName ) noexcept
            : m_Props       { Props }
            , m_Name        { VirtualName } 
            , m_Guid        { xcore::crc<guid::bit_size_v>::FromString( VirtualName.m_Str.m_pValue ).m_Value }
        {
            RegisterSystem();
        }

        inline  void    RegisterSystem  ( void ) noexcept
        {
            xassert(m_Guid.isValid());
            auto& R = xcore::get().m_SystemReg;
            xcore::lock::scope Lock( R );
            R.get().insert( {m_Guid.m_Value, this} );
        }

        inline  ~registration ( void ) noexcept
        {
            if(m_Guid.isValid())
            {
                auto& R = xcore::get().m_SystemReg;
                xcore::lock::scope Lock( R );
                R.get().erase( m_Guid.m_Value );
            }
        }

        inline   auto    getName    ( void ) const noexcept { return m_Name; }
        inline   auto    getGuid    ( void ) const noexcept { return m_Guid; }
        inline   auto&   getProps   ( void ) const noexcept { return m_Props; }

    protected:

        const guid                                              m_Guid;
        const string::const_universal                           m_Name;
        xcore::property::base&                                  m_Props; 
    };

    //-------------------------------------------------------------------------------------------
    // Description
    //      Handy global registration system. It is use in order to link/register
    //      anything the user needs in a global way.
    //  Example:
    //      class system_a { public: void register( factory_base& X ); }        // Some system where we want to register stuff
    //      using system_a_reg = x_static_registration<void(*)(system_a&)>;     // Our use of this class
    //
    //      // Now a series of classes could register like this:
    //      class c : public factory_base {...};
    //      static system_a_reg MyRegistrationForC( [](system_a& A){A.Register( *new c ); });
    //      
    //      system_a can get the ( system_a_reg::m_pHead ) and go trough the pHead linklist
    //      and register all its nodes.
    //-------------------------------------------------------------------------------------------
    template< typename T_FUNCTION >
    struct static_registration // : xproperty_v2::base
    {
        using fn = T_FUNCTION;

        constexpr static_registration( string::const_universal Str, fn&& F ) noexcept : m_Function(F), m_Next{ *m_pHead }, m_Name{Str} { m_pHead = this; }

        /*
        virtual const xproperty_v2::table& onPropertyTable( void ) const override
        {
            static const xproperty_v2::table E( this, this, [&](xproperty_v2::table& E)
            {
                E.AddProperty<xstring>
                (
                    X_STR("Name") 
                    , X_STATIC_LAMBDA_BEGIN (x_static_registration& Class, xstring& Data, const xproperty_v2::info::string& Details, bool isSend)
                    {
                        if (isSend) Data = Class.m_Name;
                        else
                        {
                            xassert( false );
                        }
                    }
                    X_STATIC_LAMBDA_END
                    , X_STR("Name of the register object")
                    , xproperty_v2::flags::MASK_DONT_SAVE | xproperty_v2::flags::MASK_READ_ONLY
                );
            });

            return E;
        }
        */

        inline static static_registration*      m_pHead{ nullptr };
        static_registration&                    m_Next;
        const fn                                m_Function;
        string::const_universal                 m_Name;
    };

}
#endif
