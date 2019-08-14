namespace xcore::math
{
    //------------------------------------------------------------------------------
    inline 
    vector3::vector3( radian Pitch, radian Yaw ) noexcept
    {
        xassume( bits::isAlign(this,16) );
        setup( Pitch, Yaw );
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector3::operator []( const std::int32_t i ) const noexcept
    {
        xassume( i >= 0 && i <= 3 );
        return  (&m_X)[i];
    }

    //------------------------------------------------------------------------------
    inline
    float& vector3::operator []( const std::int32_t i ) noexcept
    {
        xassert( i >= 0 && i <= 3 );
        return (&m_X)[i];
    }

    //------------------------------------------------------------------------------
    inline 
    void vector3::setZero( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_setzero_ps();
    #else
        m_X = m_Y = m_Z = m_W = 0;
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    void vector3::setIdentity( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_setzero_ps();
    #else
        m_X = m_Y = m_Z = m_W = 0;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::setup( float X, float Y, float Z ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_set_ps( 1, Z, Y, X );
    #else
        m_X = X;
        m_Y = Y;
        m_Z = Z;
        m_W = 1;
    #endif
        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::setup( radian Pitch, radian Yaw ) noexcept
    {
        float PS, PC;
        float YS, YC;

        math::SinCos( Pitch, PS, PC );
        math::SinCos( Yaw,   YS, YC );

        return setup( (YS * PC), -PS, (YC * PC) );
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::setup( const float n ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW =_mm_set_ps( 1, n, n, n );
    #else
        m_X = m_Y = m_Z = n;
        m_W = 1;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::setup( const floatx4& Register ) noexcept
    {
        m_XYZW = Register;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    float vector3::getLength( void ) const noexcept
    {
    #ifdef _XCORE_SSE4
        /*
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a));
              a = _mm_sqrt_ss(a);
        */
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m_XYZW, m_XYZW, 0x71)));
    #else
        return math::Sqrt( Dot( *this ) );
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector3::getLengthSquared( void ) const noexcept
    {
        return Dot( *this );
    }

    //------------------------------------------------------------------------------
    inline
    void vector3::getRotationTowards( const vector3&   DestV,
                                       vector3&        RotAxis,
                                       radian&         RotAngle ) const noexcept
    {
        //
        // First get the total length of both vectors this, and dest
        //
        const float D = getLength() * DestV.getLength();
        if( D < 0.0001f )
        {
            RotAxis  = vector3::Right();
            RotAngle = radian{ 0.0_xdeg };
            return;
        }
    
        //
        // Check for the singularity at when the vectors add to 180 degrees.
        // At this point the cross product won't be able to find a perpendicular axis of rotation.
        // So we will choose a default one.
        //
        float Dot = (*this).Dot(DestV) / D;
        if( Dot <= -0.999f )  
        {
            const vector3 absU = DestV.getAbs();

            // Get Orthogonal
            RotAxis = absU.m_X < absU.m_Y ? 
                        vector3::Right() 
                        : 
                        (absU.m_Y < absU.m_Z ? 
                            vector3::Up() 
                            : 
                            vector3::Forward());

            //
            // Finally compute the final angle
            //
            RotAxis  = Cross( RotAxis ).NormalizeSafe();
            RotAngle = PI;
        }
        else
        {
            if ( Dot >  1.0f )  Dot =  1.0f;

            //
            // get the axis to rotate those vectors by
            //
            RotAxis = Cross( DestV );
    
            //
            // Finally compute the final angle
            //
            RotAxis.NormalizeSafe();
            RotAngle = math::ACos(Dot);
        }
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector3::getDistanceSquare( const vector3& V ) const noexcept
    {
        return ((*this) - V).getLengthSquared();
    }

    //------------------------------------------------------------------------------
    inline
    float vector3::getDistance( const vector3& V ) const noexcept
    {
        return math::Sqrt(getDistanceSquare(V));
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool vector3::isValid( void ) const noexcept
    {
        return  math::isValid(m_X) && 
                math::isValid(m_Y) && 
                math::isValid(m_Z);
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::Normalize( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        // get len
        /*
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a));
              a = _mm_rsqrt_ss(a);

        const floatx4 oneDivLen = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
        */
        const floatx4 oneDivLen = _mm_rsqrt_ps( _mm_dp_ps( m_XYZW, m_XYZW, 0x77 ) );
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
    #else
        *this *= math::InvSqrt( Dot( *this ) );
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::NormalizeSafe( void ) noexcept
    {
        const float   Tof  = 0.00001f;
    #ifdef _XCORE_SSE4
        // get len
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a));

        if( sse::getElementByIndex<0>(a) < Tof )
        {
            setup( 1, 0, 0 );
            return *this;
        }

                      a = _mm_rsqrt_ss(a);
        floatx4 oneDivLen = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
    #else
        float Div = Dot( *this );
	    if( Div < Tof )
        {		
            setup( 1, 0, 0 );
            return *this;
        }
        Div = math::InvSqrt( Div );
        m_X *= Div;
        m_Y *= Div;
        m_Z *= Div;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3& vector3::operator += ( const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_add_ps( m_XYZW, V.m_XYZW );
    #else
        m_X += V.m_X;
        m_Y += V.m_Y;
        m_Z += V.m_Z;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3& vector3::operator -= ( const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_sub_ps( m_XYZW, V.m_XYZW );
    #else
        m_X -= V.m_X;
        m_Y -= V.m_Y;
        m_Z -= V.m_Z;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3& vector3::operator *= ( const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_mul_ps( m_XYZW, V.m_XYZW );
    #else
        m_X *= V.m_X;
        m_Y *= V.m_Y;
        m_Z *= V.m_Z;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3& vector3::operator *= ( float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_mul_ps( m_XYZW, _mm_set1_ps( Scalar ) );
    #else
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3& vector3::operator /= ( float Div ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_div_ps( m_XYZW, _mm_set1_ps( Div ) );
    #else
        const float Scalar = 1.0f/Div;
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
    #endif
        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 operator / ( const vector3& V, float Div ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_div_ps( V.m_XYZW, _mm_set1_ps( Div ) ) };
    #else
        const float Scalar = 1.0f/Div;
        return vector3{    V.m_X * Scalar, 
                            V.m_Y * Scalar, 
                            V.m_Z * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_mul_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector3{    V.m_X * Scalar, 
                            V.m_Y * Scalar, 
                            V.m_Z * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( float Scale, const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_mul_ps( V.m_XYZW, _mm_set1_ps( Scale ) ) };
    #else
        return vector3{    V.m_X * Scale, 
                            V.m_Y * Scale, 
                            V.m_Z * Scale };
    #endif
    }


    //------------------------------------------------------------------------------
    constexpr
    vector3 operator + ( float Scalar, const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_add_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector3{    V.m_X + Scalar, 
                            V.m_Y + Scalar, 
                            V.m_Z + Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 operator + ( const vector3& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_add_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector3{    V.m_X + Scalar, 
                            V.m_Y + Scalar, 
                            V.m_Z + Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 operator - ( float Scalar, const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_sub_ps( _mm_set1_ps( Scalar ), V.m_XYZW ) };
    #else
        return vector3{    Scalar - V.m_X, 
                            Scalar - V.m_Y, 
                            Scalar - V.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 operator - ( const vector3& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_sub_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector3{    V.m_X - Scalar, 
                            V.m_Y - Scalar, 
                            V.m_Z - Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool vector3::operator == ( const vector3& V ) const noexcept
    {
        return !(
                    (math::Abs( V.m_X - m_X) > XFLT_TOL) ||
                    (math::Abs( V.m_Y - m_Y) > XFLT_TOL) ||
                    (math::Abs( V.m_Z - m_Z) > XFLT_TOL) 
                );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3::getMin( const vector3& V ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_min_ps( m_XYZW, V.m_XYZW ) };
    #else
        return vector3{    math::Min( V.m_X, m_X ), 
                            math::Min( V.m_Y, m_Y ), 
                            math::Min( V.m_Z, m_Z ) };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3::getMax( const vector3& V ) const noexcept
    {
        return vector3{ math::Max( V.m_X, m_X ), 
                        math::Max( V.m_Y, m_Y ), 
                        math::Max( V.m_Z, m_Z ) };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator + ( const vector3& V0, const vector3& V1 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_add_ps( V0.m_XYZW, V1.m_XYZW ) };
    #else
        return vector3{    V0.m_X + V1.m_X,
                            V0.m_Y + V1.m_Y,
                            V0.m_Z + V1.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator + ( const vector3d& V0, const vector3& V1 ) noexcept
    {
        return V1 + vector3( V0 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator + ( const vector3& V0, const vector3d&  V1 ) noexcept
    {
        return V0 + vector3( V1 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3d&  V0, const vector3& V1 ) noexcept
    {
        return vector3( V0 ) - V1 ;
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3& V0, const vector3d&  V1 ) noexcept
    {
        return V0 - vector3( V1 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3d&  V0, const vector3& V1 ) noexcept
    {
        return V1 * vector3( V0 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3& V0, const vector3d&  V1 ) noexcept
    {
        return V0 * vector3( V1 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator / ( const vector3d&  V0, const vector3& V1 ) noexcept
    {
        return vector3( V0 ) / V1 ;
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator / ( const vector3& V0, const vector3d&  V1 ) noexcept
    {
        return V0 / vector3( V1 );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3& V0, const vector3& V1 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_sub_ps( V0.m_XYZW, V1.m_XYZW) };
    #else
        return vector3{ V0.m_X - V1.m_X,
                        V0.m_Y - V1.m_Y,
                        V0.m_Z - V1.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3& V0, const vector3& V1 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_mul_ps( V0.m_XYZW, V1.m_XYZW ) };
    #else
        return vector3{ V0.m_X + V1.m_X,
                        V0.m_Y + V1.m_Y,
                        V0.m_Z + V1.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator / ( const vector3& V0, const vector3& V1 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_div_ps( V0.m_XYZW, V1.m_XYZW ) };
    #else
        return vector3{ V0.m_X / V1.m_X,
                        V0.m_Y / V1.m_Y,
                        V0.m_Z / V1.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_sub_ps( _mm_setzero_ps(), V.m_XYZW ) };
    #else
        return vector3{ - V.m_X,
                        - V.m_Y,
                        - V.m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    float vector3::Dot( const vector3& V ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return sse::getElementByIndex<0>( _mm_dp_ps( m_XYZW, V.m_XYZW, 0x77 ) );
    #else
        return m_X * V.m_X + 
               m_Y * V.m_Y +
               m_Z * V.m_Z;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    vector3 vector3::Cross( const vector3& V ) const noexcept
    {

    #ifdef _XCORE_SSE4

        enum : int
        {
            X = 0,
            Y = 1,
            Z = 2,
            W = 3
        };

        const floatx4 A = _mm_mul_ps( _mm_shuffle_ps(   m_XYZW,   m_XYZW, _MM_SHUFFLE( W, X, Z, Y )),
                                    _mm_shuffle_ps( V.m_XYZW, V.m_XYZW, _MM_SHUFFLE( W, Y, X, Z )) );
        const floatx4 B = _mm_mul_ps( _mm_shuffle_ps(   m_XYZW,   m_XYZW, _MM_SHUFFLE( W, Y, X, Z )),
                                    _mm_shuffle_ps( V.m_XYZW, V.m_XYZW, _MM_SHUFFLE( W, X, Z, Y )) );

        return vector3{ _mm_sub_ps(A, B) };

    #else
        return vector3{    m_Y * V.m_Z - m_Z * V.m_Y,
                            m_Z * V.m_X - m_X * V.m_Z,
                            m_X * V.m_Y - m_Y * V.m_X };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 vector3::getAbs( void ) const noexcept
    {
        return { math::Abs( m_X ), math::Abs( m_Y ), math::Abs( m_Z ) };
    }

    //------------------------------------------------------------------------------
    inline
    bool vector3::isInRange( const float Min, const float Max ) const noexcept
    {
    #ifdef _XCORE_SSE4
        const floatx4 t1 = _mm_add_ps( _mm_cmplt_ps( m_XYZW, _mm_set1_ps(Min) ), _mm_cmpgt_ps( m_XYZW, _mm_set1_ps(Max) ) );
        return sse::getElementByIndex<0>(t1) == 0 &&
               sse::getElementByIndex<1>(t1) == 0 &&
               sse::getElementByIndex<2>(t1) == 0;
    #else
        return math::isInRange( m_X, Min, Max ) &&
               math::isInRange( m_Y, Min, Max ) &&
               math::isInRange( m_Z, Min, Max );
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 vector3::getOneOver( void ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return vector3{ _mm_rcp_ps( m_XYZW ) };
    #else
        return { 1.0f/m_X, 1.0f/m_Y, 1.0f/m_Z };
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::RotateX( radian Rx ) noexcept
    {
        if( Rx.m_Value != 0.0f )
        {
            float S, C;
            math::SinCos( Rx, S, C );

            const float Y = m_Y;
            const float Z = m_Z;
            m_Y = (C * Y) - (S * Z);
            m_Z = (C * Z) + (S * Y);
        }
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::RotateY( radian Ry ) noexcept
    {
        if( Ry.m_Value != 0.0f )
        {
            float S, C;
            math::SinCos( Ry, S, C );

            const float X = m_X;
            const float Z = m_Z;
            m_X = (C * X) + (S * Z);
            m_Z = (C * Z) - (S * X);
        }
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::RotateZ( radian Rz ) noexcept
    {
        if( Rz.m_Value != 0.0f )
        {
            float S, C;
            math::SinCos( Rz, S, C );

            const float X = m_X;
            const float Y = m_Y;
            m_X = (C * X) - (S * Y);
            m_Y = (C * Y) + (S * X);
        }
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3& vector3::Rotate( const radian3& R ) noexcept
    {
        RotateZ( R.m_Roll  );
        RotateX( R.m_Pitch );
        RotateY( R.m_Yaw   );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    float* vector3::operator()( void ) noexcept
    {
        return &m_X;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 vector3::getLerp( const float T, const vector3& End ) const noexcept
    {
        return (*this) + T * ( End - (*this) );
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 vector3::getReflection( const vector3& V ) const noexcept
    {
        return V - (2 * Dot(V)) * (*this);
    }

    //------------------------------------------------------------------------------
    constexpr
    bool vector3::isRightHanded( const vector3& P1, const vector3& P2 ) const noexcept
    {
        return ( (P1.m_X - m_X) * (P2.m_Y - m_Y) - 
                 (P1.m_Y - m_Y) * (P2.m_X - m_X) ) < 0;
    }

    //------------------------------------------------------------------------------
    inline
    vector3& vector3::GridSnap( float GridX, float GridY, float GridZ ) noexcept
    {
        setup( math::Round( m_X, GridX ), 
               math::Round( m_Y, GridY ),
               math::Round( m_Z, GridZ ) );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    radian vector3::getPitch( void ) const noexcept
    {
        const float L = math::Sqrt( m_X*m_X + m_Z*m_Z );
        return -math::ATan2( m_Y, L );

    }

    //------------------------------------------------------------------------------
    inline
    radian vector3::getYaw( void ) const noexcept
    {
        return math::ATan2( m_X, m_Z );
    }

    //------------------------------------------------------------------------------
    inline
    void vector3::getPitchYaw( radian& Pitch, radian& Yaw ) const noexcept
    {
        Pitch = getPitch();
        Yaw   = getYaw();
    }

    //------------------------------------------------------------------------------
    inline
    radian vector3::getAngleBetween( const vector3& V ) const noexcept
    {
        const float D = getLength() * V.getLength();
        if( math::Abs(D) < 0.00001f ) return radian{ 0 };
    
        float Cos = Dot( V ) / D;
    
        if     ( Cos >  1.0f )  Cos =  1.0f;
        else if( Cos < -1.0f )  Cos = -1.0f;
    
        return math::ACos( Cos );
    }

    //------------------------------------------------------------------------------
    //           * Start
    //           |
    //           <--------------(* this )
    //           | Return Vector
    //           |
    //           |
    //           * End
    //
    // Such: 
    //
    // this.getClosestVToLSeg(a,b).LengthSquared(); // Is the length square to the line segment
    // this.getClosestVToLSeg(a,b) + this;          // Is the closest point in to the line segment
    //
    //------------------------------------------------------------------------------
    inline 
    vector3 vector3::getVectorToLineSegment( const vector3& Start, const vector3& End ) const noexcept
    {
        vector3 Diff = *this - Start;
        vector3 Dir  = End   - Start;
        float      T    = Diff.Dot( Dir );

        if( T > 0.0f )
        {
            const float SqrLen = Dir.Dot( Dir );

            if ( T >= SqrLen )
            {
                Diff -= Dir;
            }
            else
            {
                T    /= SqrLen;
                Diff -= T * Dir;
            }
        }

        return -Diff;
    }

    //------------------------------------------------------------------------------

    inline
    float vector3::getSquareDistToLineSeg( const vector3& Start, const vector3& End ) const noexcept
    {
        return getVectorToLineSegment(Start,End).getLengthSquared();
    }

    //------------------------------------------------------------------------------

    inline
    vector3 vector3::getClosestPointInLineSegment( const vector3& Start, const vector3& End ) const noexcept
    {
        return getVectorToLineSegment(Start,End) + *this;
    }

    //------------------------------------------------------------------------------
    inline
    float vector3::getClosestPointToRectangle( 
        const vector3& P0,                      // Origin from the edges.
        const vector3& E0,
        const vector3& E1,
        vector3&       OutClosestPoint ) const noexcept
    {
        const vector3  kDiff    = P0 - *this;
        const float       fA00     = E0.getLengthSquared();
        const float       fA11     = E1.getLengthSquared();
        const float       fB0      = kDiff.Dot( E0 );
        const float       fB1      = kDiff.Dot( E1 );
        float             fS       = -fB0;
        float             fT       = -fB1;
        float             fSqrDist = kDiff.getLengthSquared();

        if( fS < 0.0f )
        {
            fS = 0.0f;
        }
        else if( fS <= fA00 )
        {
            fS /= fA00;
            fSqrDist += fB0*fS;
        }
        else
        {
            fS = 1.0f;
            fSqrDist += fA00 + 2.0f*fB0;
        }

        if( fT < 0.0f )
        {
            fT = 0.0f;
        }
        else if( fT <= fA11 )
        {
            fT /= fA11;
            fSqrDist += fB1*fT;
        }
        else
        {
            fT = 1.0f;
            fSqrDist += fA11 + 2.0f*fB1;
        }

        // setup the closest point
        OutClosestPoint = P0 + (E0 * fS) + (E1 * fT);

        return math::Abs(fSqrDist);
    }
}

