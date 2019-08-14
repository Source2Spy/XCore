
namespace xcore::math
{
    //==============================================================================
    //
    // Here are some helpfully notes to work with the sse instructions
    //
    // r3, r2, r1, r0
    // W,  Z,  Y,  X 
    //
    // _mm_shuffle_ps( a, b, ( bn, bm, an, am ) )
    // r0 := am
    // r1 := an
    // r2 := bm
    // r3 := bn
    //
    //__m128 _mm_movehl_ps( __m128 a, __m128 b );
    // r3 := a3
    // r2 := a2
    // r1 := b3
    // r0 := b2
    //
    //==============================================================================

    //------------------------------------------------------------------------------
    inline
    void vector4::setIdentity( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_setzero_ps();
    #else
        m_X = m_Y = m_Z = 0;
    #endif
        m_W = 1;
    }

    //------------------------------------------------------------------------------
    inline 
    vector4& vector4::setup( float X, float Y, float Z, float W ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_set_ps( W, Z, Y, X );
    #else
        m_X = X;
        m_Y = Y;
        m_Z = Z;
        m_W = W;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr 
    float vector4::Dot( const vector4& V ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return sse::getElementByIndex<0>( _mm_dp_ps( m_XYZW, V.m_XYZW, 0xff ) );
    #else
        return m_X * V.m_X + 
               m_Y * V.m_Y +
               m_Z * V.m_Z +
               m_W * V.m_W;
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    float vector4::getLengthSquared( void ) const noexcept
    {
        return Dot( *this );
    }

    //------------------------------------------------------------------------------
    inline 
    float vector4::getLength( void ) const noexcept
    {
    #ifdef _XCORE_SSE4
        /*
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
                a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );
                a = _mm_sqrt_ss(a);
        */
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m_XYZW, m_XYZW, 0xFF)));
    #else
        return math::Sqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W);
    #endif
    }

    //------------------------------------------------------------------------------
    xforceinline 
    bool vector4::isValid( void ) const noexcept
    {
        return math::isValid(m_X) && math::isValid(m_Y) && math::isValid(m_Z) && math::isValid(m_W);
    }

    //------------------------------------------------------------------------------
    inline 
    vector4& vector4::Normalize( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );
              a = _mm_rsqrt_ss(a);

        floatx4 oneDivLen = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
        return *this;

    #else
        float Div = math::InvSqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z+ m_W*m_W );
        m_X *= Div;
        m_Y *= Div;
        m_Z *= Div;
        m_W *= Div;
        return *this;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    vector4& vector4::NormalizeSafe( void ) noexcept
    {
        const float   Tof  = 0.00001f;

    #ifdef _XCORE_SSE4
        floatx4 a = _mm_mul_ps( m_XYZW, m_XYZW );
              a = _mm_add_ss( _mm_add_ss( _mm_shuffle_ps(a, a, 1),_mm_add_ps( _mm_movehl_ps(a, a), a)),_mm_shuffle_ps(a, a, 3) );

        if( ((float*)&a)[0] < Tof )
        {
            setup( 1, 0, 0, 0 );
            return *this;
        }

                      a = _mm_rsqrt_ss(a);
        floatx4 oneDivLen = _mm_shuffle_ps( a, a, 0 );
    
        m_XYZW = _mm_mul_ps( m_XYZW, oneDivLen );
        return *this;
    #else
        float dsq = m_X*m_X + m_Y*m_Y + m_Z*m_Z+ m_W*m_W;
	    if( dsq < Tof ) 
        {		
            setup( 1, 0, 0 );
            return *this;
        }

        float Div = math::InvSqrt(dsq);
        m_X *= Div;
        m_Y *= Div;
        m_Z *= Div;
        m_W *= Div;
        return *this;
    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    const vector4& vector4::operator += ( const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_add_ps( m_XYZW, V.m_XYZW );
    #else
        m_X += V.m_X;
        m_Y += V.m_Y;
        m_Z += V.m_Z;
        m_W += V.m_W;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector4& vector4::operator -= ( const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_sub_ps( m_XYZW, V.m_XYZW );
    #else
        m_X -= V.m_X;
        m_Y -= V.m_Y;
        m_Z -= V.m_Z;
        m_W -= V.m_W;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector4& vector4::operator *= ( const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_mul_ps( m_XYZW, V.m_XYZW );
    #else
        m_X *= V.m_X;
        m_Y *= V.m_Y;
        m_Z *= V.m_Z;
        m_W *= V.m_W;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector4& vector4::operator *= ( float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_mul_ps( m_XYZW, _mm_set1_ps( Scalar ) );
    #else
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
        m_W *= Scalar;
    #endif

        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector4& vector4::operator /= ( float Div ) noexcept
    {
    #ifdef _XCORE_SSE4
        m_XYZW = _mm_div_ps( m_XYZW, _mm_set1_ps( Div ) );
    #else
        float Scalar = 1.0f/Div;
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
        m_W *= Scalar;
    #endif
        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool vector4::operator == ( const vector4& V ) const noexcept
    {
        return !(
            (math::Abs( V.m_X - m_X) > XFLT_TOL)||
            (math::Abs( V.m_Y - m_Y) > XFLT_TOL)||
            (math::Abs( V.m_Z - m_Z) > XFLT_TOL)||
            (math::Abs( V.m_W - m_W) > XFLT_TOL)
            );
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 operator + ( const vector4& V1, const vector4& V2 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_add_ps( V1.m_XYZW, V2.m_XYZW ) };
    #else
        return vector4{    V1.m_X + V2.m_X,
                            V1.m_Y + V2.m_Y,
                            V1.m_Z + V2.m_Z,
                            V1.m_W + V2.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector4 operator - ( const vector4& V1, const vector4& V2 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_sub_ps( V1.m_XYZW, V2.m_XYZW ) };
    #else
        return vector4{    V1.m_X - V2.m_X,
                            V1.m_Y - V2.m_Y,
                            V1.m_Z - V2.m_Z,
                            V1.m_W - V2.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector4 operator * ( const vector4& V1, const vector4& V2 ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_mul_ps( V1.m_XYZW, V2.m_XYZW ) };
    #else
        return vector4{    V1.m_X * V2.m_X,
                            V1.m_Y * V2.m_Y,
                            V1.m_Z * V2.m_Z,
                            V1.m_W * V2.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector4 operator - ( const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_sub_ps( _mm_setzero_ps(), V.m_XYZW ) };
    #else
        return vector4{    -V.m_X,
                            -V.m_Y,
                            -V.m_Z,
                            -V.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector4 operator * ( const vector4& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_mul_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector4{   V.m_X * Scalar,
                          V.m_Y * Scalar,
                          V.m_Z * Scalar,
                          V.m_W * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector4 operator * ( float Scalar, const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_mul_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector4{ V.m_X * Scalar,
                        V.m_Y * Scalar,
                        V.m_Z * Scalar,
                        V.m_W * Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 operator + ( float Scalar, const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_add_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector4{ V.m_X + Scalar,
                        V.m_Y + Scalar,
                        V.m_Z + Scalar,
                        V.m_W + Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 operator + ( const vector4& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_add_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector4{ V.m_X + Scalar,
                        V.m_Y + Scalar,
                        V.m_Z + Scalar,
                        V.m_W + Scalar };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 operator - ( float Scalar, const vector4& V ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_sub_ps( _mm_set1_ps( Scalar ), V.m_XYZW ) };
    #else
        return vector4{ Scalar - V.m_X,
                        Scalar - V.m_Y,
                        Scalar - V.m_Z,
                        Scalar - V.m_W };
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 operator - ( const vector4& V, float Scalar ) noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_sub_ps( V.m_XYZW, _mm_set1_ps( Scalar ) ) };
    #else
        return vector4{ V.m_X - Scalar,
                        V.m_Y - Scalar,
                        V.m_Z - Scalar,
                        V.m_W - Scalar };
    #endif
    }


    //------------------------------------------------------------------------------
    constexpr 
    vector4 vector4::getMin( const vector4& V ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4{ _mm_min_ps( m_XYZW, V.m_XYZW ) };
    #else
        return vector4{ math::Min(V.m_X, m_X),
                        math::Min( V.m_Y,m_Y),
                        math::Min( V.m_Z,m_Z),
                        math::Min( V.m_W,m_W) };
    #endif
    }
 
    //------------------------------------------------------------------------------
    constexpr 
    vector4 vector4::getMax( const vector4& V ) const noexcept
    {
        return vector4{ math::Max( V.m_X,m_X),
                        math::Max( V.m_Y,m_Y),
                        math::Max( V.m_Z,m_Z),
                        math::Max( V.m_W,m_W) };
    }

    //------------------------------------------------------------------------------
    inline 
    vector4& vector4::Homogeneous( void ) noexcept
    {
        *this /= m_W;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    vector4& vector4::Abs( void ) noexcept
    {
        m_X = math::Abs(m_X);
        m_Y = math::Abs(m_Y);
        m_Z = math::Abs(m_Z);
        m_W = math::Abs(m_W);
        return *this;
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool vector4::isInRange( const float Min, const float Max ) const noexcept
    {
    #ifdef _XCORE_SSE4
        const floatx4 t1 = _mm_add_ps( _mm_cmplt_ps( m_XYZW, _mm_set1_ps(Min) ), _mm_cmpgt_ps( m_XYZW, _mm_set1_ps(Max) ) );
        return  sse::getElementByIndex<0>(t1) == 0 &&
                sse::getElementByIndex<1>(t1) == 0 &&
                sse::getElementByIndex<2>(t1) == 0 &&
                sse::getElementByIndex<3>(t1) == 0;
    #else
        return  math::isInRange( m_X, Min, Max ) &&
                math::isInRange( m_Y, Min, Max ) &&
                math::isInRange( m_Z, Min, Max ) &&
                math::isInRange( m_W, Min, Max );
    #endif
    }

    //------------------------------------------------------------------------------
    constexpr
    vector4 vector4::OneOver( void ) const noexcept
    {
    #ifdef _XCORE_SSE4
        return vector4 { _mm_rcp_ps( m_XYZW ) };
    #else
        return vector4 {   1.0f/m_X,
                            1.0f/m_Y,
                            1.0f/m_Z,
                            1.0f/m_W };
    #endif
    }
}