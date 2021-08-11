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
        unique_span( unique_span&& A ) noexcept
            : parent( A.data(), A.size() )
        {
            new( &A ) xcore::span( (value_type*)nullptr, parent::size_type(0) );
        }

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
        struct unique_span 
        {
            using value_type        = T;
            using size_type         = T_COUNTER;
            using iterator          = value_type*;
            using reverse_iterator  = std::reverse_iterator<iterator>;

            unique_span(const unique_span&) = delete;
            constexpr unique_span() = default;

            value_type*     m_pData{nullptr};
            size_type       m_Count{0};

            //--------------------------------------------------------------------------------------------
            unique_span& operator = ( unique_span&& X ) noexcept
            {
                if( false == empty() )
                {
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                }

                *this = X;
                X.m_pData = nullptr;
                X.m_Count = 0;
                return *this;
            }

            //--------------------------------------------------------------------------------------------
            constexpr bool empty( void ) const noexcept { return !m_Count; }

            //--------------------------------------------------------------------------------------------
            constexpr    value_type* data( void ) const noexcept { return m_pData; }
            xforceinline value_type* data( void )       noexcept { return m_pData; }

            //--------------------------------------------------------------------------------------------
            unique_span& operator = ( const unique_span& X ) noexcept
            {
                if( false == empty() )
                {
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                }

                #if _XCORE_COMPILER_VISUAL_STUDIO
                    m_pData = reinterpret_cast<value_type*>( _aligned_malloc( sizeof(value_type)*X.size(), std::alignment_of_v<value_type> ) );
                #else
                    m_pData = reinterpret_cast<value_type*>( std::aligned_alloc( std::alignment_of_v<value_type>, sizeof(value_type)*X.size() ) );
                #endif

                m_Count = X.m_Count;
                memcpy( m_pData, X.m_pData, X.getByteSize() );

                return *this;
            }


            //--------------------------------------------------------------------------------------------

            inline ~unique_span( void ) noexcept 
            { 
                if( false == empty() )
                {
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                }
            }

            //--------------------------------------------------------------------------------------------

            template< typename...T_ARGS > inline
            xcore::err malloc( size_type Count ) noexcept
            {
                err Error;
                xassert(Count>=0);

                if( false == empty() )
                {
                    if( Count == m_Count ) return Error;
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                }

                #if _XCORE_COMPILER_VISUAL_STUDIO
                    m_pData = reinterpret_cast<value_type*>( _aligned_malloc( sizeof(value_type)*Count, std::alignment_of_v<value_type> ) );
                #else
                    m_pData = reinterpret_cast<value_type*>( std::aligned_alloc( std::alignment_of_v<value_type>, sizeof(value_type)*Count ) );
                #endif
                if( m_pData == nullptr ) 
                {
                    new( this ) xcore::span<value_type>();
                    return xerr_code( Error, error_state::OUT_OF_MEMORY, "Memory Allocation Failure" );
                }
                m_Count = Count;
                return Error;
            }

            //--------------------------------------------------------------------------------------------
            xcore::err resize( size_type Count ) noexcept 
            {
                xassert( Count > 0 );
                err     Error;

                if( false == empty() && Count == m_Count ) return Error;

                #if _XCORE_COMPILER_VISUAL_STUDIO
                    auto p = reinterpret_cast<value_type*>( _aligned_malloc( sizeof(value_type)*Count, std::alignment_of_v<value_type> ) );
                #else
                    auto p = reinterpret_cast<value_type*>( std::aligned_alloc( std::alignment_of_v<value_type>, sizeof(value_type)*Count ) );
                #endif
                if( m_pData == nullptr ) 
                    return xerr_code( Error, error_state::OUT_OF_MEMORY, "Resize Failure" );

                if( false == empty() )
                {
                    xassert( m_Count > 0 );
                
                    // Move all entries to new buffer
                    const auto count = std::min( m_Count, Count );
                    std::memcpy( p, m_pData, count*sizeof(value_type) );
            
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                }

                m_pData = p;
                m_Count = Count;

                return Error;
            }

            //--------------------------------------------------------------------------------------------
            void clear  ( void ) noexcept 
            { 
                if( false == empty() )
                {
                    #if _XCORE_COMPILER_VISUAL_STUDIO
                        _aligned_free( m_pData );
                    #else
                        free( m_pData );
                    #endif
                    m_pData = nullptr;
                    m_Count = 0;
                }
            }

            //--------------------------------------------------------------------------------------------
            template< typename T = std::size_t >
            constexpr T size( void ) const noexcept 
            {
                return static_cast<T>(m_Count);
            }

            //--------------------------------------------------------------------------------------------
            constexpr iterator                  begin               (void)          const noexcept { return data(); }
            constexpr iterator                  end                 (void)          const noexcept { return data() + size(); }
            constexpr const iterator            cbegin              (void)          const noexcept { return begin(); }
            constexpr const iterator            cend                (void)          const noexcept { return end(); }
            constexpr reverse_iterator          rbegin              (void)          const noexcept { return reverse_iterator(end());}
            constexpr reverse_iterator          rend                (void)          const noexcept { return reverse_iterator(begin());}
            constexpr const reverse_iterator    crbegin             (void)          const noexcept { return (const reverse_iterator)(cend());}
            constexpr const reverse_iterator    crend               (void)          const noexcept { return (const reverse_iterator)(cbegin());}
            constexpr value_type&               operator[]          (size_type idx) const noexcept
            {
                xassume(idx < size());
                return *(data() + idx);
            }
            xforceinline value_type&            operator[]          (size_type idx)       noexcept
            {
                xassume(idx < size());
                return *(data() + idx);
            }

            constexpr value_type& front() const noexcept
            {
                xassume(!empty());
                return *data();
            }

            constexpr value_type& back() const noexcept
            {
                xassume(!empty());
                return *(data() + (size() - 1));
            }

            // More of the interface is found here
            #include "Implementation/xcore_linear_buffers_hardness.h"
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