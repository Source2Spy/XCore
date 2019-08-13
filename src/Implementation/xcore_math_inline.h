
namespace xcore::math
{
    //==============================================================================
    // Types
    //==============================================================================
    union float_forge
    {
        constexpr float_forge( void ) = default;
        constexpr float_forge( const std::uint32_t b ) : m_Bits(b) {}
        constexpr float_forge( const float f ) : m_float (f) {}

        std::uint32_t   m_Bits {};
        float           m_float;
    };

    //==============================================================================
    // Functions
    //==============================================================================

    // calculate the sin value of a number
    inline      float               Sin         ( const radian      X )                 noexcept{ return sinf   ( X.m_Value );                      }
    inline      double              SinDouble   ( const radian64    X )                 noexcept { return sin    ( X.m_Value );                      }
    inline      float               Cos         ( const radian      X )                 noexcept { return cosf   ( X.m_Value );                      }
    inline      double              CosDouble   ( const radian64    X )                 noexcept { return cos    ( X.m_Value );                      }
    inline      float               Tan         ( const radian      X )                 noexcept { return tanf   ( X.m_Value );                      }
    inline      double              TanDouble   ( const radian64    X )                 noexcept { return tan    ( X.m_Value );                      }
    inline      radian              ATan        ( const float       X )                 noexcept { return radian   { atanf  ( X )      };           }
    inline      radian64            ATanDouble  ( const double      X )                 noexcept { return radian64 { atan   ( X )      };           }
    inline      radian              ATan2       ( const float X, const float Y )        noexcept { return radian   { atan2f ( X, Y )   };           }
    inline      float               Exp         ( const float       X )                 noexcept { return expf   ( X );                              }
    inline      double              ExpDouble   ( const double      X )                 noexcept { return exp    ( X);                               }
    inline      float               Pow         ( const float X, const float Y )        noexcept { return powf   ( X, Y );                           }
    inline      double              PowDouble   ( const double X, const double Y )      noexcept { return pow    ( X, Y );                           }
    inline      float               FMod        ( const float X, const float Y )        noexcept { return fmodf  ( X, Y );                           }
    inline      double              FModDouble  ( const double X, const double Y )      noexcept { return fmod   ( X, Y );                           }
    inline      float               ModF        ( const float X, float& Y )             noexcept { return modff  ( X, &Y );                          }
    inline      double              ModFDouble  ( const double X, double& Y )           noexcept { return modf   ( X, &Y );                          }
    inline      float               Log         ( const float X )                       noexcept { return logf   ( X );                              }
    inline      double              LogDouble   ( const double X )                      noexcept { return std::log( X );                             }
    inline      float               Log2        ( const float X )                       noexcept { return logf   ( X ) * 1.442695041f;               }
    inline      float               Log10       ( const float X )                       noexcept { return log10f ( X );                              }
    constexpr   float               I2F         ( const std::int32_t X )                noexcept { return static_cast<const float>( X );            }
    constexpr   std::int32_t        F2I         ( const float X )                       noexcept { return static_cast<const std::int32_t>( X );     }

    //------------------------------------------------------------------------------

    constexpr float FSel( const float a, const float b, const float c ) noexcept { return (((a) >= 0) ? (b) : (c)); } 

    //------------------------------------------------------------------------------

    template< class T > constexpr  T                    Abs       ( const T a )                                         noexcept { return ( (a) < T{0} ? -(a) : (a) ); }
    template< class T > constexpr  float                Abs       ( const float a )                                     noexcept { return FSel(a, a, -a); }
    //template< typename T1, typename T2 > constexpr auto Min( const T1 a, const T2 b ) -> decltype( a + b )        { return ( ( a ) < ( b ) ? ( a ) : ( b ) ); }
    //template< typename T1, typename T2 > constexpr auto Max( const T1 a, const T2 b ) -> decltype( a + b )        { return ( ( a ) > ( b ) ? ( a ) : ( b ) ); }
                        constexpr  float                Min       ( const float a, const float b )                      noexcept { return FSel( ((a)-(b)), b, a); }
                        constexpr  float                Max       ( const float a, const float b )                      noexcept { return FSel( ((a)-(b)), a, b); }
    template< class T > constexpr  bool                 Sign      ( const T a )                                         noexcept { return a >= 0; }
    template< class T > constexpr  T                    Sqr       ( const T a )                                         noexcept { return a * a; }
    template< class T > constexpr  bool                 isInrange ( const T   X, const T   Min, const T   Max )         noexcept { return (Min <= X) && (X <= Max); }
                        constexpr  bool                 isInrange ( const float X, const float Min, const float Max )   noexcept { return (Min <= X) && (X <= Max); }  
    template< class T > constexpr  T                    Range     ( const T   X, const T   Min, const T   Max )         noexcept { return ( X < Min ) ? Min : (X > Max) ? Max : X; }
    template< class T > constexpr  T                    Lerp      ( const float t, const T   a,   const T   b )         noexcept { return (T)(a + t*(b-a)); }


    //------------------------------------------------------------------------------
    xforceinline float         Sqrt            ( float x )     noexcept { return sqrtf(x); }
    xforceinline double        SqrtDouble      ( double x )    noexcept { return sqrt(x); }

    //------------------------------------------------------------------------------
    inline 
    void SinCos( const radian Angle, float& S, float& C ) noexcept
    {
        S = Sin( Angle );
        C = Cos( Angle );
    }

    //------------------------------------------------------------------------------
    inline 
    float InvSqrt( const float x ) noexcept
    {
        // Fast invert sqrt technique similar to Jim Blinn's.
        xassert( x > 0.0f );

        const float x2 = x * 0.5f;
        const std::int32_t i  = 0x5f3759df - ( *reinterpret_cast<const std::int32_t*>(&x) >> 1 );
        float       y  = *reinterpret_cast<const float*>(&i);

        y = y * ( 1.5f - ( x2 * y * y ) );
        y = y * ( 1.5f - ( x2 * y * y ) );

        return y;
    }

    //------------------------------------------------------------------------------
    inline 
    radian ACos( const float X ) noexcept
    {
        return radian{ acosf( X ) };
    }

    //------------------------------------------------------------------------------
    inline 
    radian ASin( const float X ) noexcept
    {
        return radian{ asinf( X ) };
    }

    //------------------------------------------------------------------------------
    inline
    radian ModAngle( radian A ) noexcept
    {
        if( ( A > radian{  1440.0_xdeg } ) || 
            ( A < radian{ -1440.0_xdeg } ) )
        {
            A.m_Value = FMod( A.m_Value, (360.0_xdeg).m_Value );
        }

        while( A >= 360.0_xdeg )  A -= 360.0_xdeg;
        while( A <  0.0_xdeg   )  A += 360.0_xdeg;

        xassert( A >= ( radian(0.0_xdeg) - radian{ 0.01f } ) && A < ( radian(360.0_xdeg) + radian{ 0.01f } ) );

        return A;
    }

    //------------------------------------------------------------------------------
    inline
    radian ModAngle2( radian A ) noexcept
    {
        A += radian{ 180.0_xdeg };
        A  = ModAngle( A );
        A -= radian{ 180.0_xdeg };

        return A;
    }

    //------------------------------------------------------------------------------
    inline
    radian LerpAngle( const float t, const radian Angle1, const radian Angle2 ) noexcept
    {
        return ModAngle( Angle1 + ModAngle( Angle2 - Angle1 ) * radian{ t } );
    }

    //------------------------------------------------------------------------------
    inline
    radian MinAngleDiff( const radian a, const radian b ) noexcept
    {
        return ModAngle2( a - b );
    }

    //------------------------------------------------------------------------------
    inline
    float LPR( const float x, const float b ) noexcept
    {
        xassert( b > 0.0f );

        float a = FMod( x, b );
        if( a < 0.0f )  
            a += b;
        return a;
    }

    //------------------------------------------------------------------------------

    xforceinline bool isValid( const float a ) noexcept
    {
        //return ( (*reinterpret_cast<const std::uint32_t*>(&a)) & 0x7F800000 ) != 0x7F800000;
        return std::isfinite(a) && a == a ; //a != INFINITY && a != -INFINITY && a == a; 
    }

    //------------------------------------------------------------------------------
    inline 
    float Floor( const float a ) noexcept
    {
        float_forge f;
        std::uint32_t       s;

        // Watch out for trouble!
        xassert( isValid(a) );

        // Watch out for 0!
        if( a == 0.0f )  return 0.0f;

        // We need the exponent...
        f.m_float = a;
        const std::int32_t e = static_cast<std::int32_t>( ((f.m_Bits & 0x7F800000) >> 23) - 127 );

        // Extremely large numbers have no precision bits available for fractional
        // data, so there is no work to be done.
        if( e > 23 )  return a ;

        // We need the sign bit.
        s = f.m_Bits & 0x80000000;

        // Numbers between 1 and -1 must be handled special.
        if( e < 0 )  return s ? -1.0f : 0.0f ;

        // Mask out all mantissa bits used in storing fractional data.
        f.m_Bits &= (0xffffffff << (23-e));

        // If value is negative, we have to be careful.
        // And otherwise, we're done!
        if( s && (a < f.m_float) ) return f.m_float - 1.0f;

        return f.m_float ;
    }

    //------------------------------------------------------------------------------
    inline 
    float Ceil( const float a ) noexcept
    {
        float_forge f;
        std::uint32_t       s;

        // Watch out for trouble!
        xassert( isValid(a) );

        // Watch out for 0!
        if( a == 0.0f )  return 0.0f;

        // We need the exponent...
        f.m_float = a;
        const std::int32_t e = static_cast<std::int32_t>( ((f.m_Bits & 0x7F800000) >> 23) - 127 );

        // Extremely large numbers have no precision bits available for fractional
        // data, so there is no work to be done.
        if( e > 23 )  return a ;

        // We need the sign bit.
        s = f.m_Bits & 0x80000000;

        // Numbers between 1 and -1 must be handled special.
        if( e < 0 )  return s ? 0.0f : 1.0f ;

        // Mask out all mantissa bits used in storing fractional data.
        f.m_Bits &= (0xffffffff << (23-e));

        // If value is positive, we have to be careful.
        // And otherwise, we're done!
        if( !s && (a > f.m_float) ) return f.m_float + 1.0f;
        return f.m_float;
    }

    //------------------------------------------------------------------------------

    inline
    std::int32_t LRound( const float x ) noexcept
    {
    #if _XCORE_SSE4
        return static_cast<std::int32_t>(_mm_cvtss_si32(_mm_set_ss(x)));
    #else
        return static_cast<std::int32_t>(x + 0.5f);
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    float Round( const float a, const float b ) noexcept
    {
        xassert( !isInrange( b, -0.00001f, 0.00001f ) );

        const float Quotient = a / b;

        if( Quotient < 0.0f ) return Ceil ( Quotient - 0.5f ) * b;

        return Floor( Quotient + 0.5f ) * b;
    }

    //------------------------------------------------------------------------------
    inline
    bool SolvedQuadraticRoots( float& Root1, float& Root2, float a, float b, float c ) noexcept
    {
        if( a == 0.0f ) 
        {
            // It's not a quadratic, only one root: bx + c = 0
            if(b != 0.0f) 
            {
                Root1 = -c / b;
                Root2 = Root1;
                return true;
            }
         
            // There are no roots!
            return false;
        }

        if( b == 0.0f ) 
        {
            // We won't be able to handle this case after rearranging the formula.
            // ax^2 + c = 0
            // x = +-sqrt(-c / a)
            const float neg_c_over_a = -c / a;

            if( neg_c_over_a >= 0.0f ) 
            {
                Root1 = Sqrt( neg_c_over_a );
                Root2 = -Root1;
                return true;

            }
         
            // No real roots.
            return false;
        }

        // If b^2 >> 4ac, then "loss of significance" can occur because two numbers that are
        // almost the same are subtracted from each other.  If b^2 >> 4ac, then x is roughly equal
        // to -b +- |b| / 2a.  If b > 0 then "loss of significance" can happen to the root -b + |b| / 2a,
        // if b < 0 then it happens to the root -b - |b| / 2a.  We want to choose the better root and
        // derive the other root from it.  If you factor out a from ax^2 + bx + c = 0, then a(x^2 + xb/a + c/a) = 0
        // and a(x - r1)(x - r2) = 0 so r1 * r2 = c/a!  If we have one root, we can figure the other one out!!
        // We want to avoid an if statement on the sign of b, so factor it out:
        //		r1,r2 = (b / 2a)(-1 +- sqrt(1 - 4ac / b^2))
        //		A = b / 2a
        //		B = 4ac / b^2 (3 multiplies, 1 divide...can we do better?? YES!)
        //		  = 2c / b * 2a / b = 2c / b * 1 / A = 2c / 2aA * 1 / A 
        //		  = c / aA^2 (2 multiplies, 1 divide)
        //		C = -1 - sqrt(1 - B)
        // The good root is then:
        //		r1 = AC
        //	And the other root is:
        //		r1r2 = c / a
        //		r2 = c / ar1 = c / aAC = (A / A)(c / aAC) = Ac / aA^2C 
        //			= AB / C

        // A: part_1
        float denom = 2.0f * a;
        if(denom == 0.0f) 
        {
            return false;
        }

        float const part_1 = b / denom;

        // B: part_2
        denom = a * part_1 * part_1;
        if(denom == 0.0f) 
        {
            return false;
        }

        const float part_2 = c / denom;

        // C: part_3
        const float one_minus_part_2 = 1.0f - part_2;
        if( one_minus_part_2 < 0.0f ) 
        {
            return false;
        }

        const float part_3 = -1.0f - Sqrt(one_minus_part_2);
        xassert( part_3 < 0.0f );

        // The good root.
        Root1 = part_1 * part_3;

        // The other root.
        Root2 = (part_1 * part_2) / part_3;

        return true;
    }

    //------------------------------------------------------------------------------
    inline 
    bool FEqual( const float f0, const float f1, const float tol ) noexcept
    {
        const float f = f0-f1;
        return ((f>(-tol)) && (f<tol));
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool FLess( const float f0, const float f1, const float tol ) noexcept
    {
        return ((f0-f1)<tol);
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool FGreater( const float f0, const float f1, const std::int32_t tol ) noexcept
    {
        return ((f0-f1)>tol);
    }
}