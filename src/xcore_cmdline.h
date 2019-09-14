#ifndef _XCORE_CMD_LINE_H
#define _XCORE_CMD_LINE_H
#pragma once

namespace xcore::cmdline
{
    //------------------------------------------------------------------------------
    // Description:
    //     The command line class is used to help process command line options. These
    //     options usually come from the CLI but they can also come from any other source.
    //     The command handling is very verbose the help handle any time of commands. 
    //
    //<P>  There is two steps needed for the use of the class. First is to define valid commands.
    //     This is done with the AddCmdSwitch switch. These will define all valid switches that the
    //     program can understand. Each switch in turn can be of different types. The list can 
    //     be found here xcore::cmdline::parser::type.  The class supports response files. This means that if the your 
    //     enters @FileName it will  read command switches from that file. This is very handy to create 
    //     scripts and very large command line lists. Inside the file the user can also enter other 
    //     response file switches.
    //
    //<P>  One of the key commands which by default is supported is the questions mark '?'.
    //     This will indicate to the class that the user needs help. This can be detected by
    //     calling DoesUserNeedsHelp right after the Parse function.
    //
    //<P>  The second step is to call the appropriate Parse function. This function will start the  
    //     process of parsing. The function GetCommandCount will allow you to loop throw all the switches.
    //     Each switch will return any parameters as an xstring. You are responsible for converting 
    //     those arguments to the actual type. Via x_atof32 or what ever other function you may need.
    //     The class is design to allow reprocessing of the command line. Simply call ClearArguments
    //     and ask the class to Parse some more.
    //
    //<P>  Explaining AddCmdSwitch(         // * The return value of the AddCmdSwitch is the ID of the switch
    //          pName,                      // First is the name of the switch
    //          MinArgCount,                // The switch itself may contain arguments so this is min count of them
    //          MaxArgCount                 // This is MAX count of the arguments. Note -1 is infinity
    //          nMinTimes                   // Number of times this switch can appear in the arguments
    //          nMaxTimes                   // This how the maximum of times... and again -1 == infinity
    //          MustFallowOrder             // This indicate if the switch must follow the order of its declaration via (AddCmdSwitch)
    //          Type                        // The type that the arguments will be in
    //          bMainSwitch                 // There is only 1 main switch allowed at a time. So in a way it to make the switch exclusive.
    //                                      // Main switches may have children switches that are not main switches.
    //          iParentID );                // Switches can be hierarchical so one switch may have other children switches
    //
    // Example:
    //<CODE>
    //  void BasicTest( void )
    //  {
    //      xcore::cmdline::parser  CmdLine;
    //      const char*             pString = "-FileName SomeFile.txt -Res 640 480";
    //  
    //      // Add the command switches
    //      CmdLine.AddCmdSwitch( X_STR_CRCSTR("FileName"), 1, 1, 1, 1, false, xcore::cmdline::parser::type::STRING );
    //      CmdLine.AddCmdSwitch( X_STR_CRCSTR("Res"), 2, 2, 1, 1, false, xcore::cmdline::parser::type::INT );
    //  
    //      // Parse the command line
    //      CmdLine.Parse( pString );
    //      if( CmdLine.DoesUserNeedsHelp() )
    //      {
    //          log::Output( "-FileName <filename> -Res <xres> <yres> \n" );
    //          return;
    //      }
    //  
    //      // Handle commands
    //      for( std::int32_t i=0; i<CmdLine.getCommandCount(); i++ )
    //      {
    //          const auto  Cmd     = CmdLine.getCommand(i);
    // 
    //          switch( Cmd.getCmdCRC() )
    //          {
    //              case crc::FromString<32>( "FileName" ):
    //              {
    //                  xassert( Cmd.getArgumentCount() == 1 );
    //                  const xstring&  String = Cmd.getArgument( 0 );
    //                  xassert( x_strcmp( "SomeFile.txt", &String[0] ) == 0 );
    //                  break;
    //              }
    //              case crc::FromString<32>( "Res" ):
    //              {
    //                  xassert( Cmd.getArgumentCount() == 2 );
    //                  const std::int32_t XRes   = x_atoi32<xchar>( Cmd.getArgument( 0 ) );
    //                  const std::int32_t YRes   = x_atoi32<xchar>( Cmd.getArgument( 1 ) );
    //                  xassert( XRes == 640 );
    //                  xassert( YRes == 480 );
    //                  break;
    //              }
    //              default: 
    //                  xassert( false );
    //          }
    //     }
    // }
    //<CODE>  
    //------------------------------------------------------------------------------
    class parser;

    enum class type : std::uint32_t
    {
          NONE
        , INT
        , HEX
        , FLOAT
        , STRING
        , STRING_RETAIN_QUOTES
    };

    class cmd
    {
    public:

                                                    cmd                 ( void )                          = delete;
        xforceinline   std::int32_t                 getArgumentCount    ( void )            const noexcept;
        xforceinline   std::int32_t                 getArgumentOffset   ( void )            const noexcept;
        xforceinline   string::view<const char>     getArgument         ( int Index )       const noexcept;
        xforceinline   xcore::crc<32>               getCRC              ( void )            const noexcept;
        xforceinline   string::view<const char>     getName             ( void )            const noexcept;

    protected:

        using handle = units::handle< cmd >;

    protected:

        xforceinline                                cmd                 ( const xcore::cmdline::parser& CmdLine, handle CmdHandle ) noexcept;

    protected:

        const xcore::cmdline::parser&       m_CmdLine;
        const handle                        m_CmdHandle;
        const int                           m_CmdArgOffset;

        friend class  xcore::cmdline::parser;
    };

    class parser
    {
    public:

        xforceinline   cmd::handle       AddCmdSwitch        (  string::const_crc<char>					Name,
                                                                std::int32_t                            MinArgCount     =  0, 
                                                                std::int32_t                            MaxArgCount     = -1, 
                                                                std::int32_t                            nMinTimes       =  0, 
                                                                std::int32_t                            nMaxTimes       = -1, 
                                                                bool                                    MustFallowOrder = false, 
                                                                type                                    Type            = type::STRING, 
                                                                bool                                    bMainSwitch     = false, 
                                                                std::int32_t                            iParentID       = -1 )  noexcept;

                    void                ClearArguments      ( void )                                                            noexcept;
        inline      bool                DoesUserNeedsHelp   ( void )                                                    const   noexcept;

                    xcore::err          Parse               ( std::int32_t argc, const char** argv )                            noexcept;
                    xcore::err          Parse               ( const char* pString )                                             noexcept;
        inline      std::int32_t        getCommandCount     ( void )                                                    const   noexcept;
        inline      cmd                 getCommand          ( int Index )                                               const   noexcept { return { *this, cmd::handle{ static_cast<std::uint32_t>(Index) } }; }

    protected:

        struct cmd_def
        {
            string::const_crc<char>		m_NameInfo      {}; // switch name
            std::int32_t                m_MinArgCount   {}; // Minimum number of arguments that this switch can have
            std::int32_t                m_MaxArgCount   {}; // Max number of arguments that this switch can have    
            std::int32_t                m_nMaxTimes     {}; // MAximun of times that this switch can exits (-1) infinite
            std::int32_t                m_nMinTimes     {}; // Minimum of times that this switch can exits (-1) infinite
            std::int32_t                m_RefCount      {}; // Number of instances at this time pointing at this type
            bool                        m_bFallowOrder  {}; // This argument is in the proper order can't happen before
            type                        m_Type          {}; // Type of arguments that this switch can have
            bool                        m_bMainSwitch   {}; // This to show whether is a main switch. The user can only enter one switch from this category
            std::int32_t                m_iParentID     {}; // This indicates that this switch can only exits if iParentID command is already active
        };

        struct cmd_entry
        {
            std::int32_t                m_iCmdDef       {};
            std::int32_t                m_iArg          {};
            std::int32_t                m_ArgCount      {};
        };

    protected:

                        void                        AppendCmd               ( std::int32_t iCmd, string::view<char> String )            noexcept;
                        void                        AppendCmd               ( std::int32_t iCmd )                                       noexcept;
                        xcore::err                  ProcessResponseFile     ( string::view<char> PathName )                             noexcept;
                        xcore::err                  Parse                   ( xcore::vector<string::ref<char>>& Arguments )             noexcept;
        inline          std::int32_t                getArgumentCount        ( void )                                            const   noexcept;

        inline          xcore::crc<32>              getCmdCRC               ( cmd::handle CmdHandle )                           const   noexcept;
        inline          string::view<const char>    getCmdName              ( cmd::handle CmdHandle )                           const   noexcept;
        inline          std::int32_t                getCmdArgumentOffset    ( cmd::handle CmdHandle )                           const   noexcept;
        inline          std::int32_t                getCmdArgumentCount     ( cmd::handle CmdHandle )                           const   noexcept;
        inline          string::view<const char>    getArgument             ( int Index )                                       const   noexcept;

    protected:

        xcore::log::channel                 m_Channel       { "xcore::cmdline::parser" };
        bool                                m_bNeedHelp     { false };
        xcore::vector<cmd_def>              m_CmdDef        {};
        xcore::vector<string::ref<char>>    m_Arguments     {};
        xcore::vector<cmd_entry>            m_Command       {};

        friend class cmd;
    };
}
#endif