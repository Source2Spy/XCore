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
    //  units::radian - is a typed defined to describe units::radian values.
    //
    // Functions:
    //  
    //  float             Sin   ( units::radian Angle   )     - Sine.
    //  float             Cos   ( units::radian Angle   )     - Cosine.
    //  float             Tan   ( units::radian Angle   )     - Tangent.
    //  units::radian   ASin  ( float     Sine    )           - Arc sine.
    //  units::radian   ACos  ( float     Cosine  )           - Arc cosine.
    //  units::radian   ATan  ( float     Tangent )           - Arc tangent.
    //  units::radian   ATan2 ( float y, float x    )           - Standard "atan2" arc tangent where y can equal 0.
    //                
    // Additional functions:
    //
    //  void            SinCos       ( units::radian Angle, float& sin, float& cos )        - Sine and cosine in one function call.
    //  units::radian   ModAngle     ( units::radian Angle )                            - Provide equivalent angle in [    0, 360 ) degrees.
    //  units::radian   ModAngle2    ( units::radian Angle )                            - Provide equivalent angle in [ -180, 180 ) degrees.
    //  units::radian   MinAngleDiff ( units::radian Angle1, units::radian Angle2 )     - Provide smallest angle between two given angles.
    //
    // There are some macros also defined
    //
    //  PI                          - It is the constance value of PI. (in floating point)
    //  float DegToRad( T x )         - Converts degrees to units::radians
    //  float RadToDeg( T x )         - Converts a number from units::radiants to degrees.
    //  float units::radian( T x )    - Works the same way than x_DegToRad
    //
    //==============================================================================

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
    constexpr static units::radian      PI          { 3.14159265358979323846f }; // pi
    constexpr static units::radian64    PI_64       { 3.14159265358979323846  }; // pi 64bits 
    constexpr static units::radian      PI2         { 6.28318530717958647693f }; // pi * 2
    constexpr static units::radian      PI_OVER2    { 1.57079632679489661923f }; // pi / 2
    constexpr static units::radian      PI_OVER4    { 0.78539816339744830962f }; // pi / 4

    constexpr static auto               XFLT_TOL    { 0.001f };

    //==============================================================================
    // ANGLE TYPES
    //==============================================================================

    template< typename T > xforceinline T         DegToRad          ( const T   a )  noexcept { return a                     * static_cast<decltype(a)>  ( 0.017453292519943295769); }
                           xforceinline float     DegToRad          ( const int a )  noexcept { return static_cast<float>(a) * static_cast<float>        ( 0.017453292519943295769); }
    template< typename T > xforceinline T         RadToDeg          ( const T   a )  noexcept { return a                     * static_cast<decltype(a)>  (57.29577951308232087685);  }
                           xforceinline float     RadToDeg          ( const int a )  noexcept { return static_cast<float>(a) * static_cast<float>        (57.29577951308232087685);  }

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

    //==============================================================================
    // BASIC FUNTIONS
    //==============================================================================
    inline                          units::radian           ModAngle                ( const units::radian Angle )                                               noexcept;
    inline                          units::radian           ModAngle2               ( const units::radian Angle )                                               noexcept;
    inline                          units::radian           MinAngleDiff            ( const units::radian Angle1, const units::radian Angle2 )                  noexcept;
    inline                          units::radian           LerpAngle               ( const float t, const units::radian Angle1, const units::radian Angle2 )   noexcept;
    inline                          void                    SinCos                  ( const units::radian Angle, float& S, float& C )                           noexcept;
    inline                          float                   Sin                     ( const units::radian x )                                                   noexcept;
    inline                          float                   Cos                     ( const units::radian x )                                                   noexcept; 
    inline                          double                  Sindouble               ( const units::radian64 x )                                                 noexcept;
    inline                          double                  Cosdouble               ( const units::radian64 x )                                                 noexcept;
    inline                          units::radian           ASin                    ( const float x )                                                           noexcept;
    inline                          units::radian           ACos                    ( const float x )                                                           noexcept;
    inline                          float                   Tan                     ( const units::radian x )                                                   noexcept;
    inline                          double                  Tandouble               ( const units::radian64 x )                                                 noexcept;
    inline                          units::radian           ATan2                   ( const float x, const float y )                                            noexcept; 
    inline                          units::radian           ATan                    ( const float x )                                                           noexcept; 

    template< class T > 
    constexpr                       T                       Sqr                     ( const T   x )                                                             noexcept;
    inline                          float                   Sqrt                    ( const float x )                                                           noexcept;	
    inline                          double                  Sqrtdouble              ( const double x )                                                          noexcept;
    inline                          float                   InvSqrt                 ( const float x );
    inline                          bool                    SolvedQuadraticRoots    ( float& Root1, float& Root2, const float a, const float b, const float c ) noexcept;
    inline                          float                   Exp                     ( const float x )                                                           noexcept;
    inline                          double                  Expdouble               ( const double x )                                                          noexcept;
    inline                          float                   Pow                     ( const float a, const float b )                                            noexcept;
    inline                          double                  Powdouble               ( const double a, const double b )                                          noexcept;
    inline                          float                   FMod                    ( const float x, const float y )                                            noexcept;
    inline                          float                   ModF                    ( const float x, float& y )                                                 noexcept;
    inline                          float                   Log                     ( const float x )                                                           noexcept;
    inline                          double                  Logdouble               ( const double x )                                                          noexcept;
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
    constexpr                       bool                    isInrange               ( const T X, const T Min, const T Max )                                     noexcept;
    template< class T > 
    constexpr                       T                       Range                   ( const T X, const T Min, const T Max )                                     noexcept;
    template< class T > 
    constexpr                       T                       Abs                     ( const T x )                                                               noexcept;
}
#endif