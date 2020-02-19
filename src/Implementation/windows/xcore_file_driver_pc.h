
namespace xcore::file
{
    namespace details
    {
        struct temp_path
        {
            temp_path(void) noexcept
            {
                #ifdef UNICODE
                {
                    GetTempPath(m_WPath.size(), m_WPath.data());
                    string::Copy(m_Path, m_WPath);
                }
                #else
                {
                    GetTempPath(m_Path.size(), m_Path.data());
                    string::Copy(m_WPath, m_Path);
                }
                #endif
            }

            string::fixed<char, 512>      m_Path;
            string::fixed<wchar_t, 512>  m_WPath;
        };

        static temp_path s_TempPath{};
    }

    static const wchar_t* getTempPath (void) noexcept
    {
        return details::s_TempPath.m_WPath.data();
    }
}

namespace xcore::file::driver
{
    class system final : public xcore::file::device_i
    {
    public:

        constexpr system (void) noexcept : device_i(string::constant<char>{"a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:x:y:z:"} ) {}

    protected:

        struct alignas(std::atomic<void*>) small_file
        {
            HANDLE          m_Handle{};
            OVERLAPPED      m_Overlapped{};
            access_types    m_AccessTypes{};
            bool            m_bIOPending{ false };
        };

        xcore::lockless::pool::mpmc_bounded_jitc<small_file, 128>   m_qtlFileHPool;

    protected:

        //----------------------------------------------------------------------------------------
        void DisplayError(void) noexcept
        {
            xassert_block_basic()
            {
                #if _XCORE_COMPILER_VISUAL_STUDIO   
                #ifdef UNICODE
                {
                    std::array<WCHAR, 256> Buffer;
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        Buffer.data(),
                        static_cast<DWORD>( Buffer.size() ),
                        NULL);
                    MessageBox(NULL, Buffer.data(), L"GetLastError", MB_OK | MB_ICONINFORMATION);
                }
                #else
                {
                    std::array<CHAR, 256> Buffer;
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        Buffer.data(),
                        static_cast<DWORD>( Buffer.size() ),
                        NULL);
                    MessageBox(NULL, Buffer.data(), "GetLastError", MB_OK | MB_ICONINFORMATION);
                }
                #endif
                #endif
                xassert(false);
            }
        }

        //----------------------------------------------------------------------------------------

        virtual void* open ( const string::view<const wchar_t> FileName, access_types AccessTypes ) noexcept override
        {
            std::uint32_t FileMode      = 0;
            std::uint32_t Disposition   = 0;
            std::uint32_t AttrFlags     = 0;
            std::uint32_t ShareType     = FILE_SHARE_READ;

            xassert( FileName.empty() == false );

            #ifdef UNICODE
                string::fixed<WCHAR,256> FinalFileName;
            #else
                string::fixed<char,256> FinalFileName;
            #endif
            if( string::CompareN(FileName, L"temp:", 5) == 0 )
            {
                auto L = string::Copy(FinalFileName, details::s_TempPath.m_Path);
                xassert( FileName[5] == '/' );
                string::Copy(FinalFileName.getView().ViewFrom(L.m_Value), &FileName[6]);
            }
            else
            {
                string::Copy( FinalFileName, FileName );
            }

            FileMode = GENERIC_WRITE | GENERIC_READ;
            if( AccessTypes.m_bCreate )
            {
                Disposition = CREATE_ALWAYS;
            }
            else
            {
                if( AccessTypes.m_bWrite == false )
                {
                    FileMode &= ~GENERIC_WRITE;
                }

                Disposition = OPEN_EXISTING;
            }

            if (AccessTypes.m_bASync)
            {
                // FILE_FLAG_OVERLAPPED     -	This allows asynchronous I/O.
                // FILE_FLAG_NO_BUFFERING   -	No cached asynchronous I/O.
                AttrFlags = FILE_FLAG_OVERLAPPED;// | FILE_FLAG_NO_BUFFERING;
            }
            else
            {
                AttrFlags = FILE_ATTRIBUTE_NORMAL;
            }

            // open the file (or create a new one)
            HANDLE Handle = CreateFile(FinalFileName.data(), FileMode, ShareType, nullptr, Disposition, AttrFlags, nullptr);
            if (Handle == INVALID_HANDLE_VALUE)
            {
                return nullptr;
            }

            //
            // Okay we are in business
            //
            auto& File = *m_qtlFileHPool.pop();

            File.m_Handle      = Handle;
            File.m_AccessTypes = AccessTypes;
            std::memset(&File.m_Overlapped, 0, sizeof(File.m_Overlapped));

            if (AccessTypes.m_bASync)
            {
                // Create an event to detect when an asynchronous operation is done.
                // Please see documentation for further information.
                //HANDLE const Event = CreateEvent( NULL, TRUE, FALSE, NULL );
                //ASSERT(Event != NULL);
                //File.m_Overlapped.hEvent = Event;
            }

            // done
            return &File;
        }

        //----------------------------------------------------------------------------------------

        virtual void close ( void* pFile ) noexcept override
        {
            auto& File = *static_cast<small_file*>(pFile);

            //
            // Close the handle
            //
            if (!CloseHandle(File.m_Handle))
            {
                DisplayError();
                return;
            }

            //
            // Lets free our entry
            //
            m_qtlFileHPool.push(File);
        }

        //----------------------------------------------------------------------------------------

        virtual xcore::err Read( void* pFile, std::span<std::byte> View ) noexcept override
        {
            auto& File = *static_cast<small_file*>(pFile);

            xassert(View.size() < std::numeric_limits<DWORD>::max());

            xcore::err  Error;
            const DWORD Count      = static_cast<DWORD>(View.size());
            DWORD       nBytesRead = Count;
            BOOL        bResult = ReadFile(
                                File.m_Handle
                            ,   View.data()
                            ,   Count
                            ,   &nBytesRead
                            ,   &File.m_Overlapped );

            // Set the file pointer (We assume we didn't make any errors)
            //if( File.m_Flags&xfile_device_i::ACC_ASYNC) 
            {
                File.m_Overlapped.Offset += Count;
            }

            if (!bResult)
            {
                DWORD dwError = GetLastError();

                // deal with the error code 
                switch (dwError)
                {
                case ERROR_HANDLE_EOF:
                {
                    // we have reached the end of the file 
                    // during the call to ReadFile 

                    // code to handle that 
                    return xerr_code( Error, error::UNEXPECTED_EOF, "Unexpected End Of File while reading" );
                }

                case ERROR_IO_PENDING:
                {
                    File.m_bIOPending = true;
                    return xerr_code(Error, error::INCOMPLETE, "Still reading");
                }

                default:
                {
                    DisplayError();
                    return xerr_failure(Error, "Error while reading");
                }
                }
            }

            // Not problems
            return Error;
        }

        //----------------------------------------------------------------------------------------

        virtual xcore::err Write( void* pFile, const std::span<const std::byte> View ) noexcept override
        {
            xcore::err  Error;
            auto&       File = *static_cast<small_file*>(pFile);

            xassert( View.size() < std::numeric_limits<DWORD>::max() );

            const DWORD Count           = static_cast<DWORD>(View.size());
            DWORD       nBytesWritten   = Count;
            BOOL  bResult = WriteFile(
                            File.m_Handle
                        ,   View.data()
                        ,   Count
                        ,   &nBytesWritten
                        ,   &File.m_Overlapped );

            // Set the file pointer (We assume we didnt make any errors)
            //if( File.m_Flags&xfile_device_i::ACC_ASYNC) 
            {
                File.m_Overlapped.Offset += Count;
            }

            if (!bResult)
            {
                DWORD dwError = GetLastError();

                // deal with the error code 
                switch (dwError)
                {
                case ERROR_HANDLE_EOF:
                {
                    // we have reached the end of the file 
                    // during the call to ReadFile 

                    // code to handle that 
                    xassert(0);
                    return xerr_code(Error, error::UNEXPECTED_EOF, "Unexpected end of file while writting");
                }

                case ERROR_IO_PENDING:
                {
                    File.m_bIOPending = true;
                    return xerr_code(Error, error::INCOMPLETE, "Still writting");
                }

                default:
                {
                    DisplayError();
                    return xerr_failure(Error, "Error while writting");
                }
                }
            }
            return Error;
        }

        //----------------------------------------------------------------------------------------

        virtual xcore::err Seek(void* pFile, seek_mode Mode, units::bytes Pos) noexcept override
        {
            xcore::err  Error;
            auto&       File            = *static_cast<small_file*>(pFile);
            auto        HardwareMode = [](seek_mode Mode)
            {
                switch (Mode) //-V719
                {
                    case SKM_CURENT: return FILE_CURRENT;  
                    case SKM_END:    return FILE_END;
                }
                xassert(Mode == SKM_ORIGIN);
                return FILE_BEGIN;
            }(Mode);

            // We will make sure we are sync here
            // WARNING: Potential time wasted here
            if( File.m_AccessTypes.m_bASync )
            {
                if (Synchronize(pFile, true).isError(Error)) return Error;
            }

            // Seek!
            LARGE_INTEGER Position;
            LARGE_INTEGER NewFilePointer;

            Position.QuadPart = Pos.m_Value;
            const DWORD Result = SetFilePointerEx(File.m_Handle, Position, &NewFilePointer, HardwareMode);
            if (!Result)
            {
                if (Result == INVALID_SET_FILE_POINTER) //-V547
                {
                    // Failed to seek.
                    return xerr_failure(Error, "Fail to seek, INVALID_SET_FILE_POINTER");
                }

                return xerr_failure(Error, "Fail to seek");
            }

            // Set the position for async files
            File.m_Overlapped.Offset     = NewFilePointer.LowPart;
            File.m_Overlapped.OffsetHigh = NewFilePointer.HighPart;

            return Error;
        }

        //----------------------------------------------------------------------------------------
        
        virtual xcore::err Tell (void* pFile, units::bytes& Offset ) noexcept override
        {
            xcore::err      Error;
            auto&           File = *static_cast<small_file*>(pFile);
            LARGE_INTEGER   Position;
            LARGE_INTEGER   NewFilePointer;

            Position.LowPart  = 0;
            Position.HighPart = 0;

            if( FALSE == SetFilePointerEx(File.m_Handle, Position, &NewFilePointer, FILE_CURRENT) )
            {
                DisplayError();
                return xerr_failure(Error, "Error while Telling");
            }

            Offset.m_Value = NewFilePointer.QuadPart;
            return Error;
        }

        //----------------------------------------------------------------------------------------

        virtual void Flush( void* pFile ) noexcept override
        {
            // We will make sure we are sync here. 
            // I dont know what else to do there is not a way to flush anything the in the API
            // WARNING: Potential time wasted here
            auto E = Synchronize(pFile, true);
        }

        //----------------------------------------------------------------------------------------

        virtual xcore::err Length (void* pFile, units::bytes& Length ) noexcept override
        {
            xcore::err    Error;
            units::bytes  Cursor;

            if( Tell(pFile, Cursor).isError(Error) ) return Error;
            if (Seek(pFile, SKM_END, units::bytes{ 0 }).isError(Error)) return Error;

            if( Tell(pFile, Length).isError(Error) ) return Error;
            if( Seek(pFile, SKM_ORIGIN, Cursor).isError(Error)) return Error;

            return Error;
        }

        //----------------------------------------------------------------------------------------

        virtual bool isEOF(void* pFile) noexcept override
        {
            auto& File = *static_cast<small_file*>(pFile);
            DWORD nBytesTransfer;
            BOOL  bResult = GetOverlappedResult(File.m_Handle, &File.m_Overlapped, &nBytesTransfer, false);

            if (!bResult)
            {
                DWORD dwError = GetLastError();

                // deal with the error code 
                switch (dwError)
                {
                case ERROR_HANDLE_EOF:
                {
                    // we have reached the end of the file 
                    // during the call to ReadFile 

                    // code to handle that 
                    return true;
                }

                case ERROR_IO_PENDING:
                {
                    if (auto Err = Synchronize(pFile, true); Err)
                    {
                        if(Err.getCode().getState<error>() == error::INCOMPLETE ) return false;
                        Err.clear();
                    }

                    return true;
                }

                default:
                    xassert(0);
                    break;
                }
            }

            // Not sure about this yet
            return false;
        }

        //----------------------------------------------------------------------------------------
        virtual xcore::err Synchronize(void* pFile, bool bBlock) noexcept override
        {
            xcore::err    Error;
            auto& File = *static_cast<small_file*>(pFile);

            if( File.m_bIOPending )
                if (HasOverlappedIoCompleted(&File.m_Overlapped))
                {
                    File.m_bIOPending = false;
                    return Error; // xfile::sync_state::COMPLETED;
                }

            // Get the current status.
            DWORD nBytesTransfer = 0;
            BOOL  Result = GetOverlappedResult(File.m_Handle, &File.m_Overlapped, &nBytesTransfer, bBlock);

            // Has the asynchronous operation finished?
            if (Result != FALSE)
            {
                // Clear the event's signal since it isn't automatically reset.
                // VERIFY( ResetEvent( File.m_Overlapped.hEvent ) == TRUE );

                // The operation is complete.
                File.m_bIOPending = false;
                return Error; // xfile::sync_state::COMPLETED;
            }

            //
            // Deal with errors
            //
            DWORD dwError = GetLastError();

            if (dwError == ERROR_HANDLE_EOF)
            {
                // we have reached the end of
                // the file during asynchronous
                // operation
                return xerr_code(Error, error::UNEXPECTED_EOF, "Unexpected end of file"); //xfile::sync_state::XEOF;
            }

            if (dwError == ERROR_IO_INCOMPLETE)
            {
                File.m_bIOPending = true;
                return xerr_code(Error, error::INCOMPLETE, "Incomplete"); // xfile::sync_state::INCOMPLETE;
            }

            if (dwError == ERROR_OPERATION_ABORTED)
            {
                return xerr_failure(Error, "Operation Aborted");//xfile::sync_state::UNKNOWN_ERR;
            }

            // The result is FALSE and the error isn't ERROR_IO_INCOMPLETE, there's a real error!
            return xerr_failure(Error, "Unknown Error"); //xfile::sync_state::UNKNOWN_ERR;
        }

        //----------------------------------------------------------------------------------------
        virtual void AsyncAbort (void* pFile) noexcept override
        {
            auto& File = *static_cast<small_file*>(pFile);
            auto E = CancelIo(File.m_Handle);
            xassert(E);
        }
    };
}
