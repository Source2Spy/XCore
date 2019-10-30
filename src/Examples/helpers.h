
#include "circular_pool.h"

namespace helpers
{
    namespace details
    {
        template< typename T_ARRAY >
        constexpr void CombSort(T_ARRAY& Array) noexcept
        {
            using size_type = typename T_ARRAY::size_type;
            auto  Gap = Array.size();

            bool bSwapped = false;

            while ((Gap > size_type{ 1 }) || bSwapped)
            {
                if (Gap > size_type{ 1 })
                {
                    Gap = static_cast<size_type>(Gap / 1.247330950103979);
                }

                bSwapped = false;

                for (auto i = size_type{ 0 }; Gap + i < Array.size(); ++i)
                {
                    if (Array[i] > Array[i + Gap])
                    {
                        auto swap = Array[i];
                        Array[i] = Array[i + Gap];
                        Array[i + Gap] = swap;
                        bSwapped = true;
                    }
                }
            }
        }
    }

    template<typename T_ARRAY>
    constexpr T_ARRAY sort(T_ARRAY Array) noexcept
    {
        auto Sorted = Array;
        details::CombSort(Sorted);
        return Sorted;
    }


    //
    // From: https://codereview.stackexchange.com/questions/131194/selection-sorting-a-type-list-compile-time
    //

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
                std::tuple_element_t<i, LoopTuple>,
                std::tuple_element_t<j, LoopTuple>
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
