
//#define x_modf modf

//==============================================================================
// DEFINES
//==============================================================================
namespace xcore::string::details
{
    // --- FLAGS FOR THE VSPRINTF ---
    struct flags
    {
        std::uint16_t m_Value { 0 };

        struct
        {
                // NUMERIC VALUES
            bool        m_LONGDBL:1                 // long double; unimplemented
                ,       m_LONGINT:1                 // long integer
                ,       m_QUADINT:1                 // quad integer
                ,       m_SHORTINT:1                // short integer

                // OTHER FLAGS
                ,       m_ALT:1                     // alternate form
                ,       m_HEXPREFIX:1               // add 0x or 0X prefix
                ,       m_LADJUST:1                 // left adjustment
                ,       m_ZEROPAD:1;                // zero (as opposed to blank) pad
        };
    };

    // CONSTANTS
    constexpr   auto    SPF_LONG_MAX    =   0x7FFFFFFF;
    constexpr   auto    WORKSIZE        =   128;        // space for %c, %[diouxX], %[eEfgG]
    constexpr   auto    DEFPREC         =   6;          // number of precision for the real numbers

    //==============================================================================
    // BUFFER WRITTER
    //==============================================================================

    class wbuffer : protected xcore::string::view<char>
    {   
    public:

        constexpr wbuffer( xcore::string::view<char>& W ) noexcept :  xcore::string::view<char>{ W }{}

        constexpr int getCursor( void ) const{ return m_iCursor; }
        //------------------------------------------------------------------------------
        // WriteToBuffer
        //------------------------------------------------------------------------------
        void WriteToBuffer( const char* String, std::size_t Count ) noexcept
        {
            xassert( String );

            for( int i=0; i < Count; i++ )
            {
                xassert( i < size() );
                (*this)[m_iCursor++] = String[i];
            }
        }

        //------------------------------------------------------------------------------
        // Choose PADSIZE to trade efficiency vs. size.  If larger printf
        // fields occur frequently, increase PADSIZE and make the initialisers
        // below longer.
        //------------------------------------------------------------------------------
        void PadBuffer( int HowMany, char With ) noexcept
        {
            constexpr static std::array Blanks = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
            constexpr static std::array Zeroes = {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};
            static_assert( Blanks.size() == Zeroes.size() );

            if ( HowMany > 0 )
            {
                // Find what type we need to path with
                const char* Type = (With == ' ') ? Blanks.data() : Zeroes.data();

                std::size_t i;
                for( i = HowMany; i > Blanks.size(); i -= Blanks.size() )
                {
                    WriteToBuffer( Type, Blanks.size() );
                }

                WriteToBuffer( Type, i );
            }
        }

    protected:

        int       m_iCursor { 0 }; 
    };

    //==============================================================================
    // ARGUMENT READER
    //==============================================================================
    class arg_reader
    {
    public:

        //------------------------------------------------------------------------------

        constexpr arg_reader( const std::span<xcore::arglist::out_types> List ) noexcept : m_List{ List } {}

        //------------------------------------------------------------------------------
        template< class T >
        const T get( void ) noexcept
        {
            xassert( m_Index <= static_cast<int>(m_List.size()) );
            return xcore::arglist::get<T>( m_List[m_Index++] );
        }

        //------------------------------------------------------------------------------
        // will get an int regarless of its size
        const std::uint64_t getGenericInt( void ) noexcept
        {
            return get<std::uint64_t>();
        }

        //------------------------------------------------------------------------------
        const double getGenericF64( void ) noexcept
        {
            return get<double>();
        }

        //------------------------------------------------------------------------------
        const char* getGenericString( void ) noexcept
        {
            return get<const char*>();
        }

    protected:

        std::ptrdiff_t                              m_Index { 0 };
        const std::span<xcore::arglist::out_types>  m_List;
    };

    //==============================================================================
    // FUNCTIONS
    //==============================================================================
 
    //------------------------------------------------------------------------------
    // vsprintf_pow
    //------------------------------------------------------------------------------
    static inline
    double vsprintf_pow( double x, int p ) noexcept
    {
        if (p == 0)   return 1.0;
        if (x == 0.0) return 0.0;

        if (p < 0)
        {
            p = -p;
            x = 1.0 / x;
        }

        double r = 1.0;
        for(;;)
        {
            if (p & 1) r *= x;
            if ((p >>= 1) == 0) return r;
            x *= x;
        }

        xassume( false ); //-V779
    }

    //------------------------------------------------------------------------------
    // Check to see if it is infinite
    //------------------------------------------------------------------------------
    constexpr static bool isINF( double x ) noexcept
    {
        return x != x;
    }

    //------------------------------------------------------------------------------
    // Check to see if it is NOT a number
    //------------------------------------------------------------------------------
    constexpr static bool isNAN( double x ) noexcept
    {
        return x != x;
    }

    //------------------------------------------------------------------------------
    // fmtbase - String where the output is going to go
    //          (Make sure that the string is atleast 24 bytes long)
    // fpnum   - number which is going to be converted
    // cvt     - what type of conversion needs
    // width   - total width of the output number
    // prec    - how many digits of precision
    //------------------------------------------------------------------------------
    static
    std::size_t dtoa( string::view<char> fmtbase, double fpnum, char cvt, int width, int prec ) noexcept
    {
        constexpr std::array PowTable = {1.0,10.0,10e1,10e2,10e3,10e4,10e5,10e6,10e7,10e8,10e9,10e10,10e11,10e12,10e13,10e14,10e15,10e16,10e17,10e18,10e19,10e20,10e21,10e22,10e23 };
        std::array<char,WORKSIZE>   fwork;
        char*                       fw;

        // for integer part
        std::array<char,WORKSIZE>   iwork;
        char*                       iworkend;
        char*                       iw;

        // for exponent part
        std::array<char, 16>        ework;
        char*                       eworkend;
        char*                       ew;

        // other variables
        int     is_neg;
        double  powprec;
        double  rounder;
        int     f_fmt;
        int     iwidth;
        int     fwidth;
        int     ewidth;

        // arrange everything in returned string variables
        char*   fmt;
        int     i;
        int     showdot;
        int     fmtwidth;
        int     pad;

        xassert(fmtbase);
        //xassert(width > 0);
        xassert(prec >= 0);

        // initialize some variables
        fw        = fwork.data();

        // set up the integer part
        iworkend  = &iwork[sizeof(iwork) - 1];
        iw        = iworkend;
        *iw       = 0;

        // setup the exponent part
        eworkend  = &ework[sizeof(ework) - 1];
        ew        = eworkend;
        *ew       = 0;

        if( isINF(fpnum) )
        {
            char* inffmt  = fmtbase;
            char* inffmtp = inffmt;

            if (fpnum < 0) *inffmtp++ = '-';
            string::Copy( string::view<char>{ inffmtp, 4 }, "Inf" );
            return static_cast<int>(inffmt - fmtbase);
        }

        if( isNAN(fpnum) )
        {
            char* nanfmt = fmtbase;
            string::Copy( string::view<char>{ nanfmt, 4 }, "NaN");
            return static_cast<int>(nanfmt - fmtbase);
        }

        // grab sign & make non-negative
        is_neg = fpnum < 0;
        if (is_neg) fpnum = -fpnum;

        // precision matters

        // can't have more prec than supported
        if (prec > WORKSIZE - 2) prec = WORKSIZE - 2;

        if (prec == 6) powprec = 1.0e6;
        else
        {
            if( prec > 23 )
            {
                powprec = vsprintf_pow( 10.0, prec );
            }
            else
            {
                powprec = PowTable[prec];
            }
        }

        rounder = 0.5 / powprec;

        f_fmt = cvt == 'f' ||
              ((cvt == 'g') && (fpnum == 0.0 || (fpnum >= 1e-4 && fpnum < powprec)));

        iwidth = 0;
        fwidth = 0;
        ewidth = 0;

        if (f_fmt)  // fixed format
        {
            double ipart;
            double fpart = modf(fpnum, &ipart);

            // convert fractional part
            if (fpart >= rounder || cvt != 'g')
            {
                double  ifpart;
                int     i;
                double  ffpart;

                fpart += rounder;
                if (fpart >= 1.0)
                {
                    ipart += 1.0;
                    fpart -= 1.0;
                }

                ffpart = fpart;

                bool bAnySignificantInteger = false;
                for (i = 0; i < prec; ++i)
                {
                    ffpart = modf(ffpart * 10.0, &ifpart);
                    *fw++ = (char)('0' + static_cast<int>(ifpart));

                    // This was added to keep prec number of significan digits
                    // When the user_types is looking for higher precision we will do this
                    if( prec > DEFPREC )
                    {
                        if( bAnySignificantInteger == false && ifpart == '0' )  prec++;
                        else bAnySignificantInteger = true;
                    }

                    ++fwidth;
                }

                if (cvt == 'g')  // inhibit trailing zeroes if g-fmt
                {
                    char* p;
                    for ( p = fw - 1; p >= fwork.data() && *p == '0'; --p)
                    {
                        *p = 0;
                        --fwidth;
                    }
                }
            }

            // convert integer part
            if (ipart == 0.0)
            {
                // Removed this because I want to always see the zero I dont like things
                // that look like ".324234" I like "0.324234" to make sure we dont give
                // up precision I incremented the space for the number.
                prec++;
                //if (cvt != 'g' || fwidth < prec || fwidth < width)
                {
                    *--iw = '0'; ++iwidth;
                }
            }
            else
                if (ipart <= static_cast<double>(SPF_LONG_MAX)) // a useful speedup
                {
                    int li = static_cast<int>(ipart);
                    while (li != 0)
                    {
                        *--iw = (char)('0' + (li % 10));
                        li = li / 10;
                        ++iwidth;
                        xassert( iwidth < WORKSIZE );
                    }
                }
                else // the slow way
                {
                    while (ipart > 0.5)
                    {
                        double ff = modf(ipart / 10.0, &ipart);
                        ff = (ff + 0.05) * 10.0;
                        *--iw = (char)('0' + static_cast<int>(ff));
                        ++iwidth;
                        xassert( iwidth < WORKSIZE );
                    }
                }

            // g-fmt: kill part of frac if prec/width exceeded
            if (cvt == 'g')
            {
                int m = prec;
                int adj;

                if (m < width) m = width;

                adj = iwidth + fwidth - m;
                if (adj > fwidth) adj = fwidth;

                if (adj > 0)
                {
                    char* f;
                    for (f = &fwork[fwidth-1]; f >= fwork.data() && adj > 0; --adj, --f)
                    {
                        char ch = *f;

                        --fwidth;
                        *f = 0;

                        if (ch > '5') // properly round: unavoidable propagation
                        {
                            char* p;
                            int carry = 1;

                            for ( p = f - 1; p >= fwork.data() && carry; --p)
                            {
                                ++*p;

                                if (*p > '9') *p = '0';
                                else carry = 0;
                            }

                            if (carry)
                            {
                                for (p = iworkend - 1; p >= iw && carry; --p)
                                {
                                    ++*p;
                                    if (*p > '9') *p = '0';
                                    else carry = 0;
                                }

                                if (carry)
                                {
                                    *--iw = '1';
                                    ++iwidth;
                                    --adj;
                                    xassert( iwidth < WORKSIZE );
                                }
                            }
                        }
                    }

                    // kill any additional trailing zeros
                    {
                        char* p;
                        for ( p = f; p >= fwork.data() && *p == '0'; --p)
                        {
                            *p = 0;
                            --fwidth;
                        }
                    }
                }
            }
        }
        else  // e-fmt
        {
            double  almost_one;
            double  ipart;
            double  fpart;
            double  ffpart;
            double  ifpart;
            int     i;
            char    eneg;
            int     exp;

            // normalize
            exp = 0;

            while (fpnum >= 10.0)
            {
                fpnum *= 0.1;
                ++exp;
            }

            almost_one = 1.0 - rounder;

            while (fpnum > 0.0 && fpnum < almost_one)
            {
                fpnum *= 10.0;
                --exp;
            }

            fpart = modf(fpnum, &ipart);

            if (cvt == 'g')     // used up one digit for int part...
            {
                --prec;
                powprec /= 10.0;
                rounder = 0.5 / powprec;
            }

            // convert fractional part -- almost same as above
            if (fpart >= rounder || cvt != 'g')
            {
                fpart += rounder;

                if (fpart >= 1.0)
                {
                    fpart -= 1.0;
                    ipart += 1.0;

                    if (ipart >= 10.0)
                    {
                        ++exp;
                        ipart /= 10.0;
                        fpart /= 10.0;
                    }
                }

                ffpart = fpart;

                for (i = 0; i < prec; ++i)
                {
                    ffpart = modf(ffpart * 10.0, &ifpart);
                    *fw++ = (char)('0' + static_cast<int>(ifpart));
                    ++fwidth;
                    xassert( fwidth < WORKSIZE );
                }

                if (cvt == 'g')  // inhibit trailing zeroes if g-fmt
                {
                    char* p;

                    for ( p = fw - 1; p >= fwork.data() && *p == '0'; --p)
                    {
                        *p = 0;
                        --fwidth;
                        xassert( fwidth >= 0 );
                    }
                }
            }


            // convert exponent
            eneg = exp < 0;
            if (eneg) exp = - exp;

            while (exp > 0)
            {
                *--ew = (char)('0' + (exp % 10));
                exp /= 10;
                ++ewidth;
                xassert( ewidth < 16 );
            }

            while (ewidth <= 2)  // ensure at least 2 zeroes
            {
                *--ew = '0';
                ++ewidth;
                xassert( ewidth < 16 );
            }

            *--ew = eneg ? '-' : '+';
            *--ew = 'e';

            ewidth += 2;

            // convert the one-digit integer part
            *--iw = (char)('0' + static_cast<int>(ipart));
            ++iwidth;

        }

        // arrange everything in returned string
        showdot  = cvt != 'g' || fwidth > 0;
        fmtwidth = is_neg + iwidth + showdot + fwidth + ewidth;
        pad = width - fmtwidth;

        if (pad < 0) pad = 0;

        //fmtbase = (char *)malloc(fmtwidth + pad + 1); // NOW IS PAST AS A PARAMETER
        fmt = fmtbase;

        for (i = 0; i < pad; ++i) *fmt++ = ' ';

        if (is_neg) *fmt++ = '-';

        for (i = 0; i < iwidth; ++i) *fmt++ = *iw++;

        if (showdot)
        {
            *fmt++ = '.';
            fw = fwork.data();
            for (i = 0; i < fwidth; ++i) *fmt++ = *fw++;
        }

        for (i = 0; i < ewidth; ++i) *fmt++ = *ew++;

        *fmt = 0;

        return static_cast<int>(fmt - fmtbase);
    }

    //------------------------------------------------------------------------------
    // Convert an unsigned long to ASCII for printf purposes, returning
    // a pointer to the first character of the string representation.
    // Octal numbers can be forced to have a leading zero; hex numbers
    // use the given digits.
    //------------------------------------------------------------------------------
    // val      - Numeric value to be converted
    // endp     - String to be fill in fromt the end
    // base     - base of the number (10, 8, 16)
    // octzero  - flag to add to the oct numbers the 0 in front
    // xdigs    - Hexadecimal string array of number 0,1,2,3,4,5,9,A,or a, ..etc
    //------------------------------------------------------------------------------
    constexpr char          to_digit( int c  ) noexcept { return static_cast<char>( c - '0' ); }
    constexpr bool          is_digit( char c ) noexcept { return to_digit(c) <= 9; }
    template< typename T > 
    constexpr char          to_char ( T n  ) noexcept { return static_cast<char>( n + '0' ); }

    //------------------------------------------------------------------------------
    // Same as above but for s64
    //------------------------------------------------------------------------------
    template< typename T > static
    const char* UtoA( const T aval, string::view<char> Buffer, const int base, const bool octzero, const char* xdigs ) noexcept
    {
        using t_signed      = types::to_int_t<T>;
        using t_unsigned    = types::to_uint_t<T>;

        static_assert( std::is_same< T, t_unsigned >::value, "This function takes only unsigned types");

        t_unsigned  val = aval;
        t_signed    sval;
        int         iCursor = static_cast<int>(Buffer.size());

        // Handle the three cases separately, in the hope of getting
        // better/faster code.

        switch (base)
        {
        case 10:
                if (val < 10)
                {   // many numbers are 1 digit
                    Buffer[--iCursor] = to_char(val);
                    return &Buffer[iCursor];
                }

                // On many machines, unsigned arithmetic is harder than
                // signed arithmetic, so we do at most one unsigned mod and
                // divide; this is sufficient to reduce the range of
                // the incoming value to where signed arithmetic works.

                if (val > ((~(T)0)>>1) )
                {
                    Buffer[--iCursor] = to_char(val % 10);
                    sval = static_cast<t_signed>(val / 10);
                }
                else
                {
                    sval = static_cast<t_signed>(val);
                }

                do
                {
                    Buffer[--iCursor] = to_char(sval % 10);
                    sval /= 10;
                } while (sval != 0);

                break;

        case 8:
                do
                {
                    Buffer[--iCursor] = to_char(val & 7);
                    val >>= 3;
                } while (val);

                if (octzero && Buffer[iCursor] != '0') Buffer[--iCursor] = '0';

                break;

        case 16:
                xassert(xdigs);
                do
                {
                    Buffer[--iCursor] = xdigs[val & 15];
                    val >>= 4;
                } while (val);

                break;

        default:            /* oops */
            return nullptr;
        }

        return &Buffer[iCursor];
    }

    //------------------------------------------------------------------------------
    // Summary:
    //     Write a string formatted output into a buffer.
    // Arguments:
    //	    Buffer	            - Storage location for output. 
    //      MaxChars            - Maximum number of characters to store 
    //	    pFormatStr	        - String containing the formating specification. 
    //      Args                - Pointer to list of arguments OR the actual arguments in case of x_sprintf
    // Returns:
    //	    return the number of characters written, not including the terminating null character.
    // Description:
    //      These functions formats and stores a series of characters and values in buffer. 
    //      Each argument (if any) is converted and output according to the corresponding format 
    //      specification in format. The format consists of ordinary characters and has the same 
    //      form and function as the format argument for x_printf. A null character is appended after 
    //      the last character written. If copying occurs between strings that overlap, the behavior 
    //      is undefined.
    //
    //<P><B>Type Field Characters</B>
    //<TABLE>
    //      Character   Output format
    //      =========   --------------------------------------------------------
    //      %%	        a percent sign
    //      %c	        a character with the given number
    //      %s	        a string
    //      %d	        a signed integer, in decimal
    //      %u	        an unsigned integer, in decimal
    //      %o	        an unsigned integer, in octal
    //      %x	        an unsigned integer, in hexadecimal (lower case)
    //      %e	        a floating-point number, in scientific notation
    //      %f	        a floating-point number, in fixed decimal notation
    //      %g	        a floating-point number, in %e or %f notation
    //      %X	        like %x, but using upper-case letters
    //      %E	        like %e, but using an upper-case "E"
    //      %G	        like %g, but with an upper-case "E" (if applicable)
    //      %p	        a pointer (outputs the pointer value's address in hexadecimal)
    //      %n	        special: *stores* the number of characters output so far into the next variable in the parameter list
    //</TABLE>
    //    
    //   <B>Size flags</B>
    //<TABLE>
    //     Character    Output
    //     =========    ------------------------------------------------------------
    //         h        interpret integer as s16 or u16
    //         l        interpret integer as s32 or u32
    //        q, L      interpret integer as s64 or u64
    //         L        interpret floating point as higher precision adds a few extra floating point numbers
    //</TABLE>
    //
    // Examples:
    //<CODE>
    //      x_printf( "%Lf %Lf %Lf", Vec.X, Vec.Y, Vec.Z );  // Note that we use the higher precision floating point representation
    //      x_printf( "%Ld" (s64)123 );                      // This is a way to print an s64 bit number
    //      x_printf( "%d" 123 );                            // Nothing special here printing an 32bit integer
    //      x_printf( "%f" 123.0 );                          // Nothing special here printing a floating point number
    //      x_printf( "%+010.4f", 123.456 );                 // The printf works like you will expect
    //</CODE>
    //
    // See Also:
    //     x_printf x_printfxy
    //------------------------------------------------------------------------------
    int vsprintf( xcore::string::view<char> inBuffer, const char* pFormatStr, const xcore::arglist::view inArgList ) noexcept
    {
        const char*                 pFormat = pFormatStr;   // format string 
        char                        ch;                     // character from fmt 
        const char*                 cp;                     // handy char pointer (short term usage)
        flags                       Flags{ 0 };             // flags as above
        int                         width;                  // width from format (%8d), or 0
        int                         prec;                   // precision from format (%.3d), or -1
        char                        sign;                   // sign prefix (' ', '+', '-', or \0)
        std::uint32_t               ulval = 0;              // integer arguments %[diouxX]
        std::uint64_t               uqval = 0;              // %q integers 
        int                         base;                   // base for [diouxX] conversion
        int                         dprec;                  // a copy of prec if [diouxX], 0 otherwise
        int                         size;                   // size of converted field or string
        const char*                 xdigs = nullptr;        // digits for [xX] conversion
        std::array<char,WORKSIZE>   buf;                    // space for %c, %[diouxX], %[eEfgG]
        wbuffer                     Buffer{ inBuffer };     // Buffer to write to
        arg_reader                  ArgList{ inArgList };   // Argument reader

        xassert(pFormatStr);

        // Scan the format for conversions (`%' character).
        for (;;)
        {
            // Find the first "interesting symbol"
            for ( cp = pFormat; (ch = *pFormat) != '\0' && ch != '%'; pFormat++){}

            // Write all the caracters before the "interesting symbol"
            {
                const int n = static_cast<int>(pFormat - cp);
                if( n != 0 )
                {
                    Buffer.WriteToBuffer( cp, n );
                }
            }

            // are we done?
            if (ch == '\0') goto done;

            // skip over '%'
            pFormat++;

            // Get ready for formating the info
            Flags.m_Value = 0;
            dprec = 0;
            width = 0;
            prec = -1;
            sign = '\0';

    rflag:
            ch = *pFormat++;

    reswitch:
            switch (ch)
            {
                case ' ':

                    // ``If the space and + flags both appear, the space
                    // flag will be ignored.''
                    //  -- ANSI X3J11
                    if (!sign) sign = ' ';
                    goto rflag;

                case '#':
                    Flags.m_ALT = true;
                    goto rflag;

                case '*':
                     // ``A negative field width argument is taken as a
                     // - flag followed by a positive field width.''
                     // -- ANSI X3J11
                     // They don't exclude field widths read from args.

                    if ( (width = ArgList.get<int>()) >= 0 ) goto rflag;

                    width = -width;

                    /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////
                case '-':
                    Flags.m_LADJUST = true;
                    goto rflag;

                case '+':
                    sign = '+';
                    goto rflag;

                case '.':
                {
                    if ((ch = *pFormat++) == '*')
                    {
                        int n = ArgList.get<int>();  
                        prec = n < 0 ? -1 : n;
                        goto rflag;
                    }

                    int n = 0;

                    while ( is_digit(ch) )
                    {
                        n = 10 * n + to_digit(ch);
                        ch = *pFormat++;
                    }

                    prec = n < 0 ? -1 : n;
                    goto reswitch;
                }
                case '0':
                     // ``Note that 0 is taken as a flag, not as the
                     // beginning of a field width.''
                     // -- ANSI X3J11
                    Flags.m_ZEROPAD = true;
                    goto rflag;

                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                {
                    int n = 0;
                    do
                    {
                        n = 10 * n + to_digit(ch);
                        ch = *pFormat++;
                    } while (is_digit(ch));

                    width = n;
                    goto reswitch;
                }
                case 'L':
                    Flags.m_LONGDBL = true;

                    /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////
                case 'q':
                    Flags.m_QUADINT = true;
                    goto rflag;

                case 'h':
                    Flags.m_SHORTINT = true;
                    goto rflag;

                case 'l':
                    Flags.m_LONGINT = true;
                    goto rflag;

                case 'c':
                    buf[0] = static_cast<char>(ArgList.get<int>());  // used to be get s32
                    cp   = &buf[0];
                    size = 1;
                    sign = '\0';
                    break;

                case 'D':
                    Flags.m_LONGINT = true;
                    /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////

                case 'd':
                case 'i':

                    // Give a break to the user_types and assume it is a quadint
                    Flags.m_QUADINT = true;
                    if( Flags.m_QUADINT )
                    {
                        auto Temp = static_cast<std::int64_t>( ArgList.getGenericInt() ); //ArgList.get<s64>(); //x_va_arg( Args, s64 );

                        if( Temp < 0 )
                        {
                            Temp = -Temp;
                            sign = '-';
                        }
                    
                        uqval = static_cast<std::uint64_t>(Temp);
                    }
                    else
                    {
                        auto Temp = ArgList.get<int>(); //x_va_arg(Args, s32); //GetSignARG( Args, flags );
                        if( Temp < 0 )
                        {
                            Temp = -Temp;
                            sign = '-';
                        }
                    
                        ulval = static_cast<std::uint32_t>(Temp);
                    }
                
                    base = 10;
                    goto number;

                case 'g':
                case 'G':
                        if (prec == 0) prec = 1;
                        /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////
                case 'e':
                case 'E':
                case 'f':
                {
                    double _double;

                    if (sign == '+') 
                    {
                        Buffer.WriteToBuffer( &sign, 1 );
                        width--;
                    }

                    if (prec == -1) prec = DEFPREC;

                    if( Flags.m_LONGDBL ) 
                    {
                        // add additional precision when we say long double
                        prec += 4;
                        _double = ArgList.getGenericF64(); //(f64)x_va_arg(Args, f64); 
                    }
                    else
                    {
                        _double = ArgList.getGenericF64(); //(f64)x_va_arg(Args, f64);
                    }

                    if( Flags.m_LADJUST ) width = -width;

                    // right-adjusting zero padding
                    size = static_cast<int>(details::dtoa( buf, _double, ch, width, prec ));

                    // check whether we have to pad or not
                    if( Flags.m_ZEROPAD )
                    {
                        for( int i=0; buf[i] == ' '; i++ )
                        {
                            buf[i] = '0';
                        }
                    }

                    Buffer.WriteToBuffer( buf.data(), size );

                    if( Flags.m_LADJUST ) Buffer.PadBuffer( -width - size, ' ' );

                    continue;
                }
                        ///////////////////// FLOATING_POINT //////////////////////
                case 'n':

                    if( Flags.m_QUADINT ) 
                    {
                        auto* pPtr = ArgList.get<std::int64_t*>();
                        xassert(pPtr);
                        *pPtr = Buffer.getCursor();
                    }   
                    else if( Flags.m_LONGINT ) 
                    {
                        auto* pPtr = ArgList.get<std::int32_t*>();
                        xassert(pPtr);
                        *pPtr = Buffer.getCursor();
                    }   
                    else if( Flags.m_SHORTINT ) 
                    {
                        auto* pPtr = ArgList.get<std::int16_t*>();
                        xassert(pPtr);
                        *pPtr = Buffer.getCursor();
                    }   
                    else
                    {
                        auto* pPtr = ArgList.get<std::int32_t*>();
                        xassert(pPtr);
                        *pPtr = Buffer.getCursor();
                    }   
                    
                    // no output
                    continue;

                //////////////////////////////// THIS IS NOT ANSI STANDARD
                //case 'O':
                //    flags |= LONGINT;
                //    /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////

                case 'o':
                    // Give a break to the user_types and allow quadints
                    // Flags.m_QUADINT;
                    if (Flags.m_QUADINT) uqval = ArgList.getGenericInt(); //ArgList.get<u64>(); //(u64)x_va_arg(Args, s64); 
                    else                 ulval = ArgList.get<std::uint32_t>(); //x_va_arg(Args, u32); //GetUnSignARG( (xva_list*)&Args, flags );

                    base = 8;
                    goto nosign;

                case 'p':
                     // "The argument shall be a pointer to void.  The
                     // value of the pointer is converted to a sequence
                     // of printable characters, in an implementation-
                     // defined manner." 
                     // -- ANSI X3J11
    #if _XCORE_TARGET_64BITS
                    uqval = reinterpret_cast<std::uint64_t>(ArgList.get<void*>());//(u64)x_va_arg(Args, void *);
                    base  = 16;
                    xdigs = "0123456789abcdef";
                    Flags.m_QUADINT = true;           // | HEXPREFIX; Not prefixes
                    ch    = 'x';
                    if (prec < 0) prec = 8;     // make sure that the precision is at 8

    #else
                    ulval = reinterpret_cast<std::uint32_t>(ArgList.get<void*>());//(u32)x_va_arg(Args, void *);
                    base  = 16;
                    xdigs = "0123456789abcdef";
                    Flags = (Flags & ~QUADINT); // | HEXPREFIX; Not prefixes
                    ch    = 'x';
                    if (prec < 0) prec = 8;     // make sure that the precision is at 8
    #endif
                    goto nosign;

                case 's':

                    cp = const_cast<char*>(ArgList.getGenericString());
                    xassert( cp != nullptr );
                    if (prec >= 0)
                    {
                        // can't use strlen; can only look for the
                        // NULL in the first `prec' characters, and
                        // strlen() will go further.
                        const char* p = reinterpret_cast<const char*>( std::memchr( cp, 0, prec ) );

                        if( p != nullptr )
                        {
                            size = static_cast<int>(p - cp);
                            if (size > prec) size = prec;
                        }
                        else
                            size = prec;
                    }
                    else
                        size = string::Length(cp).m_Value;

                    sign = '\0';
                    break;

                case 'U':
                    Flags.m_LONGINT = true;
                    /////////>>>>>>>>>>>>>>>>>> FALLTHROUGH <<<<<<<<<<<<<<//////////

                case 'u':
                    // Give a break to the user_types and allow quadints
                    Flags.m_QUADINT = true;
                    if (Flags.m_QUADINT) uqval = ArgList.getGenericInt();
                    else                 ulval = ArgList.get<std::uint32_t>();

                    base = 10;
                    goto nosign;

                case 'X': xdigs = "0123456789ABCDEF"; goto hex;
                case 'x': xdigs = "0123456789abcdef"; hex:

                    // Give a break to the user_types and allow quadints
                    Flags.m_QUADINT = true;
                    if (Flags.m_QUADINT) uqval = ArgList.getGenericInt();
                    else                 ulval = ArgList.get<std::uint32_t>();

                    base = 16;

                    // leading 0x/X only if non-zero
                    if (Flags.m_ALT && (Flags.m_QUADINT ? uqval != 0 : ulval != 0))
                        Flags.m_HEXPREFIX = true;

                
        nosign:     // unsigned conversions
                    sign = '\0';

                    // ``... diouXx conversions ... if a precision is
                    // specified, the 0 flag will be ignored.''
                    //  -- ANSI X3J11
        number:     
                    dprec = prec;
                    if( dprec >= 0 ) Flags.m_ZEROPAD = false;
                
                    // ``The result of converting a zero value with an
                    // explicit precision of zero is no characters.''
                    // -- ANSI X3J11
                    if( Flags.m_QUADINT )
                    {
                        if (uqval != 0 || prec != 0)
                        {
                            cp = UtoA<std::uint64_t>(uqval, buf, base, Flags.m_ALT, xdigs );
                        }
                    }
                    else
                    {
                        if (ulval != 0 || prec != 0)
                        {
                            cp = UtoA<std::uint32_t>(ulval, buf, base, Flags.m_ALT, xdigs );
                        }
                    }

                    size = static_cast<std::int32_t>(buf.data() + WORKSIZE - cp);
                    break;

                default:    // "%?" prints ?, unless ? is NUL
                    if (ch == '\0') goto done;

                    // pretend it was %c with argument ch
                    buf[0] = ch;
                    cp     = buf.data();
                    size   = 1;
                    sign   = '\0';

                    break;
                }

                // All reasonable formats wind up here.  At this point, `cp'
                // points to a string which (if not flags&LADJUST) should be
                // padded out to `width' places.  If flags&ZEROPAD, it should
                // first be prefixed by any sign or other prefix; otherwise,
                // it should be blank padded before the prefix is emitted.
                // After any left-hand padding and prefixing, emit zeroes
                // required by a decimal [diouxX] precision, then print the
                // string proper, then emit zeroes required by any leftover
                // floating precision; finally, if LADJUST, pad with blanks.


                // Compute actual size, so we know how much to pad.
                // size excludes decimal prec; realsz includes it.
                const int realsz = 
                [&]{
                    int realsz = dprec > size ? dprec : size;
                    if (sign)                   realsz++;
                    else if (Flags.m_HEXPREFIX) realsz += 2;
                    return realsz; 
                }();

                // right-adjusting blank padding
                if( Flags.m_LADJUST || Flags.m_ZEROPAD ) Buffer.PadBuffer( width - realsz, ' ');

                // prefix
                if( sign )
                {
                    Buffer.WriteToBuffer( &sign, 1);
                }
                else if( Flags.m_HEXPREFIX )
                {
                    std::array<char,2> ox;                     // space for 0x hex-prefix
                    ox[0] = '0';
                    ox[1] = ch;
                    Buffer.WriteToBuffer( ox.data(), 2 );
                }

                // right-adjusting zero padding
                if( Flags.m_LADJUST == false && Flags.m_ZEROPAD )
                    Buffer.PadBuffer( width - realsz, '0');

                // leading zeroes from decimal precision
                Buffer.PadBuffer( dprec - size, '0');

                // writte the integer number
                Buffer.WriteToBuffer( cp, size);

                // left-adjusting padding (always blank)
                if( Flags.m_LADJUST ) Buffer.PadBuffer( width - realsz, ' ');
        }

    done:
        Buffer.WriteToBuffer( "\0", 1 );

        return Buffer.getCursor() - 1;
    }
}
