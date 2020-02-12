#ifndef _XCORE_SCHEDULER_JOB_H
#define _XCORE_SCHEDULER_JOB_H
#pragma once

namespace xcore::scheduler
{
                                class trigger_base;
    template< int T_MAX_JOBS >  class trigger;

    enum class lifetime : std::uint8_t
    {
            DONT_DELETE_WHEN_DONE
        ,   DELETE_WHEN_DONE
        ,   ENUM_COUNT
    };

    enum class jobtype : std::uint8_t
    {
            NORMAL
        ,   LIGHT
        ,   ENUM_COUNT
    };
    
    enum class affinity : std::uint8_t
    {
            NORMAL
        ,   MAIN_THREAD
        ,   NOT_MAIN_THREAD
        ,   ENUM_COUNT
    };

    enum class priority : std::uint8_t
    {
            BELOW_NORMAL
        ,   NORMAL
        ,   ABOVE_NORMAL
        ,   ENUM_COUNT
    };

    enum class triggers : std::uint8_t
    {
            CLEAN_COUNT
        ,   DONT_CLEAN_COUNT
        ,   ENUM_COUNT
    };
    
    union definition
    {
        std::uint8_t    m_Value {0};
        struct
        {
            lifetime    m_LifeTime : bits::Log2IntRoundUp( static_cast<int>(lifetime::ENUM_COUNT)-1 );
            jobtype     m_JobType  : bits::Log2IntRoundUp( static_cast<int>(jobtype::ENUM_COUNT) -1 );
            affinity    m_Affinity : bits::Log2IntRoundUp( static_cast<int>(affinity::ENUM_COUNT)-1 );
            priority    m_Priority : bits::Log2IntRoundUp( static_cast<int>(priority::ENUM_COUNT)-1 );
            triggers    m_Triggers : bits::Log2IntRoundUp( static_cast<int>(triggers::ENUM_COUNT)-1 );
        };

        constexpr definition( void ) noexcept = default;

        template< typename...T_ARGS  >
        static constexpr definition Flags( T_ARGS&&...Args ) noexcept
        {
            definition D;
            xcore::types::tuple_visit( [&]( auto&& V ) noexcept
            {
                using T = std::decay_t<decltype( V )>;
                     if constexpr ( std::is_same_v<T,xcore::scheduler::lifetime> )    D.m_LifeTime = V;
                else if constexpr ( std::is_same_v<T,xcore::scheduler::jobtype>  )    D.m_JobType  = V;
                else if constexpr ( std::is_same_v<T,xcore::scheduler::affinity> )    D.m_Affinity = V;
                else if constexpr ( std::is_same_v<T,xcore::scheduler::priority> )    D.m_Priority = V;
                else if constexpr ( std::is_same_v<T,xcore::scheduler::triggers> )    D.m_Triggers = V;
                else static_assert( types::always_false_v<T>, "You pass a type that is not recognize!" );
            }, std::tuple{ std::forward<T_ARGS>(Args)... } );
            return D;
        }

        template< typename T >
        constexpr definition operator | ( T X ) const noexcept
        {
            definition D;
            D.m_Value = m_Value | Flags( X ).m_Value;
            return D;
        }
    };
    static_assert( sizeof(definition) == 1 );

    //------------------------------------------------------------------------------
    // Description:
    //      T_BASE class for all the jobs
    //------------------------------------------------------------------------------
    class job_base
    {
    public:

        xcore_rtti_start(job_base)

    public:
    
        constexpr                   job_base                ( void )                                noexcept = default;
        constexpr                   job_base                ( definition Def )                      noexcept : m_Definition{ Def } {}
        virtual                    ~job_base                ( void )                                noexcept = default;
    
        inline      job_base&       setupDefinition         ( definition Def )                      noexcept;
        inline      job_base&       setupJobType            ( jobtype JobType )                     noexcept;
        inline      job_base&       setupLifeTime           ( lifetime LifeTime )                   noexcept;
        inline      job_base&       setupPriority           ( priority Priority )                   noexcept;
        inline      job_base&       setupAffinity           ( affinity Affinity )                   noexcept { m_Definition.m_Affinity = Affinity; return *this; }
        inline      job_base&       setupName               ( const string::const_universal Name )  noexcept { XCORE_CMD_PROFILER( m_JobName = Name ); return *this; }
        constexpr   bool            isDeletedWhenDone       ( void )                        const   noexcept { return m_Definition.m_LifeTime == lifetime::DELETE_WHEN_DONE; }
        constexpr   bool            isLightJob              ( void )                        const   noexcept { return m_Definition.m_JobType  == jobtype::LIGHT; }
        constexpr   priority        getPriority             ( void )                        const   noexcept { return m_Definition.m_Priority; }
        constexpr   affinity        getAffinity             ( void )                        const   noexcept { return m_Definition.m_Affinity; }
        inline      void            NotifyTrigger           ( trigger_base& Trigger )               noexcept;
        virtual     int             getTriggerCount         ( void )                        const   noexcept = 0; 
        XCORE_CMD_PROFILER( constexpr auto&           getName                 ( void )                        const   noexcept { return m_JobName; } )

    protected:

        virtual     void            qt_onRun                ( void )                                noexcept = 0;
        virtual     void            qt_onDone               ( void )                                noexcept { /* x_assert_quantum( m_Debug_LQ ); */ }
        virtual     void            onAppenTrigger          ( trigger_base& Trigger )               noexcept = 0;

    protected:

    #if _X_DEBUG
        x_debug_linear_quantum          m_Debug_LQ          {};
    #endif
        definition                      m_Definition{ definition::Flags( lifetime::DONT_DELETE_WHEN_DONE, jobtype::NORMAL, affinity::NORMAL, priority::NORMAL, triggers::CLEAN_COUNT ) };

        XCORE_CMD_PROFILER( string::const_universal  m_JobName{} );

    protected:

        friend class scheduler::system;
    };

    //------------------------------------------------------------------------------
    // Description:
    //      The standard Job class. A job cares how many triggers it needs to notify
    //------------------------------------------------------------------------------
    template< std::size_t T_NUM_TRIGGERS >
    class job : public job_base
    {
    public:
        xcore_rtti( job, job_base )

        constexpr static int max_triggers_v = T_NUM_TRIGGERS;

    protected:

        constexpr                   job             ( void )                            noexcept = default;
        constexpr                   job             ( definition Def )                  noexcept : job_base( Def ) {}
        virtual     void            qt_onDone       ( void )                            noexcept override;
        virtual     void            onAppenTrigger  ( trigger_base& Trigger )           noexcept override;
        virtual     int             getTriggerCount ( void )                    const   noexcept override { return m_nTriggers; } 
        inline      auto&           getTrigger      ( int Index )                       noexcept { xassert( Index < m_nTriggers); return *m_TriggerList[Index]; }
        inline      const auto&     getTrigger      ( int Index )               const   noexcept { xassert( Index < m_nTriggers); return *m_TriggerList[Index]; }

    protected:

        xcore::array<trigger_base*,T_NUM_TRIGGERS>      m_TriggerList {};
        std::atomic<int>                                m_nTriggers   {0};
    
    protected:
    
        template<int> friend class trigger;
        friend class trigger_base;
    };

    //------------------------------------------------------------------------------
    // Description:
    //      Job specialization for the (no triggers to notify) case
    //------------------------------------------------------------------------------
    template<>
    class job<0> : public job_base
    {
    public:
        xcore_rtti( job, job_base )

        constexpr static int max_triggers_v = 0;

    public:

        constexpr           job             ( void )                            noexcept = default;
        constexpr           job             ( const definition Def )            noexcept : job_base{ Def } {}
        virtual     int     getTriggerCount ( void )                    const   noexcept override { return 0; } 

    protected:

        virtual     void    onAppenTrigger  ( trigger_base&        )           noexcept override { /*x_assert_linear( m_Debug_LQ );*/ xassume( false ); }
    };
}
#endif
