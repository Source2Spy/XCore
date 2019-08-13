#ifndef _XCORE_MATH_QUATERNION_H
#define _XCORE_MATH_QUATERNION_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a quaternion. A quaternion is a rotation described as
    //     a 4D vector. Where the vector loosely represents an Axis and an Angle.
    //
    //<P>  This class is hardware accelerated and requieres a 16 byte aligment.
    // See Also:
    //     vector3 vector3d radian3 radian matrix4
    //------------------------------------------------------------------------------
    class alignas(floatx4) quaternion 
    {
    public:

        constexpr                                   quaternion             ( void )                                                                 noexcept = default;
        constexpr           explicit                quaternion             ( const float X, const float Y, const float Z, const float W )           noexcept : m_XYZW{ X,Y,Z,W } {}
        inline              explicit                quaternion             ( const floatx4& Register )                                              noexcept : m_XYZW{ Register } {}
        inline              explicit                quaternion             ( const radian3&    R )                                                  noexcept;
        inline              explicit                quaternion             ( const vector3& Axis, radian Angle )                                    noexcept;
        inline              explicit                quaternion             ( const matrix4&    M )                                                  noexcept;

        inline              bool                    isValid                ( void )                                                         const   noexcept;
        inline              void                    setIdentity            ( void )                                                                 noexcept;
        inline              void                    setZero                ( void )                                                                 noexcept;

        inline              quaternion&             setup                  ( float X, float Y, float Z, float W )                                   noexcept;
        inline              quaternion&             setup                  ( const radian3& R )                                                     noexcept;
        inline              quaternion&             setup                  ( const vector3& Axis, radian Angle )                                    noexcept;
        inline              quaternion&             setup                  ( const vector3& StartDir, const vector3& EndDir )                       noexcept;
        inline              quaternion&             setup                  ( const matrix4& M )                                                     noexcept;

        inline              quaternion&             setRotationX           ( radian Rx )                                                            noexcept;
        inline              quaternion&             setRotationY           ( radian Ry )                                                            noexcept;
        inline              quaternion&             setRotationZ           ( radian Rz )                                                            noexcept;

        inline              float                   Dot                    ( const quaternion& Q )                                          const   noexcept;
        inline              quaternion&             Normalize              ( void )                                                                 noexcept;
        inline              quaternion&             NormalizeSafe          ( void )                                                                 noexcept;

        inline              quaternion&             Conjugate              ( void )                                                                 noexcept;
        inline              quaternion&             Invert                 ( void )                                                                 noexcept;
        inline              float                   Length                 ( void )                                                         const   noexcept;
        inline              float                   LengthSquared          ( void )                                                         const   noexcept;
        inline              vector3                 getAxis                ( void )                                                         const   noexcept;
        inline              radian                  getAngle               ( void )                                                         const   noexcept;
        inline              radian3                 getRotation            ( void )                                                         const   noexcept;
        inline              vector3                 getLookDirection       ( void )                                                         const   noexcept;
        inline              quaternion              getDeltaRotation       ( const quaternion& B )                                          const   noexcept { return quaternion(*this).Invert() * B; }

        inline              quaternion&             RotateX                ( radian Rx )                                                            noexcept;
        inline              quaternion&             RotateY                ( radian Ry )                                                            noexcept;
        inline              quaternion&             RotateZ                ( radian Rz )                                                            noexcept;
        inline              quaternion&             PreRotateX             ( radian Rx )                                                            noexcept;
        inline              quaternion&             PreRotateY             ( radian Ry )                                                            noexcept;
        inline              quaternion&             PreRotateZ             ( radian Rz )                                                            noexcept;

        inline              quaternion              BlendFast              ( float T, const quaternion& End )                               const   noexcept;
        inline              quaternion              BlendAccurate          ( float T, const quaternion& End )                               const   noexcept;

        inline              quaternion&             Ln                     ( void )                                                                 noexcept;
        inline              quaternion&             Exp                    ( void )                                                                 noexcept;

        constexpr           bool                    operator ==            ( const quaternion& Q )                                          const   noexcept;
    
        inline              const quaternion&       operator /=            ( float Scalar )                                                         noexcept;
        inline              const quaternion&       operator *=            ( float Scalar )                                                         noexcept;
        inline              const quaternion&       operator *=            ( const quaternion& Q )                                                  noexcept;
        inline              const quaternion&       operator -=            ( const quaternion& Q )                                                  noexcept;
        inline              const quaternion&       operator +=            ( const quaternion& Q )                                                  noexcept;

        inline friend       quaternion              operator *             ( const quaternion& Q1, const quaternion& Q2 )                           noexcept;
        inline friend       quaternion              operator *             ( float Scalar, const quaternion& Q )                                    noexcept;
        inline friend       quaternion              operator *             ( const quaternion& Q, float Scalar )                                    noexcept;
        inline friend       vector3                 operator *             ( const quaternion& A, const vector3d& V )                               noexcept;
        inline friend       quaternion              operator -             ( const quaternion& Q )                                                  noexcept;
        inline friend       quaternion              operator -             ( const quaternion& Q1, const quaternion& Q2 )                           noexcept;
        inline friend       quaternion              operator +             ( const quaternion& Q1, const quaternion& Q2 )                           noexcept;

    public:


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
