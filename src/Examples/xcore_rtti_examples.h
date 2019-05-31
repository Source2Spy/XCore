

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
        constexpr static rtti::entry<child1,test> m_RTTI{"child1"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child2 : child1
    {
        constexpr static rtti::entry<child2,child1> m_RTTI{"child2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child3 : child2, test2
    {
        constexpr static rtti::entry<child3,child2,test2> m_RTTI{"child3"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    void Test_isKindOf( void )
    {
        {
            // casting down
            child3 X;

            xassert( xcore::rtti::isKindof<child3>(X)   );
            xassert( xcore::rtti::isKindof<child2>(X)   );
            xassert( xcore::rtti::isKindof<child1>(X)   );

            xassert( xcore::rtti::isKindof<test>(X)     );
            xassert( xcore::rtti::isKindof<test2>(X)    );

            xassert( !xcore::rtti::isKindof<unknown2>(X) );
            xassert( !xcore::rtti::isKindof<unknown>(X)  );

            // casting up
            test& Y = X;
            xassert( xcore::rtti::isKindof<child3>(Y)  );
            xassert( xcore::rtti::isKindof<child2>(Y)   );
            xassert( xcore::rtti::isKindof<child1>(Y)   );

            xassert( xcore::rtti::isKindof<test>(Y)     );
            xassert( xcore::rtti::isKindof<test2>(Y)    );

            xassert( !xcore::rtti::isKindof<unknown2>(Y) );
            xassert( !xcore::rtti::isKindof<unknown>(Y)  );
        }

        {
            child2 X;
            xassert( !xcore::rtti::isKindof<child3>(X)  );
            xassert( xcore::rtti::isKindof<child2>(X)   );
            xassert( xcore::rtti::isKindof<child1>(X)   );

            xassert( xcore::rtti::isKindof<test>(X)     );
            xassert( !xcore::rtti::isKindof<test2>(X)   );

            xassert( !xcore::rtti::isKindof<unknown>(X)  );
            xassert( !xcore::rtti::isKindof<unknown2>(X) );

            test& Y = X;
            xassert( !xcore::rtti::isKindof<child3>(Y)  );
            xassert( xcore::rtti::isKindof<child2>(Y)   );
            xassert( xcore::rtti::isKindof<child1>(Y)   );

            xassert( xcore::rtti::isKindof<test>(Y)     );
            xassert( !xcore::rtti::isKindof<test2>(Y)   );

            xassert( !xcore::rtti::isKindof<unknown>(Y)  );
            xassert( !xcore::rtti::isKindof<unknown2>(Y) );
        }

        {
            child1 X;

            xassert( !xcore::rtti::isKindof<child3>(X)  );
            xassert( !xcore::rtti::isKindof<child2>(X)   );
            xassert( xcore::rtti::isKindof<child1>(X)   );

            xassert( xcore::rtti::isKindof<test>(X)     );
            xassert( !xcore::rtti::isKindof<test2>(X)   );

            xassert( !xcore::rtti::isKindof<unknown>(X)  );
            xassert( !xcore::rtti::isKindof<unknown2>(X) );
        }

        {
            test X;

            xassert( !xcore::rtti::isKindof<child3>(X)  );
            xassert( !xcore::rtti::isKindof<child2>(X)   );
            xassert( !xcore::rtti::isKindof<child1>(X)   );

            xassert( xcore::rtti::isKindof<test>(X)     );
            xassert( !xcore::rtti::isKindof<test2>(X)   );

            xassert( !xcore::rtti::isKindof<unknown>(X)  );
            xassert( !xcore::rtti::isKindof<unknown2>(X) );
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    void Test( void )
    {
        Test_isKindOf();
    }
}