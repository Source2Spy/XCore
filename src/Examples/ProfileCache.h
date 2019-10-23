
namespace profile::cache
{
    //-----------------------------------------------------------------------------------------
    // Quick random generation
    //-----------------------------------------------------------------------------------------
    static unsigned long x = 123456789, y = 362436069, z = 521288629;

    constexpr
    unsigned long xorshf96(void) noexcept
    {          //period 2^96-1
        x ^= x << 16;
        x ^= x >> 5;
        x ^= x << 1;

        unsigned long t = x;
        x = y;
        y = z;
        z = t ^ x ^ y;

        return z;
    }

    //-----------------------------------------------------------------------------------------
    // test type
    //-----------------------------------------------------------------------------------------
    struct test
    {
        constexpr static auto                  m_Count = 2000000;
        constexpr static auto                  m_Loops = m_Count * 100;

        xcore::unique_span<xcore::matrix4>     m_Scene;
        xcore::unique_span<int>                m_lIndex;
        xcore::unique_span<xcore::matrix4*>    m_lPointers;

        void Init(void) noexcept
        {
            printf("Allocating....\n");
            m_lPointers.New(m_Count).CheckError();
            m_lIndex.New(m_Count).CheckError();
            m_Scene.New(m_Count, xcore::matrix4::Identity()).CheckError();

            printf("Initialize indices....\n");
            int i = 0;
            for (auto& E : m_lIndex) E = i++;

            printf("Randomizing....\n");
            for (i = 0; i < m_Count * 3; ++i)
            {
                auto Ia = xorshf96() % m_Count;
                auto Ib = xorshf96() % m_Count;
                std::swap(m_lIndex[Ia], m_lIndex[Ib]);
            }

            printf("Setting IndexB....\n");
            for (auto& E : m_lPointers)
            {
                E = &m_Scene[m_lIndex[m_lPointers.getIndexByEntry(E)]];
            }
        }

        //--------------------------------------------------------------------------------
        template< typename T >
        void Performance(T&& Function) noexcept
        {
            const auto t1 = std::chrono::high_resolution_clock::now();
            Function();
            const auto t2 = std::chrono::high_resolution_clock::now();
            const auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
            printf("Done: %f seconds\n", time_span.count());
        }

        //--------------------------------------------------------------------------------
        template<std::size_t T_NPREFETCH, bool T_DO_POINTERS >
        void TestIndirect(void) noexcept
        {
            printf("Indirect( %s, Prefetch:%lld ):"
                , T_DO_POINTERS ? "Ptr" : "Index"
                , T_NPREFETCH);

            Performance([&]
            {
                for (auto i = 0; i < m_Loops; i++)
                {
                    volatile auto x = xorshf96(); // pay the second const here
                    const auto I = i % m_Count;
                    if (i % (m_Count * 2) == 0) printf(".");
                    if constexpr (T_DO_POINTERS)
                    {
                        if constexpr (!!T_NPREFETCH)
                        {
                            for (auto x = 0; x < (T_NPREFETCH + 1); x++)
                            {
                                _mm_prefetch((const char*)m_lPointers[(i + x) % m_Count], _MM_HINT_NTA);
                            }
                        }
                        *m_lPointers[I] *= *m_lPointers[I];
                    }
                    else
                    {
                        if constexpr (!!T_NPREFETCH)
                        {
                            for (auto x = 0; x < (T_NPREFETCH + 1); x++)
                            {
                                _mm_prefetch((const char*)&m_Scene[m_lIndex[(i + x) % m_Count]], _MM_HINT_NTA);
                            }
                        }
                        m_Scene[m_lIndex[I]] *= m_Scene[m_lIndex[I]];
                    }
                }
            });
        }

        //--------------------------------------------------------------------------------
        template< bool T_RANDOM >
        void TestDirect(void) noexcept
        {
            printf("Direct( %s ):"
                , T_RANDOM ? "Random" : "Linear");

            Performance([&]
            {
                for (auto i = 0; i < m_Loops; i++)
                {
                    volatile auto x = xorshf96();
                    const auto I = (T_RANDOM ? x : i) % m_Count;
                    if (i % (m_Count * 2) == 0) printf(".");
                    m_Scene[I] *= m_Scene[I];
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------
    // Test Function
    //-----------------------------------------------------------------------------------------

    void Test(void) noexcept
    {
        printf("#################################################################################\n");
        printf("# Profiling Cache\n");
        printf("#################################################################################\n\n");
        test Test;
        Test.Init();

        printf("Run Tests....\n\n");
        Test.TestDirect<true>();
        Test.TestDirect<false>();
        Test.TestIndirect<0, true>();
        Test.TestIndirect<2, true>();
        Test.TestIndirect<4, true>();
        Test.TestIndirect<6, true>();
        Test.TestIndirect<8, true>();
        Test.TestIndirect<4, false>();
        Test.TestIndirect<0, false>();

        printf("\n#################################################################################\n");
        printf("# Done\n");
        printf("#################################################################################\n");
    }
}