

namespace xcore::textfile::version_2
{
    //------------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T >  
        constexpr static bool is_valid_type_v = false
            || std::is_same_v< bool                  , T >
            || std::is_same_v< std::uint8_t          , T >
            || std::is_same_v< std::uint16_t         , T >
            || std::is_same_v< std::uint32_t         , T >
            || std::is_same_v< std::uint64_t         , T >
//            || std::is_same_v< std::char           , T >
            || std::is_same_v< std::int16_t          , T >
            || std::is_same_v< std::int32_t          , T >
            || std::is_same_v< std::int64_t          , T >
            || std::is_same_v< std::int8_t           , T >
            || std::is_same_v< float                 , T >
            || std::is_same_v< double                , T >
            || std::is_same_v< string::view<char>    , T >
            || std::is_same_v< string::ref<char>     , T >;
    }

    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename... T_ARGS > inline
    xcore::err stream::Field( xcore::crc<32> UserType, const char(&pFieldName)[N], T_ARGS&... Args ) noexcept
    {
        static_assert( (details::is_valid_type_v<T_ARGS> || ... ) );
        arglist::out Out{ &Args... };
        return isReading() ? ReadColumn( UserType, pFieldName, Out ) 
                           : WriteColumn( UserType, pFieldName, Out );
    }

    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename... T_ARGS > inline
    xcore::err stream::Field( const char(&pFieldName)[N], T_ARGS&... Args ) noexcept
    {
        xcore::crc<32> UserType = xcore::crc<32>{0};
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

            if( Count == ~0 ) Count = 1;
            for( std::remove_const_t<decltype(Count)> i=0; i<Count; i++ )
            {
                Callback(i,Error);
                if( Error ) return true;
                if( WriteLine().isError(Error) ) return true;
            }
        }
        return false;
    }

    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename TT, typename T > inline
    xcore::err stream::Record( const char (&Str)[N], TT&& RecordStar, T&& Callback) noexcept
    {
        xcore::err Error;
        if( Record(Error, Str, std::forward<TT&&>(RecordStar), std::forward<T&&>(Callback) ) )
        {
            if( Error == false )
            {
                
            }
            else
            {
                
            }
        }

        return Error;
    }

    //------------------------------------------------------------------------------------------------

    template< std::size_t N, typename T > inline
    bool stream::Record( xcore::err& Error, const char (&Str)[N], T&& Callback ) noexcept
    {
        return Record( Error, Str, 
            [&]( std::size_t& C, xcore::err& Error )
            {
                if( m_File.m_States.m_isReading )   xassert( C == 1 );
                else                                C = ~0;
            }
            , std::forward<T&&>(Callback) );
    }

}