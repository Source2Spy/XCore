

namespace xcore::file::examples
{
    //-----------------------------------------------------------------------------------------

    xcore::err syncModeTest( string::view<wchar_t> FileName, bool bCloseFile )
    {
        file::stream            File;
        xcore::err              Error;
        xcore::error::scope     Scope(Error, [&]
        {
            int x = 22;
            XCORE_BREAK;
        });

        //
        // Write file
        //
        if (File.open(FileName, "w").isError(Error)) return Error;

        const string::constant Header{ "TestFileHeader" };
        if (File.WriteString(Header.getView()).isError(Error)) return Error;

        units::bytes FileSize;
        if (File.Tell(FileSize).isError(Error)) return Error;

        units::bytes RealSize = units::bytes{ string::Length(Header).m_Value } +units::bytes{ 1 };
        xassert( FileSize == RealSize );

        xcore::array< std::uint32_t, 3245> Buffer{};
        for (std::uint32_t i = 0; i < Buffer.size(); i++)
        {
            Buffer[i] = i;
        }

        if (File.WriteView(Buffer).isError(Error)) return Error;

         
        if (File.Tell(FileSize).isError(Error)) return Error;
        RealSize.m_Value += Buffer.size() * sizeof(std::uint32_t);
        xassert(FileSize == RealSize);

        if (File.Write(Buffer.size()).isError(Error)) return Error;

        if (File.Tell(FileSize).isError(Error)) return Error;
        RealSize.m_Value += sizeof(decltype(Buffer.size()));
        xassert(FileSize == RealSize);

        // Done
        if (bCloseFile) File.close();

        //
        // Clear the buffer
        //
        for (std::size_t i = 0; i < Buffer.size(); i++)
        {
            Buffer[i] = 0;
        }

        //
        // Read file
        //
        if (bCloseFile)
        {
            if (File.open(FileName, "r").isError(Error)) return Error;
        }
        else
        {
            if (File.SeekOrigin(units::bytes{ 0 }).isError(Error)) return Error;
        }

        if (File.getFileLength(FileSize).isError(Error)) return Error;
        RealSize = units::bytes{ string::Length( Header ).m_Value } +units::bytes{ 1 };
        RealSize += units::bytes{ sizeof(std::int32_t) * Buffer.size() + sizeof(decltype(Buffer.size())) };
        xassert(FileSize == RealSize);

        // Read the first string
        string::fixed<char,256> NewHeader;
        if (File.ReadString(NewHeader.getView()).isError(Error)) return Error;
        xassert(NewHeader == Header);

        // Read the buffer size
        units::bytes Position;
        if (File.Tell(Position).isError(Error)) return Error;
        if (File.SeekCurrent(units::bytes{ sizeof(std::int32_t) * Buffer.size() }).isError(Error)) return Error;

        std::size_t BufferCount;
        if (File.Read(BufferCount).isError(Error)) return Error;
        xassert(BufferCount == Buffer.size());

        // Read the buffer
        if (File.SeekOrigin(Position).isError(Error)) return Error;
        if (File.ReadView(Buffer).isError(Error)) return Error;

        for (std::int32_t i = 0; i < Buffer.size(); i++)
        {
            xassert(Buffer[i] == i);
        }

        // Done
        File.close();

        return Error;
    }

    //-----------------------------------------------------------------------------------------

    xcore::err asyncModeTest(string::view<wchar_t> FileName, bool bCloseFile)
    {
        constexpr static int                            Steps    = 10;
        constexpr static int                            DataSize = 1024 * Steps;
        std::array<xcore::unique_span<std::int32_t>, 2> Buffer;
        file::stream                                    File;
        xcore::err                                      Error;
        xcore::error::scope                             Scope(Error, [&]
            {
                XCORE_BREAK;
            });

        if (Buffer[0].New(DataSize).isError(Error)) return Error;
        if (Buffer[1].New(DataSize).isError(Error)) return Error;

        for( int t = 0; t < 10; t++ )
        {
            //
            // Write something
            //
            if (1)
            {
                //
                // Write Something
                //
                if (File.open(FileName, "w@").isError(Error)) return Error;

                //
                // Safe some random data in 10 steps
                //
                int k = 0;
                for( int i = 0; i < Steps; i++)
                {
                    // this is overlap/running in parallel with actual writing the file
                    for( auto& E : Buffer[i & 1] )
                    {
                        E = k++;
                    }

                    if (File.Synchronize(true).isError(Error)) return Error;
                    if (File.WriteView(Buffer[i & 1]).isError(Error))
                    {
                        if (Error.getCode().getState<file::error>() == file::error::INCOMPLETE) Error.clear();
                        else return Error;
                    }
                }

                if (File.Synchronize(true).isError(Error)) return Error;
                if (bCloseFile) File.close();
            }

            //
            // Clear buffer
            //
            for( auto& E : Buffer )
                std::memset( E.data(), 0, E.size() );

            //
            // Read something
            //
            if (1)
            {
                if (bCloseFile)
                {
                    if (File.open(FileName, "r@").isError(Error)) return Error;
                }
                else
                {
                    if (File.SeekOrigin(units::bytes{ 0 }).isError(Error)) return Error;
                }
                int     k = 0;

                // Read the first entry
                if (File.ReadView(Buffer[0]).isError(Error))
                {
                    if (Error.getCode().getState<file::error>() == file::error::INCOMPLETE) Error.clear();
                    else return Error;
                }

                // Start reading 
                for (int i = 1; i < Steps; i++)
                {
                    if (File.ReadView(Buffer[i & 1]).isError(Error))
                    {
                        if (Error.getCode().getState<file::error>() == file::error::INCOMPLETE) Error.clear();
                        else return Error;
                    }

                    // this is overlap/running in parallel with actual reading of the file
                    for (auto& E : Buffer[(i ^ 1) & 1])
                    {
                        xassert(E == k++);
                    }
                    if (File.Synchronize(true).isError(Error)) return Error;
                }

                // Check the last thing we read            
                for (auto& E : Buffer[(Steps ^ 1) & 1])
                {
                    xassert(E == k++);
                }

                // We are done!
                File.close();
            }
        }

        return Error;
    }

    //-----------------------------------------------------------------------------------------

    void Tests(void)
    {
        for (int i = 0; i < 2; ++i)
        {
           (void)syncModeTest(string::constant(L"temp:/test.dat"), i);
           (void)asyncModeTest(string::constant(L"temp:/asyncMode.dat"), i);
        }

        (void)syncModeTest(string::constant(L"ram:/test.dat"), false);
        (void)asyncModeTest(string::constant(L"ram:/asyncMode.dat"), false);

        int a = 22;
    }
}