#ifndef _XCORE_MATH_RADIAN3_H
#define _XCORE_MATH_RADIAN3_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 3D rotation using the classical Roll, Pitch, Yaw.
    //     The order of rotation there for is (Z,X,Y)
    //     It is very useful when trying to minimize the memory footprint needed to 
    //     represent a rotation or if you just want to deal with only angles.
    //
    //<P>  The class is not hardware accelerated and works only with floats. There
    //     is not need for special alignments except for the atomic types.
    // See Also:
    //     radian xquaternion matrix4
    //------------------------------------------------------------------------------
    class radian3 
    {
    public:
    
        constexpr                               radian3                 ( void )                                                                        noexcept = default; 
        constexpr                               radian3                 ( const radian Angles )                                                         noexcept : m_Pitch(Angles), m_Yaw(Angles), m_Roll(Angles) {}
        constexpr                               radian3                 ( radian Pitch, radian Yaw, radian Roll )                                       noexcept : m_Pitch(Pitch), m_Yaw(Yaw), m_Roll(Roll) {}
        inline                                  radian3                 ( const quaternion& Q )                                                         noexcept;
        inline                                  radian3                 ( const matrix4& M )                                                            noexcept;

        inline              void                setZero                 ( void )                                                                        noexcept;
        inline              radian3&            setup                   ( radian Pitch, radian Yaw, radian Roll )                                       noexcept;

        inline              radian3&            ModAngle                ( void )                                                                        noexcept;
        inline              radian3&            ModAngle2               ( void )                                                                        noexcept;
        inline              radian3             getMinAngleDiff         ( const radian3& R )                                                    const   noexcept;

        constexpr           radian              Difference              ( const radian3& R )                                                    const   noexcept;
        constexpr           bool                isInrange               ( radian Min, radian Max )                                              const   noexcept;
        xforceinline        bool                isValid                 ( void )                                                                const   noexcept;

        constexpr           bool                operator ==             ( const radian3& R )                                                    const   noexcept;
        inline              const radian3&      operator +=             ( const radian3& R )                                                            noexcept;
        inline              const radian3&      operator -=             ( const radian3& R )                                                            noexcept;
        inline              const radian3&      operator *=             ( float Scalar )                                                                noexcept;
        inline              const radian3&      operator /=             ( float Scalar )                                                                noexcept;

        inline friend       radian3             operator +              ( const radian3& R1, const radian3& R2 )                                        noexcept;
        inline friend       radian3             operator -              ( const radian3& R1, const radian3& R2 )                                        noexcept;
        inline friend       radian3             operator -              ( const radian3& R )                                                            noexcept;
        inline friend       radian3             operator /              ( const radian3& R,        float      S  )                                      noexcept;
        inline friend       radian3             operator *              ( const radian3& R,        float      S  )                                      noexcept;
        inline friend       radian3             operator *              (     float      S,    const radian3& R  )                                      noexcept;

    public:

        radian     m_Pitch;
        radian     m_Yaw;
        radian     m_Roll;
    };
}
#endif