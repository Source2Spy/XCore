#ifndef _XCORE_SCHEDULER_CHANNEL_H
#define _XCORE_SCHEDULER_CHANNEL_H
#pragma once

namespace xcore::scheduler
{
    namespace details
    {
        struct lambda_job;
    }

    //------------------------------------------------------------------------------
    // Description:
    //      A block of jobs very handy for doing inline jobs with lambdas
    //------------------------------------------------------------------------------
    class channel final : public trigger_base
    {
    public:
        xcore_rtti( channel, trigger_base )

        inline                  channel         ( xcore::string::const_universal Name, int MaxSimultaneousJobs = -1, scheduler::system& S = xcore::get().m_Scheduler ) noexcept;
        inline      void        setupPriority   ( priority Priority ) noexcept { m_Priority = Priority; }
        inline      void        setupAfinity    ( affinity Affinity ) noexcept { m_Affinity = Affinity; }
        virtual                ~channel         ( void )                                    noexcept;
        template< typename T >
        inline      void        SubmitJob       ( T&& func )                                noexcept;
        inline      void        join            ( void )                                    noexcept;
        inline      auto&       getScheduler    ( void )                                    noexcept { return m_Scheduler; }

        template< typename T_CONTAINER, typename T_LAMBDA > inline
        std::enable_if_t
        < 
              xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( typename T_CONTAINER::value_type& ) >
            , void 
        >
        ForeachLog         ( T_CONTAINER& Container, const std::size_t Diviser, const std::size_t Cutoff, T_LAMBDA&& func ) noexcept;

        template< typename T_CONTAINER, typename T_LAMBDA > inline
        std::enable_if_t
        < 
              xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( xcore::span<typename T_CONTAINER::value_type> ) >
            , void 
        >
        ForeachLog         ( T_CONTAINER& Container, const std::size_t Diviser, const std::size_t Cutoff, T_LAMBDA&& func ) noexcept;

        template< typename T_CONTAINER, typename T_LAMBDA > inline
        std::enable_if_t
        < 
              xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( typename T_CONTAINER::value_type& ) >
            , void 
        >
        ForeachFlat        ( T_CONTAINER& Container, const std::size_t Diviser, T_LAMBDA&& func ) noexcept;

        template< typename T_CONTAINER, typename T_LAMBDA > inline
        std::enable_if_t
        < 
              xcore::function::is_lambda_signature_same_v< T_LAMBDA, void( xcore::span<typename T_CONTAINER::value_type> ) >
            , void 
        >        
        ForeachFlat        ( T_CONTAINER& Container, const std::size_t Diviser, T_LAMBDA&& func ) noexcept;

    protected:

        enum state : std::uint8_t
        { 
                STATE_PROCESS_STATE_NOT_READY
            ,   STATE_PROCESS_STATE_READY
            ,   STATE_PROCESS_STATE_DONE
        };

    protected:
    
        virtual void    qt_onNotify     ( void )            noexcept override;
        virtual void    qt_onTriggered  ( void )            noexcept override;
        virtual int     getJobCount     ( void )    const   noexcept override { return 0; }
 

    protected:
    
        xcore::scheduler::system&                                           m_Scheduler;
        std::atomic<state>                                                  m_ProcessState  {};
        xcore::lockless::pool::mpmc_bounded_dynamic<details::lambda_job>&   m_Pool;
        const int                                                           m_nMaxJobs;
        priority                                                            m_Priority = priority::NORMAL;
        affinity                                                            m_Affinity = affinity::NORMAL;
        const xcore::string::const_universal                                m_Name;

        friend struct scheduler::details::lambda_job;
    };

    namespace details
    {
        struct lambda_job final : public job<0>
        {
            constexpr       lambda_job      ( void ) noexcept : job<0>{ definition::Flags( lifetime::DONT_DELETE_WHEN_DONE, jobtype::LIGHT, affinity::NORMAL, priority::NORMAL )} {}
            virtual void    qt_onRun        ( void ) noexcept override { m_Function(); }
            virtual void    qt_onDone       ( void ) noexcept override
            {
                // backup the pointer since after we free the node we can not access any members
                // since anything could have changed.
                channel* const m_pTempJobBlock = m_pJobBlock;

                // Free the job by Pushing the job back into the pool
                m_pJobBlock->m_Pool.push(*this);

                // Now the job block is free to delete the class
                NotifyTrigger(*m_pTempJobBlock);
                /////
                // 
                // DEAD SPACE
                //
                /////
            }

            channel*                        m_pJobBlock {};
            xcore::func<void(void)>         m_Function  {};
        };
    }

}
#endif