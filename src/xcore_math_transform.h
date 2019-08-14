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


        inline void Blend( const transform3& From, const float T, const transform3& To )
        {
            m_Scale         = From.m_Scale + T*( To.m_Scale - From.m_Scale );
            m_Rotation      = From.m_Rotation.BlendAccurate( T, To.m_Rotation );
            m_Position      = From.m_Position + T*( To.m_Position - From.m_Position );
        }

        inline void Blend( const float T, const transform3& To )
        {
            m_Scale         += T*( To.m_Scale - m_Scale );
            m_Rotation       = m_Rotation.BlendAccurate( T, To.m_Rotation );
            m_Position      += T*( To.m_Position - m_Position );
        }

        inline void setIdentity( void )
        {
            m_Position.setZero();
            m_Scale.setup( 1.0f, 1.0f, 1.0f );
            m_Rotation.setIdentity();
        }

        inline void getMatrix( matrix4& Matrix )
        {
            Matrix.setup( m_Scale, m_Rotation, m_Position );
        }

        vector3        m_Scale         {};
        quaternion     m_Rotation      {};
        vector3        m_Position      {};
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