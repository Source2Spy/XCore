#ifndef _XCORE_SCHEDULER_SYSTEM_H
#define _XCORE_SCHEDULER_SYSTEM_H
#pragma once

//--------------------------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------------------------
namespace xcore::scheduler
{
    using worker_uid = xcore::units::handle< class system, std::uint32_t >;

    class system : public property::base, xcore::system::registration 
    {
    public:
        property_vtable()
                                    system                      ( void )                                            noexcept;
        virtual                    ~system                      ( void )                                            noexcept;
                    void            Init                        ( int nWorkers = -1 )                               noexcept;
                    void            Shutdown                    ( void )                                            noexcept;
                    void            AddJobToQuantumWorld        ( job_base& Job )                                   noexcept;
                    job_base*       getJob                      ( void )                                            noexcept;
                    job_base*       getLightJob                 ( void )                                            noexcept;
                    void            ProcessWorkPrimitive        ( job_base* pJob )                                  noexcept;
        static      worker_uid      getWorkerUID                ( void )                                            noexcept;
        static      worker_uid      getWorkerUID                ( system& S )                                       noexcept;
        static      void            ProcessWhileWait            ( trigger_base& Trigger )                           noexcept;
                    void            MainThreadStartsWorking     ( void )                                            noexcept;
        inline      void            MainThreadStopWorking       ( void )                                            noexcept { m_bMainThreadShouldWork.store( false, std::memory_order_relaxed); m_SleepWorkerCV.notify_all(); }
        template< typename T = int >
        constexpr   T               getWorkerCount              ( void )                                    const   noexcept { return static_cast<T>(m_nAllocatedWorkers+1); }
        inline      bool            getExceptionRaised          ( void )                                    const   noexcept { return m_bExceptionRaised.load(); }
        inline      const char*     getWorkerExceptionInfo      ( worker_uid uidWorker = getWorkerUID() )   const   noexcept { xassert(uidWorker.m_Value >= 0 && (int)uidWorker.m_Value <=m_nAllocatedWorkers ); return m_WorkerKit[uidWorker.m_Value].m_bException ? &m_WorkerKit[uidWorker.m_Value].m_ExceptionStackInfo[0] : nullptr; }
        inline      auto&           getWorkerExceptionBuffer    ( worker_uid uidWorker = getWorkerUID() )           noexcept { xassert(uidWorker.m_Value >= 0 && (int)uidWorker.m_Value <= m_nAllocatedWorkers); return m_WorkerKit[uidWorker.m_Value].m_ExceptionStackInfo; }
        inline      void            RaiseException              ( worker_uid uidWorker = getWorkerUID() )           noexcept { xassert(uidWorker.m_Value >= 0 && (int)uidWorker.m_Value <= m_nAllocatedWorkers); m_WorkerKit[uidWorker.m_Value].m_bException = true;  }

    public:

        std::unique_ptr<xcore::log::channel>    m_LogChannel; //               {"xcore::Scheduler"};

    protected:

        enum class state : std::uint32_t
        {
                NO_INITIALIZED
            ,   WORKING
            ,   EXITING
        };

        struct worker_kit
        {
            using lambda_job = xcore::scheduler::details::lambda_job;

            constexpr worker_kit( void ) noexcept = default;
            xcore::lockless::queues::mpmc_bounded<job_base*,1024>               m_LightJobQueue         {};
            std::size_t                                                         m_iNextQueue            {~std::size_t(0)};
            xcore::lockless::pool::mpmc_bounded_dynamic<details::lambda_job>    m_LightJobPool          {};
            bool                                                                m_bException            {false};
            xcore::array<char,1024*128>                                         m_ExceptionStackInfo    {};
        };

        using priority_jobs_queue_array     = xcore::array< lockless::queues::mpmc_bounded<job_base*, 1024>, static_cast<int>(priority::ENUM_COUNT) >;
        using affinity_job_queue_array      = xcore::array< priority_jobs_queue_array, static_cast<int>(affinity::ENUM_COUNT) >;

    protected:

                    void                CleanQueues                 ( void )                                    noexcept;
                    void                MainThreadBecomesWorker     ( void )                                    noexcept;
                    void                ThreadBecomesWorker         ( void )                                    noexcept;
                    worker_kit&         getWorketKit                ( void )                                    noexcept;
  //      virtual     prop_table&         onPropertyTable             ( void )                            const   noexcept override;
                    void                setWorkerUID                ( void )                                    noexcept;
                    void                releaseWorkerUID            ( void )                                    noexcept;

    protected:

        xcore::unique_span<worker_kit>          m_WorkerKit             {};
        affinity_job_queue_array                m_JobQueue              {};
        std::mutex                              m_SleepWorkerMutex      {};
        std::condition_variable                 m_SleepWorkerCV         {};
        std::atomic<int>                        m_nWorkersActive        {0};
        xcore::unique_span<std::thread>         m_lWorkers              {};
        int                                     m_nAllocatedWorkers     { 0 };
        std::atomic<state>                      m_State                 { state::NO_INITIALIZED };

    #if _X_DEBUG
        x_debug_linear_quantum                  m_Debug_LQ              {};
    #endif
        // x_reporting::stats_metric               m_Stats_nWorkersWorking {};
        // x_reporting::stats_metric               m_Stats_nJobs           {};
        std::atomic<bool>                       m_bMainThreadShouldWork { true };
        std::atomic<bool>                       m_bExceptionRaised      { false };
        std::atomic<int>                        m_nReadyWorkers         { 0 };

    public:

        //xcore::lockless::pool::mpmc_bounded_jitc<x_scheduler_jobs::job_chain::state_block, 100>    system_JobChainBlockPool   {};
        //x_ll_fixed_pool_static_raw<x_scheduler_jobs::job_chain::lambda_job,  100>    system_LambdaJobChainPool  {};
        using kit = worker_kit;

    protected:

        friend void WorkersStartWorking( void ) noexcept;
        friend class xcore::scheduler::channel;
    };
}
#endif