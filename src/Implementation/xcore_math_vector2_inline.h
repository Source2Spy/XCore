
namespace xcore::math
{
    //------------------------------------------------------------------------------
    constexpr
    vector2 operator - ( const vector2& V1, const vector2& V2 ) noexcept
    {
        return { V1.m_X - V2.m_X, 
                 V1.m_Y - V2.m_Y };
    }

    //------------------------------------------------------------------------------
    inline 
    vector2& vector2::setup( float X, float Y ) noexcept
    {
        m_X = X; 
        m_Y = Y;
        xassert( isValid() );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    void vector2::setZero( void ) noexcept
    {
        m_X = 0; 
        m_Y = 0;
    }

    //------------------------------------------------------------------------------
    inline 
    float vector2::getLength( void ) const noexcept
    {
        return math::Sqrt( m_X*m_X + m_Y*m_Y );
    }

    //------------------------------------------------------------------------------
    constexpr 
    float vector2::getLengthSquared( void ) const noexcept
    {
        return m_X*m_X + m_Y*m_Y;
    }

    //------------------------------------------------------------------------------
    inline 
    radian vector2::getAngle( void ) const noexcept
    {
        return math::ATan2( m_Y, m_X );
    }

    //------------------------------------------------------------------------------
    inline 
    radian vector2::getAngleBetween( const vector2& V ) const noexcept
    {
        float D, Cos;

        D = getLength() * V.getLength();

        if( math::Abs(D) < 0.00001f ) return radian{ 0.0_xdeg };

        Cos = Dot( V ) / D;

        if     ( Cos >  1.0f )  Cos =  1.0f;
        else if( Cos < -1.0f )  Cos = -1.0f;

        return math::ACos( Cos );
    }

    //------------------------------------------------------------------------------
    inline 
    float vector2::getDistance( const vector2& V ) const noexcept
    {
        return math::Sqr(m_X - V.m_X) + math::Sqr(m_Y - V.m_Y);
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool vector2::isInRange( float Min, float Max ) const noexcept
    {
        return math::isInRange(m_X, Min, Max) &&
               math::isInRange(m_Y, Min, Max);
    }

    //------------------------------------------------------------------------------
    xforceinline
    bool vector2::isValid( void ) const noexcept
    {
        return math::isValid(m_X) && math::isValid(m_Y);
    }

    //------------------------------------------------------------------------------
    inline
    vector2& vector2::Rotate( radian Angle ) noexcept
    {
        float  S, C; 
        math::SinCos( Angle, S, C );

        float tX = m_X;
        float tY = m_Y;

        setup( (C * tX) - (S * tY), (C * tY) + (S * tX) );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 vector2::getLerp( float t, const vector2& V1 ) const noexcept
    {
        return { m_X + (( V1.m_X - m_X ) * t),
                 m_Y + (( V1.m_Y - m_Y ) * t) };

    }

    //------------------------------------------------------------------------------
    // Returns the closest point on the 2D LINE defined by line_v1 and line_v2
    // to the point pt.  Note that output is NOT necissarily between line_v1
    // and line_v2.
    //
    // Reference: http://astronomy.swin.edu.au/~pbourke/geometry/pointline/
    //
    // out:		(output) Closest point on the line.
    // pt:		Point
    // line_v1:	First point on the line.
    // line_v2: Second point on the line.
    //------------------------------------------------------------------------------
    inline
    vector2 vector2::getClosestPointInLine( const vector2 &V0, const vector2 &V1 ) const noexcept
    {
        // safety checks
        xassert( (V0.m_X != V1.m_X) && (V0.m_Y != V1.m_Y) );

        float u = (m_X - V1.m_X) * (V1.m_X - V0.m_X) + (m_Y - V0.m_Y) * (V1.m_Y - V0.m_Y);
        u /= (V0 - V1).getLengthSquared();

        return V0.getLerp( u, V1 );
    }

    //------------------------------------------------------------------------------
    // Returns the closest point on the 2D LINESEGMENT defined by line_v1 and line_v2
    // to the point pt.  Note that output WILL BE between line_v1 and line_v2 (or 
    // equal to one of them).
    //
    // out:		(output) Closest point on the line segment.
    // pt:		Point
    // line_v1: Endpoint of the line segment.
    // line_v2: Endpoint of the line segment.
    //------------------------------------------------------------------------------
    inline
    vector2 vector2::getClosestPointInLineSegment( const vector2 &V0, const vector2 &V1 ) const noexcept
    {
        // degenerate case
        if( V0 == V1 ) 
        {
            return V0;
        }
    
        float u = (m_X - V1.m_X) * (V1.m_X - V0.m_X) + (m_Y - V0.m_Y) * (V1.m_Y - V0.m_Y);
        u /= (V0 - V1).getLengthSquared();
    
        // cap u to the range [0..1]
        u = math::Range( u, 0.0f, 1.0f );
    
        return V0.getLerp( u, V1 );
    }

    //------------------------------------------------------------------------------

    constexpr           
    vector2 vector2::getMin ( const vector2& V ) const noexcept
    {
        return { math::Min( m_X, V.m_X ), math::Min( m_Y, V.m_Y ) };
    }

    //------------------------------------------------------------------------------

    constexpr
    vector2 vector2::getMax( const vector2& V ) const noexcept
    {
        return { math::Max( m_X, V.m_X ), math::Max( m_Y, V.m_Y ) };
    }

    //------------------------------------------------------------------------------

    constexpr
    vector2 vector2::LimitLength( float MaxLength ) const noexcept
    {
        auto l = getLengthSquared();
        if( l <= (MaxLength*MaxLength) ) return *this;
        return (*this) * ( MaxLength * xcore::InvSqrt(l) );
    }

    //------------------------------------------------------------------------------
    xforceinline
    vector2& vector2::setLength( float Length ) noexcept
    {
        Normalize() *= Length;
        return *this;
    }


    //------------------------------------------------------------------------------
    // Determines which side of a line a is point on.
    //
    // Note that the value returned divided by the distance from line_v1 to line_v2
    // is the minimum distance from the point to the line.  That may be useful 
    // for determining the distance relationship between points, without actually
    // having to calculate the distance.
    //
    // pt:		Point
    // line_v1: Endpoint of the line
    // line_v2: Endpoint of the line
    //
    // returns: > 0.0f if pt is to the left of the line (line_v1->line_v2)
    //				< 0.0f if pt is to the right of the line (line_v1->line_v2)
    //				= 0.0f if pt is on the line
    //
    //------------------------------------------------------------------------------
    constexpr
    float vector2::getWhichSideOfLine( const vector2& V0, const vector2& V1 ) const noexcept
    {
	    return ((m_Y - V0.m_Y) * (V1.m_X - V0.m_X) - (m_X - V0.m_X) * (V1.m_Y - V0.m_Y));
    }

    //------------------------------------------------------------------------------
    constexpr
    float vector2::Dot( const vector2& V ) const noexcept
    {
        return (m_X*V.m_X) + (m_Y*V.m_Y);
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 vector2::Reflect( const vector2& Normal, const vector2& Vector ) noexcept
    {
        return Vector - 2 * Normal * (Normal.Dot(Vector));
    }

    //------------------------------------------------------------------------------
    constexpr
    bool vector2::operator == ( const vector2& V ) const noexcept
    {
        return !( (math::Abs( V.m_X - m_X) > XFLT_TOL) ||
                  (math::Abs( V.m_Y - m_Y) > XFLT_TOL)
                );
    }

    //------------------------------------------------------------------------------
    inline
    const vector2& vector2::operator += ( const vector2& V ) noexcept
    {
        setup( m_X + V.m_X, m_Y + V.m_Y );
	     return (*this);
    }

    //------------------------------------------------------------------------------
    inline
    const vector2& vector2::operator -= ( const vector2& V ) noexcept
    {
        setup( m_X - V.m_X, m_Y - V.m_Y );
	     return (*this);
    }

    //------------------------------------------------------------------------------
    inline
    const vector2& vector2::operator *= ( const vector2& V ) noexcept
    {
        setup( m_X * V.m_X, m_Y * V.m_Y );
	     return (*this);
    }

    //------------------------------------------------------------------------------
    inline
    const vector2& vector2::operator *= ( float Scalar ) noexcept
    {
        setup( m_X * Scalar, m_Y * Scalar );
	     return (*this);
    }

    //------------------------------------------------------------------------------
    inline
    const vector2& vector2::operator /= ( float Div ) noexcept
    {
        float Scalar = 1.0f/Div;
        setup( m_X * Scalar, m_Y * Scalar );
	     return (*this);
    }

    //------------------------------------------------------------------------------
    inline
    vector2& vector2::Normalize( void ) noexcept
    {
        const float div = math::InvSqrt( m_X*m_X + m_Y*m_Y );
        m_X *= div;
        m_Y *= div;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    vector2& vector2::NormalizeSafe( void ) noexcept
    {
        const float sqrdis = m_X*m_X + m_Y*m_Y;
        if( sqrdis < 0.0001f )
        {
            m_X = 1;
            m_Y = 0;
            return *this;
        }
	    float imag = math::InvSqrt( sqrdis );

        m_X *= imag;
        m_Y *= imag;
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator + ( const vector2& V1, const vector2& V2 ) noexcept
    {
        return { V1.m_X + V2.m_X, V1.m_Y + V2.m_Y };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator - ( const vector2& V ) noexcept
    {
        return { -V.m_X, -V.m_Y };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator / ( const vector2& V, float S ) noexcept
    {
        S = 1.0f/S;
        return { V.m_X*S, V.m_Y*S };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator * ( const vector2& V, float S ) noexcept
    {
        return { V.m_X*S, V.m_Y*S };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator * ( float S, const vector2& V ) noexcept
    {
        return { V.m_X*S, V.m_Y*S };
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 operator * ( const vector2& V1, const vector2& V2 ) noexcept
    {
        return { V1.m_X * V2.m_X, 
                 V1.m_Y * V2.m_Y };

    }
}