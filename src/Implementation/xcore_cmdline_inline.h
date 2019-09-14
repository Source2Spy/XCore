namespace xcore::cmdline
{
    //------------------------------------------------------------------------------------

    xforceinline                                cmd::cmd                 ( const xcore::cmdline::parser& CmdLine, handle CmdHandle ) noexcept : m_CmdLine{ CmdLine }, m_CmdHandle{ CmdHandle }, m_CmdArgOffset{ m_CmdLine.getCmdArgumentOffset(CmdHandle) } {}
    xforceinline   std::int32_t                 cmd::getArgumentCount    ( void )            const noexcept { return m_CmdLine.getCmdArgumentCount(m_CmdHandle);          }
    xforceinline   std::int32_t                 cmd::getArgumentOffset   ( void )            const noexcept { return m_CmdLine.getCmdArgumentOffset(m_CmdHandle);         }
    xforceinline   string::view<const char>     cmd::getArgument         ( int Index )       const noexcept { return m_CmdLine.getArgument( Index + m_CmdArgOffset );     }
    xforceinline   xcore::crc<32>               cmd::getCRC              ( void )            const noexcept { return m_CmdLine.getCmdCRC(m_CmdHandle);                    }
    xforceinline   string::view<const char>     cmd::getName             ( void )            const noexcept { return m_CmdLine.getCmdName(m_CmdHandle);                   }

    //------------------------------------------------------------------------------------
    inline
    cmd::handle parser::AddCmdSwitch(
        string::const_crc<char>		        Name,
        std::int32_t						MinArgCount,
        std::int32_t						MaxArgCount,
        std::int32_t						nMinTimes,
        std::int32_t						nMaxTimes,
        bool								MustFallowOrder,
        type								Type,
        bool								bMainSwitch,
        std::int32_t						iParentID ) noexcept
    {
        cmd_def& CmdDef         = m_CmdDef.append();

        CmdDef.m_NameInfo       = Name;
        CmdDef.m_MinArgCount    = MinArgCount;      
        CmdDef.m_MaxArgCount    = MaxArgCount;      
        CmdDef.m_nMaxTimes      = nMaxTimes;           
        CmdDef.m_nMinTimes      = nMinTimes;           
        CmdDef.m_RefCount       = 0;         
        CmdDef.m_bFallowOrder   = MustFallowOrder;     
        CmdDef.m_Type           = Type;             
        CmdDef.m_bMainSwitch    = bMainSwitch; 
        CmdDef.m_iParentID      = iParentID;
  
        return cmd::handle{ static_cast<std::uint32_t>(m_CmdDef.size()-1) };
    }

    //------------------------------------------------------------------------------------
    inline
    std::int32_t parser::getCommandCount( void ) const noexcept
    {
        return static_cast<std::int32_t>(m_Command.size());
    }

    //------------------------------------------------------------------------------------
    inline
    std::int32_t parser::getArgumentCount( void ) const noexcept
    {
        return static_cast<std::int32_t>(m_Arguments.size());
    }

    //------------------------------------------------------------------------------------
    inline
    xcore::crc<32> parser::getCmdCRC( cmd::handle H ) const noexcept
    {
        return m_CmdDef[ m_Command[H.m_Value].m_iCmdDef ].m_NameInfo.m_CRC;
    }

    //------------------------------------------------------------------------------------
    inline
    std::int32_t parser::getCmdArgumentOffset( cmd::handle H ) const noexcept
    {
        return m_Command[H.m_Value].m_iArg;
    }

    //------------------------------------------------------------------------------------
    inline
    std::int32_t parser::getCmdArgumentCount( cmd::handle H ) const noexcept
    {
        return m_Command[H.m_Value].m_ArgCount;
    }

    //------------------------------------------------------------------------------------
    inline
    string::view<const char> parser::getCmdName( cmd::handle H ) const noexcept
    {
        return m_CmdDef[ m_Command[H.m_Value].m_iCmdDef ].m_NameInfo.m_Str;
    }

    //------------------------------------------------------------------------------------
    inline
    string::view<const char> parser::getArgument( int Index ) const noexcept
    {
        return m_Arguments[Index].getView();
    }

    //------------------------------------------------------------------------------------
    inline
    bool parser::DoesUserNeedsHelp( void ) const noexcept
    {
        return m_bNeedHelp;
    }
}

