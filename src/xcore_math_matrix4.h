#ifndef _XCORE_MATH_MATRIX4_H
#define _XCORE_MATH_MATRIX4_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a Column Major 4x4 matrix.
    //     With vectors treated as column matrices.
    //
    //<P>  This class is hardware accelerated and requieres a 16 byte aligment.
    // See Also:
    //     vector3 vector3d xvector2
    //------------------------------------------------------------------------------
    class alignas(floatx4) matrix4
    {
    public:

        constexpr                                   matrix4                 ( void )                                                                            noexcept = default;
        constexpr                                   matrix4                 ( const floatx4& C1, const floatx4& C2, const floatx4& C3, const floatx4& C4 )      noexcept : m_Column1( C1 ), m_Column2( C2 )
                                                                                                                                                                         , m_Column3( C3 ), m_Column4( C4 ) {}

        inline                                      matrix4                 ( const radian3& R )                                                                noexcept;
        inline                                      matrix4                 ( const quaternion& Q )                                                             noexcept;

                                                                            // Note, because +Z is backwards in a conventional RH
                                                                            // system, the third vector here is the "back" vector
                                                                            // of your local frame.
        inline                                      matrix4                 ( const vector3& Right, const vector3& Up,
                                                                              const vector3& Back,  const vector3& Translation )                                noexcept;

        inline                                      matrix4                 ( float m11, float m12, float m13, float m14,
                                                                              float m21, float m22, float m23, float m24,
                                                                              float m31, float m32, float m33, float m34,
                                                                              float m41, float m42, float m43, float m44 )                                      noexcept;

        constexpr                                   matrix4                 ( float Cell[4][4] )                                                                noexcept : m_Cell{  {Cell[0][0],Cell[1][0],Cell[2][0],Cell[3][0]},
                                                                                                                                                                                    {Cell[0][1],Cell[1][1],Cell[2][1],Cell[3][1]},
                                                                                                                                                                                    {Cell[0][2],Cell[1][2],Cell[2][2],Cell[3][2]},
                                                                                                                                                                                    {Cell[0][3],Cell[1][3],Cell[2][3],Cell[3][3]} }{}

        inline                                      matrix4                 ( const vector3d&   Scale,
                                                                              const radian3&    Rotation,
                                                                              const vector3d&   Translation )                                                   noexcept;

        inline                                      matrix4                 ( const vector3&    Scale,
                                                                              const quaternion& Rotation,
                                                                              const vector3&    Translation )                                                   noexcept;

        constexpr static    const matrix4&          Identity                ( void )                                                                            noexcept;
        inline              void                    setIdentity             ( void )                                                                            noexcept;
        inline              void                    setZero                 ( void )                                                                            noexcept;
        inline              bool                    isIdentity              ( void )                                                                    const   noexcept;
        inline              bool                    isValid                 ( void )                                                                    const   noexcept;
        inline              void                    SanityCheck             ( void )                                                                    const   noexcept;
        inline              matrix4&                setup                   ( float m11, float m12, float m13, float m14,
                                                                              float m21, float m22, float m23, float m24,
                                                                              float m31, float m32, float m33, float m34,
                                                                              float m41, float m42, float m43, float m44 )                                      noexcept;
        inline              matrix4&                setTranslation          ( const vector3& Translation )                                                      noexcept;
        inline              matrix4&                setRotation             ( const radian3& R )                                                                noexcept;
        inline              matrix4&                setRotation             ( const quaternion& Q )                                                             noexcept;
        inline              matrix4&                setScale                ( float Scale )                                                                     noexcept;
        inline              matrix4&                setScale                ( const vector3& Scale )                                                            noexcept;
                                                 
        inline              matrix4&                Rotate                  ( const radian3& R )                                                                noexcept;
        inline              matrix4&                RotateX                 ( radian Rx )                                                                       noexcept;
        inline              matrix4&                RotateY                 ( radian Ry )                                                                       noexcept;
        inline              matrix4&                RotateZ                 ( radian Rz )                                                                       noexcept;
        inline              matrix4&                PreRotateX              ( radian Rx )                                                                       noexcept;
        inline              matrix4&                PreRotateY              ( radian Ry )                                                                       noexcept;
        inline              matrix4&                PreRotateZ              ( radian Rz )                                                                       noexcept;
        inline              matrix4&                PreRotate               ( const quaternion& Rotation )                                                      noexcept;
        inline              matrix4&                PreRotate               ( const radian3& Rotation )                                                         noexcept;
        inline              radian3                 getRotationR3           ( void )                                                                    const   noexcept;
        inline              matrix4&                ClearRotation           ( void )                                                                            noexcept;
                                                 
        inline              matrix4&                Translate               ( const vector3& Translation )                                                      noexcept;
        inline              matrix4&                PreTranslate            ( const vector3& Translation )                                                      noexcept;
        inline              vector3                 getTranslation          ( void )                                                                    const   noexcept;
        inline              matrix4&                ClearTranslation        ( void )                                                                            noexcept;
                                                 
        inline              matrix4&                Scale                   ( const vector3d& Scale )                                                           noexcept;
        inline              matrix4&                Scale                   ( float Scale )                                                                     noexcept;
        inline              matrix4&                PreScale                ( float Scale )                                                                     noexcept;
        inline              matrix4&                PreScale                ( const vector3& Scale )                                                            noexcept;
        inline              vector3                 getScale                ( void )                                                                    const   noexcept;
        inline              matrix4&                ClearScale              ( void )                                                                            noexcept;

        inline              matrix4&                FullInvert              ( void )                                                                            noexcept;
        inline              matrix4&                InvertSRT               ( void )                                                                            noexcept;
        inline              matrix4&                InvertRT                ( void )                                                                            noexcept;

        inline              vector3                 RotateVector            ( const vector3& V )                                                        const   noexcept;
        inline              vector3                 InvRotateVector         ( const vector3& V )                                                        const   noexcept;
        inline              vector3                 getForward              ( void )                                                                    const   noexcept;
        constexpr           vector3                 getBack                 ( void )                                                                    const   noexcept;
        constexpr           vector3                 getUp                   ( void )                                                                    const   noexcept;
        inline              vector3                 getDown                 ( void )                                                                    const   noexcept;
        inline              vector3                 getLeft                 ( void )                                                                    const   noexcept;
        constexpr           vector3                 getRight                ( void )                                                                    const   noexcept;

        inline              matrix4&                Transpose               ( void )                                                                            noexcept;
        inline              float                   Determinant             ( void )                                                                    const   noexcept;
        inline              matrix4&                Orthogonalize           ( void )                                                                            noexcept;

        inline              vector3                 Transform3D             ( const vector3& V )                                                        const   noexcept;
        inline              vector4                 Transform3D             ( const vector4& V )                                                        const   noexcept;

        inline              matrix4&                LookAt                  ( const vector3& From, const vector3& To, const vector3& Up )                       noexcept;
        inline              matrix4&                Billboard               ( const vector3& From, const vector3& To, const vector3& Up )                       noexcept;
        inline              matrix4&                OrthographicProjection  ( float Width, float Height, float Near, float Far )                                noexcept;
        inline              matrix4&                OrthographicProjection  ( float Left, float Right, float Bottom, float Top, float Near, float Far )         noexcept;
        inline              matrix4&                PerspectiveProjection   ( float Left, float Right, float Bottom, float Top, float Near, float Far)          noexcept;

        inline              matrix4&                setup                   ( const vector3& Axis, radian Angle )                                               noexcept;
        inline              matrix4&                setup                   ( const vector3& From, const vector3& To, radian Angle )                            noexcept;
        inline              matrix4&                setup                   ( const radian3& Rotation )                                                         noexcept;
        inline              matrix4&                setup                   ( const quaternion& Q )                                                             noexcept;
        inline              matrix4&                setup                   ( const vector3&    Scale,
                                                                              const radian3&    Rotation,
                                                                              const vector3&    Translation )                                                   noexcept;
        inline              matrix4&                setup                   ( const vector3&    Scale,
                                                                              const quaternion& Rotation,
                                                                              const vector3&    Translation )                                                   noexcept;
        inline              vector3                 getEulerZYZ             ( void )                                                                    const   noexcept;
        inline              matrix4&                setupFromZYZ            ( const vector3d& ZYZ )                                                             noexcept;

        inline              matrix4                 getAdjoint              ( void )                                                                    const   noexcept;

        inline              float                   operator ()             ( const std::int32_t i, const std::int32_t j )                              const   noexcept;
        inline              float&                  operator ()             ( const std::int32_t i, const std::int32_t j )                                      noexcept;
        inline              const matrix4&          operator +=             ( const matrix4& M )                                                                noexcept;
        inline              const matrix4&          operator -=             ( const matrix4& M )                                                                noexcept;
        inline              const matrix4&          operator *=             ( const matrix4& M )                                                                noexcept;
        inline friend       vector4                 operator *              ( const matrix4& M,  const vector4&   V  )                                          noexcept;
        inline friend       vector3                 operator *              ( const matrix4& M,  const vector3d&  V  )                                          noexcept;
        inline friend       vector3                 operator *              ( const matrix4& M,  const vector3&   V  )                                          noexcept;
        inline friend       vector2                 operator *              ( const matrix4& M,  const vector2&   V  )                                          noexcept;
        inline friend       matrix4                 operator *              ( const matrix4& M1, const matrix4&  M2 )                                           noexcept;
        inline friend       matrix4                 operator +              ( const matrix4& M1, const matrix4&  M2 )                                           noexcept;
        inline friend       matrix4                 operator -              ( const matrix4& M1, const matrix4&  M2 )                                           noexcept;

        //
        // Missing functions
        //
        /*
        vector3d         Transform           ( const vector3d& V ) const;
        void            Transform           (       vector3d* pDest, 
                                                    const vector3d* pSource, 
                                                    std::int32_t      NVerts ) const;
        void            Transform           (       xvector4* pDest, 
                                                    const xvector4* pSource, 
                                                    std::int32_t      NVerts ) const;
        */

    protected:

        float     acof( std::int32_t r0, std::int32_t r1, std::int32_t r2, std::int32_t c0, std::int32_t c1, std::int32_t c2 ) const noexcept;

        union
        {
            struct
            {
                floatx4   m_Column1;
                floatx4   m_Column2;
                floatx4   m_Column3;
                floatx4   m_Column4;
            };

            // because the memory layout is column-major
            // the first index here is the column, not the row, and vice versa.
            // the (,) and Set operators transpose this for you automatically.
            float     m_Cell[4][4]    {};
        };

        friend class quaternion;
        friend void m4_Multiply( matrix4& dst, const matrix4& src2, const matrix4& src1 ) noexcept;
    };
}
#endif