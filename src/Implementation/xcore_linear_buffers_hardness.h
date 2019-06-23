


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
