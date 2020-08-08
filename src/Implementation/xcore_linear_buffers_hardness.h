

//------------------------------------------------------------------------------
public: template< typename T = size_type >  constexpr
auto getIndexByEntry ( const value_type& Entry ) const noexcept
{ 
    xassert( isIndexValid( static_cast<std::size_t>(&Entry - data()) ) ); 
    return static_cast<T>( &Entry - data() ); 
}

//------------------------------------------------------------------------------
// Safely copies items from a view to destination of this buffer      
public: inline
auto& CopyFromTo( const xcore::span<value_type>     View,
                  const size_type                   DestinationOffset = static_cast<size_type>(0) ) noexcept
{
    if( View.size() ) 
    {
        xassert( (DestinationOffset + View.size()) <= size() );
        std::memmove( &(*this)[DestinationOffset], View.data(), sizeof(value_type)*size() );
    }
    return *this;
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromTo( T iStart, T iEnd ) noexcept
{
    static_assert( std::is_integral_v<T> );
    xassert( iStart >= 0 ); //-V503 //-V547
    xassert( iEnd   <= size() );
    xassert(iStart  < iEnd);
    return xcore::span{ &(*this)[ iStart ], static_cast<std::size_t>(iEnd - iStart) };
}

        //------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromTo( T iStart, T iEnd ) const noexcept
{
    static_assert( std::is_integral_v<T> );
    xassert( iStart >= 0 ); //-V503 //-V547
    xassert( iEnd   <= size() );
    xassert( iStart < iEnd );
    return xcore::span{ &(*this)[ iStart ], static_cast<std::size_t>(iEnd - iStart) };
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromCount( T iStart, T Count ) noexcept
{
    static_assert( std::is_integral_v<T> );
    xassert( iStart >= 0 ); //-V503 //-V547
    xassert( iStart + Count <= size() );
    xassert( Count >= 0 );
    return xcore::span{ &(*this)[ iStart ], static_cast<std::size_t>(Count) };
}

      //------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromCount(T iStart, T Count) const noexcept
{
    static_assert(std::is_integral_v<T>);
    xassert(iStart >= 0); //-V503 //-V547
    xassert(iStart + Count <= size());
    xassert(Count >= 0);
    return xcore::span{ &(*this)[iStart], static_cast<std::size_t>(Count) };
}

//------------------------------------------------------------------------------
public: template< typename T > inline     
auto ViewFrom( T iStart ) noexcept
{
    static_assert( std::is_integral_v<T> );
    const auto s = size();
    xassert( iStart >= 0 ); //-V503 //-V547
    xassert( iStart <= s );
    return xcore::span{ &(*this)[ iStart ], s - static_cast<std::size_t>(iStart) };
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewTo( T iEnd )
{
    static_assert( std::is_integral_v<T> );
    xassert( iEnd <= size() );
    return xcore::span{ data(), iEnd };
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewTo(T iEnd) const
{
    static_assert( std::is_integral_v<T> );
    xassert(iEnd <= size());
    return xcore::span{ data(), iEnd };
}

//------------------------------------------------------------------------------
public: inline
auto View( void ) noexcept
{
    xassert( size() );
    return xcore::span{ data(), size() };
}

//------------------------------------------------------------------------------
public: constexpr
const auto View( void ) const noexcept
{
    xassert( size() );
    return xcore::span{ data(), size() };
}

//------------------------------------------------------------------------------
public: constexpr 
bool Belongs( const void* pPtr )    const   noexcept
{ 
    return pPtr >= static_cast<const void*>(data()) && 
           pPtr <  static_cast<const void*>(data() + size() ); 
}

//------------------------------------------------------------------------------
public: constexpr   
auto getByteSize ( void ) const noexcept
{ 
    return sizeof(value_type) * size(); 
}

//------------------------------------------------------------------------------
public: constexpr   
auto& last ( void ) const noexcept
{ 
    xassert(size()>=1);
    return *(data() + (size()-1)); 
}

//------------------------------------------------------------------------------
public: inline
auto& last ( void ) noexcept
{ 
    xassert(size()>=1);
    return *(data() + (size()-1)); 
}

//------------------------------------------------------------------------------
public: constexpr   
auto& first ( void ) const noexcept
{ 
    xassert(size()>=1);
    return *data(); 
}

//------------------------------------------------------------------------------
public: inline
auto& first ( void ) noexcept
{ 
    xassert(size()>=1);
    return *data(); 
}

//------------------------------------------------------------------------------
public: constexpr
operator const span<const value_type> ( void ) const noexcept 
{ 
    xassume( data() ); 
    return { data(), size() }; 
}
