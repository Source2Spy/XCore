

namespace xcore::math
{
    //==============================================================================
    // FUNCTIONS
    //==============================================================================
    constexpr static quaternion q_Identity{0.0f, 0.0f, 0.0f, 1.0f};

    //------------------------------------------------------------------------------
    inline
    quaternion::quaternion( const radian3& R ) noexcept
    {
        xassert( bits::isAlign( this, 16 ) ); 
        setup( R );
    }

    //------------------------------------------------------------------------------
    inline
    quaternion::quaternion( const vector3& Axis, radian Angle ) noexcept
    {
        xassert( bits::isAlign( this, 16 ) ); 
        setup( Axis, Angle );
    }

    //------------------------------------------------------------------------------
    inline
    quaternion::quaternion( const matrix4& M ) noexcept
    {
        setup( M );
    }

    //------------------------------------------------------------------------------
    inline
    void quaternion::setIdentity( void ) noexcept
    {
        *this = q_Identity;
    }

    //------------------------------------------------------------------------------
    inline
    void quaternion::setZero( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_setzero_ps();
    #else
        m_X = 0.0f;
        m_Y = 0.0f;
        m_Z = 0.0f;
        m_W = 0.0f;
    #endif
    }

    //------------------------------------------------------------------------------
    inline static
    void q_Multiply( quaternion& Dest, const quaternion& Q1, const quaternion& Q2 ) noexcept
    {
    #ifdef _XCORE_SSE4

        const floatx4 w = _mm_mul_ps( Q1.m_XYZW, Q2.m_XYZW );

        Dest = quaternion
        {
            _mm_sub_ps( _mm_add_ps( _mm_add_ps(
            _mm_mul_ps( Q2.m_XYZW, _mm_set1_ps( Q1.m_W ) ),
            _mm_mul_ps( Q1.m_XYZW, _mm_set1_ps( Q2.m_W ) )),
            _mm_mul_ps( _mm_shuffle_ps( Q1.m_XYZW, Q1.m_XYZW, _MM_SHUFFLE(3,0,2,1) ), _mm_shuffle_ps( Q2.m_XYZW, Q2.m_XYZW, _MM_SHUFFLE(3,1,0,2) ))),
            _mm_mul_ps( _mm_shuffle_ps( Q1.m_XYZW, Q1.m_XYZW, _MM_SHUFFLE(3,1,0,2) ), _mm_shuffle_ps( Q2.m_XYZW, Q2.m_XYZW, _MM_SHUFFLE(3,0,2,1) )) )
        };
        Dest.m_W = ((float*)&w)[3] - ((float*)&w)[2] - ((float*)&w)[1] - ((float*)&w)[0];

    #else
        Dest = quaternion
        {
            (Q1.m_W * Q2.m_X) + (Q2.m_W * Q1.m_X) + (Q1.m_Y * Q2.m_Z) - (Q1.m_Z * Q2.m_Y),
            (Q1.m_W * Q2.m_Y) + (Q2.m_W * Q1.m_Y) + (Q1.m_Z * Q2.m_X) - (Q1.m_X * Q2.m_Z),
            (Q1.m_W * Q2.m_Z) + (Q2.m_W * Q1.m_Z) + (Q1.m_X * Q2.m_Y) - (Q1.m_Y * Q2.m_X),
            (Q1.m_W * Q2.m_W) - (Q2.m_X * Q1.m_X) - (Q1.m_Y * Q2.m_Y) - (Q1.m_Z * Q2.m_Z)
        };
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::RotateX( radian Rx ) noexcept
    {
        float s, c;
        math::SinCos( Rx / radian{ 2.0f }, s, c );
        q_Multiply( *this, quaternion( s, 0, 0, c), *this );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::RotateY( radian Ry ) noexcept
    {
        float s, c;
        math::SinCos( Ry / radian{ 2.0f }, s, c );
        q_Multiply( *this, quaternion( 0, s, 0, c), *this );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::RotateZ( radian Rz ) noexcept
    {
        float s, c;
        math::SinCos( Rz / radian{ 2.0f }, s, c );
        q_Multiply( *this, quaternion( 0, 0, s, c), *this );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::PreRotateX( radian Rx ) noexcept
    {
        float s, c;
        math::SinCos( Rx / radian{ 2.0f }, s, c );
        q_Multiply( *this, *this, quaternion( s, 0, 0, c) );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::PreRotateY( radian Ry ) noexcept
    {
        float s, c;
        math::SinCos( Ry / radian{ 2.0f }, s, c );
        q_Multiply( *this, *this, quaternion( 0, s, 0, c) );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::PreRotateZ( radian Rz ) noexcept
    {
        float s, c;
        math::SinCos( Rz / radian{ 2.0f }, s, c );
        q_Multiply( *this, *this, quaternion( 0, 0, s, c) );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setRotationX( radian Rx ) noexcept
    {
        float s, c;
        math::SinCos( Rx / radian{ 2.0f }, s, c );
        setup( s, 0, 0, c);
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setRotationY( radian Ry ) noexcept
    {
        float s, c;
        math::SinCos( Ry / radian{ 2.0f }, s, c );
        setup( 0, s, 0, c);
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setRotationZ( radian Rz ) noexcept
    {
        float s, c;
        math::SinCos( Rz / radian{ 2.0f }, s, c );
        setup( 0, 0, s, c);
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::Conjugate( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW     = _mm_sub_ps( _mm_setzero_ps(), m_XYZW);
        m_W        = -m_W;
    #else
        m_X = -m_X;
        m_Y = -m_Y;
        m_Z = -m_Z;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::Invert( void ) noexcept
    {              
        const float n = LengthSquared();
        if( n > 0.0f ) (*this) /= n;
        Conjugate();   
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setup( const radian3& R ) noexcept
    {
        setIdentity();
        RotateZ( R.m_Roll  );
        RotateX( R.m_Pitch );
        RotateY( R.m_Yaw   );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setup( float X, float Y, float Z, float W ) noexcept
    {
        m_X = X;
        m_Y = Y;
        m_Z = Z;
        m_W = W;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setup( const vector3& Axis, radian Angle ) noexcept
    {
        float s, c;
        math::SinCos( Angle * radian{ 0.5f }, s, c );
        setup(s*Axis.m_X, s*Axis.m_Y, s*Axis.m_Z, c );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setup( const vector3& StartDir, const vector3& EndDir ) noexcept
    {
        vector3    Axis;
        radian     Angle;

        StartDir.getRotationTowards( EndDir, Axis, Angle );
        setup( Axis, Angle );

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion operator * ( const quaternion& Q1, const quaternion& Q2 ) noexcept
    {
        quaternion Dest;
        q_Multiply( Dest, Q1, Q2 );
        return Dest;
    }

    //------------------------------------------------------------------------------
    inline 
    const quaternion& quaternion::operator += ( const quaternion& Q ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_add_ps( m_XYZW, Q.m_XYZW );
    #else
        m_X += Q.m_X;
        m_Y += Q.m_Y;
        m_Z += Q.m_Z;
        m_W += Q.m_W;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const quaternion& quaternion::operator -= ( const quaternion& Q ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_sub_ps( m_XYZW, Q.m_XYZW );
    #else
        m_X -= Q.m_X;
        m_Y -= Q.m_Y;
        m_Z -= Q.m_Z;
        m_W -= Q.m_W;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const quaternion& quaternion::operator *= ( const quaternion& Q ) noexcept
    {
    #ifdef _XCORE_SSE4
        q_Multiply( *this, *this, Q );
    #else
        m_X *= Q.m_X;
        m_Y *= Q.m_Y;
        m_Z *= Q.m_Z;
        m_W *= Q.m_W;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const quaternion& quaternion::operator *= ( float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_mul_ps( m_XYZW, _mm_set1_ps( Scalar ) );
    #else
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
        m_W *= Scalar;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const quaternion& quaternion::operator /= ( float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_div_ps( m_XYZW, _mm_set1_ps( Scalar ) );
    #else
        m_X /= Scalar;
        m_Y /= Scalar;
        m_Z /= Scalar;
        m_W /= Scalar;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool quaternion::operator == ( const quaternion& Q ) const noexcept
    {
        return  ( math::Abs( Q.m_X - m_X) < 0.001f ) &&
                ( math::Abs( Q.m_Y - m_Y) < 0.001f ) &&
                ( math::Abs( Q.m_Z - m_Z) < 0.001f ) &&
                ( math::Abs( Q.m_W - m_W) < 0.001f );
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion operator + ( const quaternion& Q1, const quaternion& Q2 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return quaternion{ _mm_add_ps( Q1.m_XYZW, Q2.m_XYZW ) };
    #else
        return quaternion{ Q1.m_X + Q2.m_X,
                            Q1.m_Y + Q2.m_Y,
                            Q1.m_Z + Q2.m_Z,
                            Q1.m_W + Q2.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion operator - ( const quaternion& Q1, const quaternion& Q2 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return quaternion{ _mm_sub_ps( Q1.m_XYZW, Q2.m_XYZW ) };
    #else
        return quaternion{ Q1.m_X -  Q2.m_X,
                            Q1.m_Y -  Q2.m_Y,
                            Q1.m_Z -  Q2.m_Z,
                            Q1.m_W -  Q2.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion operator - ( const quaternion& Q ) noexcept
    {
    #ifdef _XCORE_SSE4
        return quaternion{ _mm_sub_ps( _mm_setzero_ps(), Q.m_XYZW ) };
    #else
        return quaternion( -  Q.m_X,
                            -  Q.m_Y,
                            -  Q.m_Z,
                            -  Q.m_W );
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion operator * ( const quaternion& Q, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return quaternion{ _mm_mul_ps( Q.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return quaternion{ Q.m_X * Scalar,
                            Q.m_Y * Scalar,
                            Q.m_Z * Scalar,
                            Q.m_W * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion operator * ( float Scalar, const quaternion& Q ) noexcept
    {
    #ifdef _XCORE_SSE4
        return quaternion{ _mm_mul_ps( Q.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return quaternion{ Q.m_X * Scalar,
                            Q.m_Y * Scalar,
                            Q.m_Z * Scalar,
                            Q.m_W * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 operator * ( const quaternion& Q, const vector3& V ) noexcept
    {
    // Two methods for v to q mul
    /*
        vector3 Result;
        vector3 v1;
        vector3 v2;

        Result.setup( Q.m_X, Q.m_Y, Q.m_Z );
        v1     = Result.Cross( V  );
        v2     = Result.Cross( v1 );
        v1    *= 2.0f * Q.m_W;
        v2    *= 2.0f;
        Result = V + v1 + v2;
        return Result;
    */
        const vector3&  u = *((vector3*)&Q);
        const float        s = Q.m_W;

        return 2.0f * u.Dot( V ) * u + 
              ( s*s - u.Dot(u) ) * V +
              2.0f * s * u.Cross(V);
    }

    //------------------------------------------------------------------------------
    inline 
    float quaternion::LengthSquared( void ) const noexcept
    {
        return (*this).Dot( *this );
    }

    //------------------------------------------------------------------------------
    inline
    float quaternion::Length( void ) const noexcept
    {
    #ifdef _XCORE_SSE4
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
        a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );
        a = _mm_sqrt_ss(a);

        return sse::getElementByIndex<0>(a);

    #else
        return math::Sqrt( m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W );
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    bool quaternion::isValid( void ) const noexcept
    {
        if( !((m_X >= -1.01f) && ( m_X <= 1.01f) &&
              (m_Y >= -1.01f) && ( m_Y <= 1.01f) &&
              (m_Z >= -1.01f) && ( m_Z <= 1.01f) &&
              (m_W >= -1.01f) && ( m_W <= 1.01f)) )
             return false;

        const float L = Length();
        return (L>=0.9f) && (L<=1.1f);
    }

    //------------------------------------------------------------------------------
    inline
    vector3 quaternion::getAxis( void ) const noexcept
    {
        vector3 Axis;
        float s;

        s = math::Sin( math::ACos( math::Range( m_W, -1.0f, 1.0f ) ) );

        if( (s > -0.00001f) && (s < 0.00001f) ) 
        {
            Axis.setZero();
        }
        else 
        {
            Axis.setup( m_X, m_Y, m_Z );
            Axis /= s; 
        }

        return Axis;
    }

    //------------------------------------------------------------------------------
    inline
    radian quaternion::getAngle( void ) const noexcept
    {
        return math::ACos( math::Range( m_W, -1.0f, 1.0f ) ) * radian{ 2.0f };
    }

    //------------------------------------------------------------------------------
    inline
    radian3 quaternion::getRotation( void ) const noexcept
    {
        const float tx  = 2.0f * m_X;   // 2x
        const float ty  = 2.0f * m_Y;   // 2y
        const float tz  = 2.0f * m_Z;   // 2z
        const float txw =   tx * m_W;   // 2x * w
        const float tyw =   ty * m_W;   // 2y * w
        const float tzw =   tz * m_W;   // 2z * w
        const float txx =   tx * m_X;   // 2x * x
        const float tyx =   ty * m_X;   // 2y * x
        const float tzx =   tz * m_X;   // 2z * x
        const float tyy =   ty * m_Y;   // 2y * y
        const float tzy =   tz * m_Y;   // 2z * y
        const float tzz =   tz * m_Z;   // 2z * z

        radian  Pitch, Yaw, Roll;

        //
        // First get pitch (Rx).
        //
        Pitch = math::ASin( -math::Range( tzy - txw, -1.0f, 1.0f ) );

        //
        // get yaw (Ry) and roll (Rz).    
        //
        if( (Pitch > -radian{ 89.0_xdeg } ) || (Pitch < radian{ 89.0_xdeg } ) )
        {
            Yaw  = math::ATan2( tzx + tyw, 1.0f-(txx+tyy) );
            Roll = math::ATan2( tyx + tzw, 1.0f-(txx+tzz) );
        }
        else
        {
            Yaw  = radian{ 0.0f };
            Roll = math::ATan2( tzx - tyw, 1.0f-(tyy+tzz) );
        }

        return radian3( Pitch, Yaw, Roll );
    }

    //------------------------------------------------------------------------------
    inline
    float quaternion::Dot( const quaternion& Q ) const noexcept
    {
    #ifdef _XCORE_SSE4
        const floatx4       A = _mm_mul_ps( m_XYZW, Q.m_XYZW );
        const floatx4       B = _mm_add_ss( _mm_shuffle_ps( A, A, _MM_SHUFFLE(3,3,3,3)), 
                              _mm_add_ss( _mm_shuffle_ps( A, A, _MM_SHUFFLE(0,0,0,0)),
                              _mm_add_ss( _mm_shuffle_ps( A, A, _MM_SHUFFLE(1,1,1,1)), 
                                          _mm_shuffle_ps( A, A, _MM_SHUFFLE(2,2,2,2)))));

        return sse::getElementByIndex<0>(B);
    #else
        return  m_X*Q.m_X +
                m_Y*Q.m_Y +
                m_Z*Q.m_Z +
                m_W*Q.m_W;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion& quaternion::Normalize( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );
              a = _mm_rsqrt_ss(a);

        const floatx4 oneDivLen = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
        return *this;

    #else
        const float Div = math::InvSqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z+ m_W*m_W );
        m_X *= Div;
        m_Y *= Div;
        m_Z *= Div;
        m_W *= Div;
        return *this;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    quaternion& quaternion::NormalizeSafe( void ) noexcept
    {
        const float   Tof  = 0.00001f;

    #ifdef _XCORE_SSE4
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );

        if( sse::getElementByIndex<0>(a) < Tof )
        {
            setup( 1, 0, 0, 0 );
            return *this;
        }

                        a           = _mm_rsqrt_ss(a);
        const floatx4     oneDivLen   = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
        return *this;

    #else
        const float dsq = m_X*m_X + m_Y*m_Y + m_Z*m_Z+ m_W*m_W;
	    if( dsq < Tof ) 
        {		
            setup( 1.0f, 0.0f, 0.0f, 0.0f );
            return *this;
        }

        const float Div = math::InvSqrt(dsq);
        m_X *= Div;
        m_Y *= Div;
        m_Z *= Div;
        m_W *= Div;
        return *this;
    #endif
    }


    //------------------------------------------------------------------------------
    inline
    quaternion quaternion::BlendFast( float T, const quaternion& End ) const noexcept
    {
        float Dot;
        float LenSquared;
        float OneOverL;
        float x0,y0,z0,w0;

        // Determine if quats are further than 90 degrees
        Dot = m_X*End.m_X + m_Y*End.m_Y + m_Z*End.m_Z + m_W*End.m_W;

        // If dot is negative flip one of the quaterions
        if( Dot < 0.0f )
        {
            x0 = -m_X;
            y0 = -m_Y;
            z0 = -m_Z;
            w0 = -m_W;
        }
        else
        {
            x0 =  m_X;
            y0 =  m_Y;
            z0 =  m_Z;
            w0 =  m_W;
        }

        // Compute interpolated values
        x0 = x0 + T*(End.m_X - x0);
        y0 = y0 + T*(End.m_Y - y0);
        z0 = z0 + T*(End.m_Z - z0);
        w0 = w0 + T*(End.m_W - w0);

        // get squared length of new quaternion
        LenSquared = x0*x0 + y0*y0 + z0*z0 + w0*w0;

        // Use home-baked polynomial to compute 1/sqrt(LenSquared)
        // Input range is 0.5 <-> 1.0
        // Ouput range is 1.414213 <-> 1.0

        if( LenSquared<0.857f )
            OneOverL = (((0.699368f)*LenSquared) + -1.819985f)*LenSquared + 2.126369f;    //0.0000792
        else
            OneOverL = (((0.454012f)*LenSquared) + -1.403517f)*LenSquared + 1.949542f;    //0.0000373

        // Renormalize and return quaternion
        return quaternion( x0*OneOverL, y0*OneOverL, z0*OneOverL, w0*OneOverL );
    }

    //------------------------------------------------------------------------------
    inline
    quaternion quaternion::BlendAccurate( float T, const quaternion& End ) const noexcept
    {
        bool bFlip = false;

        // Determine if quats are further than 90 degrees
        float Cs = Dot(End);
        if( Cs < 0.0f) 
        {
            Cs = -Cs;
            bFlip = !bFlip;
        }

        float inv_T;
        if( (1.0f - Cs) < 0.000001f ) 
        {
            inv_T = 1.0f - T;
        } 
        else 
        {
            const radian   Theta   = math::ACos( Cs );
            const float       S       = 1.0f / math::Sin( Theta );

            inv_T = math::Sin( radian{ 1.0f - T } * Theta) * S;
            T     = math::Sin( radian{ T        } * Theta) * S;
        }

        if( bFlip ) 
        {
            T = -T;
        }
    
        return End * T + (*this) * inv_T;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::setup( const matrix4& M ) noexcept
    {
        quaternion& Q = *this;
        float T;
        float X2, Y2, Z2, W2;  // squared magniudes of quaternion components
        std::int32_t i;

        // remove scale from xmatrix
        matrix4 O = M;
        O.ClearScale();

        // first compute squared magnitudes of quaternion components - at least one
        // will be greater than 0 since quaternion is unit magnitude
        W2 = 0.25f * (O.m_Cell[0][0] + O.m_Cell[1][1] + O.m_Cell[2][2] + 1.0f );
        X2 = W2 - 0.5f * (O.m_Cell[1][1] + O.m_Cell[2][2]);
        Y2 = W2 - 0.5f * (O.m_Cell[2][2] + O.m_Cell[0][0]);
        Z2 = W2 - 0.5f * (O.m_Cell[0][0] + O.m_Cell[1][1]);

        // find maximum magnitude component
        i = (W2 > X2 ) ?
            ((W2 > Y2) ? ((W2 > Z2) ? 0 : 3) : ((Y2 > Z2) ? 2 : 3)) :
            ((X2 > Y2) ? ((X2 > Z2) ? 1 : 3) : ((Y2 > Z2) ? 2 : 3));

        // compute signed quaternion components using numerically stable method
        switch( i ) 
        {
        case 0:
            Q.m_W = math::Sqrt(W2);
            T = 0.25f / Q.m_W;
            Q.m_X = (O.m_Cell[1][2] - O.m_Cell[2][1]) * T;
            Q.m_Y = (O.m_Cell[2][0] - O.m_Cell[0][2]) * T;
            Q.m_Z = (O.m_Cell[0][1] - O.m_Cell[1][0]) * T;
            break;
        case 1:
            Q.m_X = math::Sqrt(X2);
            T = 0.25f / Q.m_X;
            Q.m_W = (O.m_Cell[1][2] - O.m_Cell[2][1]) * T;
            Q.m_Y = (O.m_Cell[1][0] + O.m_Cell[0][1]) * T;
            Q.m_Z = (O.m_Cell[2][0] + O.m_Cell[0][2]) * T;
            break;
        case 2:
            Q.m_Y = math::Sqrt(Y2);
            T = 0.25f / Q.m_Y;
            Q.m_W = (O.m_Cell[2][0] - O.m_Cell[0][2]) * T;
            Q.m_Z = (O.m_Cell[2][1] + O.m_Cell[1][2]) * T;
            Q.m_X = (O.m_Cell[0][1] + O.m_Cell[1][0]) * T;
            break;
        case 3:
            Q.m_Z = math::Sqrt(Z2);
            T = 0.25f / Q.m_Z;
            Q.m_W = (O.m_Cell[0][1] - O.m_Cell[1][0]) * T;
            Q.m_X = (O.m_Cell[0][2] + O.m_Cell[2][0]) * T;
            Q.m_Y = (O.m_Cell[1][2] + O.m_Cell[2][1]) * T;
            break;
        }

        Q.Normalize();
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::Ln( void ) noexcept
    {
        float scale = math::Sqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z );

        if( scale > 0.0f ) 
        {
            radian theta = math::ATan2( scale, m_W );
            scale = theta.m_Value/scale;
        }

        m_X = scale * m_X;
        m_Y = scale * m_Y;
        m_Z = scale * m_Z;
        m_W = 0.0f;

        xassert( isValid() );

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    quaternion& quaternion::Exp( void ) noexcept
    {
        xassert( m_W == 0.0f );
        static const radian Q_EPSILON{ 1e-10f };

        float scale;
        radian theta{ math::Sqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z ) };
        if( theta > Q_EPSILON ) 
        {
            scale = math::Sin( theta ) / theta.m_Value;
        } 
        else 
        {
            scale = 1.0f;
        }

        m_X = scale * m_X;
        m_Y = scale * m_Y;
        m_Z = scale * m_Z;
        m_W = math::Cos(theta);

        xassert( isValid() );

        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3 quaternion::getLookDirection( void ) const noexcept
    {
        return (*this) * vector3::Forward();
    }


}