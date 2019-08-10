
namespace xcore::scheduler::examples
{
    enum { LOOP_COUNT = 100000000 };

    //--------------------------------------------------------------------------------------
    // Test job 1 - basic jobs
    //--------------------------------------------------------------------------------------
    namespace job1
    {
        static std::atomic<int> s_Count;
        class count : public job<0>
        {
        public:
            count( void )
            {
                s_Count++;
            }

            count( definition D ) : job<0>{D} {s_Count++;}
            
            void qt_onRun( void ) noexcept override 
            {
                auto& S = xcore::get();
                for( int x = 0; x < 10; x++ )
                {
                    //  std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                    XLOG_CHANNEL_INFO( *S.m_Scheduler.m_LogChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            auto& S = xcore::get().m_Scheduler;
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST1-----------------------------\n" );
            const int   Count = 8;
            
            for( int i = 0; i < Count; ++i )
            {
                auto* pJob = new count;
                pJob->setupLifeTime( lifetime::DELETE_WHEN_DONE );
                S.AddJobToQuantumWorld( *pJob );
            }
            
            // Make the main help
            job_base* pJob;
            while( (pJob = S.getJob()) && s_Count )
            {
                S.ProcessWorkPrimitive( pJob );
            }
            
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST1 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test job 2 - basic triggers
    //--------------------------------------------------------------------------------------
    namespace job2
    {
        class count : public job<1>
        {
        public:
            constexpr count(void) noexcept: job<1>{} {}
            constexpr count( definition D ) noexcept: job<1>{D} {}
        protected:
            void qt_onRun( void ) noexcept override
            {
                auto& S = xcore::get().m_Scheduler;
                for( int x = 0; x < 10; x++ )
                {
                    XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            auto& S = xcore::get().m_Scheduler;
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST2-----------------------------\n" );
            const int           Count = 4;
            count               Jobs1[Count];
            count               Jobs2[Count];
            trigger<4>          Trigger;
            trigger<4>          WaitingTrigger;
            
            WaitingTrigger.DontTriggerUntilReady();
            
            for( int i = 0; i < Count; ++i )
                Trigger.JobWillNotifyMe( Jobs1[i] );
            
            for( int i = 0; i < Count; ++i )
            {
                Trigger.AddJobToBeTrigger( Jobs2[i] );
                WaitingTrigger.JobWillNotifyMe( Jobs2[i] );
            }
            
            for( int i = 0; i < Count; ++i )
                S.AddJobToQuantumWorld( Jobs1[i] );
            
            // Make the main help
            WaitingTrigger.join();
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST2 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test job 3 - basic triggers with dynamic memory allocation
    //--------------------------------------------------------------------------------------
    namespace job3
    {
        class count : public job<1>
        {
        public:
            count( definition D ) : job<1>{D} {}
            void qt_onRun( void ) noexcept override
            {
                auto& S = xcore::get().m_Scheduler;
                for( int x = 0; x < 10; x++ )
                {
                    XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
                    for( int i = 0; i < LOOP_COUNT; i++ );
                }
            }
        };
        
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            auto& S = xcore::get().m_Scheduler;
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST3-----------------------------\n" );

            using mytrigger = trigger<4>;
            const int       Count           = 4;
            auto            pTrigger        = new mytrigger { lifetime::DELETE_WHEN_DONE };
            auto            pWaitingTrigger = new mytrigger { lifetime::DELETE_WHEN_DONE };
            
            pWaitingTrigger->DontTriggerUntilReady();
            pTrigger->DontTriggerUntilReady();
            
            for( int i = 0; i < Count; ++i )
            {
                count* pJob2 = new count{ definition::Flags( lifetime::DELETE_WHEN_DONE ) };
                pTrigger->AddJobToBeTrigger( *pJob2 );
                pWaitingTrigger->JobWillNotifyMe( *pJob2 );
            }
            
            for( int i = 0; i < Count; ++i )
            {
                count* pJob1 = new count{ definition::Flags( lifetime::DELETE_WHEN_DONE ) };
                pTrigger->JobWillNotifyMe( *pJob1 );
                S.AddJobToQuantumWorld( *pJob1 );
            }
            
            pTrigger->DoTriggerWhenReady();
            
            // After this function returns all pointers may be gone
            pWaitingTrigger->join();
            
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST3 DONE-----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test job 4 - basic job block
    //--------------------------------------------------------------------------------------
    namespace job4
    {
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            auto& S = xcore::get().m_Scheduler;
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST4-----------------------------\n" );
            const int       Count = 16;
            channel         JobChannel(xconst_universal_str("SchedulerTest4"));
            
            for( int i = 0; i < Count; ++i )
            {
                auto& S = xcore::get().m_Scheduler;
                JobChannel.SubmitJob( []()
                {
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                       for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            JobChannel.join();
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST4 DONE----------------------------\n" );
        }
    };
    
    //--------------------------------------------------------------------------------------
    // Test job 5 - multiple job blocks
    //--------------------------------------------------------------------------------------
    namespace job5
    {
        //--------------------------------------------------------------------------------------
        
        void Test( void )
        {
            auto& S = xcore::get().m_Scheduler;
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST5-----------------------------\n" );
            const int       Count = 16;
            channel         JobChannel1(xconst_universal_str("SchedulerTest5-Steam0"));
            channel         JobChannel2(xconst_universal_str("SchedulerTest4-Steam1"));
            
            // assumes this can work in concurrency with job APPLES but not with ORANGES are a dependency
            for( int i = 0; i < Count; ++i )
            {
                JobChannel1.SubmitJob( []()
                {
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN BANANAS %c \n", char( 'A' + x ) );
                        for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            // assumes this can work in concurrency with job BANANAS and ORANGES
            for( int i = 0; i < Count; ++i )
            {
                JobChannel2.SubmitJob( []()
                {
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN APPLES %c \n", char( 'A' + x ) );
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
                    auto& S = xcore::get().m_Scheduler;
                    for( int x = 0; x < 5; x++ )
                    {
                        XLOG_CHANNEL_INFO( *S.m_LogChannel, "WORKING IN ORANGES %c \n", char( 'A' + x ) );
                        for( int i = 0; i < LOOP_COUNT; i++ );
                    }
                } );
            }
            
            JobChannel1.join();
            JobChannel2.join();
            
            XLOG_CHANNEL_INFO( *S.m_LogChannel, "----------------------------TEST5 DONE----------------------------\n" );
        }
    };
    
    void Test( void )
    {
        job1::Test();
      //  job2::Test();
      //  job3::Test();
      //  job4::Test();
      //  job5::Test();
    }
}