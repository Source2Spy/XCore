
namespace xcore::math
{
    constexpr static matrix4 m4_Identity 
    { 
        floatx4{1.0f, 0.0f, 0.0f, 0.0f},
        floatx4{0.0f, 1.0f, 0.0f, 0.0f},
        floatx4{0.0f, 0.0f, 1.0f, 0.0f},
        floatx4{0.0f, 0.0f, 0.0f, 1.0f}
    };

    //------------------------------------------------------------------------------

    constexpr const matrix4&  matrix4::Identity( void ) noexcept
    {
        return m4_Identity;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const vector3& Right, const vector3& Up, 
                      const vector3& Back,  const vector3& Translation ) noexcept
        : m_Column1{ Right.m_XYZW       }
        , m_Column2{ Up.m_XYZW          }
        , m_Column3{ Back.m_XYZW        }
        , m_Column4{ Translation.m_XYZW }
    {
        xassert( bits::isAlign( this, 16 ) ); 
        xassert( isValid() );
        //set last row
        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;
    }

    // Mainly used for deep debugging
    #ifdef _XCORE_DEBUG_PLUS
    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const matrix4& M ) noexcept
    { 
        xassert( bits::isAlign( this, 16 ) );  
        *this = M;
        xassert( isValid() );
    }
    #endif

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const radian3& R ) noexcept
    {
        xassert( bits::isAlign( this, 16 ) ); 

        setRotation( R );
        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;
        m_Cell[3][2] = 0.0f;
        m_Cell[3][1] = 0.0f;
        m_Cell[3][0] = 0.0f;

        xassert( isValid() );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const quaternion& Q )  noexcept
    {
        xassert( bits::isAlign( this, 16 ) ); 

        setup( Q );

        xassert( isValid() );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( float m11, float m12, float m13, float m14,
                      float m21, float m22, float m23, float m24,
                      float m31, float m32, float m33, float m34,
                      float m41, float m42, float m43, float m44 ) noexcept
    { 
        xassert( bits::isAlign( this, 16 ) ); 

    #ifdef _XCORE_SSE4

        m_Column1 = _mm_set_ps( m41, m31, m21, m11 );
        m_Column2 = _mm_set_ps( m42, m32, m22, m12 );
        m_Column3 = _mm_set_ps( m43, m33, m23, m13 );
        m_Column4 = _mm_set_ps( m44, m34, m24, m14 );

    #else

        setup(	m11, m12, m13, m14,
                m21, m22, m23, m24,
                m31, m32, m33, m34,
                m41, m42, m43, m44 );

    #endif
    
        xassert( isValid() );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const vector3d&   Scale,
                      const radian3&    Rotation,
                      const vector3d&   Translation ) noexcept
    {
        xassert( bits::isAlign( this, 16 ) ); 

        setup( Scale, Rotation, Translation );
    
        xassert( isValid() );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4::matrix4( const vector3&    Scale,
                      const quaternion& Rotation,
                      const vector3&    Translation ) noexcept
    {
        xassert( bits::isAlign( this, 16 ) );
    
        setup( Scale, Rotation, Translation );
    
        xassert( isValid() );
    }
    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( float m11, float m12, float m13, float m14,
                             float m21, float m22, float m23, float m24,
                             float m31, float m32, float m33, float m34,
                             float m41, float m42, float m43, float m44 ) noexcept
    { 
        xassert( bits::isAlign( this, 16 ) ); 

    #ifdef _XCORE_SSE4

        m_Column1 = _mm_set_ps( m41, m31, m21, m11 );
        m_Column2 = _mm_set_ps( m42, m32, m22, m12 );
        m_Column3 = _mm_set_ps( m43, m33, m23, m13 );
        m_Column4 = _mm_set_ps( m44, m34, m24, m14 );

    #else

        m_Cell[0][0] = m11;	m_Cell[1][0] = m12;	m_Cell[2][0] = m13;	m_Cell[3][0] = m14;
        m_Cell[0][1] = m21;	m_Cell[1][1] = m22;	m_Cell[2][1] = m23;	m_Cell[3][1] = m24;
        m_Cell[0][2] = m31;	m_Cell[1][2] = m32;	m_Cell[2][2] = m33;	m_Cell[3][2] = m34;
        m_Cell[0][3] = m41;	m_Cell[1][3] = m42;	m_Cell[2][3] = m43;	m_Cell[3][3] = m44;

    #endif

        xassert( isValid() );

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void matrix4::setZero( void ) noexcept
    {
        std::memset( this, 0, sizeof(*this) );
    }

    //------------------------------------------------------------------------------
    inline
    void matrix4::setIdentity( void ) noexcept
    {
        *this = m4_Identity;
    }

    //------------------------------------------------------------------------------
    inline
    float matrix4::operator ()( const std::int32_t i, const std::int32_t j ) const noexcept
    {
        xassert( i >= 0 && i <= 3 );
        xassert( j >= 0 && j <= 3 );
        // because our storage is column order, we must transpose the indices
        return m_Cell[j][i];
    }

    //------------------------------------------------------------------------------
    inline
    float& matrix4::operator ()( const std::int32_t i, const std::int32_t j ) noexcept
    {
        xassert( i >= 0 && i <= 3 );
        xassert( j >= 0 && j <= 3 );
        // because our storage is column order we must transpose the indices
        return m_Cell[j][i];
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setTranslation( const vector3& Translation ) noexcept
    {
    #ifdef _XCORE_SSE4

        const float W = reinterpret_cast<float*>(&m_Column4)[3];
        m_Column4 = Translation.m_XYZW;
        reinterpret_cast<float*>(&m_Column4)[3] = W;

    #else

        m_Cell[3][0] = Translation.m_X;
        m_Cell[3][1] = Translation.m_Y;
        m_Cell[3][2] = Translation.m_Z;

    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    //matrix4 RM;
    //RM.Identity();
    //RM.M(1,1) =  c;
    //RM.M(2,1) = -s;
    //RM.M(1,2) =  s;
    //RM.M(2,2) =  c;
    //*this = RM * *this;    
    inline 
    matrix4& matrix4::RotateX( radian Rx ) noexcept
    {
        if( Rx.m_Value == 0.0f )  return *this;

        float s, c;    
        math::SinCos( Rx, s, c );

        float M01 = m_Cell[0][1];   m_Cell[0][1] = c*M01 - s*m_Cell[0][2];  m_Cell[0][2] = s*M01 + c*m_Cell[0][2];
        float M11 = m_Cell[1][1];   m_Cell[1][1] = c*M11 - s*m_Cell[1][2];  m_Cell[1][2] = s*M11 + c*m_Cell[1][2];
        float M21 = m_Cell[2][1];   m_Cell[2][1] = c*M21 - s*m_Cell[2][2];  m_Cell[2][2] = s*M21 + c*m_Cell[2][2];
        float M31 = m_Cell[3][1];   m_Cell[3][1] = c*M31 - s*m_Cell[3][2];  m_Cell[3][2] = s*M31 + c*m_Cell[3][2];
        return *this;
    }

    //------------------------------------------------------------------------------
    //matrix4 RM;
    //RM.Identity();
    //RM.M(1,1) =  c;
    //RM.M(2,1) = -s;
    //RM.M(1,2) =  s;
    //RM.M(2,2) =  c;
    //*this *= RM;    
    inline 
    matrix4& matrix4::PreRotateX( radian Rx ) noexcept
    {
        if( Rx.m_Value == 0.0f )  return *this;

        float s, c;    
        math::SinCos( Rx, s, c );

        float M10 = m_Cell[1][0];   m_Cell[1][0] = c*M10 + s*m_Cell[2][0];  m_Cell[2][0] = c*m_Cell[2][0] - s*M10;
        float M11 = m_Cell[1][1];   m_Cell[1][1] = c*M11 + s*m_Cell[2][1];  m_Cell[2][1] = c*m_Cell[2][1] - s*M11;
        float M12 = m_Cell[1][2];   m_Cell[1][2] = c*M12 + s*m_Cell[2][2];  m_Cell[2][2] = c*m_Cell[2][2] - s*M12;
        float M13 = m_Cell[1][3];   m_Cell[1][3] = c*M13 + s*m_Cell[2][3];  m_Cell[2][3] = c*m_Cell[2][3] - s*M13;

        return *this;
    }

    //------------------------------------------------------------------------------
    //matrix4 RM;
    //RM.Identity();
    //RM.M(0,0) =  c;
    //RM.M(2,0) =  s;
    //RM.M(0,2) = -s;
    //RM.M(2,2) =  c;
    //*this = RM * *this;
    inline 
    matrix4& matrix4::RotateY( radian Ry ) noexcept
    {
        if( Ry.m_Value == 0.0f )  return *this;

        float s, c;    
        math::SinCos( Ry, s, c );

        float M00 = m_Cell[0][0];   m_Cell[0][0] = c*M00 + s*m_Cell[0][2];  m_Cell[0][2] = c*m_Cell[0][2] - s*M00;
        float M10 = m_Cell[1][0];   m_Cell[1][0] = c*M10 + s*m_Cell[1][2];  m_Cell[1][2] = c*m_Cell[1][2] - s*M10;
        float M20 = m_Cell[2][0];   m_Cell[2][0] = c*M20 + s*m_Cell[2][2];  m_Cell[2][2] = c*m_Cell[2][2] - s*M20;
        float M30 = m_Cell[3][0];   m_Cell[3][0] = c*M30 + s*m_Cell[3][2];  m_Cell[3][2] = c*m_Cell[3][2] - s*M30;
        return *this;
    }

    //------------------------------------------------------------------------------
    //matrix4 RM;
    //RM.Identity();
    //RM.M(0,0) =  c;
    //RM.M(2,0) =  s;
    //RM.M(0,2) = -s;
    //RM.M(2,2) =  c;
    //*this *= *this;
    inline 
    matrix4& matrix4::PreRotateY( radian Ry ) noexcept
    {
        if( Ry.m_Value == 0.0f )  return *this;

        float s, c;    
        math::SinCos( Ry, s, c );

        float M00 = m_Cell[0][0];   m_Cell[0][0] = c*M00 - s*m_Cell[2][0];  m_Cell[2][0] = s*M00 + c*m_Cell[2][0];
        float M01 = m_Cell[0][1];   m_Cell[0][1] = c*M01 - s*m_Cell[2][1];  m_Cell[2][1] = s*M01 + c*m_Cell[2][1];
        float M02 = m_Cell[0][2];   m_Cell[0][2] = c*M02 - s*m_Cell[2][2];  m_Cell[2][2] = s*M02 + c*m_Cell[2][2];
        float M03 = m_Cell[0][3];   m_Cell[0][3] = c*M03 - s*m_Cell[2][3];  m_Cell[2][3] = s*M03 + c*m_Cell[2][3];
        return *this;
    }


    //------------------------------------------------------------------------------
    // http://nebuladevice.sourceforge.net/doc2/doxydoc/nebula2/html/__matrix44__sse_8h-source.html
    //matrix4 RM;
    //RM.Identity();
    //RM.M(0,0) =  c;
    //RM.M(1,0) = -s;
    //RM.M(0,1) =  s;
    //RM.M(1,1) =  c;
    //*this = RM * *this;
    inline 
    matrix4& matrix4::RotateZ( radian Rz ) noexcept
    {
        if( Rz.m_Value == 0.0f )  return *this;

        float s, c;    
        math::SinCos( Rz, s, c );

        float M00 = m_Cell[0][0];   m_Cell[0][0] = c*M00 - s*m_Cell[0][1];  m_Cell[0][1] = s*M00 + c*m_Cell[0][1];
        float M10 = m_Cell[1][0];   m_Cell[1][0] = c*M10 - s*m_Cell[1][1];  m_Cell[1][1] = s*M10 + c*m_Cell[1][1];
        float M20 = m_Cell[2][0];   m_Cell[2][0] = c*M20 - s*m_Cell[2][1];  m_Cell[2][1] = s*M20 + c*m_Cell[2][1];
        float M30 = m_Cell[3][0];   m_Cell[3][0] = c*M30 - s*m_Cell[3][1];  m_Cell[3][1] = s*M30 + c*m_Cell[3][1];
        return *this;
    }

    //------------------------------------------------------------------------------
    //matrix4 RM;
    //RM.Identity();
    //RM.M(0,0) =  c;
    //RM.M(1,0) = -s;
    //RM.M(0,1) =  s;
    //RM.M(1,1) =  c;
    //*this *= *this;
    inline 
    matrix4& matrix4::PreRotateZ( radian Rz ) noexcept
    {
        if( Rz.m_Value == 0.0f ) return *this;

        float s, c;    
        math::SinCos( Rz, s, c );

        const float M00 = m_Cell[0][0];   m_Cell[0][0] = c*M00 + s*m_Cell[1][0];  m_Cell[1][0] = c*m_Cell[1][0] - s*M00;
        const float M01 = m_Cell[0][1];   m_Cell[0][1] = c*M01 + s*m_Cell[1][1];  m_Cell[1][1] = c*m_Cell[1][1] - s*M01;
        const float M02 = m_Cell[0][2];   m_Cell[0][2] = c*M02 + s*m_Cell[1][2];  m_Cell[1][2] = c*m_Cell[1][2] - s*M02;
        const float M03 = m_Cell[0][3];   m_Cell[0][3] = c*M03 + s*m_Cell[1][3];  m_Cell[1][3] = c*m_Cell[1][3] - s*M03;
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::Translate( const vector3& Translation ) noexcept
    {    
        reinterpret_cast<float*>(&m_Column4)[0] += Translation.m_X;
        reinterpret_cast<float*>(&m_Column4)[1] += Translation.m_Y;
        reinterpret_cast<float*>(&m_Column4)[2] += Translation.m_Z;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::PreTranslate( const vector3& T ) noexcept
    {
    #ifdef TARGET_PC_32BIT

        matrix4 M1(*this);
        _asm
        {
            mov         edx, this                          ; matrix

            movss       xmm0, [T+0]
            shufps      xmm0, xmm0, 0
            mulps       xmm0, xmmword ptr [edx]

            movss       xmm1, [T+4]
            shufps      xmm1, xmm1, 0
            mulps       xmm1, xmmword ptr [edx+16]

            movss       xmm2, [T+8]
            shufps      xmm2, xmm2, 0
            mulps       xmm2, xmmword ptr [edx+32]

            addps       xmm1, xmm2
            addps       xmm0, xmm1
            addps       xmm0, xmmword ptr [edx+48]

            movaps      xmmword ptr [edx+48], xmm0
        }

    #elif defined _XCORE_SSE4

        m_Column4 = _mm_add_ps( _mm_mul_ps( m_Column1, _mm_set_ps1( T.m_X ) ),
                    _mm_add_ps( _mm_mul_ps( m_Column2, _mm_set_ps1( T.m_Y ) ),
                    _mm_add_ps( _mm_mul_ps( m_Column3, _mm_set_ps1( T.m_Z ) ), m_Column4 )));

    #else

        m_Cell[3][0] += (m_Cell[0][0] * T.m_X) + (m_Cell[1][0] * T.m_Y) + (m_Cell[2][0] * T.m_Z);
        m_Cell[3][1] += (m_Cell[0][1] * T.m_X) + (m_Cell[1][1] * T.m_Y) + (m_Cell[2][1] * T.m_Z);
        m_Cell[3][2] += (m_Cell[0][2] * T.m_X) + (m_Cell[1][2] * T.m_Y) + (m_Cell[2][2] * T.m_Z);
        m_Cell[3][3] += (m_Cell[0][3] * T.m_X) + (m_Cell[1][3] * T.m_Y) + (m_Cell[2][3] * T.m_Z);

    #endif

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void m4_Multiply( matrix4& dst, const matrix4& src2, const matrix4& src1 ) noexcept
    {
        xassert( bits::isAlign( &src1, 16 ) ); 
        xassert( bits::isAlign( &src2, 16 ) ); 
        xassert( bits::isAlign( &dst , 16 ) ); 

	     //NOTE, dst can be the same as src1, but not the same as src2,
	     //because it needs the column 1 of src2 when computing column 2 of dst, etc...
	     xassert( &dst != &src2 );

    #ifdef TARGET_PC_32BIT
        // http://www.intel.com/design/pentiumiii/sml/24504501.pdf
        // Note that this is a 4x3 matrix multiply
        __asm 
        {
            mov edx, dword ptr [src2] ; src2
            mov eax, dword ptr [dst]  ; dst
            mov ecx, dword ptr [src1] ; src1
            movss xmm0, dword ptr [edx]
            movaps xmm1, xmmword ptr [ecx]
            shufps xmm0, xmm0, 0
            movss xmm2, dword ptr [edx+4]
            mulps xmm0, xmm1
            shufps xmm2, xmm2, 0
            movaps xmm3, xmmword ptr [ecx+10h]
            movss xmm7, dword ptr [edx+8]
            mulps xmm2, xmm3
            shufps xmm7, xmm7, 0
            addps xmm0, xmm2
            movaps xmm4, xmmword ptr [ecx+20h]
            movss xmm2, dword ptr [edx+0Ch]
            mulps xmm7, xmm4
            shufps xmm2, xmm2, 0
            addps xmm0, xmm7
            movaps xmm5, xmmword ptr [ecx+30h]
            movss xmm6, dword ptr [edx+10h]
            mulps xmm2, xmm5
            movss xmm7, dword ptr [edx+14h]
            shufps xmm6, xmm6, 0
            addps xmm0, xmm2
            shufps xmm7, xmm7, 0
            movlps qword ptr [eax], xmm0
            movhps qword ptr [eax+8], xmm0
            mulps xmm7, xmm3
            movss xmm0, dword ptr [edx+18h]
            mulps xmm6, xmm1
            shufps xmm0, xmm0, 0
            addps xmm6, xmm7
            mulps xmm0, xmm4
            movss xmm2, dword ptr [edx+24h]
            addps xmm6, xmm0
            movss xmm0, dword ptr [edx+1Ch]
            movss xmm7, dword ptr [edx+20h]
            shufps xmm0, xmm0, 0
            shufps xmm7, xmm7, 0
            mulps xmm0, xmm5
            mulps xmm7, xmm1
            addps xmm6, xmm0
            shufps xmm2, xmm2, 0
            movlps qword ptr [eax+10h], xmm6
            movhps qword ptr [eax+18h], xmm6
            mulps xmm2, xmm3
            movss xmm6, dword ptr [edx+28h]
            addps xmm7, xmm2
            shufps xmm6, xmm6, 0
            movss xmm2, dword ptr [edx+2Ch]
            mulps xmm6, xmm4
            shufps xmm2, xmm2, 0
            addps xmm7, xmm6
            mulps xmm2, xmm5
            movss xmm0, dword ptr [edx+34h]
            addps xmm7, xmm2
            shufps xmm0, xmm0, 0
            movlps qword ptr [eax+20h], xmm7
            movss xmm2, dword ptr [edx+30h]
            movhps qword ptr [eax+28h], xmm7
            mulps xmm0, xmm3
            shufps xmm2, xmm2, 0
            movss xmm6, dword ptr [edx+38h]
            mulps xmm2, xmm1
            shufps xmm6, xmm6, 0
            addps xmm2, xmm0
            mulps xmm6, xmm4
            movss xmm7, dword ptr [edx+3Ch]
            shufps xmm7, xmm7, 0
            addps xmm2, xmm6
            mulps xmm7, xmm5
            addps xmm2, xmm7
            movaps xmmword ptr [eax+30h], xmm2
        }
    #elif defined _XCORE_SSE4

        dst.m_Column1 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src1.m_Column1, src1.m_Column1, _MM_SHUFFLE(0,0,0,0)), src2.m_Column1), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column1, src1.m_Column1, _MM_SHUFFLE(1,1,1,1)), src2.m_Column2)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column1, src1.m_Column1, _MM_SHUFFLE(2,2,2,2)), src2.m_Column3)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column1, src1.m_Column1, _MM_SHUFFLE(3,3,3,3)), src2.m_Column4));
        dst.m_Column2 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src1.m_Column2, src1.m_Column2, _MM_SHUFFLE(0,0,0,0)), src2.m_Column1), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column2, src1.m_Column2, _MM_SHUFFLE(1,1,1,1)), src2.m_Column2)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column2, src1.m_Column2, _MM_SHUFFLE(2,2,2,2)), src2.m_Column3)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column2, src1.m_Column2, _MM_SHUFFLE(3,3,3,3)), src2.m_Column4));
        dst.m_Column3 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src1.m_Column3, src1.m_Column3, _MM_SHUFFLE(0,0,0,0)), src2.m_Column1), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column3, src1.m_Column3, _MM_SHUFFLE(1,1,1,1)), src2.m_Column2)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column3, src1.m_Column3, _MM_SHUFFLE(2,2,2,2)), src2.m_Column3)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column3, src1.m_Column3, _MM_SHUFFLE(3,3,3,3)), src2.m_Column4));
        dst.m_Column4 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src1.m_Column4, src1.m_Column4, _MM_SHUFFLE(0,0,0,0)), src2.m_Column1), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column4, src1.m_Column4, _MM_SHUFFLE(1,1,1,1)), src2.m_Column2)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column4, src1.m_Column4, _MM_SHUFFLE(2,2,2,2)), src2.m_Column3)), _mm_mul_ps(_mm_shuffle_ps(src1.m_Column4, src1.m_Column4, _MM_SHUFFLE(3,3,3,3)), src2.m_Column4));

    #else

        dst.m_Cell[0][0] = src1.m_Cell[0][0]*src2.m_Cell[0][0] + src1.m_Cell[0][1]*src2.m_Cell[1][0] + src1.m_Cell[0][2]*src2.m_Cell[2][0] + src1.m_Cell[0][3]*src2.m_Cell[3][0];
        dst.m_Cell[0][1] = src1.m_Cell[0][0]*src2.m_Cell[0][1] + src1.m_Cell[0][1]*src2.m_Cell[1][1] + src1.m_Cell[0][2]*src2.m_Cell[2][1] + src1.m_Cell[0][3]*src2.m_Cell[3][1];
        dst.m_Cell[0][2] = src1.m_Cell[0][0]*src2.m_Cell[0][2] + src1.m_Cell[0][1]*src2.m_Cell[1][2] + src1.m_Cell[0][2]*src2.m_Cell[2][2] + src1.m_Cell[0][3]*src2.m_Cell[3][2];
        dst.m_Cell[0][3] = src1.m_Cell[0][0]*src2.m_Cell[0][3] + src1.m_Cell[0][1]*src2.m_Cell[1][3] + src1.m_Cell[0][2]*src2.m_Cell[2][3] + src1.m_Cell[0][3]*src2.m_Cell[3][3];

        dst.m_Cell[1][0] = src1.m_Cell[1][0]*src2.m_Cell[0][0] + src1.m_Cell[1][1]*src2.m_Cell[1][0] + src1.m_Cell[1][2]*src2.m_Cell[2][0] + src1.m_Cell[1][3]*src2.m_Cell[3][0];
        dst.m_Cell[1][1] = src1.m_Cell[1][0]*src2.m_Cell[0][1] + src1.m_Cell[1][1]*src2.m_Cell[1][1] + src1.m_Cell[1][2]*src2.m_Cell[2][1] + src1.m_Cell[1][3]*src2.m_Cell[3][1];
        dst.m_Cell[1][2] = src1.m_Cell[1][0]*src2.m_Cell[0][2] + src1.m_Cell[1][1]*src2.m_Cell[1][2] + src1.m_Cell[1][2]*src2.m_Cell[2][2] + src1.m_Cell[1][3]*src2.m_Cell[3][2];
        dst.m_Cell[1][3] = src1.m_Cell[1][0]*src2.m_Cell[0][3] + src1.m_Cell[1][1]*src2.m_Cell[1][3] + src1.m_Cell[1][2]*src2.m_Cell[2][3] + src1.m_Cell[1][3]*src2.m_Cell[3][3];

        dst.m_Cell[2][0] = src1.m_Cell[2][0]*src2.m_Cell[0][0] + src1.m_Cell[2][1]*src2.m_Cell[1][0] + src1.m_Cell[2][2]*src2.m_Cell[2][0] + src1.m_Cell[2][3]*src2.m_Cell[3][0];
        dst.m_Cell[2][1] = src1.m_Cell[2][0]*src2.m_Cell[0][1] + src1.m_Cell[2][1]*src2.m_Cell[1][1] + src1.m_Cell[2][2]*src2.m_Cell[2][1] + src1.m_Cell[2][3]*src2.m_Cell[3][1];
        dst.m_Cell[2][2] = src1.m_Cell[2][0]*src2.m_Cell[0][2] + src1.m_Cell[2][1]*src2.m_Cell[1][2] + src1.m_Cell[2][2]*src2.m_Cell[2][2] + src1.m_Cell[2][3]*src2.m_Cell[3][2];
        dst.m_Cell[2][3] = src1.m_Cell[2][0]*src2.m_Cell[0][3] + src1.m_Cell[2][1]*src2.m_Cell[1][3] + src1.m_Cell[2][2]*src2.m_Cell[2][3] + src1.m_Cell[2][3]*src2.m_Cell[3][3];

        dst.m_Cell[3][0] = src1.m_Cell[3][0]*src2.m_Cell[0][0] + src1.m_Cell[3][1]*src2.m_Cell[1][0] + src1.m_Cell[3][2]*src2.m_Cell[2][0] + src1.m_Cell[3][3]*src2.m_Cell[3][0];
        dst.m_Cell[3][1] = src1.m_Cell[3][0]*src2.m_Cell[0][1] + src1.m_Cell[3][1]*src2.m_Cell[1][1] + src1.m_Cell[3][2]*src2.m_Cell[2][1] + src1.m_Cell[3][3]*src2.m_Cell[3][1];
        dst.m_Cell[3][2] = src1.m_Cell[3][0]*src2.m_Cell[0][2] + src1.m_Cell[3][1]*src2.m_Cell[1][2] + src1.m_Cell[3][2]*src2.m_Cell[2][2] + src1.m_Cell[3][3]*src2.m_Cell[3][2];
        dst.m_Cell[3][3] = src1.m_Cell[3][0]*src2.m_Cell[0][3] + src1.m_Cell[3][1]*src2.m_Cell[1][3] + src1.m_Cell[3][2]*src2.m_Cell[2][3] + src1.m_Cell[3][3]*src2.m_Cell[3][3];

    #endif
    }

    //------------------------------------------------------------------------------
    inline
    matrix4 operator * ( const matrix4& M1, const matrix4& M2 ) noexcept
    {
        matrix4 Dest;
        m4_Multiply( Dest, M1, M2 );
        return Dest;
    }

    //------------------------------------------------------------------------------
    inline
    vector3 operator * ( const matrix4& M, const vector3& V ) noexcept
    {    
    #ifdef TARGET_PC_32BIT
        register vector3 Vdest;
        __asm 
        {
            mov         ecx, V                      ; V
            mov         edx, M                      ; matrix
            movss       xmm0, dword ptr [ecx]
            shufps      xmm0, xmm0, 0
            movss       xmm1, dword ptr [ecx+4]
            mulps       xmm0, xmmword ptr [edx]
            shufps      xmm1, xmm1, 0
            movss       xmm2, dword ptr [ecx+8]
            mulps       xmm1, xmmword ptr [edx+16]
            shufps      xmm2, xmm2, 0
            mulps       xmm2, xmmword ptr [edx+32]
            addps       xmm0, xmm1
            addps       xmm2, xmmword ptr [edx+48] 
            addps       xmm0, xmm2
            movaps      xmmword ptr Vdest, xmm0
        }
        return Vdest;
    #elif defined _XCORE_SSE4
        const floatx4 V1 = V.m_XYZW;
        return vector3( _mm_add_ps( _mm_add_ps( _mm_add_ps(
                            _mm_mul_ps(M.m_Column1, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(0,0,0,0))), 
                            _mm_mul_ps(M.m_Column2, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(1,1,1,1)))), 
                            _mm_mul_ps(M.m_Column3, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(2,2,2,2)))), 
                                       M.m_Column4));

    #else

        const float X = (M(0,0)*V.m_X) + (M(0,1)*V.m_Y) + (M(0,2)*V.m_Z) + M(0,3);
        const float Y = (M(1,0)*V.m_X) + (M(1,1)*V.m_Y) + (M(1,2)*V.m_Z) + M(1,3);
        const float Z = (M(2,0)*V.m_X) + (M(2,1)*V.m_Y) + (M(2,2)*V.m_Z) + M(2,3);
        return vector3( X, Y, Z );
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 operator * ( const matrix4& M, const vector3d& V ) noexcept
    {
        return M * vector3( V );
    }

    //------------------------------------------------------------------------------
    inline
    vector2 operator * ( const matrix4& M, const vector2& V ) noexcept
    {
        const vector3 V2 = M * vector3( V.m_X, V.m_Y, 0 );
        return vector2( V2.m_X, V2.m_Y );
    }

    //------------------------------------------------------------------------------
    inline
    vector4 operator * ( const matrix4& M, const vector4& V ) noexcept
    {
    #ifdef TARGET_PC_32BIT
        vector4 Vdest;
        __asm 
        {
            mov         ecx, V                          ; V
            mov         edx, M                          ; matrix
            movss       xmm0, dword ptr [ecx]
            shufps      xmm0, xmm0, 0
            movss       xmm1, dword ptr [ecx+4]
            mulps       xmm0, xmmword ptr [edx]
            shufps      xmm1, xmm1, 0
            movss       xmm2, dword ptr [ecx+8]
            mulps       xmm1, xmmword ptr [edx+16]
            shufps      xmm2, xmm2, 0
            movss       xmm3, dword ptr [ecx+12]
            mulps       xmm2, xmmword ptr [edx+32]
            shufps      xmm3, xmm3, 0
            addps       xmm0, xmm1
            mulps       xmm3, xmmword ptr [edx+48]
            addps       xmm2, xmm3
            addps       xmm0, xmm2
            movaps      xmmword ptr Vdest, xmm0
        }
        return Vdest;

    #elif defined _XCORE_SSE4
        const floatx4& V1 = V.m_XYZW;
        return vector4( _mm_add_ps( _mm_add_ps( _mm_add_ps(
                         _mm_mul_ps( M.m_Column1, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(0,0,0,0))), 
                         _mm_mul_ps( M.m_Column2, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(1,1,1,1)))), 
                         _mm_mul_ps( M.m_Column3, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(2,2,2,2)))), 
                         _mm_mul_ps( M.m_Column4, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(3,3,3,3)))));
    
    #else

        return vector4( (M(0,0)*V.m_X) + (M(0,1)*V.m_Y) + (M(0,2)*V.m_Z) + (M(0,3)*V.m_W),
                         (M(1,0)*V.m_X) + (M(1,1)*V.m_Y) + (M(1,2)*V.m_Z) + (M(1,3)*V.m_W),
                         (M(2,0)*V.m_X) + (M(2,1)*V.m_Y) + (M(2,2)*V.m_Z) + (M(2,3)*V.m_W),
                         (M(3,0)*V.m_X) + (M(3,1)*V.m_Y) + (M(3,2)*V.m_Z) + (M(3,3)*V.m_W) );
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::Transpose( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        _MM_TRANSPOSE4_PS( m_Column1, m_Column2, m_Column3, m_Column4 );
    #else
        float     T;    

        T            = m_Cell[1][0];
        m_Cell[1][0] = m_Cell[0][1];
        m_Cell[0][1] = T;
    
        T            = m_Cell[2][0];
        m_Cell[2][0] = m_Cell[0][2];
        m_Cell[0][2] = T;
    
        T            = m_Cell[3][0];
        m_Cell[3][0] = m_Cell[0][3];
        m_Cell[0][3] = T;

        T            = m_Cell[2][1];
        m_Cell[2][1] = m_Cell[1][2];
        m_Cell[1][2] = T;
    
        T            = m_Cell[3][1];
        m_Cell[3][1] = m_Cell[1][3];
        m_Cell[1][3] = T;

        T            = m_Cell[3][2];
        m_Cell[3][2] = m_Cell[2][3];
        m_Cell[2][3] = T;
    #endif
        return *this;
    }                      

    //------------------------------------------------------------------------------
    // http://www.intel.com/design/pentiumiii/sml/24504501.pdf
    inline
    matrix4& matrix4::FullInvert( void ) noexcept
    {
    #ifdef _XCORE_SSE4
        alignas(matrix4) float* src = &(m_Cell[0][0]);

        floatx4 minor0, minor1, minor2, minor3;
        floatx4 row0, row1, row2, row3;
        floatx4 det, tmp1;

        tmp1 = _mm_loadh_pi(_mm_loadl_pi(m_Column1, (__m64*)(src)), (__m64*)(src+ 4));
        row1 = _mm_loadh_pi(_mm_loadl_pi(m_Column1, (__m64*)(src+8)), (__m64*)(src+12));

        row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
        row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);

        tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
        row3 = _mm_loadh_pi(_mm_loadl_pi(m_Column3, (__m64*)(src+10)), (__m64*)(src+14));

        row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
        row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

        tmp1 = _mm_mul_ps(row2, row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

        minor0 = _mm_mul_ps(row1, tmp1);
        minor1 = _mm_mul_ps(row0, tmp1);

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
        minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
        minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

        tmp1 = _mm_mul_ps(row1, row2);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

        minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
        minor3 = _mm_mul_ps(row0, tmp1);

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
        minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
        minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

        tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        row2 = _mm_shuffle_ps(row2, row2, 0x4E);

        minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
        minor2 = _mm_mul_ps(row0, tmp1);

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
        minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
        minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

        tmp1 = _mm_mul_ps(row0, row1);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

        minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
        minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
        minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

        tmp1 = _mm_mul_ps(row0, row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

        minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
        minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
        minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

        tmp1 = _mm_mul_ps(row0, row2);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

        minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
        minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));

        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

        minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
        minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

        det = _mm_mul_ps(row0, minor0);
        det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
        det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
        tmp1 = _mm_rcp_ss(det);

        det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
        det = _mm_shuffle_ps(det, det, 0x00);

        minor0 = _mm_mul_ps(det, minor0);
        _mm_storel_pi((__m64*)(src), minor0);
        _mm_storeh_pi((__m64*)(src+2), minor0);

        minor1 = _mm_mul_ps(det, minor1);
        _mm_storel_pi((__m64*)(src+4), minor1);
        _mm_storeh_pi((__m64*)(src+6), minor1);

        minor2 = _mm_mul_ps(det, minor2);
        _mm_storel_pi((__m64*)(src+ 8), minor2);
        _mm_storeh_pi((__m64*)(src+10), minor2);

        minor3 = _mm_mul_ps(det, minor3);
        _mm_storel_pi((__m64*)(src+12), minor3);
        _mm_storeh_pi((__m64*)(src+14), minor3);

    #else

        float Scratch[4][8];
        float a;
        std::int32_t i, j, k, jr, Pivot;
        std::int32_t Row[4];    

        //
        // Initialize.
        //
        for( j = 0; j < 4; j++ )
        {
            for( k = 0; k < 4; k++ )
            {
                Scratch[j][k]   = m_Cell[j][k];
                Scratch[j][4+k] = 0.0f;
            }

            Scratch[j][4+j] = 1.0f;
            Row[j] = j;
        }

        //
        // Eliminate columns.
        //
        for( i = 0; i < 4; i++ )
        {
            // Find pivot.
            k = i;

            a = math::Abs( Scratch[Row[k]][k] );

            for( j = i+1; j < 4; j++ )
            {
                jr = Row[j];

                if( a < math::Abs( Scratch[jr][i] ) )
                {
                    k = j;
                    a = math::Abs( Scratch[jr][i] );
                }
            }

            // Swap the pivot row (Row[k]) with the i'th row.
            Pivot  = Row[k];
            Row[k] = Row[i];
            Row[i] = Pivot;

            // Normalize pivot row.
            a = Scratch[Pivot][i];

            if( a == 0.0f ) 
                return *this;

            Scratch[Pivot][i] = 1.0f;

            for( k = i+1; k < 8; k++ ) 
                Scratch[Pivot][k] /= a;

            // Eliminate pivot from all remaining rows.
            for( j = i+1; j < 4; j++ )
            {
                jr = Row[j];
                a  = -Scratch[jr][i];

                if( a == 0.0f ) 
                    continue;

                Scratch[jr][i] = 0.0f;

                for( k = i+1; k < 8; k++ )
                    Scratch[jr][k] += (a * Scratch[Pivot][k]);
            }
        }

        //
        // Back solve.
        //
        for( i = 3; i > 0; i-- )
        {
            Pivot = Row[i];
            for( j = i-1; j >= 0; j-- )
            {
                jr = Row[j];
                a  = Scratch[jr][i];

                for( k = i; k < 8; k++ )
                    Scratch[jr][k] -= (a * Scratch[Pivot][k]);
            }
        }

        //
        // Copy inverse back into the matrix.
        //
        for( j = 0; j < 4; j++ )
        {
            jr = Row[j];
            for( k = 0; k < 4; k++ )
            {
                m_Cell[j][k] = Scratch[jr][k+4];
            }
        }

    #endif

        // Success!
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    float matrix4::Determinant( void ) const noexcept
    {
        return  ( m_Cell[ 0 ][ 0 ] * m_Cell[ 1 ][ 1 ] - m_Cell[ 0 ][ 1 ] * m_Cell[ 1 ][ 0 ] ) * ( m_Cell[ 2 ][ 2 ] * m_Cell[ 3 ][ 3 ] - m_Cell[ 2 ][ 3 ] * m_Cell[ 3 ][ 2 ] ) -
                ( m_Cell[ 0 ][ 0 ] * m_Cell[ 1 ][ 2 ] - m_Cell[ 0 ][ 2 ] * m_Cell[ 1 ][ 0 ] ) * ( m_Cell[ 2 ][ 1 ] * m_Cell[ 3 ][ 3 ] - m_Cell[ 2 ][ 3 ] * m_Cell[ 3 ][ 1 ] ) +
                ( m_Cell[ 0 ][ 0 ] * m_Cell[ 1 ][ 3 ] - m_Cell[ 0 ][ 3 ] * m_Cell[ 1 ][ 0 ] ) * ( m_Cell[ 2 ][ 1 ] * m_Cell[ 3 ][ 2 ] - m_Cell[ 2 ][ 2 ] * m_Cell[ 3 ][ 1 ] ) +
                ( m_Cell[ 0 ][ 1 ] * m_Cell[ 1 ][ 2 ] - m_Cell[ 0 ][ 2 ] * m_Cell[ 1 ][ 1 ] ) * ( m_Cell[ 2 ][ 0 ] * m_Cell[ 3 ][ 3 ] - m_Cell[ 2 ][ 3 ] * m_Cell[ 3 ][ 0 ] ) -
                ( m_Cell[ 0 ][ 1 ] * m_Cell[ 1 ][ 3 ] - m_Cell[ 0 ][ 3 ] * m_Cell[ 1 ][ 1 ] ) * ( m_Cell[ 2 ][ 0 ] * m_Cell[ 3 ][ 2 ] - m_Cell[ 2 ][ 2 ] * m_Cell[ 3 ][ 0 ] ) +
                ( m_Cell[ 0 ][ 2 ] * m_Cell[ 1 ][ 3 ] - m_Cell[ 0 ][ 3 ] * m_Cell[ 1 ][ 2 ] ) * ( m_Cell[ 2 ][ 0 ] * m_Cell[ 3 ][ 1 ] - m_Cell[ 2 ][ 1 ] * m_Cell[ 3 ][ 0 ] );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::InvertSRT( void ) noexcept
    {
        const matrix4 Src( *this );
        float            D;

        //
        // Calculate the determinant.
        //
        D = ( Src.m_Cell[0][0] * ( Src.m_Cell[1][1] * Src.m_Cell[2][2] - Src.m_Cell[1][2] * Src.m_Cell[2][1] ) -
              Src.m_Cell[0][1] * ( Src.m_Cell[1][0] * Src.m_Cell[2][2] - Src.m_Cell[1][2] * Src.m_Cell[2][0] ) +
              Src.m_Cell[0][2] * ( Src.m_Cell[1][0] * Src.m_Cell[2][1] - Src.m_Cell[1][1] * Src.m_Cell[2][0] ) );

        xassert( math::Abs( D ) >= 0.00001f ) ;
        if( math::Abs( D ) < 0.00001f ) return *this;

        D = 1.0f / D;

        //
        // Find the inverse of the matrix.
        //
        m_Cell[0][0] =  D * ( Src.m_Cell[1][1] * Src.m_Cell[2][2] - Src.m_Cell[1][2] * Src.m_Cell[2][1] );
        m_Cell[0][1] = -D * ( Src.m_Cell[0][1] * Src.m_Cell[2][2] - Src.m_Cell[0][2] * Src.m_Cell[2][1] );
        m_Cell[0][2] =  D * ( Src.m_Cell[0][1] * Src.m_Cell[1][2] - Src.m_Cell[0][2] * Src.m_Cell[1][1] );
        m_Cell[0][3] = 0.0f;

        m_Cell[1][0] = -D * ( Src.m_Cell[1][0] * Src.m_Cell[2][2] - Src.m_Cell[1][2] * Src.m_Cell[2][0] );
        m_Cell[1][1] =  D * ( Src.m_Cell[0][0] * Src.m_Cell[2][2] - Src.m_Cell[0][2] * Src.m_Cell[2][0] );
        m_Cell[1][2] = -D * ( Src.m_Cell[0][0] * Src.m_Cell[1][2] - Src.m_Cell[0][2] * Src.m_Cell[1][0] );
        m_Cell[1][3] = 0.0f;

        m_Cell[2][0] =  D * ( Src.m_Cell[1][0] * Src.m_Cell[2][1] - Src.m_Cell[1][1] * Src.m_Cell[2][0] );
        m_Cell[2][1] = -D * ( Src.m_Cell[0][0] * Src.m_Cell[2][1] - Src.m_Cell[0][1] * Src.m_Cell[2][0] );
        m_Cell[2][2] =  D * ( Src.m_Cell[0][0] * Src.m_Cell[1][1] - Src.m_Cell[0][1] * Src.m_Cell[1][0] );
        m_Cell[2][3] = 0.0f;

        m_Cell[3][0] = -( Src.m_Cell[3][0] * m_Cell[0][0] + Src.m_Cell[3][1] * m_Cell[1][0] + Src.m_Cell[3][2] * m_Cell[2][0] );
        m_Cell[3][1] = -( Src.m_Cell[3][0] * m_Cell[0][1] + Src.m_Cell[3][1] * m_Cell[1][1] + Src.m_Cell[3][2] * m_Cell[2][1] );
        m_Cell[3][2] = -( Src.m_Cell[3][0] * m_Cell[0][2] + Src.m_Cell[3][1] * m_Cell[1][2] + Src.m_Cell[3][2] * m_Cell[2][2] );
        m_Cell[3][3] = 1.0f;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::InvertRT( void )  noexcept
    {
    #ifdef TARGET_PC_32BIT
        const matrix4 Src( *this );

        _asm
        {
            mov         eax,  dword ptr [this]

            //
            // transpose a matrix3x3 
            //
            movaps      xmm2, dword ptr [Src.m_Column1]
            shufps      xmm2, dword ptr [Src.m_Column2], _MM_SHUFFLE(1, 0, 1, 0)

            movaps      xmm3, dword ptr [Src.m_Column1]
            shufps      xmm3, dword ptr [Src.m_Column2], _MM_SHUFFLE(3, 2, 3, 2)

            movaps      xmm1, xmm2
            shufps      xmm1, dword ptr [Src.m_Column3], _MM_SHUFFLE(3, 0, 2, 0)
            movaps      xmmword ptr [eax].m_Column1, xmm1

            shufps      xmm2, dword ptr [Src.m_Column3], _MM_SHUFFLE(3, 1, 3, 1)
            movaps      xmmword ptr [eax].m_Column2, xmm2

            shufps      xmm3, dword ptr [Src.m_Column3], _MM_SHUFFLE(3, 2, 2, 0)
            movaps      xmmword ptr [eax].m_Column3, xmm3

            //
            // Do the inverse of the translation
            //
            movss       xmm1, dword ptr [Src.m_Column4+0]
            movss       xmm2, dword ptr [Src.m_Column4+4]
            movss       xmm3, dword ptr [Src.m_Column4+8]

            shufps      xmm1, xmm1, 0
            shufps      xmm2, xmm2, 0
            shufps      xmm3, xmm3, 0

            mulps       xmm1, dword ptr [eax].m_Column1      // Dest.m_Column1
            mulps       xmm2, dword ptr [eax].m_Column2      // Dest.m_Column2
            mulps       xmm3, dword ptr [eax].m_Column3      // Dest.m_Column3

            pxor        xmm0, xmm0                        // set to zero        

            addps       xmm1, xmm2
            addps       xmm1, xmm3
            subps       xmm0, xmm1

            //
            // Save it back to memory and set w
            //
            movaps      xmmword ptr [eax].m_Column4, xmm0        
            mov         dword ptr [eax].m_Column4+12, 3F800000h // set Dest.m_Column4.W equal to 1
        }

    #elif defined _XCORE_SSE4

        //
        // transpose a matrix3x3 
        //
        const matrix4 Src( *this );

        const floatx4 mm2 = _mm_shuffle_ps(Src.m_Column1, Src.m_Column2, _MM_SHUFFLE(1, 0, 1, 0));
        const floatx4 mm3 = _mm_shuffle_ps(Src.m_Column1, Src.m_Column2, _MM_SHUFFLE(3, 2, 3, 2));
        m_Column1       = _mm_shuffle_ps( mm2, Src.m_Column3, _MM_SHUFFLE(3, 0, 2, 0) );
        m_Column2       = _mm_shuffle_ps( mm2, Src.m_Column3, _MM_SHUFFLE(3, 1, 3, 1) );
        m_Column3       = _mm_shuffle_ps( mm3, Src.m_Column3, _MM_SHUFFLE(3, 2, 2, 0) );

        // Do the inverse of the translation
        m_Column4 = _mm_sub_ps( _mm_setzero_ps(), 
                    _mm_add_ps( _mm_mul_ps( m_Column1, _mm_set_ps1( ((float*)&Src.m_Column4)[0] ) ),
                    _mm_add_ps( _mm_mul_ps( m_Column2, _mm_set_ps1( ((float*)&Src.m_Column4)[1] ) ),
                                _mm_mul_ps( m_Column3, _mm_set_ps1( ((float*)&Src.m_Column4)[2] ) ))));

        m_Cell[3][3] = 1.0f;

    #else

        matrix4  Src(*this);
        matrix4& Dest = *this;

        Dest(0,0) = Src(0,0);
        Dest(0,1) = Src(1,0);
        Dest(0,2) = Src(2,0);
        Dest(1,0) = Src(0,1);
        Dest(1,1) = Src(1,1);
        Dest(1,2) = Src(2,1);
        Dest(2,0) = Src(0,2);
        Dest(2,1) = Src(1,2);
        Dest(2,2) = Src(2,2);
        Dest(0,3) = 0.0f;
        Dest(1,3) = 0.0f;
        Dest(2,3) = 0.0f;
        Dest(3,3) = 1.0f;
        Dest(3,0) = -(Src(3,0)*Dest(0,0) + Src(3,1)*Dest(1,0) + Src(3,2)*Dest(2,0));
        Dest(3,1) = -(Src(3,0)*Dest(0,1) + Src(3,1)*Dest(1,1) + Src(3,2)*Dest(2,1));
        Dest(3,2) = -(Src(3,0)*Dest(0,2) + Src(3,1)*Dest(1,2) + Src(3,2)*Dest(2,2));
    #endif

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::Rotate( const radian3& R ) noexcept
    {
        const matrix4 M( R );
        const matrix4 M2(*this);
        m4_Multiply( *this, M2, M );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setRotation( const radian3& R ) noexcept
    {
        float sx, cx;
        float sy, cy;
        float sz, cz;

        math::SinCos( R.m_Pitch, sx, cx );
        math::SinCos( R.m_Yaw,   sy, cy );
        math::SinCos( R.m_Roll,  sz, cz );

        // Fill out 3x3 rotations.
        const float sxsz = sx * sz;
        const float sxcz = sx * cz;

        m_Cell[0][0] = cy*cz + sy*sxsz;   m_Cell[0][1] = cx*sz;   m_Cell[0][2] = cy*sxsz - sy*cz;
        m_Cell[1][0] = sy*sxcz - sz*cy;   m_Cell[1][1] = cx*cz;   m_Cell[1][2] = sy*sz + sxcz*cy;
        m_Cell[2][0] = cx*sy;             m_Cell[2][1] = -sx;     m_Cell[2][2] = cx*cy;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bool matrix4::isIdentity( void ) const noexcept
    {
    #ifdef _XCORE_SSE4

        const floatx4 T4 = _mm_cmpneq_ps( m_Column1, m4_Identity.m_Column1 );
        const floatx4 T3 = _mm_cmpneq_ps( m_Column2, m4_Identity.m_Column2 );
        const floatx4 T2 = _mm_cmpneq_ps( m_Column3, m4_Identity.m_Column3 );
              floatx4 T1 = _mm_cmpneq_ps( m_Column4, m4_Identity.m_Column4 );

        T1 = _mm_add_ps( T1 , T2 );
        T1 = _mm_add_ps( T1 , T3 );
        T1 = _mm_add_ps( T1 , T4 );

        return reinterpret_cast<const float*>(&T1)[0] == 0 &&
               reinterpret_cast<const float*>(&T1)[1] == 0 &&
               reinterpret_cast<const float*>(&T1)[2] == 0 &&
               reinterpret_cast<const float*>(&T1)[3] == 0;

    #else

        return m4_Identity.getBack()          == getBack()    &&
               m4_Identity.getRight()         == getRight()   &&
               m4_Identity.getUp()            == getUp()      &&
               m4_Identity.getTranslation()   == getTranslation();

    #endif
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::Orthogonalize( void ) noexcept
    {
        vector3 VX( m_Column1 );
        vector3 VY( m_Column2 );
        vector3 VZ;    

        VX.Normalize();
        VY.Normalize();

        VZ = VX.Cross( VY );
        VY = VZ.Cross( VX );

        m_Column1 = VX.m_XYZW;
        m_Column2 = VY.m_XYZW;
        m_Column3 = VZ.m_XYZW;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bool matrix4::isValid( void ) const noexcept
    {
        const float* const p = reinterpret_cast<const float*>(this);
        for( std::int32_t i=0; i<16; i++ )
        {
            if( !math::isValid( p[i] ) )
                return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------------
    inline
    void matrix4::SanityCheck( void ) const noexcept
    {
    #ifdef _XCORE_DEBUG
        xassert( isValid() );

        vector3 Scale = getScale( );
        xassert( Scale.getLengthSquared() > 0.01f );
    
        quaternion Rotation( *this );
        xassert( Rotation.isValid() );

        vector3 Translation = getTranslation();
        xassert( Translation.isValid() );

        matrix4 Test;

        Test.setup( Scale, Rotation, Translation );

        for ( std::int32_t i = 0; i < 4 * 4; i++ )
        {
            const float a = reinterpret_cast<const float*>(this)[i];
            const float b = reinterpret_cast<const float*>(&Test)[i];
            xassert( math::Abs(a-b) < 0.1f );
        }
    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getScale( void ) const noexcept
    {
        return vector3( reinterpret_cast<const vector3*>(&m_Column1)->getLength(),
                         reinterpret_cast<const vector3*>(&m_Column2)->getLength(),
                         reinterpret_cast<const vector3*>(&m_Column3)->getLength() );
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getTranslation( void ) const noexcept
    {
        return vector3{ m_Column4 };
    }

    //------------------------------------------------------------------------------
    inline
    radian3 matrix4::getRotationR3( void ) const noexcept
    {
        // Do we have to deal with scales?
        // We loose precision and speed if we have to.
        const matrix4* pThis   = this;
        const float       l1      = reinterpret_cast<const vector3*>(&m_Column1)->getLengthSquared();
        const float       l2      = reinterpret_cast<const vector3*>(&m_Column2)->getLengthSquared();
        const float       l3      = reinterpret_cast<const vector3*>(&m_Column3)->getLengthSquared();

        xassert( l1 >= 0 );
        xassert( l2 >= 0 );
        xassert( l3 >= 0 );

        //
        // Deal with scale in the matrix
        //
        matrix4 M;
        if( l1 > (1 + 0.001f) || l1 < (1 - 0.001f) ||
            l2 > (1 + 0.001f) || l2 < (1 - 0.001f) ||
            l3 > (1 + 0.001f) || l3 < (1 - 0.001f) )
        {
            M = *this;
            M.ClearScale();
            pThis = &M;
        }

        //
        // Okay now solve the hold thing
        //
        const matrix4& MX = *pThis;
        radian         Roll  ;
        radian         Pitch ;
        radian         Yaw   ;

        if( MX.m_Cell[2][1] < 0.9998f )
        {
            if( MX.m_Cell[2][1] > -0.9998f )
            {
                Roll  =  math::ATan2( MX.m_Cell[0][1], MX.m_Cell[1][1] );
                Pitch = -math::ASin ( MX.m_Cell[2][1]);
                Yaw   =  math::ATan2( MX.m_Cell[2][0], MX.m_Cell[2][2]);
            }
            else
            {
                // WARNING.  Not unique.  ZA - YA = atan(r02,r00)
                Roll  = math::ATan2( MX.m_Cell[0][2], MX.m_Cell[0][0]);
                Pitch = PI_OVER2;
                Yaw   = radian{ 0.0f       };
            }
        }
        else
        {
            // WARNING.  Not unique.  ZA + YA = -atan2(r02,r00)
            Roll  = -math::ATan2( MX.m_Cell[0][2], MX.m_Cell[0][0]);
            Pitch = -PI_OVER2;
            Yaw   = radian{ 0.0f       };
        }

    //    if( Roll  < 0_deg ) Roll  = PI2 + Roll;
    //    if( Pitch < 0_deg ) Pitch = PI2 + Pitch;
    //    if( Yaw   < 0_deg ) Yaw   = PI2 + Yaw;

        return radian3( Pitch, Yaw, Roll );
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::ClearScale( void ) noexcept
    {
        PreScale( getScale().getOneOver() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::PreScale( float Scale ) noexcept
    {
    #ifdef _XCORE_SSE4
    
        const floatx4 GlobalScale = _mm_set_ps1( Scale );
        m_Column1 = _mm_mul_ps( m_Column1, GlobalScale );
        m_Column2 = _mm_mul_ps( m_Column2, GlobalScale );
        m_Column3 = _mm_mul_ps( m_Column3, GlobalScale );

    #else

        for( std::int32_t row = 0; row < 3; ++row )
        for( std::int32_t col = 0; col < 4; ++col )
        {
            m_Cell[row][col] *= Scale;
        }

    #endif
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::PreScale( const vector3& Scale ) noexcept
    {
    #ifdef _XCORE_SSE4
    
        const floatx4 WZYX = Scale.m_XYZW;
        m_Column1 = _mm_mul_ps( m_Column1, _mm_shuffle_ps( WZYX, WZYX, _MM_SHUFFLE(0,0,0,0))  );
        m_Column2 = _mm_mul_ps( m_Column2, _mm_shuffle_ps( WZYX, WZYX, _MM_SHUFFLE(1,1,1,1))  );
        m_Column3 = _mm_mul_ps( m_Column3, _mm_shuffle_ps( WZYX, WZYX, _MM_SHUFFLE(2,2,2,2))  );

    #else

        for( std::int32_t row = 0; row < 3; ++row )
        for( std::int32_t col = 0; col < 4; ++col )
        {
            m_Cell[row][col] *= Scale[row];
        }

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------

    inline
    matrix4& matrix4::setScale( float Scale ) noexcept
    {
        m_Cell[0][0] = Scale;
        m_Cell[1][1] = Scale;
        m_Cell[2][2] = Scale;

        return *this;
    }

    //------------------------------------------------------------------------------

    inline
    matrix4& matrix4::setScale( const vector3& Scale ) noexcept
    {
        m_Cell[0][0] = Scale.m_X;
        m_Cell[1][1] = Scale.m_Y;
        m_Cell[2][2] = Scale.m_Z;

        return *this;
    }

    //------------------------------------------------------------------------------

    inline 
    matrix4& matrix4::Scale( float Scale ) noexcept
    {
    #ifdef _XCORE_SSE4

        const floatx4 GlobalScale = _mm_set_ps1( Scale );

        // If we ever hit this we may need to have an if statement to make sure not to scale
        // anything in m_ColumnX.m128_float[3]
        xassert( reinterpret_cast<const float*>(&m_Column1)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column2)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column3)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column4)[3] == 1 );

        m_Column1 = _mm_mul_ps( m_Column1, GlobalScale );
        m_Column2 = _mm_mul_ps( m_Column2, GlobalScale );
        m_Column3 = _mm_mul_ps( m_Column3, GlobalScale );
        m_Column4 = _mm_mul_ps( m_Column4, GlobalScale );

        // Hack fix
        reinterpret_cast<float*>(&m_Column4)[3] = 1;

    #else

        for( std::int32_t row = 0; row < 4; ++row )
        for( std::int32_t col = 0; col < 4; ++col )
        {
            m_Cell[row][col] *= Scale;
        }

        m_Cell[3][3] = 1;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::Scale( const vector3d& Scale ) noexcept
    {
    #ifdef _XCORE_SSE4

        // If we ever hit this we may need to have an if statement to make sure not to scale
        // anything in m_ColumnX.m128_float[3]
        xassert( reinterpret_cast<const float*>(&m_Column1)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column2)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column3)[3] == 0 );
        xassert( reinterpret_cast<const float*>(&m_Column4)[3] == 1 );

        // DONE.
        const floatx4 WZYX = _mm_set_ps( 1, Scale.m_Z, Scale.m_Y, Scale.m_X ); 
        m_Column1 = _mm_mul_ps( m_Column1, WZYX );
        m_Column2 = _mm_mul_ps( m_Column2, WZYX );
        m_Column3 = _mm_mul_ps( m_Column3, WZYX );
        m_Column4 = _mm_mul_ps( m_Column4, WZYX );

    #else

        for( std::int32_t row = 0; row < 4; ++row )
        for( std::int32_t col = 0; col < 4; ++col )
        {
            m_Cell[row][col] *= Scale[col];
        }

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::ClearTranslation( void ) noexcept
    {
    #ifdef _XCORE_SSE4

        reinterpret_cast<float*>(&m_Column4)[0] = 
        reinterpret_cast<float*>(&m_Column4)[1] = 
        reinterpret_cast<float*>(&m_Column4)[2] = 0.0f;

    #else

        m_Cell[3][0] = m_Cell[3][1] = m_Cell[3][2] = m_Cell[3][3] = 0.0f;

    #endif

        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::ClearRotation( void ) noexcept
    {
        const vector3 S( getScale() );
        const vector3 T( getTranslation() );

        setIdentity();

        setScale( S );
        setTranslation( T );

        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3 matrix4::RotateVector( const vector3& V ) const noexcept
    {
    #if defined TARGET_PC_32BIT
        register vector3 Vdest;
        __asm 
        {
            mov         ecx, V                      ; V
            mov         edx, this                   ; matrix
            movss       xmm0, dword ptr [ecx]
            shufps      xmm0, xmm0, 0
            movss       xmm1, dword ptr [ecx+4]
            mulps       xmm0, xmmword ptr [edx]
            shufps      xmm1, xmm1, 0
            movss       xmm2, dword ptr [ecx+8]
            mulps       xmm1, xmmword ptr [edx+16]
            shufps      xmm2, xmm2, 0
            mulps       xmm2, xmmword ptr [edx+32]
            addps       xmm0, xmm1
            addps       xmm0, xmm2
            movaps      xmmword ptr Vdest, xmm0
        }
        return Vdest;

    #elif defined _XCORE_SSE4

        const floatx4 V1 = V.m_XYZW;
        return vector3( _mm_add_ps( _mm_add_ps(
                         _mm_mul_ps(m_Column1, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(0,0,0,0))), 
                         _mm_mul_ps(m_Column2, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(1,1,1,1)))), 
                         _mm_mul_ps(m_Column3, _mm_shuffle_ps(V1, V1, _MM_SHUFFLE(2,2,2,2)))));

    #else

        return vector3( (m_Cell[0][0]*V.m_X) + (m_Cell[1][0]*V.m_Y) + (m_Cell[2][0]*V.m_Z),
                         (m_Cell[0][1]*V.m_X) + (m_Cell[1][1]*V.m_Y) + (m_Cell[2][1]*V.m_Z),
                         (m_Cell[0][2]*V.m_X) + (m_Cell[1][2]*V.m_Y) + (m_Cell[2][2]*V.m_Z) );

    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    vector3 matrix4::InvRotateVector( const vector3& V ) const noexcept
    {
    #ifdef _XCORE_SSE4

        const floatx4 WZYX = V.m_XYZW;
        return vector3( _mm_add_ps( _mm_add_ps( 
                         _mm_mul_ps( m_Column1, WZYX ), 
                         _mm_mul_ps( m_Column2, WZYX )),
                         _mm_mul_ps( m_Column3, WZYX )) );
    #else

        return vector3( (m_Cell[0][0]*V.m_X) + (m_Cell[1][0]*V.m_X) + (m_Cell[2][0]*V.m_X),
                         (m_Cell[0][1]*V.m_Y) + (m_Cell[1][1]*V.m_Y) + (m_Cell[2][1]*V.m_Y),
                         (m_Cell[0][2]*V.m_Z) + (m_Cell[1][2]*V.m_Z) + (m_Cell[2][2]*V.m_Z));

    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getForward( void ) const noexcept
    {
        return -vector3{ m_Column3 };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 matrix4::getBack( void ) const noexcept
    {
        return vector3{ m_Column3 };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 matrix4::getUp( void ) const noexcept
    {
        return vector3{ m_Column2 };
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getDown( void ) const noexcept
    {
        return -vector3{ m_Column2 };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 matrix4::getRight( void ) const noexcept
    {
        return vector3{ m_Column1 };
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getLeft( void ) const noexcept
    {
        return -vector3{ m_Column1 };
    }

    //------------------------------------------------------------------------------
    inline 
    const matrix4& matrix4::operator += ( const matrix4& M ) noexcept
    {
    #ifdef _XCORE_SSE4

        m_Column1 = _mm_add_ps( m_Column1, M.m_Column1 );
        m_Column2 = _mm_add_ps( m_Column2, M.m_Column2 );
        m_Column3 = _mm_add_ps( m_Column3, M.m_Column3 );
        m_Column4 = _mm_add_ps( m_Column4, M.m_Column4 );

    #else

        for( std::int32_t row=0; row < 4; ++row )
        for( std::int32_t col=0; col < 4; ++col )
        {
            m_Cell[row][col] += M(col,row);
        }

    #endif
    
        return *this;

    }

    //------------------------------------------------------------------------------
    inline 
    const matrix4& matrix4::operator -= ( const matrix4& M ) noexcept
    {
    #ifdef _XCORE_SSE4

        m_Column1 = _mm_sub_ps( m_Column1, M.m_Column1 );
        m_Column2 = _mm_sub_ps( m_Column2, M.m_Column2 );
        m_Column3 = _mm_sub_ps( m_Column3, M.m_Column3 );
        m_Column4 = _mm_sub_ps( m_Column4, M.m_Column4 );

    #else

        for( std::int32_t row=0; row < 4; ++row )
        for( std::int32_t col=0; col < 4; ++col )
        {
            m_Cell[row][col] -= M(col,row);
        }

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const matrix4& matrix4::operator *= ( const matrix4& M ) noexcept
    {
        matrix4 Dest;
        m4_Multiply( Dest, *this, M );
        *this = Dest;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4 operator + ( const matrix4& M1, const matrix4& M2 ) noexcept
    {
    #ifdef _XCORE_SSE4

        return matrix4( _mm_add_ps( M1.m_Column1, M2.m_Column1 ),
                         _mm_add_ps( M1.m_Column2, M2.m_Column2 ),
                         _mm_add_ps( M1.m_Column3, M2.m_Column3 ),
                         _mm_add_ps( M1.m_Column4, M2.m_Column4 ) );
    #else

        matrix4 dest;
        for( std::int32_t row=0; row < 4; ++row )
        for( std::int32_t col=0; col < 4; ++col )
        {
            dest(col,row) = M1(col,row) + M2(col,row);
        }
    
        return dest;

    #endif
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4 operator - ( const matrix4& M1, const matrix4& M2 ) noexcept
    {
    #ifdef _XCORE_SSE4

        return matrix4( _mm_sub_ps( M1.m_Column1, M2.m_Column1 ),
                         _mm_sub_ps( M1.m_Column2, M2.m_Column2 ),
                         _mm_sub_ps( M1.m_Column3, M2.m_Column3 ),
                         _mm_sub_ps( M1.m_Column4, M2.m_Column4 ) );
    #else

        matrix4 dest;
        for( std::int32_t row=0; row < 4; ++row )
        for( std::int32_t col=0; col < 4; ++col )
        {
            dest(col,row) = M1(col,row) - M2(col,row);
        }

        return dest;

    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::Transform3D( const vector3& V ) const noexcept
    {
    #ifdef _XCORE_SSE4

        const float D = 1.0f / (  reinterpret_cast<const float*>(&m_Column1)[3] * V.m_X +
                                reinterpret_cast<const float*>(&m_Column2)[3] * V.m_Y +
                                reinterpret_cast<const float*>(&m_Column3)[3] * V.m_Z +
                                reinterpret_cast<const float*>(&m_Column4)[3] * 1     );
   
    #else
    
        const float D = 1.0f / (  m_Cell[0][3] * V.m_X + 
                                m_Cell[1][3] * V.m_Y +
                                m_Cell[2][3] * V.m_Z +
                                m_Cell[3][3] * 1     );
        
    #endif

        return ((*this) * V) * D;
    }

    //------------------------------------------------------------------------------
    inline
    vector4 matrix4::Transform3D( const vector4& V ) const noexcept
    {
        return ((*this) * V).Homogeneous();
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::LookAt( const vector3& From, const vector3& To, const vector3& Up ) noexcept
    {
        xassert( (From - To).getLengthSquared() >= 0.001f );            // It is too close from each other TODO: Fix this here
        xassert( math::Abs(Up.getLengthSquared() - 1.0f) <= 0.001f );       // This MUST be a unit vector

        vector3 Z( (From - To).Normalize() );
        vector3 X( Up.Cross( Z ).Normalize() );
        vector3 Y( Z.Cross( X ) );

    #if 0 //def _XCORE_SSE4
        // reinterpret_cast<float*>(&m_Column1)[0] =  X.m_X;
        // reinterpret_cast<float*>(&m_Column1)[1] =  Y.m_X;
        // reinterpret_cast<float*>(&m_Column1)[2] =  Z.m_X;
        // reinterpret_cast<float*>(&m_Column1)[3] =  0;
        m_Column1 = floatx4
        {
            X.m_X,
            Y.m_X,
            Z.m_X,
            0
        };

        // reinterpret_cast<float*>(&m_Column2)[0] =  X.m_Y;
        // reinterpret_cast<float*>(&m_Column2)[1] =  Y.m_Y;
        // reinterpret_cast<float*>(&m_Column2)[2] =  Z.m_Y;
        // reinterpret_cast<float*>(&m_Column2)[3] =  0;
        m_Column2 = floatx4
        {
            X.m_Y,
            Y.m_Y,
            Z.m_Y,
            0
        };

        // reinterpret_cast<float*>(&m_Column3)[0] =  X.m_Z;
        // reinterpret_cast<float*>(&m_Column3)[1] =  Y.m_Z;
        // reinterpret_cast<float*>(&m_Column3)[2] =  Z.m_Z;
        // reinterpret_cast<float*>(&m_Column3)[3] =  0;
        m_Column3 = floatx4
        {
            X.m_Z,
            Y.m_Z,
            Z.m_Z,
            0
        };

        // reinterpret_cast<float*>(&m_Column4)[0] = -X.Dot( From );
        // reinterpret_cast<float*>(&m_Column4)[1] = -Y.Dot( From );
        // reinterpret_cast<float*>(&m_Column4)[2] = -Z.Dot( From );
        // reinterpret_cast<float*>(&m_Column4)[3] = 1;
        m_Column4 = floatx4
        {
            -X.Dot( From ),
            -Y.Dot( From ),
            -Z.Dot( From ),
            1
        };

    #else

        m_Cell[0][0] = X.m_X;
        m_Cell[1][0] = X.m_Y;
        m_Cell[2][0] = X.m_Z;
        m_Cell[3][0] = -X.Dot( From );

        m_Cell[0][1] = Y.m_X;
        m_Cell[1][1] = Y.m_Y;
        m_Cell[2][1] = Y.m_Z;
        m_Cell[3][1] = -Y.Dot( From );

        m_Cell[0][2] = Z.m_X;
        m_Cell[1][2] = Z.m_Y;
        m_Cell[2][2] = Z.m_Z;
        m_Cell[3][2] = -Z.Dot( From );

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0;
        m_Cell[3][3] = 1;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::Billboard( const vector3& From, const vector3& To, const vector3& Up ) noexcept
    {
        vector3 Z( (From - To).Normalize() );
        vector3 X( Up.Cross( Z ).Normalize() );
        Z = X.Cross( Up );       

        m_Column1 = X.m_XYZW;
        m_Column2 = Up.m_XYZW;
        m_Column3 = Z.m_XYZW;
        m_Column4 = From.m_XYZW;

    #ifdef _XCORE_SSE4

        reinterpret_cast<float*>(&m_Column1)[3] = 0;
        reinterpret_cast<float*>(&m_Column2)[3] = 0;
        reinterpret_cast<float*>(&m_Column3)[3] = 0;
        reinterpret_cast<float*>(&m_Column4)[3] = 1;

    #else

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;

    #endif
    
        return *this;
    }


    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::OrthographicProjection( float Width, float Height, float Near, float Far ) noexcept
    {
    #ifdef _XCORE_SSE4

        // reinterpret_cast<float*>(&m_Column1)[0] =  2.0f/Width;
        // reinterpret_cast<float*>(&m_Column1)[1] =  0;
        // reinterpret_cast<float*>(&m_Column1)[2] =  0;
        // reinterpret_cast<float*>(&m_Column1)[3] =  0;
        m_Column1 = floatx4
        {
            2.0f/Width,
            0,
            0,
            0
        };

        // reinterpret_cast<float*>(&m_Column2)[0] =  0;
        // reinterpret_cast<float*>(&m_Column2)[1] =  2.0f/Height;
        // reinterpret_cast<float*>(&m_Column2)[2] =  0;
        // reinterpret_cast<float*>(&m_Column2)[3] =  0;
        m_Column2 = floatx4
        {
            0,
            2.0f/Height,
            0,
            0
        };

        // reinterpret_cast<float*>(&m_Column3)[0] =  0;
        // reinterpret_cast<float*>(&m_Column3)[1] =  0;
        // reinterpret_cast<float*>(&m_Column3)[2] =  1.0f/(Near-Far);
        // reinterpret_cast<float*>(&m_Column3)[3] =  0;
        m_Column3 = floatx4
        {
            0,
            0,
            1.0f/(Near-Far),
            0
        };

        // reinterpret_cast<float*>(&m_Column4)[0] =  0;
        // reinterpret_cast<float*>(&m_Column4)[1] =  0;
        // reinterpret_cast<float*>(&m_Column4)[2] =  Near/(Near-Far);
        // reinterpret_cast<float*>(&m_Column4)[3] =  1;
        m_Column4 = floatx4
        {
            0,
            0,
            Near/(Near-Far),
            1
        };

    #elif defined (TARGET_3DS)

        float Temp = 1.0f / Width;
        m_Cell[0][0] = 2.0f * Temp;
        m_Cell[1][0] = 0;
        m_Cell[2][0] = 0;
        m_Cell[3][0] = -Width * Temp;

        Temp = 1.0f / Height;
        m_Cell[1][1] = 2.0f * Temp;
        m_Cell[0][1] = 0;
        m_Cell[2][1] = 0;
        m_Cell[3][1] = -Height * Temp;

        m_Cell[2][2] = 1.0f / ( Far - Near);
        m_Cell[3][2] = Near / ( Far - Near);
        m_Cell[0][2] = m_Cell[1][2] = 0;

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0;
        m_Cell[3][3] = 1;

    #else

        m_Cell[0][0] = 2.0f / Width;
        m_Cell[1][0] = m_Cell[2][0] = m_Cell[3][0] = 0;

        m_Cell[1][1] = 2.0f / Height;
        m_Cell[0][1] = m_Cell[2][1] = m_Cell[3][1] = 0;

        m_Cell[2][2] = 1.0f / ( Near - Far);
        m_Cell[3][2] = Near / ( Near - Far);
        m_Cell[0][2] = m_Cell[1][2] = 0;

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0;
        m_Cell[3][3] = 1;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::OrthographicProjection( float Left, float Right, float Bottom, float Top, float Near, float Far ) noexcept
    {
    #ifdef _XCORE_SSE4

        const float Temp1 = 1.0f / ( Right - Left   );
        const float Temp2 = 1.0f / ( Top   - Bottom );
        const float Temp3 = 1.0f / ( Near  - Far    );

        // reinterpret_cast<float*>(&m_Column1)[0] =  2.0f * Temp1;
        // reinterpret_cast<float*>(&m_Column1)[1] =  0;
        // reinterpret_cast<float*>(&m_Column1)[2] =  0;
        // reinterpret_cast<float*>(&m_Column1)[3] =  0;
        m_Column1 = floatx4
        {
            2.0f * Temp1,
            0,
            0,
            0
        };

        // reinterpret_cast<float*>(&m_Column2)[0] =  0;
        // reinterpret_cast<float*>(&m_Column2)[1] =  2.0f * Temp2;
        // reinterpret_cast<float*>(&m_Column2)[2] =  0;
        // reinterpret_cast<float*>(&m_Column2)[3] =  0;
        m_Column2 = floatx4
        {
            0,
            2.0f * Temp2,
            0,
            0
        };

        // reinterpret_cast<float*>(&m_Column3)[0] =  0;
        // reinterpret_cast<float*>(&m_Column3)[1] =  0;
        // reinterpret_cast<float*>(&m_Column3)[2] =  1.0f * Temp3;
        // reinterpret_cast<float*>(&m_Column3)[3] =  0;
        m_Column3 = floatx4
        {
            0,
            0,
            1.0f * Temp3,
            0
        };

        // reinterpret_cast<float*>(&m_Column4)[0] =  -(Right + Left) * Temp1;
        // reinterpret_cast<float*>(&m_Column4)[1] =  -(Top + Bottom) * Temp2;
        // reinterpret_cast<float*>(&m_Column4)[2] =  Near * Temp3;
        // reinterpret_cast<float*>(&m_Column4)[3] =  1;
        m_Column4 = floatx4
        {
            -(Right + Left) * Temp1,
            -(Top + Bottom) * Temp2,
            Near            * Temp3,
            1
        };

    #elif defined (TARGET_3DS)
    
        float Temp = 1.0f / (Right - Left);
        m_Cell[0][0] = 2.0f * Temp;
        m_Cell[1][0] = 0;
        m_Cell[2][0] = 0;
        m_Cell[3][0] = -(Right + Left) * Temp;

        Temp = 1.0f / (Top - Bottom);
        m_Cell[1][1] = 2.0f * Temp;
        m_Cell[0][1] = 0;
        m_Cell[2][1] = 0;
        m_Cell[3][1] = -(Top + Bottom) * Temp;

        m_Cell[2][2] = 1.0f / ( Far - Near);
        m_Cell[3][2] = Near / ( Far - Near);
        m_Cell[0][2] = m_Cell[1][2] = 0;

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0;
        m_Cell[3][3] = 1;

    #else

        float Temp = 1.0f / (Right - Left);
        m_Cell[0][0] = 2.0f * Temp;
        m_Cell[1][0] = 0;
        m_Cell[2][0] = 0;
        m_Cell[3][0] = -(Right + Left) * Temp;

        Temp = 1.0f / (Top - Bottom);
        m_Cell[1][1] = 2.0f * Temp;
        m_Cell[0][1] = 0;
        m_Cell[2][1] = 0;
        m_Cell[3][1] = -(Top + Bottom) * Temp;

        m_Cell[2][2] = 1.0f / ( Near - Far);
        m_Cell[3][2] = Near / ( Near - Far);
        m_Cell[0][2] = m_Cell[1][2] = 0;

        m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0;
        m_Cell[3][3] = 1;

    #endif

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::PerspectiveProjection( float Left, float Right, float Bottom, float Top, float Near, float Far) noexcept
    {
    #if _XCORE_SSE4

        // m_Column1.m128_float[0] =  2.0f*Near / (Right - Left);
        // m_Column1.m128_float[1] =  0;
        // m_Column1.m128_float[2] =  0;
        // m_Column1.m128_float[3] =  0;
        m_Column1 = floatx4
        {
            2.0f*Near / (Right - Left),
            0,
            0,
            0
        };

        // m_Column2.m128_float[0] =  0;
        // m_Column2.m128_float[1] =  2.0f*Near / (Top - Bottom);
        // m_Column2.m128_float[2] =  0;
        // m_Column2.m128_float[3] =  0;
        m_Column2 = floatx4
        {
            0,
            2.0f*Near / (Top - Bottom),
            0,
            0
        };

        // m_Column3.m128_float[0] =  (Right + Left)/(Right - Left);
        // m_Column3.m128_float[1] =  (Top + Bottom)/(Top - Bottom);
        // m_Column3.m128_float[2] =  Far / (Far - Near);
        // m_Column3.m128_float[3] = -1;
        m_Column3 = floatx4
        {
            (Right + Left)/(Right - Left),
            (Top + Bottom)/(Top - Bottom),
            Far / (Far - Near),
            -1
        };
    
        // m_Column4.m128_float[0] =  0;
        // m_Column4.m128_float[1] =  0;
        // m_Column4.m128_float[2] =  (Near*Far) / (Far - Near);
        // m_Column4.m128_float[3] =  0;
        m_Column4 = floatx4
        {
            0,
            0,
            (Near*Far) / (Far - Near),
            0
        };
    
    #else

        m_Cell[0][0] = 2.0f * Near / (Right - Left);
        m_Cell[2][0] = (Right + Left) / (Right - Left);
        m_Cell[1][0] = m_Cell[3][0] = 0;

        m_Cell[1][1] = 2.0f * Near / (Top - Bottom);
        m_Cell[2][1] = (Top + Bottom) / (Top - Bottom);
        m_Cell[0][1] = m_Cell[3][1] = 0;

        m_Cell[2][2] = Far / (Far - Near);
        m_Cell[3][2] = (Near * Far) / (Far - Near);
        m_Cell[0][2] = m_Cell[1][2] = 0;

        m_Cell[2][3] = -1;
        m_Cell[0][3] = m_Cell[1][3] = m_Cell[3][3] = 0;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( const vector3& Axis, radian Angle ) noexcept
    {
        vector3 V( Axis );
        V.Normalize();

        float s, c;    
        math::SinCos( Angle, s, c );

        const float cb = (1.0f - c);
        const float ca = c + cb;
        const float sx = s * V.m_X;
        const float sy = s * V.m_Y;
        const float sz = s * V.m_Z;
        const float zx = V.m_Z * V.m_X;
        const float zy = V.m_Z * V.m_Y;
        const float xy = V.m_X * V.m_Y;

    #ifdef _XCORE_SSE4

        //  reinterpret_cast<float*>(&m_Column1)[0] = ca * V.m_X * V.m_X;
        //  reinterpret_cast<float*>(&m_Column1)[1] = cb * xy - sz;
        //  reinterpret_cast<float*>(&m_Column1)[2] = cb * zx + sy;
        //  reinterpret_cast<float*>(&m_Column1)[3] = 0;
        m_Column1 = floatx4
        {
            ca * V.m_X * V.m_X,
            cb * xy - sz,
            cb * zx + sy,
            0
        };

        //  reinterpret_cast<float*>(&m_Column2)[0] = cb * xy + sz;
        //  reinterpret_cast<float*>(&m_Column2)[1] = ca * V.m_Y * V.m_Y;
        //  reinterpret_cast<float*>(&m_Column2)[2] = cb * zy - sx;
        //  reinterpret_cast<float*>(&m_Column2)[3] = 0;
        m_Column2 = floatx4
        {
            cb * xy + sz,
            ca * V.m_Y * V.m_Y,
            cb * zy - sx,
            0
        };

        //  reinterpret_cast<float*>(&m_Column3)[0] = cb * zx - sy;
        //  reinterpret_cast<float*>(&m_Column3)[1] = cb * zy + sx;
        //  reinterpret_cast<float*>(&m_Column3)[2] = ca * V.m_Z * V.m_Z;
        //  reinterpret_cast<float*>(&m_Column3)[3] = 0;
        m_Column3 = floatx4
        {
            cb * zx - sy,
            cb * zy + sx,
            ca * V.m_Z * V.m_Z,
            0
        };

        //  reinterpret_cast<float*>(&m_Column4)[0] = 0;
        //  reinterpret_cast<float*>(&m_Column4)[1] = 0;
        //  reinterpret_cast<float*>(&m_Column4)[2] = 0;
        //  reinterpret_cast<float*>(&m_Column4)[3] = 1;
        m_Column4 = floatx4
        {
            0,
            0,
            1,
            0
        };

    #else

        // TODO UNIT TEST HERE
        m_Cell[0][0] = ca * V.m_X * V.m_X;
        m_Cell[0][1] = cb * xy - sz;
        m_Cell[0][2] = cb * zx + sy;
        m_Cell[0][3] = 0;
    
        m_Cell[1][0] = cb * xy + sz;
        m_Cell[1][1] = ca * V.m_Y * V.m_Y;
        m_Cell[1][2] = cb * zy - sx;
        m_Cell[1][3] = 0;
    
        m_Cell[2][0] = cb * zx - sy;
        m_Cell[2][1] = cb * zy + sx;
        m_Cell[2][2] = ca * V.m_Z * V.m_Z;
        m_Cell[2][3] = 0;
    
        m_Cell[3][0] = 0;
        m_Cell[3][1] = 0;
        m_Cell[3][2] = 0;
        m_Cell[3][3] = 1;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( const radian3& Rotation ) noexcept
    {                           
        setRotation( Rotation );

        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;
        m_Cell[3][2] = 0.0f;
        m_Cell[3][1] = 0.0f;
        m_Cell[3][0] = 0.0f;

        return *this;
    }

    //------------------------------------------------------------------------------

    inline
    matrix4& matrix4::setup( const quaternion& Q ) noexcept
    {                           
        setRotation( Q );

        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;
        m_Cell[3][2] = 0.0f;
        m_Cell[3][1] = 0.0f;
        m_Cell[3][0] = 0.0f;

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setRotation( const quaternion& Q ) noexcept
    {
        float tx  = 2.0f * Q.m_X;   // 2x
        float ty  = 2.0f * Q.m_Y;   // 2y
        float tz  = 2.0f * Q.m_Z;   // 2z
        float txw =   tx * Q.m_W;   // 2x * w
        float tyw =   ty * Q.m_W;   // 2y * w
        float tzw =   tz * Q.m_W;   // 2z * w
        float txx =   tx * Q.m_X;   // 2x * x
        float tyx =   ty * Q.m_X;   // 2y * x
        float tzx =   tz * Q.m_X;   // 2z * x
        float tyy =   ty * Q.m_Y;   // 2y * y
        float tzy =   tz * Q.m_Y;   // 2z * y
        float tzz =   tz * Q.m_Z;   // 2z * z

        // Fill out 3x3 rotations.
        m_Cell[0][0] = 1.0f-(tyy+tzz); m_Cell[0][1] = tyx + tzw;      m_Cell[0][2] = tzx - tyw;           
        m_Cell[1][0] = tyx - tzw;      m_Cell[1][1] = 1.0f-(txx+tzz); m_Cell[1][2] = tzy + txw;           
        m_Cell[2][0] = tzx + tyw;      m_Cell[2][1] = tzy - txw;      m_Cell[2][2] = 1.0f-(txx+tyy);    

        return *this;
    }


    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( const vector3& Scale,
                             const radian3& Rotation,
                             const vector3& Translation ) noexcept
    {
        setRotation( Rotation );
        setTranslation( Translation );

        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;

    #ifdef _XCORE_SSE4
        const floatx4 Scale_WZYX = Scale.m_XYZW;
        m_Column1 = _mm_mul_ps( m_Column1, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(0,0,0,0)) );
        m_Column2 = _mm_mul_ps( m_Column2, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(1,1,1,1)) );
        m_Column3 = _mm_mul_ps( m_Column3, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(2,2,2,2)) );
    
    #else

        m_Cell[0][0] *= Scale.m_X;	m_Cell[0][1] *= Scale.m_X;	m_Cell[0][2] *= Scale.m_X;
        m_Cell[1][0] *= Scale.m_Y;	m_Cell[1][1] *= Scale.m_Y;	m_Cell[1][2] *= Scale.m_Y;
        m_Cell[2][0] *= Scale.m_Z;	m_Cell[2][1] *= Scale.m_Z;	m_Cell[2][2] *= Scale.m_Z;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( const vector3&     Scale,
                             const quaternion&  Rotation,
                             const vector3&     Translation ) noexcept
    {

        setRotation( Rotation );
        setTranslation( Translation );

        m_Cell[0][3] = 0.0f;
        m_Cell[1][3] = 0.0f;
        m_Cell[2][3] = 0.0f;
        m_Cell[3][3] = 1.0f;

    #ifdef _XCORE_SSE4

        const floatx4 Scale_WZYX = Scale.m_XYZW;
        m_Column1 = _mm_mul_ps( m_Column1, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(0,0,0,0)) );
        m_Column2 = _mm_mul_ps( m_Column2, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(1,1,1,1)) );
        m_Column3 = _mm_mul_ps( m_Column3, _mm_shuffle_ps(Scale_WZYX, Scale_WZYX, _MM_SHUFFLE(2,2,2,2)) );

    #else

        m_Cell[0][0] *= Scale.m_X;	m_Cell[0][1] *= Scale.m_X;	m_Cell[0][2] *= Scale.m_X;
        m_Cell[1][0] *= Scale.m_Y;	m_Cell[1][1] *= Scale.m_Y;	m_Cell[1][2] *= Scale.m_Y;
        m_Cell[2][0] *= Scale.m_Z;	m_Cell[2][1] *= Scale.m_Z;	m_Cell[2][2] *= Scale.m_Z;

    #endif
    
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setup( const vector3& From, 
                             const vector3& To, 
                             radian         Angle ) noexcept
    {
        setup( To - From, Angle );
        PreTranslate( -From );
        Translate   (  From );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::PreRotate( const radian3& Rotation ) noexcept
    {
        matrix4 Dest;
        m4_Multiply( Dest, *this, matrix4( Rotation ) );
        *this = Dest;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    matrix4& matrix4::PreRotate( const quaternion& Q ) noexcept
    {
        matrix4 Dest;
        m4_Multiply( Dest, *this, matrix4( Q ) );
        *this = Dest;
        return *this;
    }


    //------------------------------------------------------------------------------
    inline 
    float matrix4::acof(std::int32_t r0, std::int32_t r1, std::int32_t r2, std::int32_t c0, std::int32_t c1, std::int32_t c2) const noexcept
    {
        return	m_Cell[r0][c0] * ( m_Cell[r1][c1] * m_Cell[r2][c2] - m_Cell[r2][c1] * m_Cell[r1][c2] ) -
                m_Cell[r0][c1] * ( m_Cell[r1][c0] * m_Cell[r2][c2] - m_Cell[r2][c0] * m_Cell[r1][c2] ) +
                m_Cell[r0][c2] * ( m_Cell[r1][c0] * m_Cell[r2][c1] - m_Cell[r2][c0] * m_Cell[r1][c1] );
    }

    //------------------------------------------------------------------------------
    // transpose of the Inverse of the Matrix
    inline
    matrix4 matrix4::getAdjoint( void ) const noexcept
    {
    #ifdef TARGET_PC_32BIT
        static const floatx4 SetWToZero = { 1, 1, 1, 0 }; 
        matrix4 Dest;
        _asm
        {
            // copy the vector that we will use to set 0 the w
            movaps  xmm4, SetWToZero

            // Handle the 1st row
            movaps  xmm0, [this+4*1]   //.m_Column2
            movaps  xmm2, xmm0
            movaps  xmm1, [this+4*2]   //.m_Column3
            movaps  xmm3, xmm1

            shufps  xmm0, xmm0, _MM_SHUFFLE(3, 0, 2, 1)
            shufps  xmm1, xmm1, _MM_SHUFFLE(3, 1, 0, 2)
            shufps  xmm2, xmm2, _MM_SHUFFLE(3, 1, 0, 2)
            shufps  xmm3, xmm3, _MM_SHUFFLE(3, 0, 2, 1)

            mulps   xmm0, xmm1
            mulps   xmm2, xmm3
            subps   xmm0, xmm2
            mulps   xmm0, xmm4 
            movaps  xmmword ptr Dest.m_Column1, xmm0

            // Handle the 2nd row
            movaps  xmm0, [this+4*2]   //.m_Column3
            movaps  xmm2, xmm0
            movaps  xmm1, [this+4*0]   //.m_Column1
            movaps  xmm3, xmm1

                shufps  xmm0, xmm0, _MM_SHUFFLE(3, 0, 2, 1)
                shufps  xmm1, xmm1, _MM_SHUFFLE(3, 1, 0, 2)
                shufps  xmm2, xmm2, _MM_SHUFFLE(3, 1, 0, 2)
                shufps  xmm3, xmm3, _MM_SHUFFLE(3, 0, 2, 1)

            mulps   xmm0, xmm1
            mulps   xmm2, xmm3
            subps   xmm0, xmm2
            mulps   xmm0, xmm4        
            movaps  xmmword ptr Dest.m_Column2, xmm0

            // Handle the 3rd row
            movaps  xmm0, [this+4*0]   //.m_Column1
            movaps  xmm2, xmm0
            movaps  xmm1, [this+4*1]   //.m_Column2
            movaps  xmm3, xmm1

                shufps  xmm0, xmm0, _MM_SHUFFLE(3, 0, 2, 1)
                shufps  xmm1, xmm1, _MM_SHUFFLE(3, 1, 0, 2)
                shufps  xmm2, xmm2, _MM_SHUFFLE(3, 1, 0, 2)
                shufps  xmm3, xmm3, _MM_SHUFFLE(3, 0, 2, 1)

            mulps   xmm0, xmm1
            mulps   xmm2, xmm3
            subps   xmm0, xmm2
            mulps   xmm0, xmm4        
            movaps  xmmword ptr Dest.m_Column3, xmm0
        }

        Dest.m_Column4 = m4_Identity.m_Column4;

    #elif defined _XCORE_SSE4
        matrix4 dest;

        const floatx4 t0 = _mm_shuffle_ps(m_Column1, m_Column1, _MM_SHUFFLE(3, 1, 0, 2));
        const floatx4 t1 = _mm_shuffle_ps(m_Column1, m_Column1, _MM_SHUFFLE(3, 0, 2, 1));
        const floatx4 t2 = _mm_shuffle_ps(m_Column2, m_Column2, _MM_SHUFFLE(3, 1, 0, 2));
        const floatx4 t3 = _mm_shuffle_ps(m_Column2, m_Column2, _MM_SHUFFLE(3, 0, 2, 1));
        const floatx4 t4 = _mm_shuffle_ps(m_Column3, m_Column3, _MM_SHUFFLE(3, 1, 0, 2));
        const floatx4 t5 = _mm_shuffle_ps(m_Column3, m_Column3, _MM_SHUFFLE(3, 0, 2, 1));
        dest.m_Column1    = _mm_sub_ps( _mm_mul_ps( t3, t4 ), _mm_mul_ps( t2, t5 ) );
        dest.m_Column2    = _mm_sub_ps( _mm_mul_ps( t5, t0 ), _mm_mul_ps( t4, t1 ) );
        dest.m_Column3    = _mm_sub_ps( _mm_mul_ps( t1, t2 ), _mm_mul_ps( t0, t3 ) );
        dest.m_Column4    = m4_Identity.m_Column4;

        return dest;

    #else

        matrix4 dest ( acof( 1, 2, 3, 1, 2, 3 ),  -acof( 0, 2, 3, 1, 2, 3 ),   acof( 0, 1, 3, 1, 2, 3 ),  -acof( 0, 1, 2, 1, 2, 3 ),
                       -acof( 1, 2, 3, 0, 2, 3 ),   acof( 0, 2, 3, 0, 2, 3 ),  -acof( 0, 1, 3, 0, 2, 3 ),   acof( 0, 1, 2, 0, 2, 3 ),
                        acof( 1, 2, 3, 0, 1, 3 ),  -acof( 0, 2, 3, 0, 1, 3 ),   acof( 0, 1, 3, 0, 1, 3 ),  -acof( 0, 1, 2, 0, 1, 3 ),
                       -acof( 1, 2, 3, 0, 1, 2 ),   acof( 0, 2, 3, 0, 1, 2 ),  -acof( 0, 1, 3, 0, 1, 2 ),   acof( 0, 1, 2, 0, 1, 2 ));

        dest.setTranslation(vector3(0,0,0));
        return dest;

    #endif
    }

    //------------------------------------------------------------------------------
    inline
    vector3 matrix4::getEulerZYZ( void ) const noexcept
    {
        vector3 ZYZ;

        ZYZ.m_W = 0;
        ZYZ.m_Y = math::ATan2( math::Sqrt( math::Sqr(m_Cell[0][2]) + math::Sqr(m_Cell[1][2])), m_Cell[2][2]).m_Value;

        if( math::Abs( ZYZ.m_Y ) < 0.0001f )
        {
            ZYZ.m_X = 0.0;
            ZYZ.m_Y = 0.0;          
            ZYZ.m_Z = math::ATan2(-m_Cell[1][0], m_Cell[0][0] ).m_Value;
        }
        else if( math::Abs( radian{ ZYZ.m_Y } - PI ) < radian{ 0.0001f } )
        {
            ZYZ.m_X = 0.0;
            ZYZ.m_Y = PI.m_Value;           
            ZYZ.m_Z = math::ATan2( m_Cell[1][0], -m_Cell[0][0] ).m_Value;
        }
        else
        {
            ZYZ.m_X = math::ATan2( m_Cell[2][1],  m_Cell[2][0] ).m_Value;
            ZYZ.m_Z = math::ATan2( m_Cell[1][2], -m_Cell[0][2] ).m_Value;
        }

        return ZYZ;
    }

    //------------------------------------------------------------------------------
    inline
    matrix4& matrix4::setupFromZYZ( const vector3d& ZYZ ) noexcept
    {
        float sa, sb, sg;
        float ca, cb, cg;

        math::SinCos( radian{ ZYZ.m_X }, sa, ca );
        math::SinCos( radian{ ZYZ.m_Y }, sb, cb );
        math::SinCos( radian{ ZYZ.m_Z }, sg, cg );

        setIdentity();
        m_Cell[0][0] =  ca * cb * cg - sa * sg;
        m_Cell[1][0] = -ca * cb * sg - sa * cg;
        m_Cell[2][0] =  ca * sb;

        m_Cell[0][1] =  sa * cb * cg + ca * sg;
        m_Cell[1][1] = -sa * cb * sg + ca * cg;
        m_Cell[2][1] =  sa * sb;

        m_Cell[0][2] = -sb * cg;
        m_Cell[1][2] =  sb * sg;
        m_Cell[2][2] =  cb;

        return *this;
    }




}