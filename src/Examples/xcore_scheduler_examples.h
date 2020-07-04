
namespace xcore::scheduler::examples
{
    constexpr static std::size_t LOOP_COUNT = 100000000;
    log::channel*   s_pChannel;

    //--------------------------------------------------------------------------------------
    // Test test 1 - basic jobs
    //--------------------------------------------------------------------------------------
    namespace test1
    {
        static std::atomic<int> s_Count;
        class count : public job<0>
        {
        public:
            count( void ) noexcept
            {
                XCORE_CMD_PROFILER( setupName( xconst_universal_str("Test 1, Job") ) );
                s_Count++;
            }

            count( definition D ) : job<0>{D} {s_Count++;}
            
            void qt_onRun( void ) noexcept override 
            {
                XCORE_PERF_ZONE_SCOPED_N("Job Type 1");
                for( int x = 0; x < 10; x++ )
                {
                    XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                    //  std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            XCORE_PERF_ZONE_SCOPED();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST1-----------------------------\n" );

            constexpr static int    Count = 8;
            auto&                   S = xcore::get().m_Scheduler;

            for( int i = 0; i < Count; ++i )
            {
                auto* pJob = new count;
                pJob->setupLifeTime( lifetime::DELETE_WHEN_DONE );
                S.AddJobToQuantumWorld( *pJob );
            }
            
            // Make the main thread help
            job_base* pJob;
            while( (pJob = S.getJob()) && s_Count )
            {
                S.ProcessWorkPrimitive( pJob );
            }
            
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST1 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test test 2 - basic triggers
    //--------------------------------------------------------------------------------------
    namespace test2
    {
        class count : public job<1>
        {
        public:
            count( void )         noexcept : job<1>{}  { XCORE_CMD_PROFILER( setupName( xconst_universal_str("Test 2, Job") ) ); }
            count( definition D ) noexcept : job<1>{D} { XCORE_CMD_PROFILER( setupName( xconst_universal_str("Test 2, Job") ) ); }

        protected:

            void qt_onRun( void ) noexcept override
            {
                XCORE_PERF_ZONE_SCOPED_N("Job Type 2");
                for( int x = 0; x < 10; x++ )
                {
                    XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        // Example job2
        //--------------------------------------------------------------------------------------
        //  [Job1]--->|                 [Job2]--->|    
        //  [Job1]--->|                 [Job2]--->|    
        //  [Job1]--->+--->{Trigger}--->[Job2]--->+--->{WaitingTrigger}-->*Done*
        //  [Job1]--->|                 [Job2]--->|    
        //--------------------------------------------------------------------------------------
        void Test( void )
        {
            XCORE_PERF_ZONE_SCOPED();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST2-----------------------------\n" );
            constexpr static int    Count = 16;
            count                   Jobs1[Count]{ definition::Flags( scheduler::jobtype::LIGHT ) };
            count                   Jobs2[Count]{ definition::Flags( scheduler::jobtype::LIGHT ) };
            trigger<Count>          Trigger;
            trigger<Count>          WaitingTrigger;
            
            WaitingTrigger.DontTriggerUntilReady();

            // Create the graph
            for( int i = 0; i < Count; ++i )
            {
                XCORE_CMD_PROFILER( Jobs1[i].setupName( xconst_universal_str("Example 2, Job 1") ) );
                Trigger.JobWillNotifyMe( Jobs1[i] );

                XCORE_CMD_PROFILER( Jobs2[i].setupName( xconst_universal_str("Example 2, Job 2") ) );
                Trigger.AddJobToBeTrigger( Jobs2[i] );

                // Adding a sync for all jobs at the end
                WaitingTrigger.JobWillNotifyMe( Jobs2[i] );
            }
            
            // Add graph to the quantum world
            auto& S = xcore::get().m_Scheduler;
            for( int i = 0; i < Count; ++i )
                S.AddJobToQuantumWorld( Jobs1[i] );
            
            // Wait untill the last trigger is done as ask the the main thread to help
            S.ProcessWhileWait( WaitingTrigger );
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST2 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test test 3 - basic triggers with dynamic memory allocation
    //--------------------------------------------------------------------------------------
    namespace test3
    {
        class count : public job<1>
        {
        public:
            count( definition D ) : job<1>{D} {}
            void qt_onRun( void ) noexcept override
            {
                XCORE_PERF_ZONE_SCOPED_N("Job Type 3");
                for( int x = 0; x < 10; x++ )
                {
                    XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        // This test is basically the same as test 2 except everything is dynamic
        //--------------------------------------------------------------------------------------
        void Test( void )
        {
            XCORE_PERF_ZONE_SCOPED();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST3-----------------------------\n" );

            constexpr static int    Count           = 16;
            using                   mytrigger       = trigger<Count>;
            auto                    pTrigger        = new mytrigger { lifetime::DELETE_WHEN_DONE, triggers::CLEAN_COUNT };
            auto                    pWaitingTrigger = new mytrigger { lifetime::DELETE_WHEN_DONE, triggers::CLEAN_COUNT };
            auto&                   S               = xcore::get().m_Scheduler;
            
            pWaitingTrigger->DontTriggerUntilReady();
            pTrigger->DontTriggerUntilReady();

            // Create part of the graph
            for( int i = 0; i < Count; ++i )
            {
                count* pJob1 = new count{ definition::Flags( lifetime::DELETE_WHEN_DONE, jobtype::LIGHT ) };
                XCORE_CMD_PROFILER( pJob1->setupName( xconst_universal_str("Example 3, Job 1") ) );

                count* pJob2 = new count{ definition::Flags( lifetime::DELETE_WHEN_DONE, jobtype::LIGHT ) };
                XCORE_CMD_PROFILER( pJob2->setupName( xconst_universal_str("Example 3, Job 2") ) );

                pTrigger->AddJobToBeTrigger( *pJob2 );
                pWaitingTrigger->JobWillNotifyMe( *pJob2 );

                // Finish the graph while we are still working on the graph
                // Here is why we used DontTriggerUntilReady
                pTrigger->JobWillNotifyMe( *pJob1 );
                S.AddJobToQuantumWorld( *pJob1 );
            }
            
            // Now we allow to trigger
            pTrigger->DoTriggerWhenReady();
            
            // After this function returns all pointers may be gone
            // Note that we need to use DontTriggerUntilReady for pWaitingTrigger if not the line above may
            // end up killing this trigger. By calling ProcessWhileWait we tell the trigger that we are ready.
            S.ProcessWhileWait( *pWaitingTrigger );
            
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST3 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test test 4 - basic job block
    //--------------------------------------------------------------------------------------
    namespace test4
    {
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            XCORE_PERF_ZONE_SCOPED();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST4-----------------------------\n" );

            constexpr static int    Count = 16;
            scheduler::channel      JobChannel(xconst_universal_str("Test4"));
            
            for( int i = 0; i < Count; ++i )
            {
                auto& S = xcore::get().m_Scheduler;
                JobChannel.SubmitJob( []()
                {
                    XCORE_PERF_ZONE_SCOPED_N("Job Type 4");
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                       for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            JobChannel.join();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST4 DONE----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test test 5 - multiple job blocks
    //--------------------------------------------------------------------------------------
    namespace test5
    {
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            XCORE_PERF_ZONE_SCOPED();
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST5-----------------------------\n" );
            
            constexpr static int    Count = 16;
            scheduler::channel      JobChannel1(xconst_universal_str("Test5-Steam0"));
            scheduler::channel      JobChannel2(xconst_universal_str("Test5-Steam1"));
            
            // assumes this can work in concurrency with job APPLES but not with ORANGES are a dependency
            for( int i = 0; i < Count; ++i )
            {
                JobChannel1.SubmitJob( []()
                {
                    XCORE_PERF_ZONE_SCOPED_N("Job Type 5");
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                        for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            // assumes this can work in concurrency with job BANANAS and ORANGES
            for( int i = 0; i < Count; ++i )
            {
                JobChannel2.SubmitJob( []()
                {
                    XCORE_PERF_ZONE_SCOPED_N("Job Type 6");
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
                        for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            JobChannel1.join();
            
            // assumes this can work in concurrency with job APPLES but not with BANANAS are a precondition
            for( int i = 0; i < Count; ++i )
            {
                JobChannel1.SubmitJob( []()
                {
                    XCORE_PERF_ZONE_SCOPED_N("Job Type 7");
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *s_pChannel, "WORKING IN ORANGES %c \n", char( 'A' + x ) );
                        for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            JobChannel1.join();
            JobChannel2.join();
            
            XLOG_CHANNEL_INFO( *s_pChannel, "----------------------------TEST5 DONE----------------------------\n" );
        }
    };
    
    void Test( void )
    {
        log::channel Channel("Scheduler Examples");
        s_pChannel = &Channel; //-V506
        test1::Test();
        test2::Test();
        test3::Test();
        test4::Test();
        test5::Test();
    }
}