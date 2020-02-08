
//-----------------------------------------------------------------------------------------------

xcore::global::local_storage::local_storage( void ) noexcept
{
    #if _XCORE_COMPILER_VISUAL_STUDIO
        m_dwThreadIndex = ::TlsAlloc();
        xassert( m_dwThreadIndex != TLS_OUT_OF_INDEXES );
    #endif
}

//-----------------------------------------------------------------------------------------------

xcore::global::local_storage::~local_storage( void ) noexcept
{
    #if _XCORE_COMPILER_VISUAL_STUDIO
        ::TlsFree( m_dwThreadIndex );
    #endif
}

//-----------------------------------------------------------------------------------------------

void* xcore::global::local_storage::getRaw( void ) noexcept
{
    #if _XCORE_COMPILER_VISUAL_STUDIO
        return ::TlsGetValue( m_dwThreadIndex );
    #endif
}

//-----------------------------------------------------------------------------------------------

void xcore::global::local_storage::setRaw( void* pPtr ) noexcept
{
    #if _XCORE_COMPILER_VISUAL_STUDIO
        xassert_verify( TlsSetValue( m_dwThreadIndex, pPtr ) );
    #endif
}

//-----------------------------------------------------------------------------------------------

xcore::global::state::state( const char* pAppName ) noexcept
    :   
        m_pAppName      { pAppName }
    ,   m_MainLogger    { *new log::logger }
    ,   m_Scheduler     { *new scheduler::system }
{
    xcore::file::InitSystem(m_lFileDevice);
    xassert(m_pAppName);
}

//-----------------------------------------------------------------------------------------------

xcore::global::state::~state( void ) noexcept
{
    delete(&m_MainLogger);
    delete(&m_Scheduler);
    m_lFileDevice.clear();
}

//-----------------------------------------------------------------------------------------------

std::uint64_t xcore::global::state::getProcessID( void ) noexcept
{
#if _XCORE_COMPILER_VISUAL_STUDIO
    return static_cast<std::uint64_t>(GetCurrentProcessId());
#else

#endif
}

//-----------------------------------------------------------------------------------------------

void xcore::Init( xcore::global::state& G ) noexcept
{
    if( xcore::global::state::ms_Instance.index() )
    {
        if( std::get<1>(xcore::global::state::ms_Instance) )
        {
            xassert( std::get<1>(xcore::global::state::ms_Instance).get() == &G );
        }
        else
        {
            xcore::global::state::ms_Instance.emplace<global::state*>( &G );
        }
    }
    else
    {
        if( std::get<0>(xcore::global::state::ms_Instance) )
        {
            xassert( std::get<0>(xcore::global::state::ms_Instance) == &G );
        }
        else
        {
            xcore::global::state::ms_Instance = &G;
        }
    }
}

//-----------------------------------------------------------------------------------------------

void xcore::Init( const char* pAppName, int nWorkers ) noexcept
{
    xassert(pAppName);

    //
    // For memory leak detection
    //
#if _XCORE_COMPILER_VISUAL_STUDIO && _XCORE_DEBUG
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    if( false )
    {
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
        _CrtSetBreakAlloc(299);
    }
#endif

    //
    // Allocate systems
    //
    xcore::global::state::ms_Instance.emplace<std::unique_ptr<global::state>>( new global::state(pAppName) );

    //
    // Initialize
    //
    get().m_Scheduler.Init(nWorkers);
}

//------------------------------------------------------------------------------

void xcore::Kill( void ) noexcept
{
    get().m_Scheduler.Shutdown();
    xcore::global::state::ms_Instance = nullptr;
}
