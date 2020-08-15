#ifndef _XCORE_TYPES_H
#define _XCORE_TYPES_H
#pragma once

namespace xcore::types
{
    //-------------------------------------------------------------------------------------------------------
    // Select the first type of a parameter list
    //-------------------------------------------------------------------------------------------------------
    namespace details
    {
        template< class...Args> struct select_first;
        template< class A, class ...Args> struct select_first<A,Args...>{ using type = A;};
    }
    template<typename... Ts> using select_first_t = typename details::select_first<Ts...>::type;

    //-------------------------------------------------------------------------------------------------------
    // Select the last type of a parameter list
    //-------------------------------------------------------------------------------------------------------
    namespace details
    {
        template<typename T> struct tag { using type = T; };
        template<typename... Ts> struct select_last { using type = typename decltype((tag<Ts>{}, ...))::type; };
    }
    template<typename... Ts> using select_last_t = typename details::select_last<Ts...>::type;

    //-------------------------------------------------------------------------------------------------------
    // Select the first parameter in a parameter list
    //-------------------------------------------------------------------------------------------------------
    template<typename T, typename... T_ARGS>
    constexpr T& PickFirstArgument( T&& First, T_ARGS&&...) noexcept
    {
        return First;
    }

    //------------------------------------------------------------------------------------------
    // Checks if a type is part of a pack
    //------------------------------------------------------------------------------------------

    namespace details
    {
        template <typename...>
        struct count_of;

        template< typename F >
        struct count_of<F>
        {
            static constexpr auto value = 0ull;
        };

        template <typename F, typename S, typename... T>
        struct count_of<F, S, T...>
        {
            static constexpr auto value = std::is_same<F, S>::value + count_of<F, T...>::value;
        };
    }

    template< typename T_TYPE, typename...T_ARGS >
    static constexpr auto count_of_v  = details::count_of< T_TYPE, T_ARGS...>::value;

    //-------------------------------------------------------------------------------------------------------
    // int base on size
    //-------------------------------------------------------------------------------------------------------
    template< std::size_t T_SIZE_BYTES >
    using byte_size_uint_t = std::tuple_element_t< T_SIZE_BYTES-1, std::tuple<std::uint8_t,std::uint16_t,std::uint32_t,std::uint32_t,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t>>; 

    template< std::size_t T_SIZE_BYTES >
    using byte_size_int_t  = std::tuple_element_t< T_SIZE_BYTES-1, std::tuple<std::int8_t,std::int16_t,std::int32_t,std::int32_t,std::int64_t,std::int64_t,std::int64_t,std::int64_t>>; 

    //-------------------------------------------------------------------------------------------------------
    // given a err returns the equivalent size as either a sing or unsigned err
    //-------------------------------------------------------------------------------------------------------
    template< typename T >
    using to_int_t = byte_size_int_t<sizeof(T)>; 

    template< typename T >
    using to_uint_t = byte_size_uint_t<sizeof(T)>; 

    //------------------------------------------------------------------------------
    // Description:
    //      Static casting with safe ranges 
    //------------------------------------------------------------------------------
    template< typename T_TO, typename T_FROM > constexpr
    T_TO static_cast_safe( const T_FROM a ) noexcept
    {
        xassert_block_basic()
        {
            using max_type = byte_size_uint_t< (sizeof(T_TO) > sizeof(T_FROM) ? sizeof(T_TO) : sizeof(T_FROM)) >;
            using min_type = byte_size_int_t < (sizeof(T_TO) > sizeof(T_FROM) ? sizeof(T_TO) : sizeof(T_FROM)) >;
            xassert(static_cast<min_type>(a) >= static_cast<min_type>(std::numeric_limits<T_TO>::lowest()) &&
                    static_cast<max_type>(a) <= static_cast<max_type>(std::numeric_limits<byte_size_uint_t<sizeof(T_TO)>>::max()));
        }

        return static_cast<T_TO>(a);
    }

    //------------------------------------------------------------------------------
    // Description:
    //      Converts an rvalue to an lvalue 
    //------------------------------------------------------------------------------
    template <typename T>
    constexpr T& lvalue(T&& r) noexcept { return r; }

    //------------------------------------------------------------------------------------------
    // Tuple Compose
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template< template< typename...T> class T_MAIN, typename...T_ARGS >
        T_MAIN< T_ARGS... > Compose( std::tuple<T_ARGS...>* );
    }
    template< template< typename...T> class T_MAIN, typename T_TUPLE >
    using tuple_compose_t = decltype( details::Compose<T_MAIN>( static_cast<T_TUPLE*>(nullptr) ));

    //------------------------------------------------------------------------------------------
    // Tuple err to index conversion
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template <class T, class T_ARGS>
        struct tuple_t2i;

        template <class T, class... T_ARGS>
        struct tuple_t2i<T, std::tuple<T, T_ARGS...>> 
        {
            static const std::size_t value = 0;
        };

        template <class T, class U, class... T_ARGS>
        struct tuple_t2i<T, std::tuple<U, T_ARGS...>> 
        {
            static const std::size_t value = 1 + tuple_t2i<T, std::tuple<T_ARGS...>>::value;
        };
    }
    template< typename T_TYPE, typename T_TUPLE >
    constexpr static auto tuple_t2i_v = details::tuple_t2i< T_TYPE, T_TUPLE >::value;

    //------------------------------------------------------------------------------------------
    // Concatenate a list of tuples into a simple one
    //------------------------------------------------------------------------------------------
    template<typename ... T_TUPLES>
    using tuple_cat_t = decltype(std::tuple_cat(std::declval<T_TUPLES>()...));

    //------------------------------------------------------------------------------------------
    // tuple_extract_n_t - creates a new tuple containing the first n types
    // tuple_delete_n_t  - creates a new tuple which has removed the first n entries
    //------------------------------------------------------------------------------------------
    namespace details
    {
        //----------------------------------------------------------------------------------
        template< std::size_t T_COUNT, typename T_TUPLE, bool = (T_COUNT == 0) > 
        struct tuple_extract_n
        {
            static_assert(T_COUNT <= std::tuple_size_v<T_TUPLE> );

            using type      = std::tuple<>;
            using remainder = T_TUPLE;
        };

        template< std::size_t T_COUNT, typename...T_ARGS > 
        struct tuple_extract_n< T_COUNT, std::tuple<T_ARGS...>, false >
        {
            static_assert( T_COUNT <= sizeof...(T_ARGS) );

            template< std::size_t I, typename T_ELEMENT, typename...T_A >
            struct append
            {
                using app_t     = append< I-1, T_A... >;

                using remainder = typename app_t::remainder; 
                using type      = tuple_cat_t< std::tuple<T_ELEMENT>, typename app_t::type >;
            };

            template< typename T_ELEMENT, typename...T_A >
            struct append< 0, T_ELEMENT, T_A... >
            {
                using remainder = std::tuple<T_A...>;
                using type      = std::tuple<T_ELEMENT>;
            };

            using app_t     = append< T_COUNT-1, T_ARGS...>;
            using type      = typename app_t::type;
            using remainder = typename app_t::remainder; 
        };
    }

    template< std::size_t T_COUNT, typename T_TUPLE >
    using tuple_extract_n_t = typename details::tuple_extract_n< T_COUNT, T_TUPLE>::type;

    template< std::size_t T_COUNT, typename T_TUPLE >
    using tuple_delete_n_t = typename details::tuple_extract_n< T_COUNT, T_TUPLE>::remainder;

    //------------------------------------------------------------------------------------------
    // tuple_search_and_replace_t - Given 3 tuples it will search the first tuple with the second one
    //                              and replace the match with the 3rd one.
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_TUPLE_LIST, typename T_TUPLE_FIND, typename T_TUPLE_REPLACE >
        struct tuple_search_and_replace
        {
            // nonsensical warning from visual studio.
            #pragma warning ( push )
            #pragma warning ( disable:4348 )  // warning C4348: 'details::tuple_search_and_replace<a,b,c>::sar': redefinition of default parameter: parameter 4

            template< typename T_NEW_TUPLE_LIST
                    , typename T_TUPLE_MATCH
                    , typename T_TUPLE_REMAINDER
                    , bool     = ((0 == std::tuple_size_v<T_TUPLE_MATCH>) || (0 == std::tuple_size_v<T_TUPLE_REMAINDER>)) >
            struct sar;

            #pragma warning ( pop )

            template< typename T_NEW_TUPLE_LIST, typename T_TUPLE_MATCH, typename T_TUPLE_REMAINDER >
            struct sar< T_NEW_TUPLE_LIST, T_TUPLE_MATCH, T_TUPLE_REMAINDER, true >
            {
                using type = tuple_cat_t< T_NEW_TUPLE_LIST, T_TUPLE_MATCH, T_TUPLE_REMAINDER >;
            };

            template< typename T_NEW_TUPLE_LIST,typename T_TUPLE_REMAINDER >
            struct sar< T_NEW_TUPLE_LIST, T_TUPLE_FIND, T_TUPLE_REMAINDER, true >
            {
                using type = tuple_cat_t< T_NEW_TUPLE_LIST, T_TUPLE_REPLACE, T_TUPLE_REMAINDER >;
            };

            template< typename T_NEW_TUPLE_LIST, typename T_TUPLE_MATCH, typename T_TUPLE_REMAINDER >
            struct sar< T_NEW_TUPLE_LIST, T_TUPLE_MATCH, T_TUPLE_REMAINDER, false >
            {
                using remainder = tuple_cat_t   <   tuple_delete_n_t < 1, T_TUPLE_MATCH >, T_TUPLE_REMAINDER >;                            
                using type      = typename sar  <   tuple_cat_t< T_NEW_TUPLE_LIST, std::tuple< std::tuple_element_t< 0, T_TUPLE_MATCH>>>
                                                ,   tuple_extract_n_t< std::tuple_size_v<T_TUPLE_FIND>, remainder >
                                                ,   tuple_delete_n_t < std::tuple_size_v<T_TUPLE_FIND>, remainder >
                                                >::type; 
            };

            template< typename T_NEW_TUPLE_LIST, typename T_TUPLE_REMAINDER >
            struct sar< T_NEW_TUPLE_LIST, T_TUPLE_FIND, T_TUPLE_REMAINDER, false >
            {
                using type = typename sar   <   tuple_cat_t< T_NEW_TUPLE_LIST, T_TUPLE_REPLACE >
                                            ,   tuple_extract_n_t< std::tuple_size_v<T_TUPLE_FIND>, T_TUPLE_REMAINDER >
                                            ,   tuple_delete_n_t < std::tuple_size_v<T_TUPLE_FIND>, T_TUPLE_REMAINDER >
                                            >::type; 
            };

            using type = typename sar   <   std::tuple<>
                                        ,   tuple_extract_n_t< std::tuple_size_v<T_TUPLE_FIND>, T_TUPLE_LIST >
                                        ,   tuple_delete_n_t < std::tuple_size_v<T_TUPLE_FIND>, T_TUPLE_LIST >
                                        >::type;
        };
    }

    template< typename T_TUPLE_LIST, typename T_TUPLE_FIND, typename T_TUPLE_REPLACE >
    using tuple_search_and_replace_t = typename details::tuple_search_and_replace<T_TUPLE_LIST, T_TUPLE_FIND, T_TUPLE_REPLACE>::type;

    //--------------------------------------------------------------------------------------------
    // Tuple visit similar to the variant visit
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_LAMBDA, typename T_TUPLE, std::size_t... T_SEQUENCE_V > xforceinline
        constexpr void visit( T_LAMBDA&& Lambda, T_TUPLE&& Tuple, std::index_sequence<T_SEQUENCE_V...> ) noexcept
        {
            (..., Lambda(std::get<T_SEQUENCE_V>(Tuple)));
        }
    }

    template< typename T_LAMBDA, typename T_TUPLE > xforceinline
    constexpr void tuple_visit( T_LAMBDA&& Lambda, T_TUPLE&& Tuple ) noexcept
    {
        details::visit( std::forward<T_LAMBDA>(Lambda), std::forward<T_TUPLE>(Tuple), std::make_index_sequence<std::tuple_size_v<std::decay_t<T_TUPLE>>>());
    }

    //--------------------------------------------------------------------------------------------
    // From: https://codereview.stackexchange.com/questions/131194/selection-sorting-a-type-list-compile-time
    // Sorts a tuple base on a compare function such:
    //
    // template< typename T_A, typename T_B >
    // struct compare { constexpr static bool value = T_A::type_guid_v.m_Value > T_B::type_guid_v.m_Value; };
    //
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        // swap types at index i and index j in the template argument tuple
        template <std::size_t i, std::size_t j, class Tuple>
        class tuple_element_swap
        {
            template <class IndexSequence>
            struct tuple_element_swap_impl;

            template <std::size_t... indices>
            struct tuple_element_swap_impl<std::index_sequence<indices...>>
            {
                using type = std::tuple
                <
                    std::tuple_element_t
                    <
                        indices != i && indices != j ? indices : indices == i ? j : i, Tuple
                    >...
                >;
            };

        public:
            using type = typename tuple_element_swap_impl
            <
                std::make_index_sequence<std::tuple_size<Tuple>::value>
            >::type;
        };

        // selection sort template argument tuple's variadic template's types
        template <template <class, class> class Comparator, class Tuple>
        class tuple_selection_sort
        {
            // selection sort's "loop"
            template <std::size_t i, std::size_t j, std::size_t tuple_size, class LoopTuple>
            struct tuple_selection_sort_impl
            {
                // this is done until we have compared every element in the type list
                using tuple_type = std::conditional_t
                <
                    Comparator
                    <
                          std::tuple_element_t<j, LoopTuple>
                        , std::tuple_element_t<i, LoopTuple>
                    >::value,
                    typename tuple_element_swap<i, j, LoopTuple>::type, // true: swap(i, j)
                    LoopTuple                                           // false: do nothing
                >;

                using type = typename tuple_selection_sort_impl // recurse until j == tuple_size
                <
                    i, j + 1, tuple_size, tuple_type // using the modified tuple
                >::type;
            };

            template <std::size_t i, std::size_t tuple_size, class LoopTuple>
            struct tuple_selection_sort_impl<i, tuple_size, tuple_size, LoopTuple>
            {
                // once j == tuple_size, we increment i and start j at i + 1 and recurse
                using type = typename tuple_selection_sort_impl
                <
                    i + 1, i + 2, tuple_size, LoopTuple
                >::type;
            };

            template <std::size_t j, std::size_t tuple_size, class LoopTuple>
            struct tuple_selection_sort_impl<tuple_size, j, tuple_size, LoopTuple>
            {
                // once i == tuple_size, we know that every element has been compared
                using type = LoopTuple;
            };

        public:
            using type = typename tuple_selection_sort_impl
            <
                0, 1, std::tuple_size<Tuple>::value, Tuple
            >::type;
        };
    }

    template< template <class, class> class T_COMPARE, class T_TUPLE >
    using tuple_sort_t = typename details::tuple_selection_sort< T_COMPARE, T_TUPLE >::type;

    //--------------------------------------------------------------------------------------------
    // is Type in tuple
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T, typename T_TUPLE >
        struct tuple_has_type;

        template< typename T, typename...T_ARGS >
        struct tuple_has_type< T, std::tuple<T_ARGS...> >
        {
            constexpr static bool value = xcore::types::count_of_v<T, T_ARGS...> > 0;
        };
    }

    template< typename T, typename T_TUPLE >
    static constexpr bool tuple_has_type_v = details::tuple_has_type<T, T_TUPLE >::value;

    //--------------------------------------------------------------------------------------------
    // Does tuple have duplicates
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_TUPLE >
        struct tuple_has_duplicates;

        template< typename...T_ARGS >
        struct tuple_has_duplicates< std::tuple<T_ARGS...> >
        {
            constexpr static bool value = ((xcore::types::count_of_v<T_ARGS, T_ARGS...> > 1 ) || ... ) ;
        };
    }

    template< typename T_TUPLE >
    static constexpr bool tuple_has_duplicates_v = details::tuple_has_duplicates<T_TUPLE >::value;

    //------------------------------------------------------------------------------------------
    // Variant type to index conversion
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template< class T, class T_VARIANT >
        struct variant_t2i;

        template< class T, class... T_ARGS >
        struct variant_t2i<T, std::variant<T, T_ARGS...>> 
        {
            static const std::size_t value = 0;
        };

        template< class T, class U, class... T_ARGS >
        struct variant_t2i<T, std::variant<U, T_ARGS...>> 
        {
            static const std::size_t value = 1 + variant_t2i<T, std::variant<T_ARGS...>>::value;
        };
    }
    template< typename T_TYPE, typename T_VARIANT >
    constexpr static auto variant_t2i_v = details::variant_t2i< T_TYPE, T_VARIANT >::value;

    //--------------------------------------------------------------------------------------------
    // Removes all attributes and references operators giving the basic underlaying type
    //--------------------------------------------------------------------------------------------
    template< typename T >
    using decay_full_t = std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>;

    //--------------------------------------------------------------------------------------------
    // Helpful when using variadic types
    //--------------------------------------------------------------------------------------------
    namespace details{ template< typename T > struct always_false : std::false_type {}; }
    template< typename T > constexpr static bool always_false_v =  details::always_false<T>::value;

    //--------------------------------------------------------------------------------------------
    // Determines if a type is derived from a particular template class
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< template< typename... > typename T_BASE, typename    T_DERIVED > struct   is_specialized                            : std::false_type {};
        template< template< typename... > typename T_BASE, typename... T_ARGS    > struct   is_specialized<T_BASE, T_BASE<T_ARGS...>> : std::true_type  {};
    }
    template< template< typename... > typename T_BASE, typename    T_DERIVED > constexpr static bool is_specialized_v = details::is_specialized<T_BASE, T_DERIVED>::value;

    //--------------------------------------------------------------------------------------------
    // Makes a object type unique
    //--------------------------------------------------------------------------------------------
    template< typename T, typename T_TAG = int >
    struct make_unique : public T
    {
        constexpr make_unique( void ) = default;
        constexpr make_unique( const T& x ) : T(x){}
        using T::T;
        using T::operator =;
    };

    //--------------------------------------------------------------------------------------------
    // Makes a object type unique
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< std::size_t T_MAX_SIZE, template< typename, std::size_t, typename > class T_PARENT  >
        struct container_static_to_dynamic
        {
            template< typename T, typename T_COUNTER_ARG >
            struct type : T_PARENT<T,T_MAX_SIZE,T_COUNTER_ARG>
            {
                using parent = T_PARENT<T,T_MAX_SIZE,T_COUNTER_ARG>;
                using parent::parent;
            };
        };
    }
    template< template< typename, std::size_t, typename > class T_PARENT, std::size_t T_MAX_SIZE >
    using container_static_to_dynamic = typename details::container_static_to_dynamic<T_MAX_SIZE,T_PARENT>;

    //--------------------------------------------------------------------------------------------
    // Determine if a type is an array will return true if it is a C array or an object of type array 
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template<class T>
        struct is_array :std::is_array<T> {};

        template<class T, std::size_t N>
        struct is_array<std::array<T, N>> :std::true_type {};

        template<class T, std::size_t N>
        struct is_array<xcore::array<T, N>> :std::true_type {};

        template<class T> struct is_array<T const>          : is_array<T> {};
        template<class T> struct is_array<T volatile>       : is_array<T> {};
        template<class T> struct is_array<T volatile const> : is_array<T> {};
    }
    template< typename T >
    constexpr static bool is_array_v = details::is_array<T>::value;

    //--------------------------------------------------------------------------------------------
    // Determine if a type is span object
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template<class T>
        struct is_span : std::false_type {};

        template<class T, std::size_t N>
        struct is_span<std::span<T, N>>     : std::true_type {};

        template<class T, std::size_t N>
        struct is_span<xcore::span<T, N>>   : std::true_type {};


        template<class T> struct is_span<T const> : is_span<T> {};
        template<class T> struct is_span<T volatile> : is_span<T> {};
        template<class T> struct is_span<T volatile const> : is_span<T> {};
    }
    template< typename T >
    constexpr static bool is_span_v = details::is_span<T>::value;

    //--------------------------------------------------------------------------------------------
    // Checks if a type is valid 
    // https://stackoverflow.com/questions/39816779/check-if-type-is-defined
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        template< class T, class E = void >
        struct is_defined : std::false_type {};

        template< class T >
        struct is_defined< T, std::enable_if_t< std::is_object<T>::value && !std::is_pointer<T>::value && (sizeof(T) > 0) > > : std::true_type{};
    }

    template< typename T > constexpr static bool is_defined_v = details::is_defined<T>::value;

    //--------------------------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------------------------
    struct nonesuch
    {
           ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
        void operator=(nonesuch const&) = delete;
    };
    
    //--------------------------------------------------------------------------------------------
    // Determine if a type is span object
    //--------------------------------------------------------------------------------------------
    namespace detail 
    {
        template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
        struct detector 
        {
            using value_t	= std::false_type;
            using type		= Default;
        };
        
        template <class Default, template<class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> 
        {
            // Note that std::void_t is a C++17 feature
            using value_t = std::true_type;
            using type    = Op<Args...>;
        };
    
    } // namespace detail
    template <template<class...> class Op, class... Args>
    using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;
    
    template <template<class...> class Op, class... Args>
    using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;
    
    template <class Default, template<class...> class Op, class... Args>
    using detected_or = detail::detector<Default, void, Op, Args...>;

    template <class Expected, template<class...> class Op, class... Args>
    using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

    template <class Expected, template<class...> class Op, class... Args>
    constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;
}

#endif