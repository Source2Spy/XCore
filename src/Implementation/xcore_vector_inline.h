
namespace xcore::containers::vector
{
    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::clear ( void ) noexcept
    {
        if constexpr( false == std::is_trivially_destructible_v<T> )
        {
            if( empty() == false )
            {
                auto p = std::launder(reinterpret_cast<T*>(&m_Allocator[0]));
                std::destroy( p, p + m_Count );
            }
        }
        m_Count = 0;
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::reserve ( size_type NewCap ) noexcept
    {
        GrowBy( NewCap - m_Count ).CheckError();
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    xcore::err type<T, T_ALLOCATOR>::GrowBy( size_type NewItems ) noexcept
    {
        xcore::err Error;
        if( NewItems == 0 )
            return Error;

        //
        // Allocate the new count
        //
        const auto CurCount = m_Allocator.size();
        if( CurCount == 0 )
           return m_Allocator.malloc( NewItems );

        const auto NewCapacity = m_Count + NewItems;
        return m_Allocator.resize( NewCapacity );
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::insert( const size_type Index, const size_type Count ) noexcept
    {
        xassert( Index >= 0 );
        xassert( Index <= m_Count );
    
        //
        // Make sure that we have space
        //
        xassert( (m_Count + Count) < std::numeric_limits<size_type>::max() );

        const size_type NewCount = m_Count + Count;
        if( NewCount > m_Allocator.size() )
            GrowBy( std::max( NewCount/2, Count + DEFAULT_GROWTH ) ).CheckError();

        //
        // Okay lets move all the elements
        //
        const auto ItemsToMove       = m_Count - Index; 
        const auto NewPosOldItems    = Index   + Count; 
        if( ItemsToMove > 0 ) m_Allocator.CopyFromTo( xcore::span( &m_Allocator[Index], ItemsToMove ), NewPosOldItems );
    
        //
        // Lets construct
        //
        if constexpr( std::is_trivially_constructible_v<T> == false )
        {
            for( size_type i = Index; i<NewPosOldItems; ++i )
                new( &m_Allocator[i] ) value_type{};
        }
    
        //
        // Finally set the new count
        //
        m_Count = NewCount;
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::insert( const size_type Index, const view View ) noexcept
    {
        xassert( Index >= 0 );
        xassert( Index <= m_Count );
    
        //
        // Make sure that we have space
        //
        xassert( static_cast<std::uint64_t>(m_Count + View.size()) < std::numeric_limits<size_type>::max() );

        const auto NewCount = m_Count + View.size();
        if( NewCount > m_Allocator.size() )
            GrowBy( std::max( NewCount/2, View.size() + DEFAULT_GROWTH ) ).CheckError();
        
        //
        // Okay lets move all the elements
        //
        const auto ItemsToMove       = m_Count - Index; 
        const auto NewPosOldItems    = Index   + View.size();
        if( ItemsToMove > 0 ) m_Allocator.CopyToFrom( view( &m_Allocator[NewPosOldItems], ItemsToMove ), Index );
    
        //
        // Lets copy construct all the new items
        //
        if constexpr( std::is_trivially_constructible_v<T> == false )
        {
            size_type i=0;
            for( const auto& Entry : View )
                new( &m_Allocator[Index+i] ) value_type{ Entry }, i++; //-V521
        }
        else
        {
            size_type i=0;
            for( const auto& Entry : View )
                m_Allocator[Index+i] = Entry, i++;
        }

        //
        // Finally set the new count
        //
        m_Count = NewCount;
    }

    //---------------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR >
    template<typename ...T_ARG> inline
    void type<T, T_ALLOCATOR>::appendList( const size_type Count, T_ARG&&...Args ) noexcept
    {
        //
        // Make sure that we have memory
        //
        if( (m_Count + Count) > m_Allocator.size<size_type>() )
            if( auto Error = GrowBy( std::max( (m_Allocator.size() + Count)/2, Count + DEFAULT_GROWTH ) ); Error )
            {
                // This should fail here
            }

        //
        // Construct all of them
        //
        const auto FinalCount = m_Count + Count; 
        if constexpr( std::is_trivially_constructible_v<T,T_ARG&&...> == false || sizeof...(T_ARG))
        {
            for( auto i=m_Count; i<FinalCount; i++ )
                new( &m_Allocator[i] ) value_type{ std::forward<T_ARG&&>(Args)... };
        }
    
        // Set the new count and return
        m_Count = FinalCount;
    }

    //---------------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::appendList( const view View ) noexcept
    {
        const auto Index = size();
    
        //
        // Make sure that we have memory
        //
        if( (m_Count + View.size()) > Index )
            if( auto Error = GrowBy( std::max( (size()+View.size())/2, View.size() + DEFAULT_GROWTH )); Error )
            {
                // This should fail here
            }

        //
        // Construct all of them
        //
        if constexpr ( std::is_trivially_constructible_v<T> == false )
        {
            size_type i=0;
            for( const auto& Entry : View )
                new( &m_Allocator[Index+i] ) value_type{ Entry }, i++; //-V521
        }
        else
        {
            size_type i=0;
            for( const auto& Entry : View )
                m_Allocator[Index+i] = { Entry }, i++;
        }

        // Set the new count and return
        m_Count += View.size();
    }

    //---------------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::eraseCollapse( const size_type Index, const size_type Count ) noexcept
    {
        if( m_Count <= 0)
            return;
    
        xassert( Index >= 0 );
        xassert( (Index+Count) <= m_Count );
    
        //
        // Lets free all these guys
        //
        {
            auto p = std::launder(&m_Allocator[Index]); 
            std::destroy( p, p+Count );
        }        
    
        //
        // Okay if there is any full node lets move him down
        //
        const size_type LastMovedIndex = (Index+Count);
        const size_type ItemsToMove    = m_Count - LastMovedIndex;

        xassert( ItemsToMove <= m_Count );
        if( ItemsToMove > 0 ) m_Allocator.CopyFromTo( ViewFromCount( LastMovedIndex, ItemsToMove ), Index );
    
        //
        // update the count
        //
        m_Count -= Count;
    }

    //------------------------------------------------------------------------------
    template< typename T, typename T_ALLOCATOR > inline
    void type<T, T_ALLOCATOR>::eraseSwap( const size_type Index, const size_type Count ) noexcept
    {
        if( m_Count <= 0)
            return;
    
        xassert( Index >= 0 );
        xassert( (Index+Count) <= m_Count );

        //
        // Lets free all these guys
        //
        {
            auto p = std::launder(&m_Allocator[Index]); 
            std::destroy( p, p+Count );
        }
    
        //
        // Copy all the nodes from the end to where we are
        //
        if( (Index + Count) < m_Count )
        {
            const auto iSwap = m_Count - 1;
            for( size_type i = 0; i<Count; i++ )
            {
                m_Allocator[ Index + i ] = m_Allocator[ iSwap - i ];
            }
        }
    
        //
        // update the count
        //
        m_Count -= Count;
    }


}

