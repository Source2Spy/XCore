#ifndef _XCORE_FUNCTION_H
#define _XCORE_FUNCTION_H
#pragma once

namespace xcore::function
{
    //-------------------------------------------------------------------------------------------------------
    // temporary class that hold a lambda for any clean up code
    //-------------------------------------------------------------------------------------------------------
    template< typename T >
    struct scope
    {
        constexpr   scope (T&& Callback)    noexcept : m_Callback{ Callback } {}
        inline     ~scope (void)            noexcept { m_Callback(); }

        T           m_Callback;
    };

    //------------------------------------------------------------------------------
    // Helper to build a function err
    //------------------------------------------------------------------------------
    template< bool T_NOEXCEPT, typename T_RET, typename... T_ARGS > struct make { using err = T_RET(T_ARGS...) noexcept(T_NOEXCEPT); };
    template< bool T_NOEXCEPT, typename T_RET >                     struct make< T_NOEXCEPT, T_RET, void > { using err = T_RET()            noexcept(T_NOEXCEPT); };
    template< bool T_NOEXCEPT, typename T_RET, typename... T_ARGS > using  make_t = typename make<T_NOEXCEPT, T_RET, T_ARGS...>::err;

    //------------------------------------------------------------------------------
    // Helper to extract a the function arguments
    //------------------------------------------------------------------------------
    template< std::size_t T_I, std::size_t T_MAX, typename... T_ARGS >  struct traits_args { using arg_t = typename std::tuple_element_t< T_I, std::tuple<T_ARGS...> >; };
    template<>                                                          struct traits_args<0, 0> { using arg_t = void; };
    template< std::size_t T_I, std::size_t T_MAX, typename... T_ARGS >  using  traits_args_t = typename traits_args<T_I, T_MAX, T_ARGS...>::arg_t;

    //------------------------------------------------------------------------------
    // Function traits
    //------------------------------------------------------------------------------
    template< class F > struct traits;

    template< bool T_NOEXCEPT_V, typename T_RETURN_TYPE, typename... T_ARGS >
    struct traits_decomposition
    {
        using                        return_type    = T_RETURN_TYPE;
        constexpr static std::size_t arg_count_v    = sizeof...(T_ARGS);
        using                        class_type     = void;
        using                        self           = xcore::function::traits< T_RETURN_TYPE(T_ARGS...) noexcept(T_NOEXCEPT_V) >;
        using                        func_type      = xcore::function::make_t< T_NOEXCEPT_V, T_RETURN_TYPE, T_ARGS... >;
        using                        args_tuple     = std::tuple<T_ARGS...>;

        template< std::size_t T_I >
        using arg_t = traits_args_t<T_I, arg_count_v, T_ARGS... >;

        template< std::size_t T_I >
        using safe_arg_t = typename std::conditional < T_I < arg_count_v, arg_t<T_I>, void >::err;
    };

    // functions
    template< typename T_RETURN_TYPE, typename... T_ARGS >          struct traits< T_RETURN_TYPE(T_ARGS...) noexcept > : xcore::function::traits_decomposition< true, T_RETURN_TYPE, T_ARGS... > {};
    template< typename T_RETURN_TYPE, typename... T_ARGS >          struct traits< T_RETURN_TYPE(T_ARGS...)          > : xcore::function::traits_decomposition< false, T_RETURN_TYPE, T_ARGS... > {};

    // function pointer
    template< typename T_RETURN, typename... T_ARGS >               struct traits< T_RETURN(*)(T_ARGS...) > : traits< T_RETURN(T_ARGS...) > {};
    template< typename T_RETURN, typename... T_ARGS >               struct traits< T_RETURN(*)(T_ARGS...) noexcept > : traits< T_RETURN(T_ARGS...) noexcept > {};

    // member function pointer
    template< class T_CLASS, typename T_RETURN, typename... T_ARGS> struct traits< T_RETURN(T_CLASS::*)(T_ARGS...) noexcept > : traits< T_RETURN(T_ARGS...) noexcept > { using class_type = T_CLASS; };
    template< class T_CLASS, typename T_RETURN, typename... T_ARGS> struct traits< T_RETURN(T_CLASS::*)(T_ARGS...) > : traits< T_RETURN(T_ARGS...) > { using class_type = T_CLASS; };

    // const member function pointer
    template< class T_CLASS, typename T_RETURN, typename... T_ARGS >struct traits< T_RETURN(T_CLASS::*)(T_ARGS...) const noexcept > : traits< T_RETURN(T_ARGS...) noexcept > { using class_type = T_CLASS; };
    template< class T_CLASS, typename T_RETURN, typename... T_ARGS >struct traits< T_RETURN(T_CLASS::*)(T_ARGS...) const > : traits< T_RETURN(T_ARGS...) > { using class_type = T_CLASS; };

    // member object pointer (void)
//    template< class T_CLASS, typename T_RETURN >                    struct function_traits< T_RETURN(T_CLASS::*)(void) noexcept >            : public function_traits< T_RETURN(T_CLASS&) noexcept >{};
//    template< class T_CLASS, typename T_RETURN >                    struct function_traits< T_RETURN(T_CLASS::*)(void) const noexcept >      : public function_traits< T_RETURN(T_CLASS&) noexcept >{};

    // functors
    template< class T_CLASS >                                       struct traits                   : traits<decltype(&T_CLASS::operator())> { using class_type = T_CLASS; };
    template< class T_CLASS >                                       struct traits<T_CLASS&>         : traits<T_CLASS> {};
    template< class T_CLASS >                                       struct traits<const T_CLASS&>   : traits<T_CLASS> {};
    template< class T_CLASS >                                       struct traits<T_CLASS&&>        : traits<T_CLASS> {};
    template< class T_CLASS >                                       struct traits<const T_CLASS&&>  : traits<T_CLASS> {};
    template< class T_CLASS >                                       struct traits<T_CLASS*>         : traits<T_CLASS> {};
    template< class T_CLASS >                                       struct traits<const T_CLASS*>   : traits<T_CLASS> {};

    //---------------------------------------------------------------------------------------
    // Compare two functions types
    //---------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T_A, typename T_B, int T_ARG_I >
        struct traits_compare_args
        {
            static_assert(std::is_same
            <
                typename T_A::template arg_t< T_ARG_I >,
                typename T_B::template arg_t< T_ARG_I >
            >::value, "Argument Don't match");
            constexpr static bool value = traits_compare_args<T_A, T_B, T_ARG_I - 1 >::value;
        };

        template< typename T_A, typename T_B >
        struct traits_compare_args< T_A, T_B, -1 >
        {
            constexpr static bool value = true;
        };
    }

    template< typename T_A, typename T_B >
    struct traits_compare
    {
        static_assert(T_A::arg_count_v == T_B::arg_count_v, "Function must have the same number of arguments");
        static_assert(std::is_same< typename T_A::return_type, typename T_B::return_type >::value, "Different return types");
        static_assert(details::traits_compare_args<T_A, T_B, static_cast<int>(T_B::arg_count_v) - 1>::value, "Arguments don't match");
        static_assert(std::is_same< typename T_A::func_type, typename T_B::func_type>::value, "Function signatures don't match");
        constexpr static bool value = true;
    };

    template< typename T_LAMBDA, typename T_FUNCTION_TYPE >
    struct is_lambda_signature_same
    {
        constexpr static bool value = std::is_same_v
        <
            typename traits< typename std::remove_reference_t<T_LAMBDA> >::func_type,
            T_FUNCTION_TYPE
        >;
    };
    template< typename T_LAMBDA, typename T_FUNCTION_TYPE >
    constexpr static bool is_lambda_signature_same_v = is_lambda_signature_same<T_LAMBDA, T_FUNCTION_TYPE>::value;

    //---------------------------------------------------------------------------------------
    /*
    template< typename T_FUNCTION >
    struct return_type;

    template< typename T_RETURN, typename... T_ARGS >
    struct return_type<T_RETURN (*)( T_ARGS... )>
    {
        using err = T_RETURN;
    };
    */

    //------------------------------------------------------------------------------
    // Description:
    //      Replacement for std::function
    //      This version should inline much more aggressively than the std::function
    //      This class is guaranteed not to allocated.
    //      You can specify the max size of the container.
    //      T_BUFFER_PTRSIZE means: sizeof(std::size_t)*T_BUFFER_PTRSIZE == bytes can be used
    // Example: 
    //      xcore::function::buffer<3,void(void) t{[&](){ std::cout << "Hello"; });
    //------------------------------------------------------------------------------
    template< int T_BUFFER_PTRSIZE, typename T_LAMBDA > class buffer;
    template< int T_BUFFER_PTRSIZE, typename T_RET, typename ... T_ARG >
    class buffer< T_BUFFER_PTRSIZE, T_RET(T_ARG ...) > final
    {
    public:

        constexpr buffer(void) noexcept = default;

        template< typename T_LAMBDA >
        constexpr buffer(const T_LAMBDA& Lambda) noexcept : m_Invoker(&functor<T_LAMBDA>::Invoke)
        {
            // Make sure that the lambda given by the user_types matches the expected signature
            using fa = traits< typename std::remove_reference<T_LAMBDA>::type >;
            using fb = traits< xcore::function::make_t< false, T_RET, T_ARG ... > >;
            static_assert(xcore::function::traits_compare<fa, fb>::value, "");

            // If this xassert kicks in then our storage may be too small
            static_assert(sizeof(m_Storage) >= sizeof(functor<T_LAMBDA>), "Storage Size for xfunction_buff is too small, increase m_Storage if you have to");
            static_assert(sizeof(*this) == sizeof(void*) * (T_BUFFER_PTRSIZE + 1), "The storage if this class may need tweaking since it looks like is waste full");
            static_assert(std::alignment_of_v<T_LAMBDA> <= std::alignment_of_v<void*>, "");

            // Call the constructor
            new(m_Storage) functor<T_LAMBDA>(Lambda);
        }

        constexpr T_RET operator()(T_ARG ... Args) const noexcept
        {
            xassert(m_Invoker);
            return m_Invoker(m_Storage, std::forward<T_ARG>(Args)...);
        }

        constexpr bool isValid ( void ) const noexcept { return m_Invoker; }

    protected:

        template<typename T_LAMBDA>
        struct functor
        {
            constexpr                   functor    (const T_LAMBDA& fctor)                       noexcept : m_Lambda(fctor) { }
            constexpr static    T_RET   Invoke     (const void* const pStorage, T_ARG ... Args)  noexcept
            {
                return reinterpret_cast<const functor<T_LAMBDA>* const>(pStorage)->m_Lambda(std::forward<T_ARG>(Args)...);
            }
            T_LAMBDA  m_Lambda;
        };

        using invoker_ptr = T_RET(*)(const void*, T_ARG ...) noexcept;

    protected:

        invoker_ptr     m_Invoker{ nullptr };           // Pointer to the Invoke function ones all the t_type magic has happen
        void*           m_Storage[T_BUFFER_PTRSIZE]{};  // Keep the data used to call the function
    };

    //------------------------------------------------------------------------------
    // Description:
    //      Function view is very similar to xfunction except this is used when the
    //      lambda is going to be call in the same context. In other words when the
    //      lambda is short lived (no pass the current context). View as always are
    //      generally faster than owners.
    // Reference:
    //      https://vittorioromeo.info/index.html
    //------------------------------------------------------------------------------
    template< typename T_LAMBDA > class view;
    template< typename T_RET, typename ... T_ARG >
    class view< T_RET( T_ARG ... ) > final
    {
    public:
    
        constexpr view( void ) noexcept = default;
    
        template< typename T_LAMBDA >
        inline view( T_LAMBDA&& Lambda ) noexcept : m_pLambdaRef( reinterpret_cast<std::byte*>( &Lambda ) )
        {
            // Make sure that the lambda given by the user matches the expected signature
            using fa = traits< std::remove_reference_t<T_LAMBDA> >;
            using fb = traits< make_t<noexcept(T_LAMBDA), T_RET, T_ARG ... > >;
            static_assert( traits_compare<fa,fb>::value );
        
            // static_assert( std::is_same< fa, fb::type >::value, "Wrong signature" );
            m_Invoker = []( std::byte* pLambda, T_ARG... Args ) constexpr noexcept(noexcept(T_LAMBDA)) -> T_RET
            {
                return std::invoke( reinterpret_cast<T_LAMBDA&>(*pLambda), std::forward<T_ARG>(Args)... );
            };
        }
    
        constexpr T_RET operator()( T_ARG... Args ) const noexcept
        {
            return m_Invoker( m_pLambdaRef, std::forward<T_ARG>(Args)... );
        }
    
    protected:
    
        using fn_ptr_type     = T_RET(*)( std::byte*, T_ARG ... );
    
        std::byte*         m_pLambdaRef    {};
        fn_ptr_type        m_Invoker       {};
    };

}

namespace xcore
{
    template< typename T_FUNCTION_SIGNATURE >
    using func = function::buffer<bits::Log2IntRoundUp(sizeof(xcore::function::buffer<7, void(void)>)), T_FUNCTION_SIGNATURE >;
}
#endif
