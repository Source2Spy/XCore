

namespace xcore::cmdline::examples
{

	//-----------------------------------------------------------------------------------------

	void BasicTest(void)
	{
		xcore::cmdline::parser CmdLine;
		const char* pString = "-FileName SomeFile.txt -Res 640 480";

		// Add the command switches
		CmdLine.AddCmdSwitch(xcore::string::const_crc("FileName"), 1, 1, 1, 1, false, cmdline::type::STRING );
		CmdLine.AddCmdSwitch(xcore::string::const_crc("Res"),      2, 2, 1, 1, false, cmdline::type::INT    );

		// Parse the command line
		if (auto Err = CmdLine.Parse(pString); Err || CmdLine.DoesUserNeedsHelp() )
		{
			log::Output("-FileName <filename> -Res <xres> <yres> \n");
			Err.clear();
			return;
		}

		// Handle commands
		for( int i = 0; i < CmdLine.getCommandCount(); i++ )
		{
			const auto Cmd = CmdLine.getCommand(i);

			switch( Cmd.getCRC().m_Value )
			{
            case xcore::crc<32>::FromString("FileName").m_Value:
			{
				xassert(Cmd.getArgumentCount() == 1);
				auto String = Cmd.getArgument(0);
				xassert( string::constant("SomeFile.txt") == String );
				break;
			}
            case xcore::crc<32>::FromString("Res").m_Value:
			{
				xassert( Cmd.getArgumentCount() == 2 );
				const auto XRes = std::atoi( Cmd.getArgument(0) );
				const auto YRes = std::atoi( Cmd.getArgument(1) );
				xassert(XRes == 640);
				xassert(YRes == 480);
				break;
			}
			default:
				xassert(false);
			}
		}
	}

	//-----------------------------------------------------------------------------------------

	void Test(void)
	{
		BasicTest();
	}
}