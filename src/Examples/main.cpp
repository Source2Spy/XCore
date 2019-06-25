
#include "xcore_examples.h"

#include <cstdlib>
#include <new>


//------------------------------------------------------------------------------------

namespace details
{
    template< class T, std::size_t T_MAX_SIZE >
    struct fixsize_allocator 
    {
        using           value_type = T;
        constexpr static bool  is_always_equal = true;
        constexpr static auto  size_type       = sizeof(T);

        fixsize_allocator() = default;

        template <class U> constexpr fixsize_allocator( const fixsize_allocator<U,T_MAX_SIZE>& ) noexcept {}

        [[nodiscard]] constexpr value_type* allocate( std::size_t n ) noexcept
        {
            assert( n == T_MAX_SIZE || n == 1 );
            return reinterpret_cast<value_type*>(m_Buffer);
        }

        constexpr void deallocate(value_type* p, std::size_t) noexcept { assert(p == reinterpret_cast<value_type*>(m_Buffer)); }

        xcore::containers::raw::entry<T> m_Buffer[T_MAX_SIZE];
    };
}

//------------------------------------------------------------------------------------

template< typename T, std::size_t T_MAX_SIZE >
struct fix_vector : std::vector< T, xcore::types::container_static_to_dynamic<T,details::fixsize_allocator, T_MAX_SIZE > >
{
    using parent = std::vector< T, xcore::types::container_static_to_dynamic<T,details::fixsize_allocator, T_MAX_SIZE > >;
    fix_vector( void ) : parent{} { parent::reserve(T_MAX_SIZE); }
    using parent::operator [];
    using parent::operator =;
};

//------------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------------
int main(void)
{
    xcore::Init("TestApp");
  //  xcore::examples::Test();

    fix_vector<char,25> x;
    for(int i=0; i<25; i++ )
        x.push_back( 5 );

    assert( x[0] == 5 );

    xcore::array<char,25> XX;
    XX.CopyFromTo( XX.ViewFrom( 10 ), 5 );


    return 0;
}