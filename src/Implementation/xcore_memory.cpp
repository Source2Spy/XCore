
namespace xcore::memory
{
    //----------------------------------------------------------------------------------------
    void* AlignMalloc(units::bytes Size, std::size_t Align) noexcept
    {
        #if _XCORE_PLATFORM_WINDOWS
        return _aligned_malloc(Size.m_Value, Align);
        #else
        #endif
    }

    //----------------------------------------------------------------------------------------
    void AlignFree(void* pData) noexcept
    {
        xassert(pData);
        #if _XCORE_PLATFORM_WINDOWS
        _aligned_free(pData);
        #else
        #endif
    }
}