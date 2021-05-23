#ifndef _XCORE_SCHEDULER_TRIGGER_H
#define _XCORE_SCHEDULER_TRIGGER_H
#pragma once

namespace xcore::scheduler
{
    //------------------------------------------------------------------------------
    // Description:
    //      This is the base trigger class used for any future trigger types
    //------------------------------------------------------------------------------
    class trigger_base 
    {
    public:
        xcore_rtti_start( trigger_base )

    public:
    
                                        trigger_base                ( void )                                        noexcept = default;
        constexpr                       trigger_base                ( lifetime LifeTime, triggers ClearCounter )    noexcept;
        virtual                        ~trigger_base                ( void )                                        noexcept { xassume(m_Definition.m_Triggers == triggers::DONT_CLEAN_COUNT || m_NotificationCounter == 0); }
        constexpr       bool            isGoingToBeDeletedWhenDone  ( void )                                const   noexcept;
        inline          void            join                        ( void )                                        noexcept;
        template< class T_JOB >
        inline          void            JobWillNotifyMe             ( T_JOB& Job )                                  noexcept;
        inline          void            DontTriggerUntilReady       ( void )                                        noexcept;
        inline          bool            isTriggered                 ( void )                                const   noexcept { return m_NotificationCounter.load( std::memory_order_relaxed ) == 0; }
        inline          void            DoTriggerWhenReady          ( std::atomic<bool>* pNotify = nullptr )        noexcept;
        virtual         int             getJobCount                 ( void )                                const   noexcept = 0;

    protected:

        virtual         void            qt_onNotify                 ( void )                                        noexcept; 
        virtual         void            qt_onTriggered              ( void )                                        noexcept = 0;

    protected:

    #if _XCORE_DEBUG
        bool                            m_Debug_bReady              { true };
        x_debug_linear_quantum          m_Debug_LQ                  {};
    #endif

        std::atomic<bool>*              m_pNotifyVar                { nullptr };
        std::atomic<std::uint16_t>      m_NotificationCounter       { 0 };
        std::uint16_t                   m_NotificationResetCounter  { 0 };
        xcore::scheduler::definition    m_Definition                {};

    protected:

        friend class job_base;
    };

    //------------------------------------------------------------------------------
    // Description:
    //      A general trigger able to notify jobs
    //------------------------------------------------------------------------------
    template< int T_MAX_JOBS >
    class trigger : public trigger_base
    {
    public:
        xcore_rtti( trigger, trigger_base )

        static constexpr int max_jobs_v = T_MAX_JOBS;
    
    public:

        using trigger_base::trigger_base;

        inline                 ~trigger                 ( void )                    noexcept;
        inline      void        AddJobToBeTrigger       ( job_base& Job )           noexcept;
        virtual     int         getJobCount             ( void )            const   noexcept override { return m_nJobsToTrigger; }
        inline      job_base&   getJob                  ( int Index )       const   noexcept          { xassert( Index < m_nJobsToTrigger ); return m_JobToTrigger[Index]; }

    protected:

        virtual     void        qt_onTriggered          ( void )                    noexcept override;

    protected:

        std::atomic<std::uint16_t>          m_nJobsToTrigger    {0};
        xcore::array<job_base*,max_jobs_v>  m_JobToTrigger      {};
    };
}

#endif