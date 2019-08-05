
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

        ~unique_span( void ) noexcept 
        { 
            auto p = parent::data();
            if( p ) delete []p;
        }

        //--------------------------------------------------------------------------------------------

        template< typename...T_ARGS > inline
        xcore::err New( size_type Count, T_ARGS&&...Args ) noexcept
        {
            xassert(Count>=0);
            err Error;
            auto op = parent::data();
            if( op ) delete []op;

            auto p = new entry[Count]{ std::forward<T_ARGS>(Args)... };
            if( p == nullptr ) return xerr_code( Error, error_state::OUT_OF_MEMORY, "Memory Allocation Failure" );
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        xcore::err resize( size_type Count ) noexcept 
        {
            xassert( Count > 0 );
            err     Error;

            auto    p       = new entry[ Count ];
            if( p == nullptr ) 
            {
                delete []p;
                return xerr_code( Error, error_state::OUT_OF_MEMORY, "Resize Failure" );
            }

            auto    op      = parent::data();
            if( op )
            {
                xassert( parent::size() > 0 );
                
                // Move all entries to new buffer
                const auto count = std::min( static_cast<size_type>(parent::size()), Count );
                std::memcpy( p, op, count*sizeof(entry) );
            
                delete []op;
            }
            
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        void clear  ( void ) noexcept 
        { 
            auto p = parent::data();
            if( p ) delete []p;
            p = nullptr;
            new( this ) xcore::span( p, parent::index_type{0} );
        }

        //--------------------------------------------------------------------------------------------
    };

    //--------------------------------------------------------------------------------------------
    // unique_span
    //--------------------------------------------------------------------------------------------
    template
    < 
       typename T               // Node to be allocated ( new T )
     , typename T_COUNTER       // type of the counter  ( T_COUNTER m_Count; ) 
    >
    struct unique_rawspan : xcore::span<T>
    {
        using value_type = T;
        using size_type  = T_COUNTER;
        using entry      = std::remove_pointer_t<std::remove_reference_t<T>>;
        using parent     = xcore::span<T>;
        
        unique_rawspan& operator=(const unique_rawspan&) = delete;
        unique_rawspan(const unique_rawspan&) = delete;
        constexpr unique_rawspan() = default;

        //--------------------------------------------------------------------------------------------

        ~unique_rawspan( void ) noexcept 
        { 
            auto p = parent::data();
            if( p )
            {
                #if _XCORE_COMPILER_VISUAL_STUDIO
                    _aligned_free( p );
                #else
                    free( p );
                #endif
            }
        }

        //--------------------------------------------------------------------------------------------

        template< typename...T_ARGS > inline
        xcore::err malloc( size_type Count ) noexcept
        {
            err Error;
            xassert(Count>=0);
            auto op = parent::data();

            if( op )
            {
                if( Count == parent::size() ) return Error;
                #if _XCORE_COMPILER_VISUAL_STUDIO
                    _aligned_free( op );
                #else
                    free( op );
                #endif
            }

            #if _XCORE_COMPILER_VISUAL_STUDIO
                auto p = reinterpret_cast<entry*>( _aligned_malloc( std::alignment_of_v<entry>, sizeof(entry)*Count ) );
            #else
                auto p = reinterpret_cast<entry*>( std::aligned_alloc( std::alignment_of_v<entry>, sizeof(entry)*Count ) );
            #endif
            if( p == nullptr ) return xerr_code( Error, error_state::OUT_OF_MEMORY, "Memory Allocation Failure" );
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        xcore::err resize( size_type Count ) noexcept 
        {
            xassert( Count > 0 );
            err     Error;

            auto op = parent::data();
            if( op )
            {
                if( Count == parent::size() ) return Error;
            }

            #if _XCORE_COMPILER_VISUAL_STUDIO
                auto p = reinterpret_cast<entry*>( _aligned_malloc( std::alignment_of_v<entry>, sizeof(entry)*Count ) );
            #else
                auto p = reinterpret_cast<entry*>( std::aligned_alloc( std::alignment_of_v<entry>, sizeof(entry)*Count ) );
            #endif
            if( p == nullptr ) 
                return xerr_code( Error, error_state::OUT_OF_MEMORY, "Resize Failure" );

            if( op )
            {
                xassert( parent::size() > 0 );
                
                // Move all entries to new buffer
                const auto count = std::min( static_cast<size_type>(parent::size()), Count );
                std::memcpy( p, op, count*sizeof(entry) );
            
                #if _XCORE_COMPILER_VISUAL_STUDIO
                    _aligned_free( op );
                #else
                    free( op );
                #endif
            }
            
            new( this ) xcore::span( p, Count );
            return Error;
        }

        //--------------------------------------------------------------------------------------------
        void clear  ( void ) noexcept 
        { 
            auto p = parent::data();
            if( p )
            {
                #if _XCORE_COMPILER_VISUAL_STUDIO
                    _aligned_free( p );
                #else
                    free( p );
                #endif
            }
            p = nullptr;
            new( this ) xcore::span( p, parent::index_type{0} );
        }
    };

}


namespace xcore
{
    template< typename T, typename T_COUNTER = std::size_t>
    using unique_span = xcore::allocator::unique_span<T,T_COUNTER>;

    template< typename T, typename T_COUNTER = std::size_t>
    using unique_rawspan = xcore::allocator::unique_rawspan<T,T_COUNTER>;
}

#endif