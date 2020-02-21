namespace xcore::allocator
{

    //--------------------------------------------------------------------------------------------

    template < typename T, typename T_COUNTER > inline
    unique_span<T,T_COUNTER>::~unique_span( void ) noexcept 
    { 
        clear();
    }

    //--------------------------------------------------------------------------------------------

    template < typename T, typename T_COUNTER > inline
    typename unique_span<T,T_COUNTER>::value_type* unique_span<T,T_COUNTER>::AllocStatic( size_type Count ) noexcept
    {
        xassert(Count>=0);
        return reinterpret_cast<value_type*>
        (
            xcore::memory::AlignedMalloc
            ( 
                  units::bytes{ static_cast<std::int64_t>(sizeof(value_type)*Count) }
                , std::alignment_of_v<value_type>
            )
        );
    }

    //--------------------------------------------------------------------------------------------

    template < typename T, typename T_COUNTER >
    template< typename...T_ARGS > inline
    typename unique_span<T,T_COUNTER>::value_type* unique_span<T,T_COUNTER>::NewStatic( size_type Count, T_ARGS&&...Args ) noexcept
    {
        XCORE_PERF_ZONE_SCOPED()
        auto p = AllocStatic(Count);

        if( p )
        {
            if constexpr ( false == std::is_trivially_constructible_v<value_type> || sizeof...(Args) )
            {
                xcore::span<value_type> Span{p,Count};
                if( Count > 5000 )
                {
                    xcore::scheduler::channel Channel( xconst_universal_str("xcore::unique_span::NewStatic") );
                    Channel.ForeachLog( Span, 8, 1000, [&]( xcore::span<value_type> View )
                    {
                        XCORE_PERF_ZONE_SCOPED_N("xcore::unique_span::NewStatic::Constructing")

                        // Call constructors
                        if constexpr (sizeof...(Args))
                        {
                            for( auto& E : View ) new(&E) value_type{ std::forward<T_ARGS&&>(Args)... };
                        }
                        else
                        {
                            for( auto& E : View ) new(&E) value_type;
                        }
                    });
                    Channel.join();
                }
                else
                {
                    if constexpr (sizeof...(Args))
                    {
                        for( auto& E : Span ) new(&E) value_type{ std::forward<T_ARGS&&>(Args)... };
                    }
                    else
                    {
                        for( auto& E : Span ) new(&E) value_type;
                    }
                }
            }
        }

        return p;
    }


    //--------------------------------------------------------------------------------------------

    template < typename T, typename T_COUNTER > inline
    xcore::err unique_span<T,T_COUNTER>::Alloc( size_type Count ) noexcept
    {
        xassert(Count>=0);
        clear();

        auto p = AllocStatic(Count);
        if( p == nullptr ) return xerr_code_s( error_state::OUT_OF_MEMORY, "xcore::unique_span::Alloc Failure" );

        // Update the class
        new( this ) xcore::span( p, Count );

        return {};
    }

    //--------------------------------------------------------------------------------------------
    template < typename T, typename T_COUNTER >
    template< typename...T_ARGS > inline
    xcore::err unique_span<T,T_COUNTER>::New( size_type Count, T_ARGS&&...Args ) noexcept
    {
        xassert(Count>=0);
        clear();

        auto p = NewStatic(Count, std::forward<T_ARGS&&>(Args)... );
        if( p == nullptr ) return xerr_code_s( error_state::OUT_OF_MEMORY, "xcore::unique_span::New Failure" );

        // Update the class
        new( this ) xcore::span( p, Count );

        return {};
    }

    //--------------------------------------------------------------------------------------------
    template < typename T, typename T_COUNTER > inline
    xcore::err unique_span<T,T_COUNTER>::resize( size_type Count ) noexcept 
    {
        XCORE_PERF_ZONE_SCOPED()
        xassert( Count > 0 );

        if( false == parent::empty() && Count == parent::size() ) return {};

        auto p = AllocStatic(Count);
        if( p == nullptr ) return xerr_code_s( error_state::OUT_OF_MEMORY, "xcore::unique_span::resize Failure" );

        if( false == parent::empty() )
        {
            auto op = parent::data();

            // Move all entries to new buffer
            const auto count = std::min( static_cast<size_type>(parent::size()), Count );
            std::memcpy( p, op, count*sizeof(value_type) );

            if constexpr (false == std::is_trivially_destructible_v<value_type>)
            {
                if( parent::size() > Count )
                {
                    for( auto& E : ViewFrom(Count) ) std::destroy_at(&E);
                }
            }

            const auto OldCount = size();
            new( this ) xcore::span( p, Count );

            if constexpr (false == std::is_trivially_constructible_v<value_type>)
            {
                if( Count > OldCount )
                {
                    for( auto& E : ViewFrom(OldCount) ) new (&E) value_type{};
                }
            }
            
            xcore::memory::AlignedFree(op);
        }
        else
        {
            new( this ) xcore::span( p, Count );
        }
            
        return {};
    }

    //--------------------------------------------------------------------------------------------
    template < typename T, typename T_COUNTER > inline
    void unique_span<T,T_COUNTER>::clear  ( void ) noexcept 
    { 
        XCORE_PERF_ZONE_SCOPED()
        if( false == parent::empty() )
        {
            auto p = parent::data();
            if constexpr (false == std::is_trivially_destructible_v<value_type>)
            {
                if( size() > 5000 )
                {
                    xcore::scheduler::channel Channel( xconst_universal_str("xcore::unique_span::clear") );
                    Channel.ForeachLog( *this, 8, 1000, [&]( xcore::span<value_type> View )
                    {
                        XCORE_PERF_ZONE_SCOPED_N("xcore::unique_span::NewStatic::Destructing")

                        // Call constructors 
                        for( auto& E : View ) std::destroy_at(&E);
                    });
                    Channel.join();
                }
                else
                {
                    for( auto& E : *this ) std::destroy_at(&E);
                }
            }

            xcore::memory::AlignedFree(p);
            p = nullptr;
            new( this ) xcore::span( p, std::size_t{0} );
        }
    }
}
