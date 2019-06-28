
#ifndef _XCORE_UNITE_Q_H
#define _XCORE_UNITE_Q_H

namespace xcore::allocator
{
    //--------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------
    enum class error_state : std::uint32_t
    {
          GUID      = 0xbadcdcd0
        , OK        = 0
        , FAILURE
        , OUT_OF_MEMORY
    };

    //--------------------------------------------------------------------------------------------
    // unique_span
    //--------------------------------------------------------------------------------------------
    template
    < 
       typename T               // Node to be allocated ( new T )
     , typename T_COUNTER       // type of the counter  ( T_COUNTER m_Count; ) 
    >
    struct unique_span : xcore::span<T>
    {
        using value_type = T;
        using size_type  = T_COUNTER;
        using entry      = std::remove_pointer_t<std::remove_reference_t<T>>;
        using parent     = xcore::span<T>;

        unique_span& operator=(const unique_span&) = delete;
        unique_span(const unique_span&) = delete;
        constexpr unique_span() = default;

        //--------------------------------------------------------------------------------------------

        template< typename...T_ARGS > inline
        xcore::err New( size_type Count, T_ARGS&&...Args ) noexcept
        {
            xassert(Count>=0);
            err Error;
            auto p = new entry[Count]{ std::forward<T_ARGS>(Args)... };
            if( p == nullptr ) return xerr_code( Error, error_state::OUT_OF_MEMORY, "Memory Allocation Failure" );
            clear();
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        xcore::err Resize( size_type Count ) noexcept 
        {
            xassert( Count >= 0 );
            err     Error;
            auto    op      = parent::data();
            auto    p       = new entry[ Count ];
            if( p == nullptr ) return xerr_code( Error, error_state::OUT_OF_MEMORY, "Resize Failure" );
            for( decltype(parent::size()) i = 0; i<parent::size(); ++i )
                p[i] = std::move(op[i]);
            clear();
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        void clear  ( void )        noexcept 
        { 
            auto p = parent::data();
            if( p ) delete []p;
            p = nullptr;
            new( this ) xcore::span( p, parent::index_type{0} );
        }
    };
}


namespace xcore
{
    template< typename T, typename T_COUNTER = std::size_t>
    using unique_span = xcore::allocator::unique_span<T,T_COUNTER>;
}

#endif