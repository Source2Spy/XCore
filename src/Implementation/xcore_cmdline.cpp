
namespace xcore::cmdline
{
    //---------------------------------------------------------

    xcore::err parser::Parse( std::int32_t argc, const char** argv ) noexcept
    {    
        xcore::err Error;

        // User needs help
        if( argc == 1 ) 
        {
            std::int32_t TotalMustHave = 0;
            for( std::int32_t i=0; i<m_CmdDef.size(); i++ )
            {
                TotalMustHave += m_CmdDef[i].m_nMinTimes;
            }

            if( TotalMustHave > 0 )
            {
                ClearArguments();
                m_bNeedHelp = true;
                return Error;
            }
        }

        // Put all the arguments into the list
        // skip argv 0 sinec it is the file name.
        vector<string::ref<char>> Arguments;
        for( std::int32_t i=1; i<argc; i++ )
        {
            xassert( argv[i] );
        
            string::Copy( Arguments.append(), argv[i] );
            if( Arguments[i-1][0] == '-' ) 
                Arguments[i-1][0] = 1;
        }

        // Parse these arguments
        return Parse( Arguments );
    }

    //---------------------------------------------------------

    xcore::err  parser::Parse( const char* pString ) noexcept 
    {
        xcore::err                  Error;
        std::int32_t                iStart;
        std::int32_t                iEnd;
        std::int32_t                iNext;
        std::int32_t                Length = string::Length(pString).m_Value;
        string::ref<char>           strCmd;
        vector<string::ref<char>>   Arguments;

        string::Copy( strCmd, pString );

        // process all the arguments
        iStart = 0;
        while( iStart < Length )
        {
            // Skip Whitespace
            while( (iStart < Length) && string::isCharSpace( strCmd[iStart] ) )
            {
                iStart++;
            }

            // At end of string?
            if( iStart < Length )
            {
                // Find limits of string
                iEnd = iStart+1;
                bool bStartArgument = false;
                if( strCmd[iStart] == '"' )
                {
                    bStartArgument = true;
                    if( iStart > 0 )
                    {
                        if( strCmd[iStart-1]=='\\')
                            bStartArgument = false;
                    }
                }
                else if (strCmd[iStart] == '-' && !string::isCharDigit(strCmd[iStart+1]) && strCmd[iStart+1]!='.')
                {
                    // Replace the switch identifier to ascii 1
                    strCmd[iStart] = 1;
                }

                if(bStartArgument)
                {
                    // iStart++; // old vesion removed the quotes new version will keep them
                    while( iEnd<Length )
                    {
                        if(strCmd[iEnd] == '"')
                        {
                            if(strCmd[iEnd-1]!='\\')
                            {
                                iEnd++;     // old version removed the quotes but the new version keeps it
                                break;
                            }
                        }
                        iEnd++;
                    }
                    iNext = iEnd+1;
                }
                else
                {
                    while( (iEnd<Length) && !string::isCharSpace(strCmd[iEnd]) )
                        iEnd++;
                    iNext = iEnd;
                }

                // Add to argument array
                string::Copy( Arguments.append(), strCmd.getView().ViewFromTo( iStart, iEnd ) );

                // Set start past end of string
                iStart = iNext;
            }
        }

        // Parse these arguments
        return Parse( Arguments );
    }

    //---------------------------------------------------------
    xcore::err parser::ProcessResponseFile( string::view<char> PathName ) noexcept
    {
        unique_span<char>   Buff;

        // Read file to memory
        {
            xcore::err  Error;
            FILE*       pFile = nullptr;
            error::scope Cleanup( Error, [&]
            {
                if(pFile) fclose(pFile);
            });

            // Open the file
            if( auto Err = fopen_s( &pFile, PathName, "rt"); Err )
                return xerr_failure( Error, "Error: Unable to open the response file" );

            //
            // Read the hold file in
            //
            fseek(pFile,0,SEEK_END);
            auto Length = ftell(pFile);
            fseek(pFile,0,SEEK_SET);

            if( Buff.New( Length+1 ).isError( Error ) ) 
                return Error;

            if( Length != fread( &Buff[0], 1, Length, pFile ) )
                return xerr_failure( Error, "Error: Reading the response file" );

            Buff[Length] = 0;
        }        

        //
        // Now process the data
        //
        return Parse( &Buff[0] );
    }

    //---------------------------------------------------------

    xcore::err parser::Parse( vector<string::ref<char>>& Args ) noexcept
    {
        xcore::err      Error;
        std::int32_t    i;
        std::int32_t    iMainSwitch=-1;
    /*
        for( i=0; i<Args.getCount(); i++)
        {
            xstring&  curArg = Args[i];
            std::int32_t iLength = curArg.GetLength();
            char* pReading = new char[iLength+1];
            char* pWriting = new char[iLength+1];
            char* pSource = pReading;
            char* pTarget = pWriting;
            x_strcpy(pReading, iLength+1, curArg);
            x_strcpy(pWriting, iLength+1, pReading);
            std::int32_t iSlash = FindCharacterInString(pSource, '\\');
            while( iSlash>=0 )
            {
                std::int32_t iOffset = 0;
                if(pSource+iSlash < pReading+iLength-1)
                    if( pSource[iSlash+1]=='\"' )
                        iOffset = 1;

                if(iOffset)
                    x_strcpy( pTarget+iSlash, x_strlen(pSource)-iSlash+1-iOffset, pSource+iSlash+iOffset );
                pSource = pSource+iSlash+1;
                pTarget = pTarget+iSlash+1-iOffset;
                if( pSource-pReading >= iLength )
                    break;
                iSlash = FindCharacterInString(pSource, '\\');
            }

            curArg.Clear();
            curArg.Format(pWriting);
            delete[] pReading;
            delete[] pWriting;
        }
    */

        // Process Args
        for( i=0 ; i<Args.size(); i++ )
        {
            auto&  a = Args[i];

            // Check for Help?
            if( (a == "?") || (a == "\1?") || (a == "\1HELP") )
            {
                ClearArguments();
                m_bNeedHelp = true;
                return Error;
            }

            // Check for response file
            if( a[0] == '@' )
            {
                if( ProcessResponseFile( a.getView().ViewFrom(1) ).isError(Error))
                    return Error;
                continue;
            }

            // Check for a command
            // The switch identifier is replaced by ascii 1 at this time
            // if( a[0] == '-' && ( (!x_isdigit(a[1])) && (a[1] != '.')) )
            if( a[0] == 1 && ( (!string::isCharDigit(a[1])) && (a[1] != '.')) )
            {
                bool   Found = false;

                // Remove leading '-' and find crc
                auto CRC = xcore::crc<32>::FromString( &a[1] );

                // check for minimum rage of the previous switch
                if( m_Command.size() )
                {
                    cmd_entry& Cmd = m_Command[ m_Command.size()-1 ];

                    if( m_CmdDef[ Cmd.m_iCmdDef ].m_MinArgCount > Cmd.m_ArgCount )
                    {
                        ClearArguments();
                        //x_throw( "Error: We found this switch [%s] had too few arguments we expected at least [%d].", (const char*)a, m_CmdDef[ Cmd.m_iCmdDef ].m_MinArgCount );
                        return xerr_failure( Error, "Error: We found this switch that had too few arguments" );
                    }                
                }

                // Search for option and read value into option list
                for( std::int32_t j=0 ; j<m_CmdDef.size(); j++ )
                {
                    // Check if found.
                    // The switch we are looking for must match the name and its parent id
                    if( m_CmdDef[j].m_NameInfo.m_CRC == CRC && 
                       iMainSwitch == m_CmdDef[j].m_iParentID  )
                    {
                        //xstring OptionValue;

                        Found = true;

                        // Add the cmd entry
                        cmd_entry& Cmd = m_Command.append();
                        Cmd.m_iCmdDef  = j;
                        Cmd.m_iArg     = static_cast<int>(m_Arguments.size());
                        Cmd.m_ArgCount = 0;

                        // if it is a main switch make sure that there are not other ones active
                        if( m_CmdDef[j].m_bMainSwitch )
                        {
                            // Set the variable to have the id of the main switch
                            iMainSwitch = j;
                        
                            for( std::int32_t t=0; t<m_CmdDef.size(); t++ )
                            {
                                if( t == j ) continue;
                                if( m_CmdDef[t].m_RefCount <= 0 ) continue;
                                if( m_CmdDef[t].m_bMainSwitch == false ) continue;

                                ClearArguments();
                                // x_throw( "Error: We found two main switches [%s] and [%s]. You can only enter one of this type of switches.", (const char*)m_CmdDef[t].m_Name, (const char*)m_CmdDef[j].m_Name );
                                return xerr_failure( Error, "Error: We found two main switches. You can only enter one of this type of switches." ); 
                            }
                        }

                        // make sure that we don't have this reference too many times
                        m_CmdDef[j].m_RefCount++;
                        if( m_CmdDef[j].m_RefCount > m_CmdDef[j].m_nMaxTimes && m_CmdDef[j].m_nMaxTimes != -1 )
                        {
                            ClearArguments();
                            //x_throw( "Error: We found this switch [%s] too many times in the command-line. We were expecting to find it only [%d] times", (const char*)a, m_CmdDef[j].m_nMaxTimes );
                            return xerr_failure( Error, "Error: We found a switch too many times in the command-line. We were expecting to find it less times" );
                        }

                        // make sure that fallows the proper order in the sequence
                        if( m_CmdDef[j].m_bFallowOrder )
                        {
                            for( std::int32_t t=0; t<m_Command.size(); t++ )
                            {
                                if( j < m_Command[t].m_iCmdDef )
                                {
                                    ClearArguments();
                                    //x_throw( "Error: This switch:[%s] is out of order check help for proper usage.", (const char*)a );
                                    return xerr_failure( Error, "Error: a switch: is out of order check help for proper usage." );
                                }
                            }
                        }

                        break;
                    }
                    else
                    {
                        //
                        // We are going to be nice and search to see if there is another switch
                        // that the user may have been talking about but it is the wrong parenting
                        //
                        if( m_CmdDef[j].m_NameInfo.m_CRC == CRC )
                        {
                            bool bFoundSameName = false;
                            for( std::int32_t k=j+1 ; k<m_CmdDef.size(); k++ )
                            {
                                if( m_CmdDef[k].m_NameInfo.m_CRC == CRC && iMainSwitch == m_CmdDef[k].m_iParentID )
                                {
                                    bFoundSameName = true;
                                }
                            }
                        
                            if( bFoundSameName == false )
                            {
                                // check whether it needs another switch to exits
                                if( m_CmdDef[j].m_iParentID != -1 )
                                {
                                    std::int32_t Index = m_CmdDef[j].m_iParentID;
                                    if( m_CmdDef[ Index ].m_RefCount <= 0 )
                                    {
                                        ClearArguments();
                                        //x_throw( "Error: We found a switch:[%s] which can only be use with this other switch:[%s]", (const char*)m_CmdDef[j].m_Name, (const char*)m_CmdDef[Index].m_Name );
                                        return xerr_failure( Error, "Error: We found a switch which can only be use with another switch" );
                                    }
                                }
                            }
                        }
                    }
                }

                // Check if option was found
                if( !Found )
                {
                    ClearArguments();
                    // x_throw( "Error: Unable to find a match for this switch [%s]", (const char*)a );
                    return xerr_failure( Error, "Unable to find a match for a switch" );
                }
            }
            else
            {
                // Add to argument list
                m_Arguments.append() = a;

                // make sure that we at least have one command going on
                if( m_Command.size() <= 0 )
                {
                    // We forgive argument zero because it is the name of the exe
                    if( m_Arguments.size() > 1 )
                    {
                        ClearArguments();
                        //x_throw( "Error: Arguments been pass without setting switches Arg:[%s]", (const char*)a );
                        return xerr_failure( Error, "Error: Arguments been pass without setting switches" );
                    }
                }
                else
                {
                    // Notify the cmd entry about its new arg
                    std::int32_t iCommand = static_cast<std::int32_t>(m_Command.size()-1);
                    m_Command[ iCommand ].m_ArgCount++;

                    // Make sure that we have the righ number of maximun arguments
                    cmd_def& Def = m_CmdDef[ m_Command[ iCommand ].m_iCmdDef ];
                    if( m_Command[ iCommand ].m_ArgCount > Def.m_MaxArgCount && Def.m_MaxArgCount != -1 )
                    {
                        ClearArguments();
                        // x_throw( "Error: The command has too many Arguments for the switch:[%s]", (const char*)Def.m_Name );
                        return xerr_failure( Error, "Error: The command has too many Arguments for a switch" );
                    }

                    // Make sure that the type matches with the expected type
                    if( Def.m_Type != type::NONE )
                    {
                        const char* pTypeString = nullptr;
                        const char* pExpectType = nullptr;
                        switch( Def.m_Type )
                        {
                        case type::NONE:
                                xassert(0);
                                break;
                        case type::INT:
                            {
                                int L = string::Length(a).m_Value;
                                for( int i=0; i<L; i++ )
                                {
                                    if ( string::isCharDigit<char>( a[i] ) == false
                                        && a[i] != '-' 
                                        && a[i] != '+' )
                                    {
                                        pExpectType = "INT";
                                        break;
                                    }
                                }
                                
                                if( pExpectType == nullptr )
                                    pTypeString = "INT";
                                
                                break;
                            }
                        case type::FLOAT:
                            {
                                int L = string::Length(a).m_Value;
                                for( int i=0; i<L; i++ )
                                {
                                    if ( string::isCharDigit<char>( a[i] ) == false
                                        && a[i] != '-' 
                                        && a[i] != '+' 
                                        && a[i] != '.' )
                                    {
                                        pExpectType = "FLOAT";
                                        break;
                                    }
                                }
                                
                                if( pExpectType == nullptr )
                                    pTypeString = "FLOAT";

                                break;
                            }
                        case type::HEX:
                            {
                                int L = string::Length(a).m_Value;
                                for( int i=0; i<L; i++ )
                                {
                                    if ( string::isCharHex<char>( a[i] ) == false )
                                    {
                                        pExpectType = "HEX";
                                        break;
                                    }
                                }
                                
                                if( pExpectType == nullptr )
                                    pTypeString = "HEX";

                                break;
                            }
                        case type::STRING:
                            {
                                pTypeString = "STRING";

                                // The new version removed quotes for string only
                                if( a[0] == '"' )
                                {
                                    std::int32_t l = string::Length(a).m_Value;
                                    for( std::int32_t i=0;i<l; i++ )
                                    {
                                        a[i] = a[i+1];
                                    }
                                    a[l-2] = 0;
                                }
                                break;
                            }
                        case type::STRING_RETAIN_QUOTES:
                            {
                                pTypeString = "STRING_RETAIN_QUOTES";
                                break;
                            }
                        default:
                            {
                                xassert(0);
                            }
                        }

                        if( !pTypeString )
                        {
                            ClearArguments();
                            //x_throw( "Error: expecting a [%s] but found something different for the argument of the switch:[%s]", pExpectType, (const char*)Def.m_Name );
                            return xerr_failure( Error, "Error: expecting a type but found something different for the argument of a switch" );
                        }
                    }            
                }
            }
        }

        // check for minimum rage of the last switch (how many arguments can it have)
        if( m_Command.size() )
        {
            cmd_entry& Cmd = m_Command[ m_Command.size()-1 ];

            if( m_CmdDef[ Cmd.m_iCmdDef ].m_MinArgCount > Cmd.m_ArgCount )
            {
                ClearArguments();
                // x_throw( "Error: We found this switch [%s] had too few arguments we expected at least [%d].", (const char*)m_CmdDef[ Cmd.m_iCmdDef ].m_Name, m_CmdDef[ Cmd.m_iCmdDef ].m_MinArgCount );
                return xerr_failure( Error, "Error: We found a switch had too few arguments we expected more" );
            }                
        }

        // check whether the command line was expecting for a switch but it never happen
        for( i=0; i<m_CmdDef.size(); i++ )
        {
            const cmd_def& CmdDef = m_CmdDef[i];
         
            if( CmdDef.m_iParentID != iMainSwitch && 
                CmdDef.m_iParentID != -1 )
                continue;
        
            if( CmdDef.m_RefCount < CmdDef.m_nMinTimes )
            {
                ClearArguments();
                // x_throw( "Error: We were expecting this switch[%s] to happen [%d] times, but we found it [%d] times.", (const char*)CmdDef.m_Name, CmdDef.m_nMinTimes, CmdDef.m_RefCount );
                return xerr_failure( Error, "Error: We were expecting a switch to happen n times, but we found it <n times." );
            }
        }

        return Error;
    }


    //---------------------------------------------------------

    void parser::ClearArguments( void ) noexcept
    {
        m_bNeedHelp = false;
        m_Arguments.clear();
        m_Command.clear();

        for( std::int32_t i=0; i<m_CmdDef.size(); i++ )
        {
            m_CmdDef[i].m_RefCount = 0;
        }
    }
}
