

namespace xcore::textfile::version_2
{
    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename... T_ARGS > inline
    xcore::err stream::Field( crc::units<32>& UserType, const char(&pFieldName)[N], T_ARGS&... Args ) noexcept
    {
        static_assert( (std::is_const_v<T_ARGS> || ... ) == false );
        if( isReading() )
        {
            arglist::out Out{ &Args... };
            return ReadFieldC( UserType, pFieldName, Out );
        }

        arglist::out Out{ &Args... };
        return WriteColumn( UserType, pFieldName, Out );
    }

    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename... T_ARGS > inline
    xcore::err stream::Field( const char(&pFieldName)[N], T_ARGS&... Args ) noexcept
    {
        crc::units<32> UserType = crc::units<32>{0};
        return Field( UserType, pFieldName, Args... );
    }
    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename TT, typename T > inline
    bool stream::Record( xcore::err& Error, const char (&Str)[N], TT&& RecordStar, T&& Callback ) noexcept
    {
        if( m_File.m_States.m_isReading )
        {
            if( getRecordName() != Str )
            { 
                (void)xerr_failure( Error, "Unexpected record" ); 
                return true; 
            }
            std::size_t Count = getRecordCount();
            RecordStar(Count,Error);
            for( std::remove_const_t<decltype(Count)> i=0; i<Count; i++ )
            {
                if( ReadLine().isError(Error) ) return true;
                Callback(i,Error);
                if( Error ) return true;
            }
            // Read the next record
            if( ReadRecord().isError( Error ) ) 
            {
                if( Error.getCode().getState<error_state>() == error_state::UNEXPECTED_EOF ) Error.clear();
                else return true;
            }
        }
        else 
        {
            std::size_t Count;
            RecordStar(Count,Error);
            if( WriteRecord( Str, Count ).isError( Error ) ) return true;

            for( std::remove_const_t<decltype(Count)> i=0; i<Count; i++ )
            {
                Callback(i,Error);
                if( Error ) return true;
                if( WriteLine().isError(Error) ) return true;
            }
        }
        return false;
    }
}