#ifndef _XCORE_MATH_VECTOR4_H
#define _XCORE_MATH_VECTOR4_H
#pragma once

namespace xcore::math
{
//------------------------------------------------------------------------------
// Description:
//     This class represents a 4D vector. 
//
//<P>  This class is hardware accelerated and requieres a 16 byte aligment.
// See Also:
//     xvector3 xvector3d xvector2
//------------------------------------------------------------------------------
class alignas(floatx4) vector4 
{
public:

    constexpr                               vector4                         ( void )                                                                noexcept = default;
    constexpr                               vector4                         ( const float x )                                                       noexcept : m_XYZW { x,x,x,1 } {}
    constexpr                               vector4                         ( const float X, const float Y, const float Z, const float W=1 )        noexcept : m_XYZW { X,Y,Z,W } {}
    constexpr                               vector4                         ( const vector3& V, const float W=1 )                                   noexcept : m_XYZW { V.m_X,V.m_Y,V.m_Z, W } {}
    constexpr explicit                      vector4                         ( const floatx4& Register )                                             noexcept : m_XYZW( Register ) {}

    inline              vector4&            setup                           ( const float X, const float Y, const float Z, const float W=0 )        noexcept;

    inline              void                setZero                         ( void )                                                                noexcept;
    inline              void                setIdentity                     ( void )                                                                noexcept;
    inline              float               getLength                       ( void )                                                        const   noexcept;
    constexpr           float               getLengthSquared                ( void )                                                        const   noexcept;
    constexpr           float               Dot                             ( const vector4& V )                                            const   noexcept;
    xforceinline        bool                isValid                         ( void )                                                        const   noexcept;

    inline              const vector4&      operator +=                     ( const vector4& V )                                                    noexcept;
    inline              const vector4&      operator -=                     ( const vector4& V )                                                    noexcept;
    inline              const vector4&      operator *=                     ( const vector4& V )                                                    noexcept;
    inline              const vector4&      operator *=                     ( const float Scalar )                                                  noexcept;
    inline              const vector4&      operator /=                     ( const float Scalar )                                                  noexcept;

    inline              vector4&            Normalize                       ( void )                                                                noexcept;
    inline              vector4&            NormalizeSafe                   ( void )                                                                noexcept;
    inline              vector4&            Abs                             ( void )                                                                noexcept;
    xforceinline        bool                isInRange                       ( const float Min, const float Max )                            const   noexcept;
    constexpr           vector4             getMax                          ( const vector4& V )                                            const   noexcept;
    constexpr           vector4             getMin                          ( const vector4& V )                                            const   noexcept;
    inline              vector4&            Homogeneous                     ( void )                                                                noexcept;
    constexpr           vector4             OneOver                         ( void )                                                        const   noexcept;
    constexpr           bool                operator ==                     ( const vector4& V )                                            const   noexcept;

    constexpr friend    vector4             operator +                      ( const vector4& V1, const vector4& V2 )                                noexcept;
    constexpr friend    vector4             operator -                      ( const vector4& V1, const vector4& V2 )                                noexcept;
    constexpr friend    vector4             operator *                      ( const vector4& V1, const vector4& V2 )                                noexcept;
    constexpr friend    vector4             operator -                      ( const vector4& V )                                                    noexcept;
    constexpr friend    vector4             operator /                      ( const vector4& V,        float     S  )                               noexcept;
    constexpr friend    vector4             operator *                      ( const vector4& V,        float     S  )                               noexcept;
    constexpr friend    vector4             operator *                      (    float       S,  const vector4&  V  )                               noexcept;
    constexpr friend    vector4             operator +                      ( const vector4& V,        float     S  )                               noexcept;
    constexpr friend    vector4             operator +                      (    float       S,  const vector4&  V  )                               noexcept;
    constexpr friend    vector4             operator -                      ( const vector4& V,        float     S  )                               noexcept;
    constexpr friend    vector4             operator -                      (    float       S,  const vector4&  V  )                               noexcept;

public:

    union
    {
        floatx4       m_XYZW  {};
        struct
        {
            float     m_X;
            float     m_Y;
            float     m_Z;
            float     m_W;
        };
    };
};


}
#endif