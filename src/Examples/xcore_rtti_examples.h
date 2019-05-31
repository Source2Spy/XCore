

namespace xcore::rtti::examples
{
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    struct unknown
    {
        constexpr static rtti::base m_RTTI{"unknown"};
        virtual const rtti::base& getVRTTI( void ) const noexcept { return m_RTTI; }
    };

    struct unknown2 : unknown
    {
        constexpr static rtti::entry<unknown2,unknown> m_RTTI{"unknown2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    struct test
    {
        constexpr static rtti::base m_RTTI{"test"};
        virtual const rtti::base& getVRTTI( void ) const noexcept { return m_RTTI; }
    };

    struct test2
    {
        constexpr static rtti::base m_RTTI{"test2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    struct child1 : test
    {
        constexpr static rtti::entry<test> m_RTTI{"child1"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child2 : child1
    {
        constexpr static rtti::entry<child1> m_RTTI{"child2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child3 : child2, test2
    {
        constexpr static rtti::entry<child2,test2> m_RTTI{"child3"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    void Test_isKindOf( void )
    {
        {
            child3 X;

            static_assert( xcore::rtti::isKindof<child3>(X)   );
            static_assert( xcore::rtti::isKindof<child2>(X)   );
            static_assert( xcore::rtti::isKindof<child1>(X)   );

            static_assert( xcore::rtti::isKindof<test>(X)     );
            static_assert( xcore::rtti::isKindof<test2>(X)    );

            static_assert( !xcore::rtti::isKindof<unknown2>(X) );
            static_assert( !xcore::rtti::isKindof<unknown>(X)  );
        }

        {
            child2 X;

            static_assert( !xcore::rtti::isKindof<child3>(X)  );
            static_assert( xcore::rtti::isKindof<child2>(X)   );
            static_assert( xcore::rtti::isKindof<child1>(X)   );

            static_assert( xcore::rtti::isKindof<test>(X)     );
            static_assert( !xcore::rtti::isKindof<test2>(X)   );

            static_assert( !xcore::rtti::isKindof<unknown>(X)  );
            static_assert( !xcore::rtti::isKindof<unknown2>(X) );
        }

        {
            child1 X;

            static_assert( !xcore::rtti::isKindof<child3>(X)  );
            static_assert( !xcore::rtti::isKindof<child2>(X)   );
            static_assert( xcore::rtti::isKindof<child1>(X)   );

            static_assert( xcore::rtti::isKindof<test>(X)     );
            static_assert( !xcore::rtti::isKindof<test2>(X)   );

            static_assert( !xcore::rtti::isKindof<unknown>(X)  );
            static_assert( !xcore::rtti::isKindof<unknown2>(X) );
        }

        {
            test X;

            static_assert( !xcore::rtti::isKindof<child3>(X)  );
            static_assert( !xcore::rtti::isKindof<child2>(X)   );
            static_assert( !xcore::rtti::isKindof<child1>(X)   );

            static_assert( xcore::rtti::isKindof<test>(X)     );
            static_assert( !xcore::rtti::isKindof<test2>(X)   );

            static_assert( !xcore::rtti::isKindof<unknown>(X)  );
            static_assert( !xcore::rtti::isKindof<unknown2>(X) );
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    void Test( void )
    {
        Test_isKindOf();
    }
}