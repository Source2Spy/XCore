#ifndef _XCORE_MATH_TRANSFORM_H
#define _XCORE_MATH_TRANSFORM_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    // See Also:
    //     xvector3 xvector3d xmatrix4 xquaternion
    //------------------------------------------------------------------------------
    class transform3 
    {
    public:


        inline static transform3 Blend( const transform3& From, const float T, const transform3& To ) noexcept
        {
            transform3 B;
            B.m_Scale         = From.m_Scale + T*( To.m_Scale - From.m_Scale );
            B.m_Rotate        = From.m_Rotate.BlendAccurate( T, To.m_Rotate );
            B.m_Translate     = From.m_Translate + T*( To.m_Translate - From.m_Translate );
            return B;
        }

        inline transform3& Blend( const float T, const transform3& To ) noexcept
        {
            m_Scale         += T*( To.m_Scale - m_Scale );
            m_Rotate         = m_Rotate.BlendAccurate( T, To.m_Rotate );
            m_Translate     += T*( To.m_Translate - m_Translate );
            return *this;
        }

        inline void setIdentity( void ) noexcept
        {
            m_Translate.setZero();
            m_Scale.setup( 1.0f, 1.0f, 1.0f );
            m_Rotate.identity();
        }

        inline matrix4 getMatrix( void ) const noexcept 
        {
            matrix4 Matrix;
            Matrix.setup( m_Scale, m_Rotate, m_Translate );
            return Matrix;
        }

        vector3        m_Scale         {};
        quaternion     m_Rotate        {};
        vector3        m_Translate     {};
    };

    //------------------------------------------------------------------------------
    // Description:
    // See Also:
    //     vector2 
    //------------------------------------------------------------------------------
    class transform2
    {
    public:

        inline void setIdentity( void )
        {
            m_Position.setZero();
            m_Scale.setup( 1.0f, 1.0f );
            m_Rotation = 0_xdeg;
        }

        vector2        m_Scale         {};
        radian         m_Rotation      {};
        vector2        m_Position      {};
    };
}

#endif