
namespace xcore::math
{
    //------------------------------------------------------------------------------

    constexpr
    vector3d::vector3d( const floatx4& Register ) noexcept 
        : m_X{ xcore::sse::getElementByIndex<0>(Register) }
        , m_Y{ xcore::sse::getElementByIndex<1>(Register) }
        , m_Z{ xcore::sse::getElementByIndex<2>(Register) } {}

    //------------------------------------------------------------------------------
    inline
    float* vector3d::operator()( void ) noexcept
    {
        return &m_X;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3d::vector3d( const vector3& V ) noexcept : m_X(V.m_X), m_Y(V.m_Y), m_Z(V.m_Z) {}

    //------------------------------------------------------------------------------
    constexpr
    float vector3d::operator []( const std::int32_t i ) const noexcept
    {
        xassert( i >= 0 && i <= 3 );
        return (&m_X)[i];
    }

    //------------------------------------------------------------------------------
    inline
    float& vector3d::operator []( const std::int32_t i ) noexcept
    {
        xassert( i >= 0 && i <= 3 );
        return (&m_X)[i];
    }

    //------------------------------------------------------------------------------
    inline 
    void vector3d::setZero( void ) noexcept
    {
        m_X=m_Y=m_Z=0;
    }

    //------------------------------------------------------------------------------
    inline
    void vector3d::setIdentity( void ) noexcept
    {
        m_X=m_Y=m_Z=0;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3d& vector3d::setup( float X, float Y, float Z ) noexcept
    {
        m_X = X;
        m_Y = Y;
        m_Z = Z;
	    xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3d& vector3d::setup( const float n ) noexcept
    {
        m_X = n;
        m_Y = n;
        m_Z = n;
        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    float vector3d::getLength( void ) const noexcept
    {
        return math::Sqrt( m_X*m_X + m_Y*m_Y + m_Z*m_Z );
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector3d::getLengthSquared( void ) const noexcept
    {
        return m_X*m_X + m_Y*m_Y + m_Z*m_Z;
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector3d::getDistanceSquare( const vector3d& V ) const noexcept
    {
        return math::Sqr(m_X - V.m_X) +
               math::Sqr(m_Y - V.m_Y) +
               math::Sqr(m_Z - V.m_Z);
    }

    //------------------------------------------------------------------------------
    inline
    float vector3d::getDistance( const vector3d& V ) const noexcept
    {
        return math::Sqrt( getDistanceSquare(V) );
    }

    //------------------------------------------------------------------------------
    inline 
    vector3d& vector3d::Normalize( void ) noexcept
    {
        const float imag = math::InvSqrt( m_X*m_X + m_Y*m_Y + m_Z*m_Z );
        m_X *= imag;
        m_Y *= imag;
        m_Z *= imag;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3d& vector3d::NormalizeSafe( void ) noexcept
    {
        const float sqrdis = m_X*m_X + m_Y*m_Y + m_Z*m_Z;

        if( sqrdis < 0.0001f )
        {
            m_X = 1;
            m_Y = 0;
            m_Z = 0;
            return *this;
        }

	    const float imag = math::InvSqrt( sqrdis );

        m_X *= imag;
        m_Y *= imag;
        m_Z *= imag;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3d& vector3d::operator += ( const vector3d& V ) noexcept
    {
        m_X += V.m_X;
        m_Y += V.m_Y;
        m_Z += V.m_Z;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3d& vector3d::operator -= ( const vector3d& V ) noexcept
    {
        m_X -= V.m_X;
        m_Y -= V.m_Y;
        m_Z -= V.m_Z;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3d& vector3d::operator *= ( const vector3d& V ) noexcept
    {
        m_X *= V.m_X;
        m_Y *= V.m_Y;
        m_Z *= V.m_Z;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3d& vector3d::operator *= ( float Scalar ) noexcept
    {
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const vector3d& vector3d::operator /= ( float Div ) noexcept
    {
        const float Scalar = 1.0f/Div;
        m_X *= Scalar;
        m_Y *= Scalar;
        m_Z *= Scalar;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    vector3 operator / ( const vector3d& V, float Div ) noexcept
    {
        const float Scalar = 1.0f/Div;
        return vector3( V.m_X * Scalar, V.m_Y * Scalar, V.m_Z * Scalar );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3d& V, float Scalar ) noexcept
    {
        return vector3( V.m_X * Scalar, V.m_Y * Scalar, V.m_Z * Scalar );
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( float Scalar, const vector3d& V ) noexcept
    {
        return vector3( V.m_X * Scalar, V.m_Y * Scalar, V.m_Z * Scalar );
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool vector3d::operator == ( const vector3d& V ) const noexcept
    {
        return 
        (math::Abs( V.m_X - m_X) <= XFLT_TOL) &&
        (math::Abs( V.m_Y - m_Y) <= XFLT_TOL) &&
        (math::Abs( V.m_Z - m_Z) <= XFLT_TOL);
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3d::getMin( const vector3d& V ) const noexcept
    {
        return { math::Min( V.m_X, m_X ), math::Min( V.m_Y, m_Y ), math::Min( V.m_Z, m_Z ) };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3d::getMax( const vector3d& V ) const noexcept
    {
        return { math::Max( V.m_X, m_X ), math::Max( V.m_Y, m_Y ), math::Max( V.m_Z, m_Z ) };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator + ( const vector3d& V0, const vector3d& V1 ) noexcept
    {
        return { V0.m_X + V1.m_X, V0.m_Y + V1.m_Y, V0.m_Z + V1.m_Z };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3d& V0, const vector3d& V1 ) noexcept
    {
        return { V0.m_X - V1.m_X, V0.m_Y - V1.m_Y, V0.m_Z - V1.m_Z };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator * ( const vector3d& V0, const vector3d& V1 ) noexcept
    {
        return { V0.m_X * V1.m_X, V0.m_Y * V1.m_Y, V0.m_Z * V1.m_Z };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator / ( const vector3d& V0, const vector3d& V1 ) noexcept
    {
        return { V0.m_X / V1.m_X, V0.m_Y / V1.m_Y, V0.m_Z / V1.m_Z };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 operator - ( const vector3d& V ) noexcept
    {
        return { -V.m_X, -V.m_Y, -V.m_Z };
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3d::getLerp( float t, const vector3d& V ) const noexcept
    {
        return { m_X + (( V.m_X - m_X ) * t),
                 m_Y + (( V.m_Y - m_Y ) * t),
                 m_Z + (( V.m_Z - m_Z ) * t) };
    }

    //------------------------------------------------------------------------------
    constexpr 
    float vector3d::Dot( const vector3d& V ) const noexcept
    {
        return V.m_X * m_X + V.m_Y * m_Y + V.m_Z * m_Z;
    }

    //------------------------------------------------------------------------------
    constexpr 
    vector3 vector3d::Cross( const vector3d& V ) const noexcept
    {
        return { m_Y * V.m_Z - m_Z * V.m_Y,
                 m_Z * V.m_X - m_X * V.m_Z,
                 m_X * V.m_Y - m_Y * V.m_X };   
    }


    //------------------------------------------------------------------------------
    xforceinline 
    bool vector3d::isValid( void ) const noexcept
    {
        return math::isValid(m_X) && math::isValid(m_Y) && math::isValid(m_Z);
    }

    //------------------------------------------------------------------------------
    inline
    vector3d& vector3d::Abs( void ) noexcept
    {
         m_X = math::Abs( m_X );
         m_Y = math::Abs( m_Y );
         m_Z = math::Abs( m_Z );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool vector3d::isInRange( const float Min, const float Max ) const noexcept
    {
        return math::isInRange( m_X, Min, Max ) && math::isInRange( m_Y, Min, Max ) && math::isInRange( m_Z, Min, Max );
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 vector3d::getOneOver( void ) const noexcept
    {
        return vector3( 1.0f/m_X, 1.0f/m_Y, 1.0f/m_Z  );
    }

    /*
    //------------------------------------------------------------------------------
    inline
    vector3d vector3d::getEulerZYZ( void ) const noexcept
    {
        // http://vered.rose.utoronto.ca/people/david_dir/GEMS/GEMS.html
    }
    */
}
