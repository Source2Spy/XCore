

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
        int m_X;
        constexpr static rtti::base m_RTTI{"test"};
        virtual const rtti::base& getVRTTI( void ) const noexcept { return m_RTTI; }
    };

    struct test2
    {
        int m_Y;
        constexpr static rtti::base m_RTTI{"test2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    struct child1 : test
    {
        int m_A;
        constexpr static rtti::entry<child1,test> m_RTTI{"child1"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child2 : child1
    {
        int m_B;
        constexpr static rtti::entry<child2,child1> m_RTTI{"child2"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    struct child3 : child2, test2
    {
        int m_C;
        constexpr static rtti::entry<child3,child2,test2> m_RTTI{"child3"};
        virtual const rtti::base& getVRTTI( void ) const noexcept override { return m_RTTI; }
    };

    //--------------------------------------------------------------------------------------------------------
    // Test is kind of
    //--------------------------------------------------------------------------------------------------------
    void Test_isKindOf( log::channel& )
    {
        {
            // casting down
            child3 X;

            xassert( xcore::rtti::isKindOf<child3>(X)   );
            xassert( xcore::rtti::isKindOf<child2>(X)   );
            xassert( xcore::rtti::isKindOf<child1>(X)   );

            xassert( xcore::rtti::isKindOf<test>(X)     );
            xassert( xcore::rtti::isKindOf<test2>(X)    );

            xassert( !xcore::rtti::isKindOf<unknown2>(X) );
            xassert( !xcore::rtti::isKindOf<unknown>(X)  );

            // casting up
            test& Y = X;
            xassert( xcore::rtti::isKindOf<child3>(Y)  );
            xassert( xcore::rtti::isKindOf<child2>(Y)   );
            xassert( xcore::rtti::isKindOf<child1>(Y)   );

            xassert( xcore::rtti::isKindOf<test>(Y)     );
            xassert( xcore::rtti::isKindOf<test2>(Y)    );

            xassert( !xcore::rtti::isKindOf<unknown2>(Y) );
            xassert( !xcore::rtti::isKindOf<unknown>(Y)  );
        }

        {
            child2 X;
            xassert( !xcore::rtti::isKindOf<child3>(X)  );
            xassert( xcore::rtti::isKindOf<child2>(X)   );
            xassert( xcore::rtti::isKindOf<child1>(X)   );

            xassert( xcore::rtti::isKindOf<test>(X)     );
            xassert( !xcore::rtti::isKindOf<test2>(X)   );

            xassert( !xcore::rtti::isKindOf<unknown>(X)  );
            xassert( !xcore::rtti::isKindOf<unknown2>(X) );

            test& Y = X;
            xassert( !xcore::rtti::isKindOf<child3>(Y)  );
            xassert( xcore::rtti::isKindOf<child2>(Y)   );
            xassert( xcore::rtti::isKindOf<child1>(Y)   );

            xassert( xcore::rtti::isKindOf<test>(Y)     );
            xassert( !xcore::rtti::isKindOf<test2>(Y)   );

            xassert( !xcore::rtti::isKindOf<unknown>(Y)  );
            xassert( !xcore::rtti::isKindOf<unknown2>(Y) );
        }

        {
            child1 X;

            xassert( !xcore::rtti::isKindOf<child3>(X)  );
            xassert( !xcore::rtti::isKindOf<child2>(X)   );
            xassert( xcore::rtti::isKindOf<child1>(X)   );

            xassert( xcore::rtti::isKindOf<test>(X)     );
            xassert( !xcore::rtti::isKindOf<test2>(X)   );

            xassert( !xcore::rtti::isKindOf<unknown>(X)  );
            xassert( !xcore::rtti::isKindOf<unknown2>(X) );
        }

        {
            test X;

            xassert( !xcore::rtti::isKindOf<child3>(X)  );
            xassert( !xcore::rtti::isKindOf<child2>(X)   );
            xassert( !xcore::rtti::isKindOf<child1>(X)   );

            xassert( xcore::rtti::isKindOf<test>(X)     );
            xassert( !xcore::rtti::isKindOf<test2>(X)   );

            xassert( !xcore::rtti::isKindOf<unknown>(X)  );
            xassert( !xcore::rtti::isKindOf<unknown2>(X) );
        }
    }

    //--------------------------------------------------------------------------------------------------------
    // Test Safe Cast
    //--------------------------------------------------------------------------------------------------------
    void Test_SafeCast( log::channel& )
    {
        // down cast const and non const
        {
            child3 X;
            rtti::SafeCast<child3>(X).m_C = 3;
            rtti::SafeCast<child2>(X).m_B = 2;
            rtti::SafeCast<child1>(X).m_A = 1;

            rtti::SafeCast<test>(X).m_X  = 10;
            rtti::SafeCast<test2>(X).m_Y = 100;

            const child3& Y = X;
            xassert( rtti::SafeCast<const child3>(Y).m_C == 3 );
            xassert( rtti::SafeCast<const child2>(Y).m_B == 2 );
            xassert( rtti::SafeCast<const child1>(Y).m_A == 1 );

            xassert( rtti::SafeCast<const test>(Y).m_X  == 10 );
            xassert( rtti::SafeCast<const test2>(Y).m_Y == 100 );
        }

        // cast up const and non const
        {
            child3 xx;
            test& X = xx;

            rtti::SafeCast<child3>(X).m_C = 3;
            rtti::SafeCast<child2>(X).m_B = 2;
            rtti::SafeCast<child1>(X).m_A = 1;

            rtti::SafeCast<test>(X).m_X  = 10;
            rtti::SafeCast<test2>(X).m_Y = 100;

            const test& Y = X;
            xassert( rtti::SafeCast<const child3>(Y).m_C == 3 );
            xassert( rtti::SafeCast<const child2>(Y).m_B == 2 );
            xassert( rtti::SafeCast<const child1>(Y).m_A == 1 );

            xassert( rtti::SafeCast<const test>(Y).m_X  == 10 );
            xassert( rtti::SafeCast<const test2>(Y).m_Y == 100 );

            const test2& Z = xx;
            xassert( rtti::SafeCast<const child3>(Z).m_C == 3 );
            xassert( rtti::SafeCast<const child2>(Z).m_B == 2 );
            xassert( rtti::SafeCast<const child1>(Z).m_A == 1 );

            xassert( rtti::SafeCast<const test>(Z).m_X  == 10 );
            xassert( rtti::SafeCast<const test2>(Z).m_Y == 100 );
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    void Test( void )
    {
        log::channel Channel("xcore::rtti");
        Test_isKindOf( Channel );
        Test_SafeCast( Channel );
    }
}