#ifndef _XCORE_MATH_VECTOR2_H
#define _XCORE_MATH_VECTOR2_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 2D vector. 
    //
    //     The class is not hardware accelerated and works only with floats. There
    //     is not need for special alignments except for the atomic types.
    // See Also:
    //     vector3 vector4
    //------------------------------------------------------------------------------
    class vector2 
    {
    public:

        constexpr                               vector2                         ( void )                                            noexcept = default;
        constexpr                               vector2                         ( float X, float Y )                                noexcept : m_X(X), m_Y(Y) {}
        constexpr                               vector2                         ( float n )                                         noexcept : m_X(n), m_Y(n) {}

        inline              vector2&            setup                           ( float X, float Y )                                noexcept;
        inline              void                setZero                         ( void )                                            noexcept ;
                                            
        constexpr           float               Dot                             ( const vector2& V )                        const   noexcept;
        constexpr           vector2             getLerp                         ( float t, const vector2& V1 )              const   noexcept;
        inline              float               getLength                       ( void )                                    const   noexcept;
        constexpr           float               getLengthSquared                ( void )                                    const   noexcept;
        inline              radian              getAngle                        ( void )                                    const   noexcept;
        inline              radian              getAngleBetween                 ( const vector2& V )                        const   noexcept;
        inline              float               getDistance                     ( const vector2& V )                        const   noexcept;
        constexpr           bool                isInRange                       ( float Min, float Max )                    const   noexcept;
        xforceinline        bool                isValid                         ( void )                                    const   noexcept; 
        inline              vector2&            Rotate                          ( radian Angle )                             noexcept; 
        inline              vector2&            Normalize                       ( void )                                            noexcept;
        inline              vector2&            NormalizeSafe                   ( void )                                            noexcept;
        constexpr           vector2             getMin                          ( const vector2& V )                        const   noexcept;
        constexpr           vector2             getMax                          ( const vector2& V )                        const   noexcept;
        constexpr           vector2             LimitLength                     ( float MaxLength )                         const   noexcept;
        xforceinline        vector2&            setLength                       ( float Length )                                    noexcept;

        constexpr           float               getWhichSideOfLine              ( const vector2& V0, const vector2& V1 )    const   noexcept;
        inline              vector2             getClosestPointInLine           ( const vector2& V0, const vector2& V1 )    const   noexcept;
        inline              vector2             getClosestPointInLineSegment    ( const vector2& V0, const vector2& V1 )    const   noexcept;

        constexpr           bool                operator ==                     ( const vector2& V )                        const   noexcept;
        inline              const vector2&      operator +=                     ( const vector2& V )                                noexcept;
        inline              const vector2&      operator -=                     ( const vector2& V )                                noexcept;
        inline              const vector2&      operator *=                     ( const vector2& V )                                noexcept;
        inline              const vector2&      operator *=                     ( float Scalar )                                    noexcept;
        inline              const vector2&      operator /=                     ( float Scalar )                                    noexcept;  

        friend constexpr    vector2             operator +                      ( const vector2& V1, const vector2& V2 )            noexcept;
        friend constexpr    vector2             operator -                      ( const vector2& V1 )                               noexcept;
        friend constexpr    vector2             operator -                      ( const vector2& V1, const vector2& V2 )            noexcept;
        friend constexpr    vector2             operator /                      ( const vector2& V,        float    S  )            noexcept;
        friend constexpr    vector2             operator *                      ( const vector2& V,        float    S  )            noexcept;
        friend constexpr    vector2             operator *                      ( const vector2& V1, const vector2& V2 )            noexcept;
        friend constexpr    vector2             operator *                      (       float    S,  const vector2& V  )            noexcept;

        static constexpr    vector2             Reflect                         ( const vector2& Normal, const vector2& Vector )    noexcept;

    public:

        float       m_X;
        float       m_Y;
    };
}
#endif