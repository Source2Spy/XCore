

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
    xassert( iStart >= 0 );
    xassert( iEnd   <= size() );
    xassert( iStart <= iEnd );
    return xcore::span{ &(*this)[ iStart ], iEnd - iStart };
}

        //------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromTo( T iStart, T iEnd ) const noexcept
{
    xassert( iStart >= 0 );
    xassert( iEnd   <= size() );
    xassert( iStart <= iEnd );
    return xcore::span{ &(*this)[ iStart ], iEnd - iStart };
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewFromCount( T iStart, T Count ) noexcept
{
    xassert( iStart >= 0 );
    xassert( iStart + Count <= size() );
    xassert( Count > 0 );
    return xcore::span{ &(*this)[ iStart ], Count };
}

//------------------------------------------------------------------------------
public: template< typename T > inline     
auto ViewFrom( T iStart ) noexcept
{
    const auto s = size();
    xassert( iStart >= 0 );
    xassert( iStart <= s );
    return xcore::span{ &(*this)[ iStart ], std::ptrdiff_t(s - static_cast<std::size_t>(iStart)) };
}

//------------------------------------------------------------------------------
public: template< typename T > inline
auto ViewTo( T iEnd )
{
    xassert( iEnd <= size() );
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
