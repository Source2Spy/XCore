
#if defined (_MSC_VER) && _XCORE_JOB_PROFILER
    #include <cvmarkersobj.h>               
    #include <cvmarkers.h>                  
    using namespace Concurrency::diagnostic;
#endif

namespace xcore::scheduler
{
    inline 
    auto& getLocalStorageData( void ) noexcept
    {
        return xcore::get().getLocalStorage<scheduler::local_storage_data>();
        /*
        if( Data.m_ThreadID == ~0 )
        {
        
        }
        */
        /*
        #if defined (_MSC_VER)

            #if true //_X_TARGET_DYNAMIC_LIB

                // per thread data
                local_storage_data* pData = (local_storage_data*)::TlsGetValue( m_dwThreadIndex );
                if( nullptr == pData )
                {
                    auto& S = g_context::get().m_Scheduler;
                    pData = (local_storage_data*)::TlsGetValue( S.m_dwThreadIndex );
                    //x_assert(pData);
                }
                return pData;

            #else
               // __declspec( thread ) int                 s_WorkerID;
                static thread_local int                 s_WorkerID;
            #endif
    
        #else

            static __thread int                     s_WorkerID;
        //    static __thread global_context*         s_pLocalContex;
        #endif
        */
    }


    //--------------------------------------------------------------------------------------
    // FUNCTIONS
    //--------------------------------------------------------------------------------------

    void WorkersStartWorking( void ) noexcept;

    //--------------------------------------------------------------------------------------
    // Description:
    //          Setting the name of a thread.
    //          For windows: https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    //--------------------------------------------------------------------------------------
    #if _XCORE_PLATFORM_WINDOWS
        #include <windows.h>
        const DWORD MS_VC_EXCEPTION = 0x406D1388;
        #pragma pack(push,8)  
        typedef struct tagTHREADNAME_INFO
        {
            DWORD   dwType;         // Must be 0x1000.  
            LPCSTR  szName;         // Pointer to name (in user addr space).  
            DWORD   dwThreadID;     // Thread ID (-1=caller thread).  
            DWORD   dwFlags;        // Reserved for future use, must be zero.  
        } THREADNAME_INFO;
        #pragma pack(pop)  
        static
        void SetThreadName( xcore::string::view<char> outView, const char* pAppName )
        {
            // Force each thread to stay in its core
            // Performance have shown that this is not helping
            //SetThreadAffinityMask(GetCurrentThread(), static_cast<u64>(1) << system::getWorkerUID().m_Value );


            sprintf_s( outView.data(), outView.size(), "xWorker (%s): %02d", pAppName, system::getWorkerUID().m_Value );

            DWORD dwThreadID = GetCurrentThreadId();
            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = outView.data();
            info.dwThreadID = dwThreadID;
            info.dwFlags = 0;
        #pragma warning(push)  
        #pragma warning(disable: 6320 6322)  
            __try {
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        #pragma warning(pop)
    }
    #else
        void SetThreadName( xcore::string::view<char> outView, const char* pAppName ) {}
    #endif


    //--------------------------------------------------------------------------------------

    system::system( void ) noexcept
        : xcore::system::registration{ nullptr, *this, xconst_universal_str("xBase/Scheduler") }
    {}

    //--------------------------------------------------------------------------------------

    system::~system( void ) noexcept
    {
        Shutdown();
        xassert_linear( m_Debug_LQ );
    }

    //--------------------------------------------------------------------------------------

    void system::Init( int nWorkers ) noexcept
    {
        //
        // Register the system officially
        //
        RegisterSystem();

        //
        // Create the channel
        //
        m_LogChannel = std::make_unique<xcore::log::channel>("xcore::Scheduler");
        m_LogChannel->TurnOff();

        //
        // Make sure some basic stuff is reset
        //
        m_bExceptionRaised.store( false );

        //
        // Do all the move to initialize
        //
        {
            xassert_linear( m_Debug_LQ );
            xassert( m_State == state::NO_INITIALIZED );

            // Initialize how many workers are currently active
            m_nWorkersActive    = 0;

            // We give ID=0 for the main thread
            setWorkerUID();

            // get how many cores we have -1 because the main thread is already created
            m_nAllocatedWorkers = std::thread::hardware_concurrency() - 1;
            if( nWorkers != -1 ) m_nAllocatedWorkers = nWorkers;

            // Allocate the worker kits. These are used by each worker 
            m_WorkerKit.New( m_nAllocatedWorkers + 1 ).CheckError();
            
            // Initialize the kits
            for( std::size_t i=0; i<(m_nAllocatedWorkers + 1); i++ )
            {
                auto& Kit = m_WorkerKit[i];

                Kit.m_LightJobPool.Init( (m_nAllocatedWorkers+1) * 16 ).CheckError();
                if( i == m_nAllocatedWorkers )  Kit.m_iNextQueue = 0;
                else                            Kit.m_iNextQueue = i+1;

                Kit.m_bException = false;
                Kit.m_ExceptionStackInfo[0]=0;
            }
        }
    
        //
        // Rename our main thread
        //
        {
            string::fixed<char,256> Name;
            SetThreadName( Name, xcore::get().m_pAppName );
        }

        //
        // Ok ready to start working
        //
        if( m_nAllocatedWorkers > 0 )
        {
            m_lWorkers.New( m_nAllocatedWorkers ).CheckError();

            std::lock_guard<std::mutex> Lock(m_SleepWorkerMutex );
            for( auto& Worker : m_lWorkers )
            {
                Worker = std::thread( WorkersStartWorking );
            }

            while (m_nWorkersActive.load(std::memory_order_relaxed) != (m_nAllocatedWorkers + 1));
        }
        else
        {
            m_nAllocatedWorkers = 0;
        }

        // Wait for the workers to be running
        while (m_nReadyWorkers != m_nAllocatedWorkers ) std::this_thread::yield();

        // Tell the workers we are ready to go now
        m_State = state::WORKING;
        m_SleepWorkerCV.notify_all();

    }

    //--------------------------------------------------------------------------------------

    void system::Shutdown( void ) noexcept
    {
        if( m_lWorkers.size() )
        {
            //
            // Get everyone to stop working
            //
            {
                xassert_quantum( m_Debug_LQ );

                m_State = state::EXITING;
                m_SleepWorkerCV.notify_all();

                for( auto& Worker : m_lWorkers )
                {
                    Worker.join();
                }
                xassert( m_nWorkersActive==1);
            }

            //
            // Clean the memory now
            //
            {
                xassert_linear( m_Debug_LQ );

                //
                // Clean any jobs pending in the queues
                //
                CleanQueues();

                //
                // Clean the workers
                //
                m_lWorkers.clear();
            }
        
            //
            // Destroy all the threads
            //
            m_lWorkers.clear();

            //
            // Release the main worker0 UID
            //
            releaseWorkerUID();
        }

        //
        // Reset the state
        //
        m_State = state::NO_INITIALIZED;
    }


    //--------------------------------------------------------------------------------------

    void system::AddJobToQuantumWorld( job_base& Job ) noexcept
    {
        xassert_quantum( m_Debug_LQ );

        //TA m_Stats_nJobs.inc();

        if( Job.isLightJob() )
        {
            //TA XLOG_CHANNEL_INFO( *m_LogChannel, "Scheduler: A New Light Job, Jobs Left [%d] There are[%d] workers working", m_Stats_nJobs.get(), m_Stats_nWorkersWorking.get() );
        
            // Currently we do not support light jobs with different affinities because that will require a mpmc_queue and we dont want to spend 
            // performance on optimizing that case. If we really need it to we could use the high priority slots of the non-normal affinity queues.
            xassert( Job.getAffinity() == affinity::NORMAL );
            const auto WID = getWorkerUID(*this);
            auto& Queue = m_WorkerKit[ WID.m_Value ].m_LightJobQueue;
            Queue.push( &Job );
        }
        else
        {   
            //TA XLOG_CHANNEL_INFO( *m_LogChannel, "Scheduler: A New Job, Jobs Left [%d] There are [%d] workers working", m_Stats_nJobs.get(), m_Stats_nWorkersWorking.get() );
            auto& Queue = m_JobQueue[static_cast<int>(Job.getAffinity())][static_cast<int>(Job.getPriority())];
            Queue.push( &Job );
        }
    
        //
        // If we have a job for the main thread just notify everyone since we don't know which CV is the main thread sitting on
        // Other wise we just need one guy to pick the job.
        //
        if( Job.getAffinity() == affinity::MAIN_THREAD ) m_SleepWorkerCV.notify_all(); 
        else                                             m_SleepWorkerCV.notify_one();
    }
   

    //--------------------------------------------------------------------------------------

    system::worker_kit& system::getWorketKit( void ) noexcept
    {
        const auto x = getWorkerUID(*this).m_Value;
        xassert( x >= 0 );
        xassert( x <= static_cast<decltype(x)>(m_nAllocatedWorkers) );
        return m_WorkerKit[ x ];
    }

    //--------------------------------------------------------------------------------------

    job_base* system::getLightJob( void ) noexcept
    {
        worker_kit& WorkerKit = getWorketKit();

        job_base* pJob;
        if( WorkerKit.m_LightJobQueue.pop(pJob) ) 
        {
            //TA m_Stats_nJobs.dec();
            return pJob;
        }

        // Search to steal any other light job from other workers
        std::size_t i = WorkerKit.m_iNextQueue;
        do 
        {
            if( m_WorkerKit[i].m_LightJobQueue.steal(pJob) ) 
            {
                //TA m_Stats_nJobs.dec();
                WorkerKit.m_iNextQueue = i;
                return pJob;
            }

            ++i;
            if( i >= (m_nAllocatedWorkers+1) ) i = 0;

        } while( i != WorkerKit.m_iNextQueue );

        return nullptr;
    }

    //--------------------------------------------------------------------------------------

    job_base* system::getJob( void ) noexcept
    {
        xassert_quantum( m_Debug_LQ );

        //
        // Lets focus on getting a job and only return if we fail after a while
        // if we fail we probably will go to sleep
        //
        for( int iLoop = 0; iLoop <100; iLoop++ )
        {
            // Try to get a light job first
            auto pJob = getLightJob();
            if( pJob ) 
                return pJob;

            // Get a job with the right affinity first
            if( getWorkerUID(*this).m_Value == 0 )
            {
                for( auto& Queue : m_JobQueue[ static_cast<int>(affinity::MAIN_THREAD) ] )
                {
                    if( Queue.pop(pJob) )
                    {
                        //TA m_Stats_nJobs.dec();
                        return pJob;
                    }
                }
            }
            else
            {
                for( auto& Queue : m_JobQueue[ static_cast<int>(affinity::NOT_MAIN_THREAD) ] )
                {
                    if( Queue.pop(pJob) )
                    {
                        //TA m_Stats_nJobs.dec();
                        return pJob;
                    }
                }
            }

            // get a regular job
            for( auto& Queue : m_JobQueue[ static_cast<int>(affinity::NORMAL) ] )
            {
                if( Queue.pop(pJob) )
                {
                    //TA m_Stats_nJobs.dec();
                    return pJob;
                }
            }
        }

        return nullptr;
    }

    //--------------------------------------------------------------------------------------
    #if _X_COMPILER_VISUAL_STUDIO
        X_CMD_JOB_PROFILER_ON
        (
            struct profile
            {
                xarray<marker_series,8>     mySeries { _T("0"), _T("1"),_T("2"),_T("3"),_T("4"),_T("5"),_T("6"),_T("7") };
                PCV_PROVIDER                MakerWriter; 
                GUID                        Guid;

                profile()
                {
                  //  Guid.Data1 = 123;
                 //   CvInitProvider( &Guid, &MakerWriter );
                    for( auto& E : mySeries )
                    {
                    //    CvCreatemarkerSeries( MakerWriter, _T("My Series"), &E );
                    }
                }
            };
            static thread_local int  s_SeriesStack=0;
            profile Profile;
        )
    #endif

    //-------------------------------------------------------------------------------------------
    static
    std::uint64_t xExceptionHandler( system::kit& Kit, EXCEPTION_POINTERS* pExp, DWORD dwExpCode ) noexcept
    {
        Kit.m_bException = true;
        /*TA
        x_MyStackWalker sw
        ( 
            StackWalker::StackWalkOptions::RetrieveSymbol
            | StackWalker::StackWalkOptions::RetrieveLine
            | StackWalker::StackWalkOptions::SymBuildPath
            , Kit.m_ExceptionStackInfo 
        );
        sw.ShowCallstack( GetCurrentThread(), pExp->ContextRecord );
        */

        return EXCEPTION_EXECUTE_HANDLER;
    }

    //--------------------------------------------------------------------------------------

    void system::ProcessWorkPrimitive( job_base* pJob ) noexcept
    {
        XCORE_PERF_ZONE_SCOPED();
        XCORE_PERF_ZONE_NAME( pJob->m_JobName.m_Str.m_pValue, pJob->m_JobName.m_Str.m_Size.m_Value );

        xassert_quantum( m_Debug_LQ );
        xassume( pJob );

    #if _X_COMPILER_VISUAL_STUDIO
        X_CMD_JOB_PROFILER_ON( span s(Profile.mySeries[s_SeriesStack++], s_SeriesStack, (LPCTSTR)pJob->getName().m_pValue ) );
    #endif

        //
        // Do the job
        //
        //TA m_Stats_nWorkersWorking.inc();
        //XCORE_PERF_PLOT(name, val)

        //TA XLOG_CHANNEL_INFO( *m_LogChannel, "Worker[%d] took a job, jobs left[%d] There are [%d] workers working", getWorkerUID(*this).m_Value, m_Stats_nJobs.get(), m_Stats_nWorkersWorking.get() );

        const bool bDeleteWhenDone = pJob->isDeletedWhenDone();


        [&]
        { 
            __try 
            {
                pJob->qt_onRun(); 
            }
            __except( xExceptionHandler( getWorketKit(), GetExceptionInformation(), GetExceptionCode() ) )
            {
                m_State = state::EXITING;
                m_bExceptionRaised.store( true );
                m_SleepWorkerCV.notify_all();
            }
        }();
 
        pJob->qt_onDone();

        if( bDeleteWhenDone ) delete( pJob );

        //TA m_Stats_nWorkersWorking.dec();

        //TA XLOG_CHANNEL_INFO( *m_LogChannel, "Worker[%d] finished the job, jobs left[%d], There are [%d] workers working", getWorkerUID(*this).m_Value, m_Stats_nJobs.get(), m_Stats_nWorkersWorking.get() );
        
    #if _X_COMPILER_VISUAL_STUDIO
        X_CMD_JOB_PROFILER_ON( s_SeriesStack-- );
    #endif
    }

    //--------------------------------------------------------------------------------------
    // static function
    worker_uid system::getWorkerUID( system& S ) noexcept
    {
        auto Data = getLocalStorageData();
        return worker_uid{ Data.m_ThreadID };
    }

    //--------------------------------------------------------------------------------------

    // static function
    worker_uid system::getWorkerUID( void ) noexcept
    {
        return getWorkerUID( xcore::get().m_Scheduler );
    }

    //--------------------------------------------------------------------------------------

    void system::setWorkerUID( void ) noexcept
    {
        const int WorkerID = m_nWorkersActive++;
        auto&     LocalData = getLocalStorageData();

        LocalData.m_ThreadID = WorkerID;

        /*
        #if defined (_MSC_VER)

            #if true //_X_TARGET_DYNAMIC_LIB

                local_storage_data* pData = (local_storage_data*) ::LocalAlloc(LPTR, sizeof(local_storage_data));
                x_assert (pData != 0);

                // Associate the thread to memory
                ::TlsSetValue( m_dwThreadIndex, (LPVOID)pData );

                // Set the actual ID
                pData->m_ThreadID = WorkerID;
            #else
                s_WorkerID = WorkerID;
            #endif

        #else
            s_WorkerID = WorkerID;
        #endif
        */
    }

    //--------------------------------------------------------------------------------------
    
    void system::releaseWorkerUID( void ) noexcept
    {
        const int WorkerID  = m_nWorkersActive--;
        auto&     LocalData = getLocalStorageData();

        LocalData.m_ThreadID = ~0;
        /*
        #if defined (_MSC_VER)

            #if true //_X_TARGET_DYNAMIC_LIB

                // release memory for this thread
                // local_storage_data* pData = (local_storage_data*)::TlsGetValue(m_dwThreadIndex);
                local_storage_data* pData = getLocalStorageData();
                if ( pData ) ::LocalFree((HLOCAL) pData);

            #else
                s_WorkerID = WorkerID;
            #endif

        #else
            s_WorkerID = WorkerID;
        #endif
        */
    }

    //--------------------------------------------------------------------------------------

    void system::ProcessWhileWait( trigger_base& Trigger ) noexcept
    {
        XCORE_PERF_ZONE_SCOPED_C(tracy::Color::ColorType::DarkGoldenrod); 
        xassert_quantum( xcore::get().m_Scheduler.m_Debug_LQ );
        std::atomic<bool> Bool { true };
    
        Trigger.DoTriggerWhenReady( &Bool );
    
        auto& S = xcore::get().m_Scheduler;

        // This version may be better for low end devices since it tries to save power
        // But there is more overhead and the system would need to be notifies on light jobs as well...
        // Left for reference
        #if 0
            std::unique_lock<std::mutex> Lock( S.m_SleepWorkerMutex, std::defer_lock );
            while( Bool )
            {
                job_base* pJob = S.getLightJob();

                if( pJob == nullptr ) 
                {
                    XCORE_PERF_ZONE_SCOPED_NC( "Sleeping", 0x4f2f2f );
                    Lock.lock();
                    S.m_SleepWorkerCV.wait( Lock, [ &pJob, &S, &Bool ]()
                    { 
                        pJob = S.getLightJob();
                        return Bool == false || pJob;
                    });
                    Lock.unlock();
                    if( pJob == nullptr ) 
                    {
                        xassert( Bool == false );
                        break;
                    }
                }
            
                S.ProcessWorkPrimitive( pJob );
            }
        #endif

        //
        // Actively consume work if we can
        //
        do 
        {
            job_base* pJob;

            // Get me a job!
            {
                XCORE_PERF_ZONE_SCOPED_NC( "Actively Sleeping", 0x4f2f2f );
                do
                {
                    pJob = S.getLightJob();
                } while( Bool.load( std::memory_order_relaxed ) && pJob == nullptr );
                if( pJob == nullptr && Bool.load( std::memory_order_relaxed ) == false ) break;
            }
            
            S.ProcessWorkPrimitive( pJob );

        } while( Bool.load( std::memory_order_relaxed ) );
        
/*
        while( Bool )
        {
            job_base* pJob = S.getLightJob();
            if( pJob ) 
            {
                S.ProcessWorkPrimitive( pJob );
            }
        }
*/
        ////////////
        // 
        // At this point the trigger could be freed
        // 
        ////////////
    }

    //--------------------------------------------------------------------------------------

    void system::CleanQueues( void ) noexcept
    {
        xassert_linear( m_Debug_LQ );

        job_base* pJop;

        for( int i = 0; i < m_nAllocatedWorkers + 1; i++ )
        {
            while( m_WorkerKit[i].m_LightJobQueue.pop(pJop) )
            {
                if( pJop->isDeletedWhenDone() ) delete( pJop );
            }
        }

        // Affinity first
        for( auto& Affinity : m_JobQueue )
        {
            // Then Priority
            for( auto& Queue : Affinity )
            {
                // Then we have a queue
                while( Queue.pop(pJop) )
                {
                    if( pJop->isDeletedWhenDone() ) delete( pJop );
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------

    void system::ThreadBecomesWorker() noexcept
    {
        xassert_quantum( m_Debug_LQ );

        std::unique_lock<std::mutex> Lock( m_SleepWorkerMutex, std::defer_lock );

        // At this point we should be working
        while( m_State == system::state::WORKING )
        {
            job_base* pJob;

            //
            // Get next job or Sleep when we need to
            //
            pJob = getJob();
            if( pJob == nullptr )
            {
                XCORE_PERF_ZONE_SCOPED_NC("Sleeping",0x4f2f2f)
                XLOG_CHANNEL_INFO( *m_LogChannel, "Worker[%d] Sleeping", getWorkerUID(*this).m_Value );
                Lock.lock();
                m_SleepWorkerCV.wait( Lock, [ &pJob, this ]()
                { 
                    pJob = getJob();
                    return pJob || m_State == system::state::EXITING;
                });
                Lock.unlock();
                if( pJob == nullptr )
                {
                    if( m_State == system::state::EXITING )
                    {
                        XLOG_CHANNEL_INFO( *m_LogChannel, "Worker [%d] Awakes to terminate itself\n", getWorkerUID(*this).m_Value );
                        return;
                    }

                    XLOG_CHANNEL_INFO( *m_LogChannel, "Worker [%d] Falsely Awake up", getWorkerUID(*this).m_Value );
                    continue;
                }
                else
                {
                    XLOG_CHANNEL_INFO( *m_LogChannel, "Worker [%d] Awakes Ready to work", getWorkerUID(*this).m_Value );
                }
            }

            //
            // Process any job
            //
            ProcessWorkPrimitive( pJob );
        }
    }

    //--------------------------------------------------------------------------------------

    void system::MainThreadBecomesWorker() noexcept
    {
        xassert_quantum( m_Debug_LQ );

        std::unique_lock<std::mutex> Lock(m_SleepWorkerMutex, std::defer_lock);

        // At this point we should be working
        while( m_State == system::state::WORKING )
        {
            job_base* pJob;

            //
            // Stop the main thread from working if requested
            //
            if( m_bMainThreadShouldWork.load( std::memory_order_relaxed ) == false )
            {
                // Reset to default
                m_bMainThreadShouldWork.store( true, std::memory_order_relaxed );
                break;
            }

            //
            // Get next job or Sleep when we need to
            //
            pJob = getJob();
            if( pJob == nullptr )
            {
                XLOG_CHANNEL_INFO( *m_LogChannel, "Worker[%d] Sleeping", getWorkerUID(*this).m_Value );
                Lock.lock();
                if( m_bMainThreadShouldWork.load( std::memory_order_relaxed ) == false )
                {
                    m_SleepWorkerCV.wait( Lock, [&pJob, this]()
                    {
                        pJob = getJob();
                        return pJob || m_bMainThreadShouldWork.load( std::memory_order_relaxed ) == false || m_State == system::state::EXITING;
                    });
                }
                Lock.unlock();

                if( pJob == nullptr )
                {
                    if( m_bMainThreadShouldWork.load( std::memory_order_relaxed ) == false )
                    {
                        XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread worker[%d] Awakes to terminate itself", getWorkerUID(*this).m_Value );

                        // Reset to default
                        m_bMainThreadShouldWork.store( true, std::memory_order_relaxed );
                        return;
                    }

                    if( m_State == system::state::EXITING )
                    {
                        XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread worker[%d] Awakes to terminate itself", getWorkerUID(*this).m_Value );
                        return;
                    }

                    XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread Worker[%d] Falsely Awake up", getWorkerUID(*this).m_Value );
                    continue;
                }
                else
                {
                    XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread  Worker[%d] Awakes Ready to work", getWorkerUID(*this).m_Value  );
                }
            }

            //
            // Process any job
            //
            ProcessWorkPrimitive( pJob );
        }
    }

    //--------------------------------------------------------------------------------------

    void system::MainThreadStartsWorking( void ) noexcept
    {
        XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread becomes a worker[%d]", getWorkerUID(*this).m_Value );

        //
        // Wait for the system to finish its initialization
        //
        while(m_State.load(std::memory_order_relaxed) == system::state::NO_INITIALIZED);

        MainThreadBecomesWorker();
        XLOG_CHANNEL_INFO( *m_LogChannel, "Main thread worker[%d] Stops contributing to the workers [%d]", getWorkerUID(*this).m_Value, m_nWorkersActive.load()  );
    }

    //--------------------------------------------------------------------------------------

    //static
    void WorkersStartWorking( void ) noexcept
    {
        system& Scheduler = xcore::get().m_Scheduler;

        //
        // Init basic stuff
        //
        Scheduler.setWorkerUID();

        {
            string::fixed<char,256> Name;
            SetThreadName( Name, xcore::get().m_pAppName );

            const auto ID = system::getWorkerUID(Scheduler).m_Value;
            XCORE_CMD_PROFILER( xcore::profile::SetThreadName( Name.data() ); )// Scheduler.m_lWorkers[ID-1] );//, Name.data() );

            XLOG_CHANNEL_INFO( *Scheduler.m_LogChannel, "%s Created", Name.data() );
        }

        //
        // Wait for the system to finish its initialization
        //
        {
            std::unique_lock<std::mutex> Lock(Scheduler.m_SleepWorkerMutex);
            Scheduler.m_nReadyWorkers++;
            Scheduler.m_SleepWorkerCV.wait(Lock, [&Scheduler]()
            {
                return Scheduler.m_State != system::state::NO_INITIALIZED;
            });
        }

        //
        // At this point we should be working
        //
        Scheduler.ThreadBecomesWorker();

        // Done
        Scheduler.releaseWorkerUID();
        XLOG_CHANNEL_INFO( *Scheduler.m_LogChannel, "Worker[%d] Destroyed Number of workers active [%d]", system::getWorkerUID(Scheduler).m_Value, Scheduler.m_nWorkersActive.load() );
    }
}

//--------------------------------------------------------------------------------------
// Properties
//--------------------------------------------------------------------------------------
property_begin( xcore::scheduler::system )
{
    property_var_fnbegin( "nWorkers", int )
    { 
        if (isRead) InOut = Self.m_nWorkersActive;
    } property_var_fnend()
        .Help("Number of workers active")
        .Flags( property::flags::SHOW_READONLY )

    , property_var_fnbegin( "DebugLog", bool )
    { 
        if (isRead)
        {
            InOut = Self.m_LogChannel->isPrint();
        }
        else
        {
            if( InOut ) Self.m_LogChannel->TurnOn();
            else        Self.m_LogChannel->TurnOff();
        }
    } property_var_fnend()
        .Help("Enable or disable the logging system for the scheduler")

} property_vend_cpp( xcore::scheduler::system )
