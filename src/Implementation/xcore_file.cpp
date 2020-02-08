
#include "Implementation/windows/xcore_file_driver_pc.h"
#include "Implementation/xcore_file_driver_ram.cpp"

namespace xcore::file
{
    //------------------------------------------------------------------------------

    void InitSystem( std::vector<std::unique_ptr<device_i>>& List ) noexcept
    {
        List.emplace_back( std::unique_ptr<device_i>{ new xcore::file::driver::system      } );
        List.emplace_back( std::unique_ptr<device_i>{ new xcore::file::driver::ram::device } );
    }

    //------------------------------------------------------------------------------
    static
    device_i* FileSystemFindDevice( const char* pDeviceName ) noexcept
    {
        auto& DeviceList  = xcore::get().m_lFileDevice;

        // User must be asking for the default device
        // Or if it is a temp folder we also will use the default device
        if (pDeviceName == nullptr || std::strncmp(pDeviceName, "temp:", sizeof("temp:") - 1) == 0)
        {
            return DeviceList.front().get();
        }

        // Search throw all the devices    
        for(auto& pDevice : DeviceList )
        {
            device_i&   Device  = *pDevice;
            const char* pName   = Device.getName().m_pValue;

            for (int i = 0; pDeviceName[i] && *pName; i++, pName++)
            {
                if (pDeviceName[i] == ':' && *pName == ':')
                    return &Device;

                if ( string::ToCharLower(*pName) != string::ToCharLower(pDeviceName[i]))
                {
                    // Skip character and see if it has more devices mapped
                    while (*pName != ':' && *pName) pName++;

                    // Check to see if a has more devices
                    if (*pName == ':')
                    {
                        if (pName[1])
                        {
                            i = -1;
                            continue;
                        }
                        break;
                    }
                }
            }
        }

        return nullptr;
    }

    //------------------------------------------------------------------------------

    xcore::err stream::open( const string::view<const wchar_t> Path, const char* pMode) noexcept
    {
        xassert(pMode);

        // We cant open a new file in the middle of using another
        xassert( m_pDevice == nullptr );

        //
        // Find the device in question
        //
        xcore::err                              Error;
        xcore::array< char, max_length::path_v> DeviceName;
        int                                     i;

        // Copy our device into a separate string
        for( i = 0; (DeviceName[i] = static_cast<char>(Path[i]))
            &&      (i < (DeviceName.size() - 1))
            &&      (Path[i] != L':');
            i++)
        {
        }

        device_i* pDevice = nullptr;
        if (DeviceName[i] != ':')
        {
            // We didn't find any device we are going to assume the default device
            pDevice = FileSystemFindDevice(nullptr);
        }
        else
        {
            // lets find it!
            DeviceName[++i] = 0;
            pDevice = FileSystemFindDevice(DeviceName.data());
        }

        // Make sure that we got a device
        if (pDevice == nullptr) return xerr_code(Error, error::DEVICE_FAILURE, "Unable to find requested device");

        //
        // Okay now lets make sure that the mode is correct
        //
        access_types       AccessType;
        bool               bSeekToEnd = false;

        for (i = 0; pMode[i]; i++)
        {
            switch (pMode[i])
            {
            case 'a':   AccessType.m_bRead      = AccessType.m_bWrite = true;   bSeekToEnd = true;      break;
            case 'r':   AccessType.m_bRead      = true;                                                 break;
            case '+':   AccessType.m_bWrite     = true;                                                 break;
            case 'w':   AccessType.m_bRead      = AccessType.m_bWrite = AccessType.m_bCreate = true;    break;
            case 'c':   AccessType.m_bCompress  = true;                                                 break;
            case '@':   AccessType.m_bASync     = true;                                                 break;
            case 't':   AccessType.m_bText      = true;                                                 break;
            case 'b':   AccessType.m_bText      = false;                                                break;
            default:
                // "Don't understand this[%c] access mode while opening file (%s)", pMode[i], (const char*)Path)
                xassert(false);
            }
        }

        // next thing is to open the file using the device
        // Note this hold initialization could be VERY WORNG as opening the file 
        // may the one of the slowest part of the file access as the DVD may need to seek
        // to it. This ideally should happen Async when an Async mode is requested.
        // May need to review this a bit more careful later.
        void* pFile = nullptr;
        if ( AccessType.m_bCreate )
        {
            pFile = pDevice->open( Path, AccessType );
            if( pFile == nullptr ) return xerr_code(Error, error::CREATING_FILE, "Unable to create file" );
        }
        else
        {
            pFile = pDevice->open(Path, AccessType);
            if (pFile == nullptr) return xerr_code(Error, error::OPENING_FILE, "Unable to open file" );
            if (bSeekToEnd)
            {
                if (pDevice->Seek(pFile, device_i::SKM_END, units::bytes{ 0 }).isError(Error)) return Error;
            }
        }

        //
        // Set all the member variables
        //
        string::Copy( m_FileName, Path );
        m_pDevice       = pDevice;
        m_pFile         = pFile;
        m_AccessType    = AccessType;

        return Error;
    }

    //------------------------------------------------------------------------------

    void stream::close(void) noexcept
    {
        if (m_pFile)
        {
            xassert(m_pDevice);
            m_pDevice->close(m_pFile);
        }

        //
        // Done with the file
        //
        m_pFile                 = nullptr;
        m_pDevice               = nullptr;
        m_AccessType.m_Value    = 0;
        m_FileName.clear();
    }

    //------------------------------------------------------------------------------

    xcore::err stream::ReadRaw(std::span<std::byte> View) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);
        xassert(View.empty() == false);

        xcore::err Error;

        if( m_pDevice->Read( m_pFile, View ).isError(Error) )
            return Error;

        // If it is text mode try finding '\r\n' to remove the '\r'
        if (m_AccessType.m_bText)
        {
            char* pCharSrc = (char*)View.data();
            char* pCharDst = (char*)View.data();

            for (std::size_t i = 0; i < View.size(); i++)
            {
                *pCharDst = *pCharSrc;
                pCharSrc++;
                if (!(*pCharSrc == '\n' && *(pCharSrc - 1) == '\r'))
                {
                    pCharDst++;
                }
            }

            // Read any additional data that we may need
            if (pCharSrc != pCharDst)
            {
                const auto Delta = (std::size_t)(pCharSrc - pCharDst);

                // Recurse
                return ReadRaw({ (std::byte*)&View[View.size() - Delta], Delta });// &((char*)pBuffer)[TotalCount - Delta], 1, Delta);
            }

            // Sugar a bad case here. We need to read one more character to know what to do.
            if (*--pCharDst == '\r')
            {
                char C;

                if ( m_pDevice->Read(m_pFile, { (std::byte*)&C, 1 }).isError(Error) )
                    return Error;//{ err::state::FAILURE, "Fail to read characters from file" };

                if (C == '\n')
                {
                    *pCharDst = '\n';
                }
                else
                {
                    // Upss the next character didnt match the sequence
                    // lets rewind one
                    if (m_pDevice->Seek(m_pFile, device_i::SKM_CURENT, units::bytes{ -1 }).isError(Error))
                        return Error;
                }
            }
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    xcore::err  stream::WriteRaw( std::span<std::byte> View ) noexcept
    {
        xassert(m_pFile);
        xassert(m_pDevice);
        xassert(View.empty() == false);

        xcore::err Error;

        // If it is text mode try finding '\n' and add a '\r' in front so that it puts in the file '\r\n'
        if (m_AccessType.m_bText)
        {
            auto* pFound = View.data();
            std::uint64_t   iLast = 0;
            std::uint64_t   i;

            for (i = 0; i < View.size(); i++)
            {
                if (pFound[i] == std::byte{ '\n' } )
                {
                    constexpr static std::uint16_t Data = (std::uint16_t{ '\n' } << 8) | (std::uint16_t{ '\r' } << 0);

                    if (m_pDevice->Write(m_pFile, { &pFound[iLast], i - iLast }).isError(Error)) return Error;
                    if (m_pDevice->Write(m_pFile, { (std::byte*) & Data, sizeof(Data) }).isError(Error)) return Error;

                    // Update the base
                    iLast = i + 1;
                }
            };

            // Write the remainder 
            if (iLast != i)
            {
                if (m_pDevice->Write(m_pFile, { &pFound[iLast], i - iLast }).isError(Error)) return Error;
            }
        }
        else
        {
            if (m_pDevice->Write(m_pFile, View).isError(Error)) return Error;
        }

        if (m_AccessType.m_bForceFlush )
        {
            m_pDevice->Flush(m_pFile);
        }

        return Error;
    }

    /*
    //------------------------------------------------------------------------------
    device_i* __x_file_new_ram_device(void);

    void __x_file_init(g_context& GContext)
    {
#if _X_TARGET_WINDOWS
        GContext.getFileSystemDevice() = x_new(pc_device);
#elif _X_TARGET_MAC || _X_TARGET_IOS
        GContext.getFileSystemDevice() = x_new(darwin_file_device);
#elif _X_TARGET_ANDROID

#endif

        GContext.getFileSystemDevice()->m_pNext = __x_file_new_ram_device();
    }

    //------------------------------------------------------------------------------

    void __x_file_kill(g_context& GContext)
    {
        //
        // Delete all devices
        //
        for (auto pData = GContext.getFileSystemDevice(); pData; )
        {
            auto pNext = pData->m_pNext;
            x_delete(pData);
            pData = pNext;
        }
    }
    */
}
