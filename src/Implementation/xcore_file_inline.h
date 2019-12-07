
namespace xcore::file 
{
    //------------------------------------------------------------------------------

    template<class T> inline
    xcore::err stream::Write( const T&& Val ) noexcept
    {
        static_assert(std::is_enum<T>::value || std::is_fundamental<T>::value, "std::is_enum<T>::value || std::is_fundamental<T>::value");
        xassert( m_pFile );
        return WriteRaw({ (std::byte*) &Val, sizeof(T) });
    }

    //------------------------------------------------------------------------------

    template<class T> inline
    xcore::err stream::Write(const T& Val) noexcept
    {
        return Write(std::move(Val));
    }

    //------------------------------------------------------------------------------

    template<typename T> inline
    xcore::err stream::WriteView( const T&& A ) noexcept
    {
        const std::span View{ A };
        xassert(m_pFile);
        return WriteRaw({ (std::byte*)View.data(), sizeof( decltype(View[0]) ) * View.size() });
    }

    //------------------------------------------------------------------------------

    template<typename T> inline
    xcore::err stream::WriteView(const T& Val) noexcept
    {
        return WriteView(std::move(Val));
    }

    //------------------------------------------------------------------------------

    template<class T> inline
    xcore::err stream::Read( T&& Val ) noexcept
    {
        static_assert(std::is_enum<T>::value || std::is_fundamental<T>::value, "std::is_enum<T>::value || std::is_fundamental<T>::value");
        xassert(m_pFile);
        return ReadRaw({ (std::byte*)&Val, sizeof(T) });
    }

    //------------------------------------------------------------------------------

    template<class T> inline
    xcore::err stream::Read(T& Val) noexcept
    {
        return Read(std::move(Val));
    }

    //------------------------------------------------------------------------------

    template<typename T> inline
    xcore::err stream::ReadView( T&& A ) noexcept
    {
        xassert(m_pFile);
        std::span       View = { A };
        using t = decltype(View[0]);
        return ReadRaw(
            { 
                (std::byte*)View.data()
                , sizeof(t) * View.size() 
            });
    }

    //------------------------------------------------------------------------------

    template<typename T> inline
    xcore::err stream::ReadView(T& Val) noexcept
    {
        return ReadView(std::move(Val));
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool stream::isBinaryMode( void ) const noexcept
    {
        xassert(m_pFile);
        return !m_AccessType.m_bText;
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool stream::isReadMode( void ) const noexcept
    {
        xassert(m_pFile);
        return m_AccessType.m_bRead;
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool stream::isWriteMode(void) const noexcept
    {
        xassert(m_pFile);
        return m_AccessType.m_bWrite;
    }

    //------------------------------------------------------------------------------
    xforceinline
    void stream::setForceFlush(bool bOnOff) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        m_AccessType.m_bForceFlush = bOnOff;
    }

    //------------------------------------------------------------------------------
    xforceinline
    void stream::AsyncAbort(void) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        if( m_AccessType.m_bASync == false ) return;

        m_pDevice->AsyncAbort(m_pFile);
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::Synchronize(bool bBlock) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        xcore::err Error;

        if( m_AccessType.m_bASync == false )
        {
            if (isEOF()) return xerr_code( Error, error::UNEXPECTED_EOF, "Synchronize end of file");
            return Error;
        }

        return m_pDevice->Synchronize(m_pFile, bBlock);
    }

    //------------------------------------------------------------------------------
    xforceinline
    void stream::Flush(void) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);
        m_pDevice->Flush(m_pFile);
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::SeekOrigin( units::bytes Offset ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        return m_pDevice->Seek( m_pFile, device_i::SKM_ORIGIN, Offset );
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::SeekEnd( units::bytes Offset ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        return m_pDevice->Seek( m_pFile, device_i::SKM_END, Offset );
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::SeekCurrent( units::bytes Offset ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        return m_pDevice->Seek( m_pFile, device_i::SKM_CURENT, Offset );
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::Tell( units::bytes& Pos ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        return m_pDevice->Tell(m_pFile, Pos);
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool stream::isEOF(void) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        return m_pDevice->isEOF(m_pFile);
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::getC( int& C ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        xcore::err      Error;
        std::uint8_t    x;
        if (ReadRaw(std::span<std::byte>{ (std::byte*)& x, sizeof(x)}).isError(Error)) return Error;

        C = x;
        return Error;
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::putC( int aC, int Count, bool bUpdatePos ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        xcore::err      Error;
        units::bytes    iPos{ 0 };
        std::uint8_t    C   = types::static_cast_safe<std::uint8_t>(aC);

        if (Count == 0) return Error;
        if (bUpdatePos == false) if (Tell(iPos).isError(Error)) return Error;
        for( int i = 0; i < Count; i++ )
        {
            if (WriteRaw(std::span<std::byte>{ (std::byte*)&C, sizeof(C) }).isError(Error))
                return Error;
        }
        if (bUpdatePos == false) if (SeekOrigin(iPos).isError(Error)) return Error;

        return Error;
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err stream::AlignPutC( int C, int Count, int Aligment, bool bUpdatePos ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);

        xcore::err      Error;
        units::bytes    Pos{ 0 };

        // First solve the alignment issue
        if (Tell(Pos).isError(Error)) return Error;
        int PutCount = static_cast<int>(xcore::bits::Align(Count + Pos.m_Value, Aligment) - Pos.m_Value);

        // Put all the necessary characters
        return putC(C, PutCount, bUpdatePos);
    }

    //------------------------------------------------------------------------------
    xforceinline
    xcore::err stream::getFileLength( units::bytes& Length ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);
        return m_pDevice->Length(m_pFile, Length);
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err stream::ToFile( stream& File ) noexcept
    {
        xcore::err Error;

        // Seek at the begging of the file
        if( SeekOrigin(units::bytes{ 0 }).isError(Error) ) return Error;

        units::bytes                        i;
        xcore::array<std::byte, 2 * 256>    Buffer;
        units::bytes                        Length;
        
        if (getFileLength(Length).isError(Error)) return Error;

        if (Length.m_Value > static_cast<std::int64_t>(Buffer.size()))
        {
            Length -= units::bytes{ Buffer.size() };
            for (i = units::bytes{ 0 }; i < Length; i += units::bytes{ 256 })
            {
                if (ReadRaw(Buffer).isError(Error)) return Error;
                if ( File.WriteRaw(Buffer).isError(Error)) return Error;
            }
            Length += units::bytes{ Buffer.size() };
        }

        // Write trailing bytes
        auto S = static_cast<std::size_t>((Length - i).m_Value);
        if (S)
        {
            auto FinalView = std::span<std::byte>{ (std::byte*)Buffer.data(), static_cast<std::size_t>((Length - i).m_Value) };
            if (ReadRaw(FinalView).isError(Error)) return Error;
            return File.WriteRaw(FinalView);
        }
        return Error;
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err stream::ToMemory( std::span<std::byte> View ) noexcept
    {
        xcore::err Error;

        // Seek at the begging of the file
        if (SeekOrigin(units::bytes{ 0 }).isError(Error)) return Error;

        units::bytes Length;
        if (getFileLength(Length).isError(Error)) return Error;
        if (static_cast<std::uint64_t>(Length.m_Value) < View.size()) return xerr_failure(Error, "Buffer is too small");

        return ReadRaw({ View.data(), static_cast<std::size_t>(Length.m_Value) });
    }

    //------------------------------------------------------------------------------
    // TODO: Add the fail condition
    template<class T> inline
    xcore::err stream::ReadString( string::view<T> Buffer) noexcept
    {
        xcore::err              Error;
        int                     C;
        int                     i = 0;

        do 
        {
            if (getC(C).isError(Error)) return Error;
            Buffer[i++] = static_cast<T>(C);
        
        } while( C );

        return Error;
    }

    //------------------------------------------------------------------------------

    template<typename T> inline
    xcore::err stream::WriteString( const string::view<T> String ) noexcept
    {
        const auto View = String.ViewTo(static_cast<int>(string::Length(String).m_Value + 1));
        return WriteView(View);
    }

    //------------------------------------------------------------------------------

    template<typename... T_ARGS> inline
    xcore::err stream::Printf( const char* pFormatStr, const T_ARGS& ... Args ) noexcept
    {
        string::ref<char> Data;
        Data.MakeUnique(512);
        const auto c = xcore::string::sprintf( Data.getView(), pFormatStr, xarg_list(sizeof...(T_ARGS), Args...));
        return WriteList( Data.getView().ViewTo(c) );
    }
}
