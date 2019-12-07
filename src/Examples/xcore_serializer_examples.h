

namespace xcore::serializer
{
#if 0
    //----------------------------------------------------------------------------------
    // Examples of data structures and their save functions
    //----------------------------------------------------------------------------------
    namespace examples
    {
        struct data1
        {
            std::int16_t        m_A;
        };

        struct data2
        {
            std::size_t         m_Count;
            data_ptr<data1>     m_Data;
        };

        struct data3 : public data1
        {
            data2                   m_GoInStatic;
            data2                   m_DontDynamic;
            xcore::array<data2, 8>  m_GoTemp;

            static constexpr std::uint32_t DYNAMIC_COUNT = (1024 * 1024) / sizeof(data1) + 4;
            static constexpr std::uint32_t STATIC_COUNT = (1024 * 1024) / sizeof(data1) + 4;
            static constexpr std::uint32_t STATIC_MAX = 0xffffffff;

            // Initialize all the data
            data3(void) noexcept
            {
                m_A = 100;

                m_DontDynamic.m_Count = DYNAMIC_COUNT;
                m_DontDynamic.m_Data.m_pValue = new data1[m_DontDynamic.m_Count];
                for (std::uint32_t i = 0; i < m_DontDynamic.m_Count; i++)
                {
                    m_DontDynamic.m_Data.m_pValue[i].m_A = 22 + i;
                }

                m_GoInStatic.m_Count = STATIC_COUNT;
                m_GoInStatic.m_Data.m_pValue = new data1[m_GoInStatic.m_Count];
                for (std::uint32_t i = 0; i < m_GoInStatic.m_Count; i++)
                {
                    m_GoInStatic.m_Data.m_pValue[i].m_A = (std::int16_t)(100 / (i + 1));
                }

                for (auto& Temp : m_GoTemp)
                {
                    Temp.m_Count = STATIC_COUNT;
                    Temp.m_Data.m_pValue = new data1[Temp.m_Count];
                    for (std::uint32_t i = 0; i < Temp.m_Count; i++)
                    {
                        Temp.m_Data.m_pValue[i].m_A = (std::int16_t)(100 / (i + 1));
                    }
                }
            }

            // This is the loading constructor by the time is call the file already loaded
            data3(stream& Stream) noexcept
            {
                xassert(Stream.getResourceVersion() == 1);

                // *** Only reason to have a something inside this constructor is to deal with dynamic data
                // We move the memory to some other random place
                auto* pData = new data1[m_DontDynamic.m_Count];
                std::memcpy(pData, m_DontDynamic.m_Data.m_pValue, m_DontDynamic.m_Count * sizeof(data1));

                // Now we can overwrite the dynamic pointer without a worry
                delete(m_DontDynamic.m_Data.m_pValue);
                m_DontDynamic.m_Data.m_pValue = pData;

                // make sure everything is ok
                SanityCheck();
            }

            // Only when saving we need to destroy separate static allocations
            void DestroyStaticStuff(void)
            {
                for (auto& Temp : m_GoTemp)
                {
                    if (Temp.m_Data.m_pValue) delete[]Temp.m_Data.m_pValue;
                }

                if (m_GoInStatic.m_Data.m_pValue) delete[]m_GoInStatic.m_Data.m_pValue;
            }

            ~data3(void)
            {
                // Here to deal with dynamic stuff
                if (m_DontDynamic.m_Data.m_pValue) delete[]m_DontDynamic.m_Data.m_pValue;
            }

            // Sanity check
            void SanityCheck(void)
            {
                xassert(m_A == 100);

                xassert(m_DontDynamic.m_Count == DYNAMIC_COUNT);
                for (std::uint32_t i = 0; i < m_DontDynamic.m_Count; i++)
                {
                    if (m_DontDynamic.m_Data.m_pValue[i].m_A != (std::int16_t)(22 + i))
                    {
                        xassert(m_DontDynamic.m_Data.m_pValue[i].m_A == (std::int16_t)(22 + i));
                    }
                }

                xassert(m_GoInStatic.m_Count == STATIC_COUNT);
                for (std::uint32_t i = 0; i < m_GoInStatic.m_Count; i++)
                {
                    if (m_GoInStatic.m_Data.m_pValue[i].m_A != (std::int16_t)(100 / (i + 1)))
                    {
                        xassert(m_GoInStatic.m_Data.m_pValue[i].m_A == (std::int16_t)(100 / (i + 1)));
                    }
                }

                for (auto& Temp : m_GoTemp)
                {
                    xassert(Temp.m_Count == STATIC_COUNT);
                    for (std::uint32_t i = 0; i < Temp.m_Count; i++)
                    {
                        if (Temp.m_Data.m_pValue[i].m_A != (std::int16_t)(100 / (i + 1)))
                        {
                            xassert(Temp.m_Data.m_pValue[i].m_A == (std::int16_t)(100 / (i + 1)));
                        }
                    }
                }
            }
        };
    }

    //----------------------------------------------------------------------------------
    // Examples of functions that will be used to save data.
    // Note that all saving functions must be named the same and must be in:
    // namespace xcore::serializer::io_functions
    //----------------------------------------------------------------------------------
    namespace io_functions
    {
        //----------------------------------------------------------------------------------
        xcore::err SerializeIO(stream& Stream, const examples::data1& Data) noexcept
        {
            xcore::err Error;
            if (Stream.Serialize(Data.m_A).isError(Error)) return Error;
            return Error;
        }

        //----------------------------------------------------------------------------------
        xcore::err SerializeIO(stream& Stream, const examples::data2& Data) noexcept
        {
            xcore::err Error;
            if (Stream.Serialize(Data.m_Count).isError(Error)) return Error;
            if (Stream.Serialize(std::span{ Data.m_Data.m_pValue, Data.m_Count }).isError(Error)) return Error;
            return Error;
        }

        //----------------------------------------------------------------------------------
        xcore::err SerializeIO(stream& Stream, const examples::data3& Data) noexcept
        {
            xcore::err Error;

            // Make sure that it is the first version
            Stream.setResourceVersion(1);

            // Tell the structure to save it self
            if (Stream.Serialize(Data.m_GoInStatic).isError(Error)) return Error;

            // Don't always need to go into structures
            if (Stream.Serialize(Data.m_DontDynamic.m_Count).isError(Error)) return Error;

            if (Stream.Serialize(Data.m_DontDynamic.m_Data.m_pValue, Data.m_DontDynamic.m_Count, mem_type::Flags( mem_type::flags::UNIQUE ) ).isError(Error)) return Error;

            // Serialize the temp
            for (auto& Temp : Data.m_GoTemp)
            {
                if (Stream.Serialize(Temp.m_Count).isError(Error)) return Error;
                if (Stream.Serialize(Temp.m_Data.m_pValue, Temp.m_Count, mem_type::Flags(mem_type::flags::TEMP_MEMORY) ).isError(Error)) return Error;
            }

            // Tell our parent to save it self
            if (SerializeIO(Stream, static_cast<const examples::data1&>(Data)).isError(Error)) return Error;

            return Error;
        }
    }

    //----------------------------------------------------------------------------------
    // Actual code for the examples
    //----------------------------------------------------------------------------------
    namespace examples
    {
        //----------------------------------------------------------------------------------
        void Test01(void)
        {
            string::constant    FileName(L"temp:/SerialFile.bin");

            // Save
            {
                xcore::serializer::stream   SerialFile;
                data3                       TheData;

                TheData.SanityCheck();
                SerialFile.Save(FileName, TheData).CheckError();
                TheData.DestroyStaticStuff();
            }

            // Load
            /*
            {
                xserialfile  SerialFile;
                data3* pTheData;

                // This hold thing could happen in one thread
                SerialFile.Load(FileName, pTheData);

                // Okay one pointer to nuck
                x_delete(pTheData);
            }
            */
        }

        //----------------------------------------------------------------------------------
        void Test(void)
        {
            Test01();
        }
    }
#endif
}

