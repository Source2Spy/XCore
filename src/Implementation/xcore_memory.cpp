
namespace xcore::memory
{
    //----------------------------------------------------------------------------------------
    void* AlignedMalloc(units::bytes Size, std::size_t Align) noexcept
    {
        void* ptr;
#if _XCORE_PLATFORM_WINDOWS
        ptr = _aligned_malloc(Size.m_Value, Align);
#else
    #error "need to define an align alloc"
#endif
        XCORE_PERF_ALLOC_S(ptr, Size.m_Value, 10);
        return ptr;
    }

    //----------------------------------------------------------------------------------------
    void AlignedFree(void* pData) noexcept
    {
        xassert(pData);
#if _XCORE_PLATFORM_WINDOWS
        _aligned_free(pData);
#else
        free(pData);
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
    auto ptr =  xcore::memory::AlignedMalloc
    ( 
          xcore::units::bytes{ static_cast<std::int64_t>(size) }
        , __STDCPP_DEFAULT_NEW_ALIGNMENT__
    );
    return ptr ? ptr : throw std::bad_alloc{};
}

void operator delete(void* ptr, std::size_t size)
{
    xcore::memory::AlignedFree(ptr);
}

void operator delete(void* ptr)
{
    xcore::memory::AlignedFree(ptr);
}

// "new" over-aligned overloads
void* operator new(std::size_t size, std::align_val_t align)
{
    auto ptr = xcore::memory::AlignedMalloc
    ( 
          xcore::units::bytes{ static_cast<std::int64_t>(size) }
        , static_cast<std::size_t>(align)
    );
    return ptr ? ptr : throw std::bad_alloc{};
}

void operator delete(void* ptr, std::size_t size, std::align_val_t align)
{
    xcore::memory::AlignedFree(ptr);
}

void operator delete(void* ptr, std::align_val_t align)
{
    xcore::memory::AlignedFree(ptr);
}
