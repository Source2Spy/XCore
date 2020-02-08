#ifndef _XCORE_UNIQUE_SPAN_H
#define _XCORE_UNIQUE_SPAN_H

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
       typename T                           // Node to be allocated ( new T )
     , typename T_COUNTER = std::size_t     // type of the counter  ( T_COUNTER m_Count; ) 
    >
    struct unique_span : xcore::span<T>
    {
        using value_type = T;
        using size_type  = T_COUNTER;
        using parent     = xcore::span<T>;

        unique_span& operator=(const unique_span&) = delete;
        unique_span(const unique_span&) = delete;
        constexpr unique_span() = default;

        //--------------------------------------------------------------------------------------------

        inline ~unique_span( void ) noexcept;

        //--------------------------------------------------------------------------------------------
        template< typename...T_ARGS > inline
        unique_span(size_type Count, T_ARGS&&...Args) noexcept
            : parent( NewStatic(Count, std::forward<T_ARGS&&>(Args)... ), Count )
        {}

        //--------------------------------------------------------------------------------------------

        inline xcore::err Alloc( size_type Count ) noexcept;

        //--------------------------------------------------------------------------------------------

        template< typename...T_ARGS > 
        inline xcore::err New( size_type Count, T_ARGS&&...Args ) noexcept;

        //--------------------------------------------------------------------------------------------
        inline xcore::err resize( size_type Count ) noexcept;

        //--------------------------------------------------------------------------------------------
        inline void clear  ( void ) noexcept;

        //--------------------------------------------------------------------------------------------
        template< typename T = std::size_t >
        constexpr T size( void ) const noexcept 
        {
            return static_cast<T>(parent::size());
        }

    protected:

        template< typename...T_ARGS >
        inline static value_type* NewStatic  ( size_type Count, T_ARGS&&...Args ) noexcept;
        inline static value_type* AllocStatic( size_type Count ) noexcept;
    };

    //--------------------------------------------------------------------------------------------
    // RAW Versions
    //--------------------------------------------------------------------------------------------
    namespace raw
    {
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
            using parent     = xcore::span<T>;
        
            unique_span& operator=(const unique_span&) = delete;
            unique_span(const unique_span&) = delete;
            constexpr unique_span() = default;

            //--------------------------------------------------------------------------------------------

            ~unique_span( void ) noexcept 
            { 
                if( false == parent::empty() )
                {
                    auto p = parent::data();
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

                if( false == parent::empty() )
                {
                    auto op = parent::data();
                    if( Count == parent::size() ) return Error;
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( op );
                    #else
                        free( op );
                    #endif
                }

                #if _XCORE_COMPILER_VISUAL_STUDIO
                    auto p = reinterpret_cast<value_type*>( _aligned_malloc( sizeof(value_type)*Count, std::alignment_of_v<value_type> ) );
                #else
                    auto p = reinterpret_cast<value_type*>( std::aligned_alloc( std::alignment_of_v<value_type>, sizeof(value_type)*Count ) );
                #endif
                if( p == nullptr ) 
                {
                    new( this ) xcore::span<value_type>( nullptr, static_cast<std::size_t>( 0 ) );
                    return xerr_code( Error, error_state::OUT_OF_MEMORY, "Memory Allocation Failure" );
                }
                new( this ) xcore::span( p, Count );
                return Error;
            }

            //--------------------------------------------------------------------------------------------
            xcore::err resize( size_type Count ) noexcept 
            {
                xassert( Count > 0 );
                err     Error;

                if( false == parent::empty() && Count == parent::size() ) return Error;

                #if _XCORE_COMPILER_VISUAL_STUDIO
                    auto p = reinterpret_cast<value_type*>( _aligned_malloc( sizeof(value_type)*Count, std::alignment_of_v<value_type> ) );
                #else
                    auto p = reinterpret_cast<value_type*>( std::aligned_alloc( std::alignment_of_v<value_type>, sizeof(value_type)*Count ) );
                #endif
                if( p == nullptr ) 
                    return xerr_code( Error, error_state::OUT_OF_MEMORY, "Resize Failure" );

                if( false == parent::empty() )
                {
                    xassert( parent::size() > 0 );
                
                    auto op = parent::data();

                    // Move all entries to new buffer
                    const auto count = std::min( static_cast<size_type>(parent::size()), Count );
                    std::memcpy( p, op, count*sizeof(value_type) );
            
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
                if( false == parent::empty() )
                {
                    auto p = parent::data();
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( p );
                    #else
                        free( p );
                    #endif
                    p = nullptr;
                    new( this ) xcore::span( p, parent::index_type{0} );
                }
            }

            //--------------------------------------------------------------------------------------------
            template< typename T = std::size_t >
            constexpr T size( void ) const noexcept 
            {
                return static_cast<T>(parent::size());
            }
        };
    }
}

//--------------------------------------------------------------------------------------------
// Shortcuts
//--------------------------------------------------------------------------------------------
namespace xcore
{
    template< typename T, typename T_COUNTER = std::size_t>
    using unique_span = xcore::allocator::unique_span<T,T_COUNTER>;

    template< typename T, typename T_COUNTER = std::size_t>
    using unique_rawspan = xcore::allocator::raw::unique_span<T,T_COUNTER>;
}

#endif