
namespace xcore::math
{
    //------------------------------------------------------------------------------
    inline
    void radian3::setZero( void ) noexcept
    {
        m_Pitch = m_Yaw = m_Roll = radian{ 0.0_xdeg };
    }   

    //------------------------------------------------------------------------------
    inline
    radian3::radian3( const quaternion& Q ) noexcept
    {
        *this = Q.getRotation();
    }

    //------------------------------------------------------------------------------
    inline
    radian3::radian3( const matrix4& M ) noexcept
    {
        *this = M.getRotationR3();
    }

    //------------------------------------------------------------------------------
    inline
    radian3& radian3::setup( radian Pitch, radian Yaw, radian Roll ) noexcept
    {
        m_Pitch = Pitch;
        m_Yaw   = Yaw;
        m_Roll  = Roll;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    radian3& radian3::ModAngle( void ) noexcept
    {
        m_Pitch = math::ModAngle( m_Pitch );
        m_Yaw   = math::ModAngle( m_Yaw );
        m_Roll  = math::ModAngle( m_Roll );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    radian3& radian3::ModAngle2( void ) noexcept
    {
        m_Pitch = math::ModAngle2( m_Pitch );
        m_Yaw   = math::ModAngle2( m_Yaw );
        m_Roll  = math::ModAngle2( m_Roll );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    radian3 radian3::getMinAngleDiff( const radian3& R ) const noexcept
    {
        return radian3( math::MinAngleDiff( m_Pitch, R.m_Pitch ),
                        math::MinAngleDiff( m_Yaw,   R.m_Yaw   ),
                        math::MinAngleDiff( m_Roll,  R.m_Roll  ) );
    }

    //------------------------------------------------------------------------------
    inline
    constexpr radian radian3::Difference( const radian3& R ) const noexcept
    {
        return ( m_Pitch - R.m_Pitch ) * ( m_Pitch - R.m_Pitch ) +
               ( m_Yaw   - R.m_Yaw   ) * ( m_Yaw   - R.m_Yaw   ) +
               ( m_Roll  - R.m_Roll  ) * ( m_Roll  - R.m_Roll  );
    }

    //------------------------------------------------------------------------------
    inline
    constexpr bool radian3::isInrange( radian Min, radian Max ) const noexcept
    {
        return ( m_Pitch >= Min ) && ( m_Pitch <= Max ) &&
               ( m_Yaw   >= Min ) && ( m_Yaw   <= Max ) &&
               ( m_Roll  >= Min ) && ( m_Roll  <= Max );
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool radian3::isValid( void ) const noexcept
    {
        return xcore::math::isValid( m_Pitch.m_Value ) 
            && xcore::math::isValid( m_Yaw.m_Value ) 
            && xcore::math::isValid( m_Roll.m_Value );
    }

    //------------------------------------------------------------------------------
    inline
    const radian3& radian3::operator += ( const radian3& R ) noexcept
    {
        m_Pitch += R.m_Pitch; 
        m_Yaw   += R.m_Yaw; 
        m_Roll  += R.m_Roll;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    const radian3& radian3::operator -= ( const radian3& R ) noexcept
    {
        m_Pitch -= R.m_Pitch; 
        m_Yaw   -= R.m_Yaw; 
        m_Roll  -= R.m_Roll;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    const radian3& radian3::operator *= ( float Scalar  ) noexcept
    {
        m_Pitch.m_Value *= Scalar; 
        m_Yaw.m_Value   *= Scalar; 
        m_Roll.m_Value  *= Scalar;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    const radian3& radian3::operator /= ( float Scalar  ) noexcept
    {
        Scalar           = 1.0f/Scalar;
        m_Pitch.m_Value *= Scalar; 
        m_Yaw.m_Value   *= Scalar; 
        m_Roll.m_Value  *= Scalar;

        return *this;
    }

    //------------------------------------------------------------------------------

    constexpr bool radian3::operator == ( const radian3& R ) const noexcept
    {
        return  !( ( math::Abs( R.m_Pitch - m_Pitch ) > radian{ XFLT_TOL } ) ||
                   ( math::Abs( R.m_Yaw   - m_Yaw   ) > radian{ XFLT_TOL } ) ||
                   ( math::Abs( R.m_Roll  - m_Roll  ) > radian{ XFLT_TOL } ) );
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator + ( const radian3& R1, const radian3& R2 ) noexcept
    {
        return radian3( R1.m_Pitch + R2.m_Pitch,
                        R1.m_Yaw   + R2.m_Yaw,
                        R1.m_Roll  + R2.m_Roll );
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator - ( const radian3& R1, const radian3& R2 ) noexcept
    {
        return radian3( R1.m_Pitch - R2.m_Pitch,
                        R1.m_Yaw   - R2.m_Yaw,
                        R1.m_Roll  - R2.m_Roll );
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator - ( const radian3& R ) noexcept
    {
        return radian3(  -R.m_Pitch,
                         -R.m_Yaw,
                         -R.m_Roll );
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator * ( const radian3& R, float S ) noexcept
    {
        return radian3( R.m_Pitch * radian{ S },
                        R.m_Yaw   * radian{ S },
                        R.m_Roll  * radian{ S } );
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator / ( const radian3& R, float S ) noexcept
    {
        S = 1.0f/S;
        return R * S;
    }

    //------------------------------------------------------------------------------
    inline
    radian3 operator * ( float S, const radian3& R ) noexcept
    {
        return radian3( R.m_Pitch * radian{ S },
                        R.m_Yaw   * radian{ S },
                        R.m_Roll  * radian{ S } );
    }
}
