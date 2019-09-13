
namespace xcore::memory
{
    //----------------------------------------------------------------------------------------
    void* AlignedMalloc(units::bytes Size, std::size_t Align) noexcept
    {
        void* ptr;
#if _XCORE_PLATFORM_WINDOWS
        ptr = _aligned_malloc(Size.m_Value, Align);
#else
#endif
        XCORE_PERF_ALLOC_S(ptr, Size.m_Value, 10);
        return ptr;
    }

    //----------------------------------------------------------------------------------------
    void AlignedFree(void* pData) noexcept
    {
        xassert(pData);
        free(pData);
#if _XCORE_PLATFORM_WINDOWS
        _aligned_free(pData);
#else
#endif
        XCORE_PERF_FREE_S(pData, 10);
    }
}

//----------------------------------------------------------------------------------------
// Custom New/Delete (We added to the profiler)
//----------------------------------------------------------------------------------------

// "old" unaligned overloads
void* operator new(std::size_t size)
{
    auto ptr = malloc(size);
    XCORE_PERF_ALLOC_S(ptr, size, 10);
    return ptr ? ptr : throw std::bad_alloc{};
}

void operator delete(void* ptr, std::size_t size)
{
    free(ptr);
    XCORE_PERF_FREE_S(ptr, 10);
}

void operator delete(void* ptr)
{
    free(ptr);
    XCORE_PERF_FREE_S(ptr, 10);
}

// "new" over-aligned overloads
void* operator new(std::size_t size, std::align_val_t align)
{
    auto ptr = _aligned_malloc(size, static_cast<std::size_t>(align));
    XCORE_PERF_ALLOC_S(ptr, size, 10);
    return ptr ? ptr : throw std::bad_alloc{};
}

void operator delete(void* ptr, std::size_t size, std::align_val_t align)
{
    _aligned_free(ptr);
    XCORE_PERF_FREE_S(ptr, 10);
}

void operator delete(void* ptr, std::align_val_t align)
{
    _aligned_free(ptr);
    XCORE_PERF_FREE_S(ptr, 10);
}
