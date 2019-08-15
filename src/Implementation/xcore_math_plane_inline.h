namespace xcore::math
{
    //------------------------------------------------------------------------------
    constexpr 
    plane::plane( const vector3& P1, const vector3& P2, const vector3& P3 ) noexcept
        : m_Normal { (P2-P1).Cross(P3-P1).Normalize()   }
        , m_D      { -m_Normal.Dot( P1 )                }
    {}

    //------------------------------------------------------------------------------
    constexpr 
    plane::plane( const vector3d& Normal, float Distance ) noexcept 
        : m_Normal( Normal )
        , m_D( Distance ) 
    {}

    //------------------------------------------------------------------------------
    constexpr 
    plane::plane( float A, float B, float C, float D ) noexcept
        : m_Normal( A, B, C )
        , m_D( D ) 
    {}

    //------------------------------------------------------------------------------
    inline
    plane::plane( const vector3d& Normal, const vector3d& Point ) noexcept
    {
        setup( Normal, Point );
    }

    //------------------------------------------------------------------------------
    inline
    plane& plane::setup( float A, float B, float C, float D ) noexcept
    {
        m_Normal.setup( A, B, C );
        m_Normal.Normalize();
        m_D = D;
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    void plane::ComputeD( const vector3d& P ) noexcept
    {
        m_D = -m_Normal.Dot( P );
    }

    //------------------------------------------------------------------------------
    inline
    plane& plane::setup( const vector3& P1, const vector3& P2, const vector3& P3 ) noexcept
    {
        m_Normal = (P2-P1).Cross(P3-P1).Normalize();
        m_D      = -m_Normal.Dot( P1 );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    plane& plane::setup( const vector3d& Normal, float Distance ) noexcept
    {
        m_Normal = Normal;
        m_Normal.Normalize();
        m_D      = Distance;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    plane& plane::setup( const vector3d& Normal, const vector3d& Point ) noexcept
    {
        m_Normal = Normal;
        m_Normal.Normalize();
        m_D = -Normal.Dot( Point );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void plane::getOrthovectors ( vector3d& AxisA,
                                  vector3d& AxisB ) const noexcept
    {
        float      AbsA, AbsB, AbsC;
        vector3 Dir;

        // get a non-parallel axis to normal.
        AbsA = math::Abs( m_Normal.m_X );
        AbsB = math::Abs( m_Normal.m_Y );
        AbsC = math::Abs( m_Normal.m_Z );

        if( (AbsA<=AbsB) && (AbsA<=AbsC) ) Dir.setup(1,0,0);
        else
            if( (AbsB<=AbsA) && (AbsB<=AbsC) ) Dir.setup(0,1,0);
            else                               Dir.setup(0,0,1);

        AxisA = m_Normal.Cross(Dir).Normalize();
        AxisB = m_Normal.Cross(AxisA).Normalize();
    }

    //------------------------------------------------------------------------------
    constexpr 
    float plane::Dot( const vector3d& P ) const noexcept
    {
        return m_Normal.Dot( P );
    }

    //------------------------------------------------------------------------------
    constexpr 
    float plane::getDistance( const vector3d& P ) const noexcept
    {
        return m_Normal.Dot( P ) + m_D;
    }

    //------------------------------------------------------------------------------
    constexpr 
    std::int32_t plane::getWhichSide( const vector3d& P ) const noexcept
    {
        float Distance = m_Normal.Dot( P ) + m_D;
        if( Distance < -0.99f ) return -1;
        return Distance > 0.01f;
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool plane::IntersectLineSegment( float& T, const vector3d& P0, const vector3d& P1 ) const noexcept
    {
        T = (P1 - P0).Dot( m_Normal );

        if( T == 0.0f ) 
            return false;

        T = -getDistance( P0 ) / T;

        return true;
    }

    //------------------------------------------------------------------------------
    constexpr 
    bool plane::IntersectLine( float& T, const vector3d& Start, const vector3d& Direction ) const noexcept
    {
        float dist = getDistance( Start );

	    // behind plane
        if( dist < 0.0f ) 
            return false ;

	    float len = -Direction.Dot( m_Normal );
	    if( len < dist ) 
        {
		    // moving away from plane or point too far away
		    return false;
	    }

	    T = dist/len;
	    return true;
    }

    //------------------------------------------------------------------------------
    constexpr
    void plane::getComponents( const vector3d&    V,
                               vector3d&          Parallel,
                               vector3d&          Perpendicular ) const noexcept
    {
        Perpendicular = m_Normal.Dot( V ) * m_Normal;
        Parallel      = V - Perpendicular;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 plane::getReflection( const vector3& V ) const noexcept
    {
        return vector3( m_Normal ).getReflection( V );
    }

    //------------------------------------------------------------------------------
    inline 
    bool plane::isValid( void ) const noexcept
    {
        return m_Normal.isValid() && math::isValid(m_D);
    }

    //------------------------------------------------------------------------------
    inline
    bool plane::ClipNGon( vector3d*       pDst, 
                          std::int32_t&   nDstVerts,
                          const vector3d* pSrc, 
                          std::int32_t    nSrcVerts ) const noexcept
    {
        float   D0, D1;
        std::int32_t   P0, P1;
        bool bClipped = false;

        nDstVerts = 0;
        P1 = nSrcVerts-1;
        D1 = getDistance( pSrc[P1] );

        for( std::int32_t i=0; i<nSrcVerts; i++ ) 
        {
            P0 = P1;
            D0 = D1;
            P1 = i;
            D1 = getDistance(pSrc[P1]);

            // Do we keep starting vert?
            if( D0 >= 0 )
            {
                pDst[nDstVerts++] = pSrc[P0];
            }

            // Do we need to compute intersection?
            if( ((D0>=0)&&(D1<0)) || ((D0<0)&&(D1>=0)) )
            {
                float d = (D1-D0);
                if( math::Abs(d) < 0.00001f )  d = 0.00001f;
                float t = (0-D0) / d;
                pDst[nDstVerts++] = pSrc[P0] + t*(pSrc[P1]-pSrc[P0]);
                bClipped = true;
            }
        }

        return bClipped;
    }

    //------------------------------------------------------------------------------
    inline
    bool plane::ClipNGon(   vector3*        pDst, 
                            std::int32_t&   nDstVerts,
                            const vector3*  pSrc, 
                            std::int32_t    nSrcVerts ) const noexcept
    {
        float           D0, D1;
        std::int32_t    P0, P1;
        bool            bClipped = false;

        nDstVerts = 0;
        P1 = nSrcVerts-1;
        D1 = getDistance( pSrc[P1] );

        for( std::int32_t i=0; i<nSrcVerts; i++ ) 
        {
            P0 = P1;
            D0 = D1;
            P1 = i;
            D1 = getDistance(pSrc[P1]);

            // Do we keep starting vert?
            if( D0 >= 0 )
            {
                pDst[nDstVerts++] = pSrc[P0];
            }

            // Do we need to compute intersection?
            if( ((D0>=0)&&(D1<0)) || ((D0<0)&&(D1>=0)) )
            {
                float d = (D1-D0);
                if( math::Abs(d) < 0.00001f )  d = 0.00001f;
                float t = (0-D0) / d;
                pDst[nDstVerts++] = pSrc[P0] + t*(pSrc[P1]-pSrc[P0]);
                bClipped = true;
            }
        }

        return bClipped;
    }

    //------------------------------------------------------------------------------
    constexpr
    plane& plane::operator - ( void ) noexcept
    {
        m_Normal = -m_Normal;
        m_D      = -m_D;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    plane operator * ( const matrix4& M, const plane& Plane ) noexcept
    {
        plane       Newplane;
        vector3     V;

        // Transform a point in the plane by M
        V = M * ( Plane.m_Normal * - Plane.m_D );

        // Compute the transpose of the Inverse of the Matrix (adjoin)
        // and Transform the normal with it
        Newplane.m_Normal = (M.getAdjoint() * Plane.m_Normal).Normalize();

        // Recompute D by in the transform point
        Newplane.ComputeD( V );

        return Newplane;
    }
}

