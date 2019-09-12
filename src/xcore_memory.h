

namespace xcore::memory
{
    void* AlignedMalloc( units::bytes Size, std::size_t Align )   noexcept;
    void  AlignedFree  ( void* pData )                            noexcept;
}