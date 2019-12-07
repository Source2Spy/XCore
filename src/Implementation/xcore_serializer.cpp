

namespace xcore::serializer
{
#if 0
    //------------------------------------------------------------------------------
    static
    void* MemoryAllocaterDefaultFunction( units::bytes Size, mem_type Flags) noexcept
    {
        if (Flags.m_bVRam )
        {
            // allocate default longterm vram memory
        }
        else if (Flags.m_bTempMemory)
        {
            // deal with temp memory (system memory)
        }
        else
        {
            // default longterm system memory
        }

        return xcore::memory::AlignedMalloc( Size, 16 );
    }

    //------------------------------------------------------------------------------

    stream::stream(void) noexcept
    {
        m_pMemoryCallback = MemoryAllocaterDefaultFunction;
    }

    //------------------------------------------------------------------------------

    std::uint32_t stream::writting::AllocatePack( mem_type DefaultPackFlags ) noexcept
    {
        const string::constant Name(L"ram:\\Whatever");

        // Create the default pack
        auto& WPack = m_Packs.append();
        if( auto Error = WPack.m_Data.open(Name, "w+"); Error )
        {
            xassert(false);
            xcore::log::Output("xcore::serializer::AllocatePack Failed with (%s)", Error.getCode().m_pString);
            return ~0;
        }
        WPack.m_PackFlags = DefaultPackFlags;

        return static_cast<std::uint32_t>(m_Packs.size() - 1);
    }

    //------------------------------------------------------------------------------

    xcore::err stream::HandlePtrDetails( const std::byte* pA, std::size_t SizeofA, std::size_t Count, mem_type MemoryFlags ) noexcept
    {
        xcore::err Error;

        // If the parent is in not in a common pool then its children must also not be in a common pool.
        // The theory is that if the parent is not in a common pool it could be deallocated and if the child 
        // is in a common pool it could be left orphan. However this may need to be thought out more carefully
        // so I am playing it safe for now.
        if( m_pWrite->m_Packs[m_iPack].m_PackFlags.m_bUnique )
        {
            xassert(MemoryFlags.m_bUnique);
        }
        else if (m_pWrite->m_Packs[m_iPack].m_PackFlags.m_bTempMemory)
        {
            xassert(MemoryFlags.m_bTempMemory);
        }

        //
        // If we don't have any elements then just write the pointer raw
        //
        if (Count == 0)
        {
            // always write 64 bits worth (assuming user is using xserialfile::ptr)
            // if we do not do this the upper bits of the 64 bits may contain trash
            // and if the compiler is 32bits and the game 64 then that trash can crash
            // the game.
            if( Serialize(*((std::uint64_t*)(pA))).isError(Error) )
                return Error;

            return Error;
        }

        //
        // Choose the right pack for this allocation        
        //

        // Back up the current pack
        auto BackupPackIndex = m_iPack;

        if( MemoryFlags.m_bUnique )
        {
            // Create a pack
            m_iPack = m_pWrite->AllocatePack(MemoryFlags);
        }
        else
        {
            // Search for a pool which matches our attributes
            std::uint32_t i;
            for (i = 0; i < m_pWrite->m_Packs.size(); i++)
            {
                if( (m_pWrite->m_Packs[i].m_PackFlags.m_Value & ~mem_type::Flags(mem_type::flags::UNIQUE).m_Value) == MemoryFlags.m_Value )
                    break;
            }

            // Could not find a pack with compatible flags so just create a new one
            if (i == m_pWrite->m_Packs.size())
            {
                // Create a pack
                m_iPack = m_pWrite->AllocatePack(MemoryFlags);
            }
            else
            {
                // Set the index to the compatible pack
                m_iPack = i;
            }
        }

        // Make sure we are at the end of the buffer before preallocating
        // I have change the alignment from 4 to 8 because of 64 bits OS.
        // it may help. In the future will be nice if the user could specify the alignment.
        if ( getW().SeekEnd(units::bytes{ 0 }).isError(Error) ) return Error;

        if (getW().AlignPutC(' ', static_cast<int>(SizeofA) * static_cast<int>(Count), 8, false).isError(Error))
            return Error;

        //
        // Store the pointer
        //
        {
            auto& Ref = m_pWrite->m_PointerTable.append();

            units::bytes Pos;
            if (getW().Tell(Pos).isError(Error)) return Error;

            Ref.m_PointingAT        = static_cast<std::uint32_t>(Pos.m_Value);
            Ref.m_OffsetPack        = BackupPackIndex;
            Ref.m_OffSet            = m_ClassPos + ComputeLocalOffset(pA);
            Ref.m_Count             = static_cast<std::uint32_t>(Count);
            Ref.m_PointingATPack    = m_iPack;

            // We better be at the write spot that we are pointing at 
            xassert_block_basic()
            {
                getW().Tell(Pos).CheckError();
                xassert( Ref.m_PointingAT == Pos.m_Value );
            }
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::SaveFile(void) noexcept
    {
        xcore::err      Error;
        //std::uint32_t   i;

        //
        // Go throw all the packs and compress them
        //
        for(std::uint32_t i = 0; i < m_pWrite->m_Packs.size(); i++ )
        {
            xcore::unique_span<std::byte>   RawData;
            pack_writting&                  Pack = m_pWrite->m_Packs[i];

            Pack.m_CompressSize     = 0;
            Pack.m_BlockSize        = std::min( max_block_size_v, Pack.m_UncompressSize );
            {
                units::bytes Length;
                if (Pack.m_Data.getFileLength(Length).isError(Error)) return Error;
                xassert( Length.m_Value <= std::numeric_limits<std::uint32_t>::max() );
                Pack.m_UncompressSize = static_cast<std::uint32_t>(Length.m_Value);
            }

            // Copy the pack into a memory buffer
            if (RawData.New(Pack.m_UncompressSize).isError(Error)) return Error;        // This memory it is short term we should tell the mem system about it
            if( Pack.m_Data.ToMemory(RawData).isError(Error) ) return Error;

            //
            // Now compress the memory
            //
            auto nBlocks = Pack.m_UncompressSize / Pack.m_BlockSize;
            nBlocks += static_cast<std::uint32_t>(nBlocks * Pack.m_BlockSize) < Pack.m_UncompressSize ? 1 : 0;
            if( Pack.m_CompressData.New(nBlocks * Pack.m_BlockSize + 1024*8 ).isError(Error)) return Error;

            // Do actual compression
            {
                xcore::compression::compress Compress;
                if(Compress.Init(Pack.m_BlockSize, RawData).isError(Error)) return Error;

                bool            bDone = false;
                std::uint32_t   j;
                for (j = 0; j < nBlocks; j++)
                {
                    xassert(false==bDone);
                    if (Compress.Pack(m_pWrite->m_CSizeStream.append(), Pack.m_CompressData.ViewFrom(Pack.m_CompressSize)).isError(Error))
                    {
                        if (Error.getCode().getState<xcore::compression::compress::error_state>() != xcore::compression::compress::error_state::NOT_DONE)
                            return Error;

                        Error.clear();
                    }
                    else bDone = true;
                    Pack.m_CompressSize += m_pWrite->m_CSizeStream.last();
                }
                xassert(bDone);
            }

            //
            // Close the pack file
            //
            Pack.m_Data.close();
        }

        //
        // Take the references and the packs headers and compress them as well
        //
        xcore::unique_span<std::byte>   CompressInfoData;
        std::uint32_t                   CompressInfoDataSize;
        {
            xcore::unique_span<std::byte> InfoData;

            // First update endianess 
            if( m_pWrite->m_bEndian )
            {
                for ( auto& E : m_pWrite->m_PointerTable )
                {
                    E.m_OffSet         = xcore::endian::Convert(E.m_OffSet);
                    E.m_Count          = xcore::endian::Convert(E.m_Count);
                    E.m_PointingAT     = xcore::endian::Convert(E.m_PointingAT);
                    E.m_OffsetPack     = xcore::endian::Convert(E.m_OffsetPack);
                    E.m_PointingATPack = xcore::endian::Convert(E.m_PointingATPack);
                }

                for( auto& E : m_pWrite->m_Packs )
                {
                    E.m_PackFlags.m_Value   = xcore::endian::Convert(E.m_PackFlags.m_Value);
                    E.m_UncompressSize      = xcore::endian::Convert(E.m_UncompressSize);
                }

                for( auto& E : m_pWrite->m_CSizeStream )
                {
                    E = xcore::endian::Convert(E);
                }
            }

            // Allocate all the memory that we will need
            if( InfoData.New( sizeof(pack)              * m_pWrite->m_Packs.size() 
                              + sizeof(ref)             * m_pWrite->m_PointerTable.size() 
                              + sizeof(std::uint32_t)   * m_pWrite->m_CSizeStream.size() ).isError(Error)) return Error;

            auto pPack          = reinterpret_cast<pack*>           (InfoData.data());
            auto pRef           = reinterpret_cast<ref*>            (&pPack[m_pWrite->m_Packs.size()]);
            auto pBlockSizes    = reinterpret_cast<std::uint32_t*>  (&pRef[m_pWrite->m_PointerTable.size()]);

            if (CompressInfoData.New(InfoData.size()).isError(Error)) return Error;

            xassert_block_basic()
            {
                std::memset( CompressInfoData.data(), 0xBE, CompressInfoData.size() );
            }


            // Now copy all the info starting with the packs
            for( std::uint32_t i = 0; i < m_pWrite->m_Packs.size(); i++)
            {
                pPack[i] = m_pWrite->m_Packs[i];
            }

            // Now we copy all the references
            for( std::uint32_t i = 0; i < m_pWrite->m_PointerTable.size(); i++)
            {
                pRef[i] = m_pWrite->m_PointerTable[i];
            }

            // Now we copy all the block sizes
            for(std::uint32_t i = 0; i < m_pWrite->m_CSizeStream.size(); i++)
            {
                pBlockSizes[i] = m_pWrite->m_CSizeStream[i];
            }

            // 
            // to compress it
            //
            {
                xcore::compression::compress Compress;
                if (Compress.Init(types::static_cast_safe<std::uint32_t>(InfoData.size()), InfoData).isError(Error)) return Error;
                if (Compress.Pack(CompressInfoDataSize, CompressInfoData).isError(Error)) return Error;
            }
        }

        //
        // Fill up all the header information
        //
        m_Header.m_SerialFileVersion    = version_id_v;       // Major and minor version ( version pattern helps Identify file format as well)
        m_Header.m_nPacks               = types::static_cast_safe<std::uint16_t>(m_pWrite->m_Packs.size());
        m_Header.m_nPointers            = types::static_cast_safe<std::uint16_t>(m_pWrite->m_PointerTable.size());
        m_Header.m_nBlockSizes          = types::static_cast_safe<std::uint16_t>(m_pWrite->m_CSizeStream.size());
        m_Header.m_SizeOfData           = 0;
        m_Header.m_PackSize             = CompressInfoDataSize;
        m_Header.m_AutomaticVersion     = m_ClassSize;

        header  Header;

        if (m_pWrite->m_bEndian)
        {
            Header.m_SerialFileVersion  = xcore::endian::Convert(m_Header.m_SerialFileVersion);
            Header.m_PackSize           = xcore::endian::Convert(m_Header.m_PackSize);
            Header.m_MaxQualities       = xcore::endian::Convert(m_Header.m_MaxQualities);
            Header.m_SizeOfData         = xcore::endian::Convert(m_Header.m_SizeOfData);
            Header.m_nPointers          = xcore::endian::Convert(m_Header.m_nPointers);
            Header.m_nPacks             = xcore::endian::Convert(m_Header.m_nPacks);
            Header.m_nBlockSizes        = xcore::endian::Convert(m_Header.m_nBlockSizes);
            Header.m_ResourceVersion    = xcore::endian::Convert(m_Header.m_ResourceVersion);
            Header.m_AutomaticVersion   = xcore::endian::Convert(m_Header.m_AutomaticVersion);
        }
        else
        {
            Header = m_Header;
        }

        //
        // Save everything into a file
        //
        std::size_t Pos;
        if (units::bytes BytesPos; m_pWrite->m_pFile->Tell(BytesPos).isError(Error)) return Error;
        else Pos = types::static_cast_safe<std::size_t>(BytesPos.m_Value);

        if( m_pWrite->m_pFile->WriteView(std::span<std::byte>(reinterpret_cast<std::byte*>(&Header), sizeof(Header))).isError(Error)) return Error;
        if( m_pWrite->m_pFile->WriteView(CompressInfoData.ViewTo(CompressInfoDataSize)).isError(Error)) return Error;

        for( auto& Pack : m_pWrite->m_Packs )
        {
            if( m_pWrite->m_bEndian )
            {
                if( m_pWrite->m_pFile->WriteView( Pack.m_CompressData.ViewTo(xcore::endian::Convert(Pack.m_CompressSize))).isError(Error))
                    return Error;
            }
            else
            {
                if( m_pWrite->m_pFile->WriteView( Pack.m_CompressData.ViewTo(Pack.m_CompressSize)).isError(Error))
                    return Error;
            }
        }

        // Write the size of the data
        

        if(units::bytes BytesPos; m_pWrite->m_pFile->Tell(BytesPos).isError(Error)) return Error;
        else Pos = types::static_cast_safe<std::size_t>(BytesPos.m_Value) - Pos - sizeof(header);

        Header.m_SizeOfData = types::static_cast_safe<std::uint32_t>(Pos);
        if (m_pWrite->m_pFile->SeekOrigin(units::bytes{ types::static_cast_safe<std::int64_t>(Pos + offsetof(header, m_SizeOfData)) }).isError(Error)) return Error;

        if (m_pWrite->m_bEndian)
        {
            Header.m_SizeOfData = xcore::endian::Convert(Header.m_SizeOfData);
        }

        if( m_pWrite->m_pFile->Write(Header.m_SizeOfData).isError(Error) )
            return Error;

        // Go to the end of the file
        if (m_pWrite->m_pFile->SeekEnd(units::bytes{ 0 }).isError(Error)) return Error;

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::LoadHeader( file::stream& File, std::size_t SizeOfT) noexcept
    {
        xcore::err Error;

        //
        // Check signature (version is encoded in signature)
        //
        if (File.ReadView(std::span<std::byte>{ reinterpret_cast<std::byte*>(&m_Header), sizeof(m_Header) }).isError(Error))
            return Error;

        if( File.Synchronize(true).isError(Error) ) return Error;

        if (m_Header.m_SerialFileVersion != version_id_v)
        {
            if (xcore::endian::Convert(m_Header.m_SerialFileVersion) == version_id_v)
            {
                return xerr_failure( Error, "File can not be read. Probably it has the wrong endian." );
            }

            return xerr_failure(Error, "Unknown file format (Could be an older version of the file format)" );
        }

        if (m_Header.m_AutomaticVersion != SizeOfT)
        {
            return xerr_failure(Error, "The size of the structure that was used for writing this file is different from the one reading it" );
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    void* stream::LoadObject( file::stream& File ) noexcept
    {
        xcore::err                              Error;
        xcore::unique_span<std::byte>           InfoData;                   // Buffer which contains all those arrays
        xcore::unique_span<decompress_block>    ReadBuffer;
        std::uint32_t                           iCurrentBuffer = 0;

        //
        // Allocate the read temp double buffer
        //
        ReadBuffer.New(2).CheckError();

        //
        // Read the refs and packs
        //
        {
            // Create uncompress buffer for the packs and references
            const auto DecompressSize = m_Header.m_nPacks * sizeof(pack)
                + m_Header.m_nPointers * sizeof(ref)
                + m_Header.m_nBlockSizes * sizeof(std::uint32_t);

            // InfoData.Alloc( x_Max( (s32)m_Header.m_PackSize, DecompressSize ) + m_Header.m_nPacks * sizeof(xbyte**) );

            xassert(m_Header.m_PackSize <= DecompressSize);
            InfoData.New(DecompressSize + m_Header.m_nPacks * sizeof(std::byte * *)).CheckError();

            // Uncompress in place for packs and references
            if ( m_Header.m_PackSize < DecompressSize )
            {
                xcore::unique_span<std::byte>  CompressData;

                CompressData.New(m_Header.m_PackSize).CheckError();

                if ( File.ReadView(CompressData).isError(Error) )
                {
                    xcore::log::Output("ERROR:Serializer Load (1) Error(%s)", Error.getCode().m_pString);
                    Error.clear();
                    return nullptr;
                }
                File.Synchronize(true).CheckError();

                // Actual decompress the block
                {
                    std::uint32_t DecompressSize;
                    xcore::compression::decompress Decompress;
                    Decompress.Init(static_cast<uint32_t>(InfoData.size())).CheckError();
                    Decompress.Unpack(DecompressSize, InfoData, CompressData).CheckError();
                }
                //x_DecompressMem(xbuffer_view<xbyte>(InfoData, DecompressSize), xbuffer_view<xbyte>(reinterpret_cast<xbyte*>(&CompressData[0]), m_Header.m_PackSize));
            }
            else
            {
                if ( File.ReadView(std::span<std::byte>(&InfoData[0], m_Header.m_PackSize)).isError(Error) )
                {
                    xcore::log::Output("ERROR:Serializer Load (2) Error(%s)", Error.getCode().m_pString);
                    Error.clear();
                    return nullptr;
                }
                File.Synchronize(true).CheckError();
            }
        }

        //
        // Set up the all the pointers
        //
        auto const   pPack           = reinterpret_cast<const pack*>            (&InfoData[0]);
        auto const   pRef            = reinterpret_cast<const ref*>             (&pPack[m_Header.m_nPacks]);
        auto const   pBlockSizes     = reinterpret_cast<const std::uint32_t*>   (&pRef[m_Header.m_nPointers]);
        auto const   pPackPointers   = const_cast<std::byte**>(reinterpret_cast<const std::byte* const*>(&pBlockSizes[m_Header.m_nBlockSizes]));

        //
        // Start the reading and decompressing of the packs
        //
        {
            std::uint32_t iBlock = 0;

            for (std::uint32_t iPack = 0; iPack < m_Header.m_nPacks; iPack++)
            {
                const pack&     Pack        = pPack[iPack];
                std::uint32_t   nBlocks     = 0;
                std::uint32_t   ReadSoFar   = 0;

                // Start reading block immediately
                // Note that the rest of the first block of a pack is interleave with the last pack last block
                // except for the very first one (this one)
                if (iPack == 0)
                {
                    if (File.ReadView(std::span<std::byte>(reinterpret_cast<std::byte*>(&ReadBuffer[iCurrentBuffer]), pBlockSizes[iBlock])).isError(Error))
                    {
                        xcore::log::Output("ERROR:Serializer Load (3) Error(%s)", Error.getCode().m_pString);
                        Error.clear();
                        return nullptr;
                    }
                }

                // Allocate the size of this pack
                pPackPointers[iPack] = reinterpret_cast<std::byte*>(m_pMemoryCallback( xcore::units::bytes{ Pack.m_UncompressSize }, Pack.m_PackFlags));

                // Store a block that is mark as temp (can/should only be one)
                if (Pack.m_PackFlags.m_bTempMemory )
                {
                    xassert(m_pTempBlockData == nullptr);
                    m_pTempBlockData = pPackPointers[iPack];
                }

                // Make sure that is just one that we need to read
                if( Pack.m_UncompressSize > max_block_size_v )
                {
                    nBlocks  = Pack.m_UncompressSize / max_block_size_v;
                    nBlocks += ((nBlocks * max_block_size_v) == Pack.m_UncompressSize) ? 0 : 1;
                }

                // Read each block
                for (std::uint32_t i = 1; i < nBlocks; i++)
                {
                    iCurrentBuffer = !iCurrentBuffer;
                    iBlock++;

                    // Start reading the next block
                    File.Synchronize(true).CheckError();
                    if (File.ReadView(std::span<std::byte>{ reinterpret_cast<std::byte*>(&ReadBuffer[iCurrentBuffer]), static_cast<std::size_t>(pBlockSizes[iBlock]) }).isError(Error))
                    {
                        xcore::log::Output("ERROR:Serializer Load (4) Error(%s)", Error.getCode().m_pString);
                        Error.clear();
                        return nullptr;
                    }

                    //
                    // Start the decompressing at the same time
                    //
                    xassume(pPackPointers[iPack]);

                    {
                        std::uint32_t DecompressSize;
                        xcore::compression::decompress Decompress;
                        Decompress.Init(max_block_size_v).CheckError();

                        Decompress.Unpack(DecompressSize
                                        , std::span<std::byte>{&pPackPointers[iPack][ReadSoFar], max_block_size_v}
                                        , std::span<const std::byte>{reinterpret_cast<const std::byte*>(&ReadBuffer[!iCurrentBuffer]), pBlockSizes[iBlock - 1]}).CheckError();
                    }
                    //x_DecompressMem(xbuffer_view<xbyte>(&pPackPointers[iPack][ReadSoFar], MAX_BLOCK_SIZE),
                    //    xbuffer_view<xbyte>(reinterpret_cast<xbyte*>(&ReadBuffer[!iCurrentBuffer]), pBlockSizes[iBlock - 1]));

                    ReadSoFar += max_block_size_v;
                }

                // Finish reading the block
                File.Synchronize(true).CheckError();

                // Interleave next pack block with this last pack block
                if ((iPack + 1) < m_Header.m_nPacks)
                {
                    if (File.ReadView(std::span<std::byte>(reinterpret_cast<std::byte*>(&ReadBuffer[!iCurrentBuffer]), pBlockSizes[iBlock + 1])).isError(Error))
                    {
                        xcore::log::Output("ERROR:Serializer Load (5) Error(%s)", Error.getCode().m_pString);
                        Error.clear();
                        return nullptr;
                    }
                }

                //
                // Decompress last block for this pack
                //
                xassume(pPackPointers[iPack]);
                
                // Actual decompress the block
                {
                    std::uint32_t DecompressSize;
                    xcore::compression::decompress Decompress;
                    Decompress.Init(max_block_size_v).CheckError();
                    Decompress.Unpack(DecompressSize
                                    , std::span<std::byte>{&pPackPointers[iPack][ReadSoFar], Pack.m_UncompressSize - ReadSoFar}
                                    , std::span<std::byte>{reinterpret_cast<std::byte*>(&ReadBuffer[iCurrentBuffer]), pBlockSizes[iBlock]}).CheckError();
                }
                // x_DecompressMem(xbuffer_view<xbyte>(&pPackPointers[iPack][ReadSoFar], Pack.m_UncompressSize - ReadSoFar),
                //    xbuffer_view<xbyte>(reinterpret_cast<xbyte*>(&ReadBuffer[iCurrentBuffer]), pBlockSizes[iBlock]));

                // Get ready for next block
                iCurrentBuffer = !iCurrentBuffer;
                iBlock++;
            }
        }

        //
        // Resolve pointers
        //
        for (std::uint32_t i = 0; i < m_Header.m_nPointers; i++)
        {
            const ref&                  Ref       = pRef[i];
            void* const                 pSrcData  = &pPackPointers[Ref.m_PointingATPack][Ref.m_PointingAT];
            serializer::data_ptr<void>* pDestData = reinterpret_cast<serializer::data_ptr<void>*>(&pPackPointers[Ref.m_OffsetPack][Ref.m_OffSet]);

            pDestData->m_pValue = pSrcData;
        }

        // Return the basic pack
        return pPackPointers[0];
    }
#endif
}