#ifndef _XCORE_MATH_H
#define _XCORE_MATH_H
#pragma once

namespace xcore::math
{
    //==============================================================================
    //  TRIGONOMETRIC MATH FUNCTIONS
    //==============================================================================
    //
    // Types:
    //
    //  radian - is a typed defined to describe radian values.
    //
    // Functions:
    //  
    //  float             Sin   ( radian Angle   )     - Sine.
    //  float             Cos   ( radian Angle   )     - Cosine.
    //  float             Tan   ( radian Angle   )     - Tangent.
    //  radian   ASin  ( float     Sine    )           - Arc sine.
    //  radian   ACos  ( float     Cosine  )           - Arc cosine.
    //  radian   ATan  ( float     Tangent )           - Arc tangent.
    //  radian   ATan2 ( float y, float x    )           - Standard "atan2" arc tangent where y can equal 0.
    //                
    // Additional functions:
    //
    //  void            SinCos       ( radian Angle, float& sin, float& cos )        - Sine and cosine in one function call.
    //  radian   ModAngle     ( radian Angle )                            - Provide equivalent angle in [    0, 360 ) degrees.
    //  radian   ModAngle2    ( radian Angle )                            - Provide equivalent angle in [ -180, 180 ) degrees.
    //  radian   MinAngleDiff ( radian Angle1, radian Angle2 )     - Provide smallest angle between two given angles.
    //
    // There are some macros also defined
    //
    //  PI                          - It is the constance value of PI. (in floating point)
    //  float DegToRad( T x )         - Converts degrees to radians
    //  float RadToDeg( T x )         - Converts a number from radiants to degrees.
    //  float radian( T x )    - Works the same way than x_DegToRad
    //
    //==============================================================================

    //==============================================================================
    // ANGLE TYPES
    //==============================================================================

    template< typename T > xforceinline T         DegToRad          ( const T   a )  noexcept { return a                     * static_cast<decltype(a)>  ( 0.017453292519943295769); }
                           xforceinline float     DegToRad          ( const int a )  noexcept { return static_cast<float>(a) * static_cast<float>        ( 0.017453292519943295769); }
    template< typename T > xforceinline T         RadToDeg          ( const T   a )  noexcept { return a                     * static_cast<decltype(a)>  (57.29577951308232087685);  }
                           xforceinline float     RadToDeg          ( const int a )  noexcept { return static_cast<float>(a) * static_cast<float>        (57.29577951308232087685);  }

    //------------------------------------------------------------------------------
    // degree
    //------------------------------------------------------------------------------
    struct degree : units::type<degree, float>
    {
        using type::type;
        using type::operator =;
        constexpr explicit                      degree                  ( const unsigned long long    X )       noexcept : type{ static_cast<float>(X) } {} 
        constexpr explicit                      degree                  ( const double X )                      noexcept : type{ static_cast<float>(X) } {} 
    };

    //------------------------------------------------------------------------------
    // radian
    //------------------------------------------------------------------------------
    struct radian : units::type<radian, float>
    {
        using type::type;
        using type::operator =;
        constexpr explicit                      radian                  ( const int X )                 noexcept : type{ static_cast<float>(X) }                      {} 
        constexpr explicit                      radian                  ( const long double X )         noexcept : type{ static_cast<float>(X) }                      {} 
        constexpr                               radian                  ( const degree X )              noexcept : type{ X.m_Value * 0.017453292519943295769f }       {} 
        constexpr degree                        getDegrees              ( void )                const   noexcept { return degree{ m_Value * 57.29577951308232087685f }; }
    };

    struct radian64 : units::type<radian, double>
    {
        using type::type;
        using type::operator =;
        constexpr explicit                      radian64                ( const int X )                 noexcept : type{ static_cast<double>(X) }                     {} 
        constexpr explicit                      radian64                ( const long double X )         noexcept : type{ static_cast<double>(X) }                     {} 
        constexpr                               radian64                ( const degree X )              noexcept : type{ X.m_Value * 0.017453292519943295769 }        {} 
        constexpr degree                        getDegrees              ( void )                const   noexcept { return degree{ m_Value * 57.29577951308232087685 }; }
    };

    //==============================================================================
    // CONSTANTS
    //==============================================================================
    #ifdef PI
        #undef PI
    #endif

    #ifdef M_E
        #undef M_E
    #endif

    constexpr static auto               M_E         { 2.7182818284590452354f  }; // e 
    constexpr static auto               LOG2E       { 1.4426950408889634074f  }; // log 2e 
    constexpr static auto               LOG10E      { 0.43429448190325182765f }; // log 10e 
    constexpr static auto               LN2         { 0.69314718055994530942f }; // log e2 
    constexpr static auto               LN10        { 2.30258509299404568402f }; // log e10 
    constexpr static auto               SQRT2       { 1.41421356237309504880f }; // sqrt(2)
    constexpr static radian             PI          { 3.14159265358979323846f }; // pi
    constexpr static radian64           PI_64       { 3.14159265358979323846  }; // pi 64bits 
    constexpr static radian             PI2         { 6.28318530717958647693f }; // pi * 2
    constexpr static radian             PI_OVER2    { 1.57079632679489661923f }; // pi / 2
    constexpr static radian             PI_OVER4    { 0.78539816339744830962f }; // pi / 4

    constexpr static auto               XFLT_TOL    { 0.001f };

    //==============================================================================
    // BASIC FUNTIONS
    //==============================================================================
    inline                          radian                  ModAngle                ( const radian Angle )                                                      noexcept;
    inline                          radian                  ModAngle2               ( const radian Angle )                                                      noexcept;
    inline                          radian                  MinAngleDiff            ( const radian Angle1, const radian Angle2 )                                noexcept;
    inline                          radian                  LerpAngle               ( const float t, const radian Angle1, const radian Angle2 )                 noexcept;
    inline                          void                    SinCos                  ( const radian Angle, float& S, float& C )                                  noexcept;
    inline                          float                   Sin                     ( const radian x )                                                          noexcept;
    inline                          float                   Cos                     ( const radian x )                                                          noexcept; 
    inline                          double                  SinDouble               ( const radian64 x )                                                        noexcept;
    inline                          double                  CosDouble               ( const radian64 x )                                                        noexcept;
    inline                          radian                  ASin                    ( const float x )                                                           noexcept;
    inline                          radian                  ACos                    ( const float x )                                                           noexcept;
    inline                          float                   Tan                     ( const radian x )                                                          noexcept;
    inline                          double                  TanDouble               ( const radian64 x )                                                        noexcept;
    inline                          radian                  ATan2                   ( const float x, const float y )                                            noexcept; 
    inline                          radian                  ATan                    ( const float x )                                                           noexcept; 

    template< class T > 
    constexpr                       T                       Sqr                     ( const T   x )                                                             noexcept;
    xforceinline                    float                   Sqrt                    ( const float x )                                                           noexcept;	
    xforceinline                    double                  SqrtDouble              ( const double x )                                                          noexcept;
    inline                          float                   InvSqrt                 ( const float x )                                                           noexcept;
    inline                          bool                    SolvedQuadraticRoots    ( float& Root1, float& Root2, const float a, const float b, const float c ) noexcept;
    inline                          float                   Exp                     ( const float x )                                                           noexcept;
    inline                          double                  ExpDouble               ( const double x )                                                          noexcept;
    inline                          float                   Pow                     ( const float a, const float b )                                            noexcept;
    inline                          double                  PowDouble               ( const double a, const double b )                                          noexcept;
    inline                          float                   FMod                    ( const float x, const float y )                                            noexcept;
    inline                          float                   ModF                    ( const float x, float& y )                                                 noexcept;
    inline                          float                   Log                     ( const float x )                                                           noexcept;
    inline                          double                  LogDouble               ( const double x )                                                          noexcept;
    inline                          float                   Log2                    ( const float x )                                                           noexcept; 
    inline                          float                   Log10                   ( const float x )                                                           noexcept;
    inline                          float                   LPR                     ( const float a, const float b )                                            noexcept;
    template< class T > 
    constexpr                       T                       Lerp                    ( const float t, const T  a, const T  b )                                   noexcept;
    template< typename T1, typename T2 > 
    constexpr                       auto                    Min                     ( const T1  a, const T2 b )                                                 noexcept -> decltype( a + b ) { return a < b ? a : b; }
    template< typename T1, typename T2 > 
    constexpr                       auto                    Max                     ( const T1  a, const T2 b )                                                 noexcept -> decltype( a + b ) { return a > b ? a : b; }
    inline                          bool                    FEqual                  ( const float f0, const float f1, const float tol = XFLT_TOL )              noexcept;
    constexpr                       bool                    FLess                   ( const float f0, const float f1, const float tol = XFLT_TOL )              noexcept;
    constexpr                       bool                    FGreater                ( const float f0, const float f1, const float tol = XFLT_TOL )              noexcept; 
    constexpr                       float                   FSel                    ( const float A,  const float B,  const float C )                           noexcept;
    constexpr                       float                   I2F                     ( const float i )                                                           noexcept;
    constexpr                       std::int32_t            F2I                     ( const float f )                                                           noexcept;
    xforceinline                    bool                    isValid                 ( const float x )                                                           noexcept;
    inline                          std::int32_t            LRound                  ( const float x )                                                           noexcept;
    inline                          float                   Round                   ( const float a, const float b )                                            noexcept;
    inline                          float                   Ceil                    ( const float x )                                                           noexcept;
    inline                          float                   Floor                   ( const float x )                                                           noexcept;
    template< class T > 
    constexpr                       bool                    Sign                    ( const T x )                                                               noexcept;
    template< class T > 
    constexpr                       bool                    isInRange               ( const T X, const T Min, const T Max )                                     noexcept;
    template< class T > 
    constexpr                       T                       Range                   ( const T X, const T Min, const T Max )                                     noexcept;
    template< class T > 
    constexpr                       T                       Abs                     ( const T x )                                                               noexcept;

    //==============================================================================
    // PREDEFINITIONS 
    //==============================================================================
    class      vector2;             // Software base 2 floats no alignment
    class      vector3d;            // Software base 3 floats no alignment
    class      vector3;             // Hardware accelerated 4 floats and 16 bytes align
    class      vector4;             // Hardware accelerated with 16 bytes align
    class      matrix4;             // Hardware base matrix Row major with 16 bytes align compatible with d3d
    class      quaternion;          // Hardware accelerated and 16 bytes align
    class      plane;               // Software base 4 floats no alignment 
    class      bbox;                // Software base 6 floats no alignment 
    class      sphere;              // Software base 4 floats no alignment
    class      irect;               // Software base 6 integers no alignment 
}

constexpr xcore::math::degree   operator "" _xdeg    ( long double deg )          noexcept { return xcore::math::degree{ static_cast<float>(                    deg  ) }; }
constexpr xcore::math::degree   operator "" _xdeg    ( unsigned long long deg )   noexcept { return xcore::math::degree{ static_cast<float>(static_cast<double>(deg) ) }; }

#endif