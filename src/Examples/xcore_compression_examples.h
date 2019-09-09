

namespace xcore::compression::examples
{
    // Some random data
    constexpr static xcore::string::constant s_Buffer{ R"(
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
    )" };

    void DictionaryCompression( const std::uint32_t BlockSize = 64 )
    {
        xcore::compression::dictionary::compress Compress;
        xcore::err Error;

        Compress.Init(BlockSize, { (std::byte*)s_Buffer.get(), s_Buffer.size() }).CheckError();

        xcore::vector<std::pair<std::uint32_t,xcore::unique_span<std::byte>>> Vec;
        std::uint32_t TotalCompressSize = 0;
        while (true)
        {
            Vec.append().second.New(BlockSize *2).CheckError();
            if (Compress.Pack( Vec.last().first, Vec.last().second ).isError(Error))
            {
                TotalCompressSize += Vec.last().first;
                if (Error.getCode().getState<xcore::compression::dictionary::error_state>() != xcore::compression::dictionary::error_state::NOT_DONE)
                {
                    xassert(false);
                }
                Error.clear();
            }
            else
            {
                TotalCompressSize += Vec.last().first;
                break;
            }
        }

        xcore::compression::dictionary::decompression Decompress;

        Decompress.Init(BlockSize).CheckError();
        xcore::unique_span<std::byte> Final;
        Final.New(s_Buffer.size()).CheckError();

        std::uint32_t Pos = 0;
        for (const auto& [Size, Data] : Vec )
        {
            std::uint32_t DecompressSize = 0;
            Decompress.Unpack(DecompressSize, { &Final[Pos], Final.size() - Pos }, Data.ViewTo( Size ) ).CheckError();
            Pos += DecompressSize;
        }

        xassert(Pos == s_Buffer.size());
        xassert( std::memcmp(Final.data(), s_Buffer.get(), Final.size()) == 0 );

        int a = 22;
    }

    void Test(void)
    {
        DictionaryCompression();

    }
}