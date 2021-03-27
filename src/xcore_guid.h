#ifndef _XCORE_GUID_H
#define _XCORE_GUID_H
#pragma once

//---------------------------------------------------------------------------------------
// Useful references:
//      * Time based GUID:   https://github.com/mariusbancila/stduuid
//      * meow hash project: https://github.com/RedSpah/meow_hash_cpp 
//      * Very nice article: https://preshing.com/20110504/hash-collision-probabilities/ 
//---------------------------------------------------------------------------------------
namespace xcore::guid
{
    namespace details
    {
        //---------------------------------------------------------------------------------------
        // Using the probability space to generate a unique id
        //---------------------------------------------------------------------------------------
        struct ui
        {
            std::uint64_t   m_Time;
            std::uint64_t   m_UID;
            std::uint64_t   m_ProcessID;
            ui*             m_pPtr;
        };
    
        //---------------------------------------------------------------------------------------
        template< std::size_t T_BITS_SIZE >
        auto Generate( void ) noexcept
        {
            using namespace std;
            using namespace std::chrono;
            static std::atomic<std::uint64_t> C{0x423849};
            ui UI;
            
            UI.m_Time        = system_clock::to_time_t(system_clock::now());
            UI.m_UID         = std::hash<std::thread::id>()(std::this_thread::get_id());
            UI.m_ProcessID   = C++ ^ xcore::global::state::getProcessID();
            UI.m_pPtr        = &UI;
            return meowh::meow_hash<(T_BITS_SIZE < 128) ? 128 : T_BITS_SIZE>( &UI, sizeof(UI) ).as<T_BITS_SIZE>(0);
        }
    }

    //---------------------------------------------------------------------------------------
    // Standard GUID
    //---------------------------------------------------------------------------------------
    template< std::size_t T_BITS_SIZE=64, typename T_TAG = void >
    struct unit
    {
        static constexpr auto bytes_size_v  = T_BITS_SIZE/8;
        static constexpr auto bit_size_v    = T_BITS_SIZE;
        using                 t             = xcore::types::byte_size_uint_t<bytes_size_v>;
        using                 h             = xcore::types::byte_size_uint_t<bytes_size_v/2>;

        template< std::size_t N >
        constexpr                           unit                ( const char (&Str)[N] )                                        noexcept : m_Value{ xcore::crc<bit_size_v>::FromString(Str).m_Value }{}
        constexpr                           unit                ( std::nullptr_t )                                              noexcept : m_Value{ 0 }{}
        constexpr explicit                  unit                ( t X )                                                         noexcept : m_Value{ X }{}
        inline                              unit                ( void )                                                        noexcept = default;
        constexpr explicit                  unit                ( h High, h Low )                                               noexcept : m_Value{ (static_cast<t>(High)<<(bit_size_v/2)) | Low }{}
        constexpr explicit                  unit                (xcore::not_null_t)                                             noexcept : m_Value{ details::Generate<bit_size_v>() } {}
        constexpr            bool           operator ==         ( const unit A )                                        const   noexcept { return m_Value == A.m_Value; }
        constexpr            bool           operator !=         ( const unit A )                                        const   noexcept { return m_Value != A.m_Value; }
        constexpr            bool           operator <          ( const unit A )                                        const   noexcept { return m_Value < A.m_Value; }
        constexpr            bool           operator >          ( const unit A )                                        const   noexcept { return m_Value > A.m_Value; }
        constexpr            bool           operator <=         ( const unit A )                                        const   noexcept { return m_Value <= A.m_Value; }
        constexpr            bool           operator >=         ( const unit A )                                        const   noexcept { return m_Value >= A.m_Value; }
        constexpr            bool           isValid             ( void )                                                const   noexcept { return !!m_Value;  }
        constexpr            bool           isNull              ( void )                                                const   noexcept { return 0 == m_Value;  }
        inline               void           setNull             ( void )                                                        noexcept { m_Value = 0; }
        inline               void           Reset               ( void )                                                        noexcept { m_Value = details::Generate<bit_size_v>(); }
        template< typename T_CHAR >
        inline              void            setFromStringHex    ( const T_CHAR* pStr )                                          noexcept { m_Value = string::ToGuid( pStr ); }
        template< typename T_CHAR >
        inline              void            getStringHex        ( string::view<T_CHAR> String )                         const   noexcept { string::sprintf( String, "%llX", static_cast<std::uint64_t>(m_Value) ); }
        template< typename T_CHAR >
        inline         string::ref<T_CHAR>  getStringHex        ( void )                                                const   noexcept { string::ref<T_CHAR> String{string::units<char>{128}}; getStringHex(String.getView()); return std::move(String); }

        t m_Value;
    };

    template< typename T_TAG >
    struct unit<128,T_TAG>
    {
        constexpr                           unit                ( void )                                                        noexcept = default;
        template< std::size_t N >
        constexpr                           unit                ( const char (&Str)[N] )                                        noexcept : m_Value{ xcore::crc<64>::FromString(Str).m_Value, xcore::crc<64>::FromString(Str+1).m_Value }{}
        constexpr                           unit                ( std::nullptr_t )                                              noexcept : m_Value{ 0u, 0u }{}
        constexpr explicit                  unit                ( std::uint64_t High, std::uint64_t Low )                       noexcept : m_Value{ High, Low }{}
        constexpr explicit                  unit                ( xcore::not_null_t )                                           noexcept : m_Value{ []{ std::array<std::uint64_t,2> Value; std::memcpy( Value.data(), details::Generate<128>().data(), sizeof(m_Value) ); return Value; }() } {}
        constexpr            bool           operator ==         ( const unit A )                                        const   noexcept { return m_Value[0] == A.m_Value[0] && m_Value[1] == A.m_Value[1]; }
        constexpr            bool           operator !=         ( const unit A )                                        const   noexcept { return m_Value[0] != A.m_Value[0] || m_Value[1] != A.m_Value[1]; }
        constexpr            bool           operator <          ( const unit A )                                        const   noexcept { return m_Value[0] == A.m_Value[0] ? m_Value[1] <  A.m_Value[1] : m_Value[0] < A.m_Value[0]; }
        constexpr            bool           operator >          ( const unit A )                                        const   noexcept { return m_Value[0] == A.m_Value[0] ? m_Value[1] >  A.m_Value[1] : m_Value[0] > A.m_Value[0]; }
        constexpr            bool           operator <=         ( const unit A )                                        const   noexcept { return m_Value[0] == A.m_Value[0] ? m_Value[1] <= A.m_Value[1] : m_Value[0] < A.m_Value[0]; }
        constexpr            bool           operator >=         ( const unit A )                                        const   noexcept { return m_Value[0] == A.m_Value[0] ? m_Value[1] >= A.m_Value[1] : m_Value[0] > A.m_Value[0]; }
        constexpr            bool           isValid             ( void )                                                const   noexcept { return !! ( m_Value[0] | m_Value[1] );  }
        constexpr            bool           isNull              ( void )                                                const   noexcept { return 0 == ( m_Value[0] | m_Value[1] );  }
        inline               void           setNull             ( void )                                                        noexcept { m_Value[0] = m_Value[1] = 0; }
        inline               void           Reset               ( void )                                                        noexcept { std::memcpy( m_Value.data(), details::Generate<128>().m128i_u64, sizeof(m_Value) ); }
        template< typename T_CHAR >
        inline              void            setFromStringHex    ( const T_CHAR* pStr )                                          noexcept { m_Value[0] = string::ToGuid( pStr ); m_Value[1] = string::ToGuid( pStr+1 ); }
        template< typename T_CHAR >
        inline              void            getStringHex        ( string::view<T_CHAR> String )                         const   noexcept { string::sprintf( String, "%llX,llX", m_Value[0],m_Value[1] ); }
        template< typename T_CHAR >
        inline         string::ref<T_CHAR>  getStringHex        ( void )                                                const   noexcept { string::ref<T_CHAR> String{string::units<char>{128}}; getStringHex(String.getView()); return std::move(String); }

        std::array<std::uint64_t,2> m_Value;
    };


    //------------------------------------------------------------------------------
    // Description:
    //          group like guids are intended to create full 64 guids base on two ids
    //              1. group    - this is a general category
    //              2. subgroup - specific classification of the guid
    // Example: 
    //    class component 
    //    { 
    //        constexpr static guid::group<component>        group_v{ "Component" };
    //        class type
    //        {
    //            constexpr static guid::subgroup            group_v{ component::group_v, "Type" };
    //            using                                      guid   = guid::subgroup<component>::unit;
    //        };
    //    };
    //------------------------------------------------------------------------------
    template< typename T_TAG, std::size_t T_BITS_SIZE_V = 32 >
    struct group
    {
        static constexpr auto bits_size_v =  T_BITS_SIZE_V;
        using type = types::byte_size_uint_t<bits_size_v/8>;
        using tag  = T_TAG;

        type                            m_Value;
        const string::constant<char>    m_String;

        template< std::size_t N >
        constexpr                               group           ( const char (&Str)[N] )                            noexcept : m_Value{ crc::FromString<bits_size_v>(Str).m_Value }, m_String{Str} {}
        constexpr               bool            operator ==     ( const group X )                           const   noexcept { return m_Value == X.m_Value; }
        constexpr               bool            operator !=     ( const group X )                           const   noexcept { return m_Value != X.m_Value; }
        constexpr               bool            operator <      ( const group X )                           const   noexcept { return m_Value <  X.m_Value; }
        constexpr               bool            isValid         ( void )                                    const   noexcept { return !!m_Value;            }
    };

    //------------------------------------------------------------------------------
    template< typename T_GROUP >
    struct subgroup : unit<T_GROUP::bits_size_v*2, T_GROUP>
    {
        using group  = T_GROUP;
        using parent = guid::unit<T_GROUP::bits_size_v*2,T_GROUP>;
        struct units : xcore::units::type< units, typename T_GROUP::type >
        {
            using parent = xcore::units::type< units, typename T_GROUP::type >;
            using parent::type;
        };

        template< std::size_t N >
        constexpr                               subgroup        ( T_GROUP Group, const char (&Str)[N] )             noexcept : parent{crc::FromString<T_GROUP::bits_size_v>(Str).m_Value, Group.m_Value} {}
        constexpr       units                   getSubgroup     ( void )                                    const   noexcept { return units  { static_cast<typename T_GROUP::type>(parent::m_Value>>T_GROUP::bits_size_v) }; }
        constexpr       T_GROUP                 getGroup        ( void )                                    const   noexcept { return T_GROUP{ static_cast<typename T_GROUP::type>(parent::m_Value) }; }
    };

    //------------------------------------------------------------------------------
    // Description:
    //      Resource Type Guid has a 32bit unique identifies xplugin_guid and another 32bits of subgroup.
    //      xplugin_guid    - Is the plugin guid where the type is base of. 
    //      subgroup        - This is a unique identifier that the user_types of the plugin can give in order to
    //                        identify which sub-type this resource type is talking about.
    // Example:
    //      constexpr static rctype<>   rctype_v( plugin<>( "Texture" ), "Normal Map");
    //------------------------------------------------------------------------------
    template< std::size_t T_BITS_SIZE_V = 32 >
    using plugin = group< struct resource_plugin_tag, T_BITS_SIZE_V  >;

    template< std::size_t T_BITS_SIZE_V = 64 >
    using rctype = subgroup<plugin<T_BITS_SIZE_V/2>>;

    //------------------------------------------------------------------------------
    // Description:
    //     The RSC GUID consist in a U64 Unique Identifier where is broken up as follows.
    //     0xGGGGGGGGGGGGGGG1
    //       G = 64 - 1 Bits worth of Evenly distributed global bits
    //       1 = The very first bit is a flag that signifies that it is a resource GUID.
    //           This flag later own is used to differentiate between a pointer which the
    //           fist bit is always 0 vs a GUID which is always 1.
    //------------------------------------------------------------------------------
    template< std::size_t T_BITS_SIZE_V = 64 >
    struct rcinstance : unit<T_BITS_SIZE_V, struct resource_instance_tag >
    {
        using parent = unit<T_BITS_SIZE_V, struct resource_instance_tag >;
        using t = typename parent::t;
        using h = typename parent::h;

        constexpr                           rcinstance          ( void )                                    noexcept = default;
        constexpr explicit                  rcinstance          ( t X )                                     noexcept : parent{ X }{ xassert(X&1); }
        constexpr explicit                  rcinstance          ( h High, h Low )                           noexcept : parent{ (static_cast<t>(High)<<(parent::bit_size_v/2)) | Low }{xassert(Low&1); }
        inline               void           Reset               ( void )                                    noexcept { parent::m_Value = details::Generate<parent::bit_size_v>() | 1; }
    };

    //------------------------------------------------------------------------------
    // Description:
    //      Generic plugin GUID
    // Example:
    //      Usefully container to keep the full 128bits of the GUID. Which fully 
    //      describes all the details in a generic way.
    //------------------------------------------------------------------------------
    template< std::size_t T_TYPE_BITS_SIZE_V = 64, std::size_t T_INSTANCE_BITS_SIZE_V = 64 >
    struct rcfull
    {
        rctype<T_TYPE_BITS_SIZE_V>          m_Type;
        rcinstance<T_INSTANCE_BITS_SIZE_V>  m_Instance;
    };
}

//---------------------------------------------------------------------------------------
// Keep it compatible with the std::lib
//---------------------------------------------------------------------------------------
template< typename T_TAG > 
struct std::hash< typename xcore::guid::subgroup<T_TAG> > 
{ 
    auto operator()(const typename xcore::guid::subgroup<T_TAG> obj) const { return hash<std::uint64_t>()(obj.m_Value); } 
};

template< std::size_t T_SIZE, typename T_TAG >
struct std::hash< typename xcore::guid::unit<T_SIZE, T_TAG> >
{
    auto operator()(const typename xcore::guid::unit<T_SIZE, T_TAG> obj) const { return hash<std::uint64_t>()(obj.m_Value); }
};

#endif