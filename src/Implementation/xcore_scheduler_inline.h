
namespace xcore::scheduler
{
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    // JOB BASE
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    inline      
    job_base& job_base::setupDefinition ( definition Def )  noexcept 
    { 
        xassert_linear( m_Debug_LQ ); 
        m_Definition = Def; 
        return *this;
    }

    //------------------------------------------------------------------------------
    inline      
    job_base& job_base::setupJobType ( jobtype JobType ) noexcept 
    { 
        xassert_linear( m_Debug_LQ ); 
        m_Definition.m_JobType  = JobType; 
        return *this;                 
    }

    //------------------------------------------------------------------------------
    inline      
    job_base& job_base::setupLifeTime ( lifetime LifeTime )   noexcept 
    { 
        xassert_linear( m_Debug_LQ ); 
        m_Definition.m_LifeTime = LifeTime; 
        return *this;                
    }

    //------------------------------------------------------------------------------
    inline      
    job_base& job_base::setupPriority ( priority Priority ) noexcept 
    { 
        xassert_linear( m_Debug_LQ ); 
        m_Definition.m_Priority = Priority; 
        return *this;                
    }

    //------------------------------------------------------------------------------

    inline 
    void job_base::NotifyTrigger( trigger_base& Trigger ) noexcept
    {
        Trigger.qt_onNotify();
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    // JOB
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    template< std::size_t T_NUM_TRIGGERS > inline
    void job<T_NUM_TRIGGERS>::qt_onDone( void ) noexcept  
    { 
        xassert_linear( m_Debug_LQ );
        auto nTriggers = m_nTriggers.load(std::memory_order_relaxed);
        if( m_Definition.m_Triggers == triggers::CLEAN_COUNT ) m_nTriggers.store(0,std::memory_order_relaxed);
        while( nTriggers )
        {
            NotifyTrigger( *m_TriggerList[--nTriggers] ); 
        }
    }

    //------------------------------------------------------------------------------
    template< std::size_t T_NUM_TRIGGERS > inline
    void job<T_NUM_TRIGGERS>::onAppenTrigger( trigger_base& Trigger ) noexcept  
    { 
        xassert_linear( m_Debug_LQ ); 
        xassume( m_nTriggers < T_NUM_TRIGGERS ); 
        m_TriggerList[m_nTriggers++] = &Trigger; 
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    // TRIGGER BASE
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    constexpr
    trigger_base::trigger_base( lifetime LifeTime, triggers ClearCounter) noexcept
    {
        m_Definition.m_LifeTime = LifeTime;
        m_Definition.m_Triggers = ClearCounter;

        xassert_linear( m_Debug_LQ );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool trigger_base::isGoingToBeDeletedWhenDone( void ) const noexcept
    {
        xassert_quantum( m_Debug_LQ );
        // this could run in quantum or linear
        return m_Definition.m_LifeTime == lifetime::DELETE_WHEN_DONE;
    }

    //------------------------------------------------------------------------------
    inline
    void trigger_base::join ( void ) noexcept
    {
        xcore::get().m_Scheduler.ProcessWhileWait( *this );
    }

    //------------------------------------------------------------------------------
    template< class T_JOB > inline
    void trigger_base::JobWillNotifyMe ( T_JOB& Job ) noexcept
    {
        xassert_quantum( m_Debug_LQ );
        m_NotificationResetCounter = ++m_NotificationCounter;
        Job.onAppenTrigger( *this );
    }

    //------------------------------------------------------------------------------
    inline
    void trigger_base::DontTriggerUntilReady( void ) noexcept
    {
        xassert_linear( m_Debug_LQ );

        // we increment by one to make sure we don't trigger
        // this function should be call in the linear world
        xassert( m_Debug_bReady == true );
        xassume( m_NotificationCounter == 0 );
        m_NotificationCounter.fetch_add( 1, std::memory_order_relaxed );
        XCORE_CMD_DEBUG( m_Debug_bReady = false );
    }

    //------------------------------------------------------------------------------
    inline
    void trigger_base::DoTriggerWhenReady( std::atomic<bool>* pNotify ) noexcept
    {
        xassert_quantum( m_Debug_LQ );
        xassume( m_NotificationCounter > 0 );

        // This assert will happen if you did not call DontTriggerUntilReady
        // this is require for triggers that are dealing with the quantum world
        // if everything happens in the linear world then DontTriggerUntilReady function 
        // is not needed, so this function should not be call neither.
        xassert(         m_Debug_bReady == false );
        XCORE_CMD_DEBUG( m_Debug_bReady = true );
        m_pNotifyVar        = pNotify;

        // Life time for "this" could be gone after qt_onNotify
        qt_onNotify();
        //////////////////
        //
        // DEAD SPACE - by qt_onNotify
        //
        //////////////////
    }

    //------------------------------------------------------------------------------
    inline
    void trigger_base::qt_onNotify( void ) noexcept 
    {
        // Can not have this assert here since by the time the destructor is call the class could be gone
        // xassert_quantum( m_Debug_LQ );
        XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel, "Trigger: notify: %d\n", m_NotificationCounter.load() );
        xassume( m_NotificationCounter > 0 );
        if( 0 == --m_NotificationCounter )
        {
            XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel,  "Trigger: Ready to Releasing Jobs!\n" )

            // Lets backup the notification flag
            // notification should happen at the very end since after that we are back into some unknown state
            std::atomic<bool>*   pNotifyVar = m_pNotifyVar;
            m_pNotifyVar = nullptr;

            // Backup also the delete flag
            const bool bDelete = isGoingToBeDeletedWhenDone();

            // If the user ask us to reset the counter we will
            if (m_Definition.m_Triggers == triggers::DONT_CLEAN_COUNT) m_NotificationCounter.store(m_NotificationResetCounter, std::memory_order_relaxed);

            // Notify our OO-hierarchy that we are ready to release jobs
            // Here the user may do some nasty things, so that is why we play safe and backup the delete flag
            // This is the last safe point in this function after this we assume dead space
            qt_onTriggered();

            ///// DEAD SPACE FROM HERE DOWN
            // lets release the object if we have to
            // note that deleting this here is not a great since we are still inside the class
            // but because other workers could be here anyways it does not matter too much.
            if( bDelete ) delete(this);
            //////
            //
            // DEAD SPACE !!!
            //
            //////
            // now we are really officially done, so notify people that may care
            if( pNotifyVar ) pNotifyVar->store( false, std::memory_order_release );
        }
        ////////////////
        //
        // DEAD SPACE!!! By self (means the class could be deleted at this point)
        //
        ////////////////
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    // TRIGGER 
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    template< int T_MAX_JOBS > inline
    trigger<T_MAX_JOBS>::~trigger ( void ) noexcept
    {
        xassume( m_Definition.m_Triggers == triggers::DONT_CLEAN_COUNT || m_NotificationCounter == 0 );
        xassert( m_Debug_bReady == true );
    }

    //------------------------------------------------------------------------------
    template< int T_MAX_JOBS > inline
    void trigger<T_MAX_JOBS>::AddJobToBeTrigger( job_base& Job ) noexcept
    {
        const int iLocal = m_nJobsToTrigger++;
    #if _X_DEBUG
        if( m_Debug_bReady == false )
        {
            xassert_quantum( m_Debug_LQ );
            m_JobToTrigger[iLocal] = &Job;
        }
        else
        {
            xassert_linear( m_Debug_LQ );
            m_JobToTrigger[iLocal] = &Job;
        }
    #else
        m_JobToTrigger[iLocal] = &Job;
    #endif
    }

    //------------------------------------------------------------------------------
    template< int T_MAX_JOBS > inline
    void trigger<T_MAX_JOBS>::qt_onTriggered( void ) noexcept
    {
        xassert_quantum( m_Debug_LQ ); 

        const int nJobs = m_nJobsToTrigger.load(std::memory_order_relaxed);
        for( int i=0; i<nJobs; i++ )
        {
            xcore::get().m_Scheduler.AddJobToQuantumWorld( *m_JobToTrigger[i] );
        }

        if( nJobs )
        {
            XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel,  "Trigger: Released [ %d ] Jobs.\n", nJobs );
        }
        else
        {
            XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel,  "Trigger: is Completed with no jobs to release\n" );
        }
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    // CHANNEL 
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------


    //------------------------------------------------------------------------------
    inline
    channel::channel( const xcore::string::const_universal Name, int MaxSimultaneousJobs, xcore::scheduler::system& S ) noexcept 
        : m_Scheduler   { S }
        , m_Pool        { S.getWorketKit().m_LightJobPool }
        , m_nMaxJobs    { MaxSimultaneousJobs == -1 ? (S.getWorkerCount() * 3) : MaxSimultaneousJobs }
        , m_Name        { Name }
    {
        xassert_linear(m_Debug_LQ);
        DontTriggerUntilReady();
        m_ProcessState.store( STATE_PROCESS_STATE_READY );
    }

    //------------------------------------------------------------------------------
    inline
    channel::~channel( void ) noexcept
    {
        join();
        xassume( isGoingToBeDeletedWhenDone() == false );
        xassume( m_ProcessState == STATE_PROCESS_STATE_DONE );
        xassert_linear( m_Debug_LQ );
    }

    //------------------------------------------------------------------------------
    template< typename T > inline
    void channel::SubmitJob( T&& func ) noexcept
    {
        using fa = xcore::function::traits< std::remove_reference_t<T> >;
        using fb = xcore::function::traits< xcore::function::make_t<true, void, void> >;
        static_assert( xcore::function::traits_compare<fa,fb>::value );

        // Get everything ready
        if( m_ProcessState == STATE_PROCESS_STATE_DONE )
        {
            xassert_linear( m_Debug_LQ );
            DontTriggerUntilReady();
            m_ProcessState.store( STATE_PROCESS_STATE_READY );
        }

        details::lambda_job* pFreeJob;
        {
            xassert_quantum( m_Debug_LQ );

            // If we have not empty slots then help process work
            while( m_NotificationCounter.load( std::memory_order_relaxed ) >= m_nMaxJobs || (pFreeJob = m_Pool.pop()) == nullptr )
            {
                auto& S = xcore::get().m_Scheduler;
                job_base* pJob = S.getLightJob();
                if( pJob ) S.ProcessWorkPrimitive( pJob );
            }

            // increment the total jobs that must notify me 
            ++m_NotificationCounter;

            // setup the callback
            pFreeJob->m_pJobBlock = this;
            pFreeJob->m_Function  = func;
            pFreeJob->setupPriority(m_Priority);
            pFreeJob->setupAffinity(m_Affinity);
            XCORE_CMD_PROFILER(pFreeJob->setupName(m_Name));
        }

        m_Scheduler.AddJobToQuantumWorld( *pFreeJob );
    }

    //------------------------------------------------------------------------------

    template< typename T_CONTAINER, typename T_LAMBDA > inline 
    std::enable_if_t
    < 
          xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( xcore::span<typename T_CONTAINER::value_type> ) >
        , void 
    >
    channel::ForeachLog( T_CONTAINER& Container, const std::size_t Diviser, const std::size_t Cutoff, T_LAMBDA&& func ) noexcept
    {
        auto        Total       = Container.size();
        std::size_t Start       = 0;

    //    m_Scheduler.m_SleepWorkerCV.notify_all();

        // Do the log
        auto R = Total / Diviser;
        if( R > Cutoff ) do
        {
            SubmitJob( [ &Container, Start, R, &func ]()
            {
                func( Container.ViewFromCount( Start, R ) );
            });

            Start += R;
            Total -= R;
            R   = Total / Diviser;

        } while( R > Cutoff );

        // Do the linear 
        if( Total > 0 ) do
        {
            R = std::min( Cutoff, Total );
            SubmitJob( [ &Container, Start, R, &func ]()
            {
                func( Container.ViewFromCount( Start, R ) );
            });
            Start += R;
            Total -= R;

        } while ( Total > 0 );
    }

    //------------------------------------------------------------------------------
    template< typename T_CONTAINER, typename T_LAMBDA > inline
    std::enable_if_t
    < 
          xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( typename T_CONTAINER::value_type& ) >
        , void 
    >
    channel::ForeachLog( T_CONTAINER& Container, const std::size_t Diviser, const std::size_t Cutoff, T_LAMBDA&& func ) noexcept
    {
        ForeachLog( Container, Diviser, Cutoff, [&func]( xcore::span<typename T_CONTAINER::value_type> View )
        {
            for( auto& E : View )
            {
                func( E );
            }
        });
    }

    //------------------------------------------------------------------------------
    template< typename T_CONTAINER, typename T_LAMBDA > inline
    std::enable_if_t
    < 
          xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( xcore::span<typename T_CONTAINER::value_type> ) >
        , void 
    >
    channel::ForeachFlat( T_CONTAINER& Container, const std::size_t Diviser, T_LAMBDA&& func ) noexcept
    {
        // get ready
        const auto   Total   = Container.size();
    
        if( Total == 0 ) return;

        xassert(Diviser);
        xassert(Total / Diviser);
        std::size_t   Start     = 0;
        const auto    nEntries  = std::max<std::size_t>( 1, Total/Diviser );

        // Process all the entries
        do
        {
            const auto Count = std::min( Total - Start, Diviser);
            SubmitJob( [ View = Container.ViewFromCount(Start, Count), &func ]()
            {
                func(View);
            });

            Start  += Count;
        } while( Start < Total );
    }

    //------------------------------------------------------------------------------
    template< typename T_CONTAINER, typename T_LAMBDA > inline
    std::enable_if_t
    < 
          xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( typename T_CONTAINER::value_type& ) >
        , void 
    > 
    channel::ForeachFlat( T_CONTAINER& Container, const std::size_t Diviser, T_LAMBDA&& func ) noexcept
    {
        ForeachFlat( Container, Diviser, [&func]( xcore::span<typename T_CONTAINER::value_type>& View )
        {
            for( auto& E : View )
            {
                func( E );
            }
        });
    }

    //------------------------------------------------------------------------------
    inline
    void channel::join( void ) noexcept
    {
        xassert_quantum( m_Debug_LQ );
        xassume( isGoingToBeDeletedWhenDone() == false );

        if( m_ProcessState == STATE_PROCESS_STATE_READY )
        {
            xcore::get().m_Scheduler.ProcessWhileWait( *this );
            ////////
            //
            // Dead Space -- at this point the this pointer could be invalid
            //
            ////////
        }
    }

    //------------------------------------------------------------------------------
    inline // virtual
    void channel::qt_onNotify( void ) noexcept 
    {
        // Can not have this assert here since by the time the destructor is call the class could be gone
        // xassert_quantum( m_Debug_LQ );
        
        XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel,  "Trigger: notify: %d", m_NotificationCounter.load( std::memory_order_relaxed ) );
        xassume( m_NotificationCounter > 0 );
        if( --m_NotificationCounter == 0 )
        {
            XLOG_CHANNEL_INFO( *xcore::get().m_Scheduler.m_LogChannel,  "Trigger: Ready to Releasing Jobs!\n" )

            // Notify our OO-hierarchy that we are ready to release jobs
            qt_onTriggered();
            
            xassume( isGoingToBeDeletedWhenDone() == false );
            
            // now we are really officially done, so notify people that may care
            if( m_pNotifyVar ) *m_pNotifyVar = false;
        }
    }

    //------------------------------------------------------------------------------
    inline // virtual
    void channel::qt_onTriggered( void ) noexcept  
    {
        xassert_quantum( m_Debug_LQ );
        xassume( m_ProcessState == STATE_PROCESS_STATE_READY );
        m_ProcessState.store( STATE_PROCESS_STATE_DONE, std::memory_order_release );
    }

}