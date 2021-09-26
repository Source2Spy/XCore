namespace xcore::file::driver::ram
{
    //==============================================================================
    //  MEMORY FILE CLASS
    //==============================================================================
    //  memfile 
    //      memfile is a class that contains the interface to access the memory files.
    //      currently, it is implemented as an array of memblock's of the size
    //      block_size_v
    //==============================================================================

    struct memfile
    {
        xcore::err          Read            (std::span<std::byte> View)                 noexcept;
        xcore::err          Write           (const std::span<const std::byte> View)     noexcept;
        void                SeekOrigin      (std::int64_t Offset)                       noexcept { m_SeekPosition  = Offset;         xassert(m_SeekPosition<=m_EOF && m_SeekPosition >= 0); }
        void                SeekCurrent     (std::int64_t Offset)                       noexcept { m_SeekPosition += Offset;         xassert(m_SeekPosition<=m_EOF && m_SeekPosition >= 0); }
        void                SeekEnd         (std::int64_t Offset)                       noexcept { m_SeekPosition  = m_EOF - Offset; xassert(m_SeekPosition<=m_EOF && m_SeekPosition >= 0); }
        auto                Tell            (void)                                      noexcept { return m_SeekPosition; }
        bool                isEOF           (void)                                      noexcept { return m_SeekPosition > m_EOF; }
        auto                getFileLength   (void)                                      noexcept { return m_EOF; }

        constexpr static std::size_t block_size_v = 1024 * 10;
        using block = std::array< std::byte, block_size_v >;

        std::vector<std::unique_ptr<block>>     m_lBlock        {};
        std::int64_t                            m_SeekPosition  { 0 };
        std::int64_t                            m_EOF           { 0 };
    };

    //------------------------------------------------------------------------------

    class device final : public xcore::file::device_i
    {
    public:
        constexpr                           device          (void)                                                                  noexcept : xcore::file::device_i{ "ram:" } {}

    protected:

        virtual         void*               open            (const string::view<const wchar_t> FileName, access_types Flags)        noexcept override;
        virtual         void                close           (void* pFile)                                                           noexcept override;
        virtual         xcore::err          Read            (void* pFile, std::span<std::byte> View)                                noexcept override;
        virtual         xcore::err          Write           (void* pFile, const std::span<const std::byte> View)                    noexcept override;
        virtual         xcore::err          Seek            (void* pFile, seek_mode Mode, units::bytes Pos)                         noexcept override;
        virtual         xcore::err          Tell            (void* pFile, units::bytes& Pos)                                        noexcept override;
        virtual         void                Flush           (void* pFile)                                                           noexcept override;
        virtual         xcore::err          Length          (void* pFile, units::bytes& L)                                          noexcept override;
        virtual         bool                isEOF           (void* pFile)                                                           noexcept override;
        virtual         xcore::err          Synchronize     (void* pFile, bool bBlock)                                              noexcept override;
        virtual         void                AsyncAbort      (void* pFile)                                                           noexcept override;
    };

    //------------------------------------------------------------------------------
    inline
    void* device::open( const string::view<const wchar_t> FileName, access_types Flags ) noexcept 
    { 
        return new(memfile); 
    }

    //------------------------------------------------------------------------------
    inline
    void device::close (void* pFile) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        delete(&M);
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err device::Read (void* pFile, std::span<std::byte> View) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        return M.Read(View);
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err device::Write (void* pFile, const std::span<const std::byte> View) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        return M.Write(View);
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err device::Tell (void* pFile, units::bytes& Pos) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        Pos.m_Value = M.Tell();
        return {}; 
    }

    //------------------------------------------------------------------------------
    inline
    void device::Flush (void* pFile) noexcept 
    { 
        xassert(pFile); 
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err device::Length (void* pFile, units::bytes& L) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        L.m_Value = M.getFileLength();
        return {}; 
    }

    //------------------------------------------------------------------------------
    inline
    bool device::isEOF (void* pFile) noexcept 
    {
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        return M.isEOF();
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err device::Synchronize (void* pFile, bool bBlock) noexcept 
    { 
        xassert(pFile); 
        auto& M = *reinterpret_cast<memfile*>(pFile);
        return {};
    }

    //------------------------------------------------------------------------------
    inline
    void device::AsyncAbort (void* pFile) noexcept 
    { 
        xassert(pFile);
        auto& M = *reinterpret_cast<memfile*>(pFile);
    }

    //------------------------------------------------------------------------------

    xcore::err device::Seek(void* pFile, seek_mode Mode, units::bytes Pos) noexcept
    {
        xassert(pFile);
        auto& M = *reinterpret_cast<memfile*>(pFile);

        xassert(Pos.m_Value >= 0);
        switch (Mode)
        {
        case seek_mode::SKM_ORIGIN: M.SeekOrigin(Pos.m_Value); break;
        case seek_mode::SKM_CURENT: M.SeekCurrent(Pos.m_Value); break;
        case seek_mode::SKM_END:    M.SeekEnd(Pos.m_Value); break;
        default: xassert(0); break;
        }

        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err memfile::Read(std::span<std::byte> View) noexcept
    {
        if (m_SeekPosition >= m_EOF)
            return xerr_code_s( xcore::file::error::UNEXPECTED_EOF, "Unexpected End of File" );

        auto currentBlockIndex  = m_SeekPosition / block_size_v;
        auto currentBlockOffset = m_SeekPosition % block_size_v;
        std::uint64_t bufferOffset = 0;

        xassert(currentBlockIndex >= 0 && currentBlockIndex < m_lBlock.size());

        while (bufferOffset < View.size())
        {
            // Copy data from blocks.
            View[bufferOffset] = (*m_lBlock[currentBlockIndex])[currentBlockOffset];
            currentBlockOffset++;
            bufferOffset++;
            m_SeekPosition++;
            if (currentBlockOffset >= block_size_v)
            {
                // Since we've completed the current block, increment to next block.
                currentBlockIndex++;
                currentBlockOffset = 0;

                if (currentBlockIndex >= m_lBlock.size())
                    return xerr_failure_s("Failt to read all the bytes from the ram drive");
            }
        }

        return {};
    }

    //------------------------------------------------------------------------------

    xcore::err memfile::Write(const std::span<const std::byte> View) noexcept
    {
        // Check current position and size of data being added.
        const auto NewDataPosition = m_SeekPosition + View.size();
        if (m_EOF < static_cast<std::int64_t>(NewDataPosition)) m_EOF = NewDataPosition;

        // If we need to allocate more memory for the blocks, then do so.
        if (m_EOF >= static_cast<std::int64_t>(m_lBlock.size() * block_size_v))
        {
            auto NumBlocksRequired = m_EOF / block_size_v + 1;
            NumBlocksRequired -= m_lBlock.size();

            // Allocate list of blocks
            for (int i = 0; i < NumBlocksRequired; i++)
            {
                m_lBlock.push_back(std::unique_ptr<block>{ new block });
            }
        }

        auto currentBlockIndex  = m_SeekPosition / block_size_v;
        auto currentBlockOffset = m_SeekPosition % block_size_v;
        std::uint64_t bufferOffset = 0;

        xassert(currentBlockIndex >= 0 && currentBlockIndex < m_lBlock.size());

        while (bufferOffset < View.size())
        {
            // Copy data into blocks.
            (*m_lBlock[currentBlockIndex])[currentBlockOffset] = View[bufferOffset];

            bufferOffset++;
            currentBlockOffset++;
            m_SeekPosition++;

            if (currentBlockOffset >= block_size_v)
            {
                // Since we've completed the current block, increment to next block.
                currentBlockIndex++;
                currentBlockOffset = 0;
                xassert(currentBlockIndex < m_lBlock.size());
            }
        }

        return {};
    }
}