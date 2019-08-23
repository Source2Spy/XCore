#include <chrono>

namespace xcore::lockless::queues::examples
{
    static constexpr int testItems = xcore::target::isDebug() ? (25000 * 64 / 3 * 10) : (250000 * 64 / 3 * 10);
    static constexpr int testThreadConsumerThreads = 8;
    static constexpr int testThreadProducerThreads = 8;
    static constexpr int testItemsPerConsumerThread = testItems / testThreadConsumerThreads;
    static constexpr int testItemsPerProducerThread = testItems / testThreadProducerThreads;

    //------------------------------------------------------------------------------

    template<typename T> 
    class IQueue
    {
    public:
        virtual std::size_t size() const = 0;
        virtual std::size_t capacity() const = 0;
        virtual bool push(const T& data) = 0;
        virtual bool pop(T& result) = 0;
    };

    //------------------------------------------------------------------------------

    template<typename T, class Q > 
    class TestQueue : public IQueue<T>
    {
    public:

        std::size_t size() const { return queue.size(); }
        std::size_t capacity() const { return queue.capacity(); }
        bool push(const T& data) { return queue.push(data); }
        bool pop(T& result) { return queue.pop(result); }

    private:
        Q queue;
    };

    //------------------------------------------------------------------------------

    std::atomic< std::size_t > producerSum;
    std::atomic< std::size_t > consumerSum;
    std::atomic<std::chrono::duration<double>> maxPushDuration;
    std::atomic<std::chrono::duration<double>> maxPopDuration;

    //------------------------------------------------------------------------------

    std::uint32_t producerThread(void* param)
    {
        auto pQueue = reinterpret_cast<IQueue<int>*>(param);

        for( int i = 0; i < testItemsPerProducerThread; ++i )
        {
            for (;;)
            {
                auto startTime = std::chrono::high_resolution_clock::now();
                while( !pQueue->push(i) )
                {
                    std::this_thread::yield();
                    startTime = std::chrono::high_resolution_clock::now();
                }
                {
                    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - startTime;
                    for (;;)
                    {
                        auto lmaxPushDuration = maxPushDuration.load();
                        if( duration <= lmaxPushDuration || maxPushDuration.compare_exchange_weak( lmaxPushDuration, duration) )
                            break;
                    }
                    break;
                }
            }
            producerSum.fetch_add( i );
        }
        return 0;
    }

    //------------------------------------------------------------------------------

    std::uint32_t consumerThread( void* param )
    {
        auto pQueue = reinterpret_cast<IQueue<int>*>(param);

        int val;
        for (int i = 0; i < testItemsPerConsumerThread; ++i)
        {
            for (;;)
            {
                auto startTime = std::chrono::high_resolution_clock::now();
                while (!pQueue->pop(val))
                {
                    std::this_thread::yield();
                    startTime = std::chrono::high_resolution_clock::now();
                }
                {
                    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - startTime;
                    for (;;)
                    {
                        auto lmaxPopDuration = maxPopDuration.load();
                        if( duration <= lmaxPopDuration || maxPopDuration.compare_exchange_weak( lmaxPopDuration, duration) )
                            break;  
                    }
                    break;
                }
            }
            consumerSum.fetch_add( i );
        }
        return 0;
    }

    //------------------------------------------------------------------------------

    template< template< typename T, std::size_t > class Q > 
    void testQueue(const char* pName, bool fifo = false )
    {
        printf("Testing %s... \n", pName );
        {
            std::unique_ptr<TestQueue<int, Q<int,1024*16>>> queue { new TestQueue<int, Q<int,1024*16>>{} };
            int result=0;
            xassert(queue->capacity() >= 1024*16);
            xassert(!queue->pop(result));
            xassert(queue->push(42));
            xassert(queue->size()==1);
            xassert(queue->pop(result));
            xassert(result == 42);
            xassert(!queue->pop(result));
            xassert(queue->size()==0);
        }

        {
            std::unique_ptr<TestQueue<int, Q<int,2>>> queue { new TestQueue<int, Q<int,2>>{} };
            int result=0;
            xassert(queue->capacity() >= 2);
            xassert(!queue->pop(result));
            xassert(queue->push(42));
            xassert(queue->size()==1);
            xassert(queue->push(43));
            xassert(queue->size()==2);
            xassert(queue->pop(result));
            xassert(queue->size()==1);
            xassert(result == (fifo ? 43 : 42));
            xassert(queue->pop(result));
            xassert(queue->size()==0);
            xassert(result == (fifo ? 42 : 43));
            xassert(!queue->pop(result));
            xassert(queue->push(44));
            xassert(queue->push(45));
            xassert(queue->pop(result));
            xassert(result == (fifo ? 45 : 44));
            xassert(queue->push(47));
        }

        producerSum = 0;
        consumerSum = 0;
        maxPushDuration.store( std::chrono::duration<double>{0} );
        maxPopDuration.store( std::chrono::duration<double>{0} );

        std::vector<std::unique_ptr<std::thread>> lThreads;
        auto microStartTime = std::chrono::high_resolution_clock::now();
        {
            std::unique_ptr<TestQueue<int, Q<int,128>>>     queue { new TestQueue<int, Q<int,128>>{} };

            for (int i = 0; i < testThreadProducerThreads; ++i)
            {
                lThreads.push_back( std::unique_ptr<std::thread>{new std::thread( producerThread, queue.get() )} );
            }

            for (int i = 0; i < testThreadConsumerThreads; ++i)
            {
                lThreads.push_back( std::unique_ptr<std::thread>{new std::thread(consumerThread, queue.get() )} );
            }

            for( auto& pE : lThreads )
                pE->join();

            lThreads.clear();

            xassert(queue->size() == 0);
            xassert(producerSum == consumerSum);
        }

        std::chrono::duration<double> microDuration = std::chrono::high_resolution_clock::now() - microStartTime;
        printf( "%f ms, maxPush: %f microseconds, maxPop: %f microseconds\n", microDuration.count() / 1000, maxPushDuration.load().count(), maxPopDuration.load().count() );
    }

    void Test(void)
    {
        for (int i = 0; i < 5; ++i)
        {
            printf("--- Run %d ---\n", i);
            testQueue<xcore::lockless::queues::v1::mpmc_bounded >("V1");
            testQueue<xcore::lockless::queues::v2::mpmc_bounded >("V2");
        }

        int a=22;
    }
}