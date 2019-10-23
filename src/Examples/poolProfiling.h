
namespace containers
{
    //--------------------------------------------------------------------------------
    template< typename T >
    void Performance(const char* pName, T&& Function) noexcept
    {
        printf(pName);
        const auto t1 = std::chrono::high_resolution_clock::now();
        Function();
        const auto t2 = std::chrono::high_resolution_clock::now();
        const auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        printf("Done: %f seconds\n", time_span.count());
    }

    //--------------------------------------------------------------------------------
    // Test
    //--------------------------------------------------------------------------------
    void Test() noexcept
    {
        constexpr static auto       Count = 200000000;

        printf("#################################################################################\n");
        printf("# Profiling Containers\n");
        printf("#################################################################################\n\n");

        //
        // Test pool
        //
        {
            pool<xcore::matrix4, 1024 * 4 > Pool;

            Performance("Pool Allocation: ", [&]
            {
                for (int i = 0; i < Count; i++)
                {
                    if ((i % (Count / 60)) == 0) printf(".");
                    Pool.append(xcore::matrix4::Identity());
                }
            });

            Performance("Pool Iteration: ", [&]
            {
                int i = 0;
                for (auto& E : Pool)
                {
                    if ((i++ % (Count / 60)) == 0) printf(".");
                    E *= E;
                }
            });

            Performance("Pool Deletion: ", [&]
            {
                for (int i = 0; i < Count; i++)
                {
                    if ((i % (Count / 60)) == 0) printf(".");
                    Pool.deleteBySwap(0);
                }
            });
        }

        //
        // Test vector
        //
        {
            std::vector<xcore::matrix4> Vector;

            Performance("Vector Allocation: ", [&]
            {
                for (int i = 0; i < Count; i++)
                {
                    if ((i % (Count / 60)) == 0) printf(".");
                    Vector.push_back(xcore::matrix4::Identity());
                }
            });

            Performance("Vector Iteration: ", [&]
            {
                int i = 0;
                for (auto& E : Vector)
                {
                    if ((i++ % (Count / 60)) == 0) printf(".");
                    E *= E;
                }
            });

            Performance("Vector Deletion: ", [&]
            {
                for (int i = 0; i < Count; i++)
                {
                    if ((i % (Count / 60)) == 0) printf(".");
                    Vector.erase(Vector.begin());
                }
            });
        }
        printf("\n#################################################################################\n");
        printf("# Done\n");
        printf("#################################################################################\n");
    }
}