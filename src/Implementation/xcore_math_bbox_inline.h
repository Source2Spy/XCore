
namespace xcore::math
{
    //------------------------------------------------------------------------------
    constexpr
    bbox::bbox( void ) noexcept
        : m_Min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() }
        , m_Max{ std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() }
    {}

    //------------------------------------------------------------------------------
    constexpr
    bbox::bbox( const vector3d& P1 ) noexcept
        : m_Min( P1 )
        , m_Max( P1 ) 
    {}

    //------------------------------------------------------------------------------
    constexpr
    bbox::bbox( const vector3d& P1, const vector3d& P2 ) noexcept
        : m_Min{ P1.getMin( P2 ) }
        , m_Max{ P1.getMax( P2 ) }
    {}

    //------------------------------------------------------------------------------
    inline
    bbox::bbox( const vector3d* pVerts, const std::int32_t nVerts ) noexcept
    {
        xassert( nVerts >= 0 );

        if( nVerts > 0 )
        {
            Intersect( pVerts[0] );
            Intersect( &pVerts[1], nVerts-1 );
        }
        else
        {
            setIdentity();
        }
    }

    //------------------------------------------------------------------------------
    constexpr
    bbox::bbox( const vector3d& Center, const float Radius ) noexcept
        : m_Min{ Center - vector3d(Radius) }
        , m_Max{ Center + vector3d(Radius) }
    {}

    //------------------------------------------------------------------------------
    inline
    void bbox::setZero( void ) noexcept
    {
        m_Min.setZero();
        m_Max.setZero();
    }

    //------------------------------------------------------------------------------
    inline
    void bbox::setIdentity( void ) noexcept
    {
        m_Min.setup( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );
        m_Max.setup( std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() );
    }

    //------------------------------------------------------------------------------
    inline
    bool bbox::isValid( void ) const noexcept
    {
        if( !(m_Min.isValid() && m_Max.isValid()) )
            return false;

        if( !( reinterpret_cast<const vector3d&>(m_Min.getMin( m_Max )) == m_Min) ) 
            return false;

        return reinterpret_cast<const vector3d&>(m_Max.getMax( m_Min )) == m_Max;
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::setup( const vector3d& Center, const float Radius ) noexcept
    {
        m_Min = Center - vector3d(Radius);
        m_Max = Center + vector3d(Radius);
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::setup( const vector3d& P1, const vector3d& P2 ) noexcept
    {
        m_Min = P1.getMin( P2 );
        m_Max = P1.getMax( P2 );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 bbox::getSize( void ) const noexcept
    {
        return m_Max - m_Min;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector3 bbox::getCenter( void ) const noexcept
    {
        return (m_Min + m_Max) * 0.5f;
    }

    //------------------------------------------------------------------------------
    inline
    float bbox::getRadius( void ) const noexcept
    {
        return getSize().getLength() * 0.5f;
    }

    //------------------------------------------------------------------------------
    constexpr
    float bbox::getRadiusSquared( void ) const noexcept
    {
        return getSize().getLengthSquared() * 0.5f;
    }

    //------------------------------------------------------------------------------
    constexpr
    float bbox::getSurfaceArea( void ) const noexcept
    {
        vector3d Size( getSize() );
        return ( Size.m_X*Size.m_Y + 
                 Size.m_Y*Size.m_Z +
                 Size.m_Z*Size.m_X ) * 2.0f;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::isInRange( const float Min, const float Max ) const noexcept
    {
        return m_Min.isInRange( Min, Max ) && m_Max.isInRange( Min, Max );
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::Inflate( const vector3d& S ) noexcept
    {
        m_Min -= S;
        m_Max += S;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::Deflate( const vector3d& S ) noexcept
    {
        m_Min += S;
        m_Max -= S;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::Translate( const vector3d& Delta ) noexcept
    {
        m_Min += Delta;
        m_Max += Delta;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    bbox& bbox::Transform( const matrix4& M ) noexcept
    {
        vector3d Min( m_Min ), Max( m_Max );
        float     a, b;
        std::int32_t     i, j;

        // setup the bbox to be at the translation point
        m_Max = m_Min.setup( M(0,3), M(1,3), M(2,3) );

        // Find extreme points by considering product of 
        // min and max with each component of M.
        for( j=0; j<3; j++ )
        {
            for( i=0; i<3; i++ )
            {
                a = M(j,i) * Min[i ];
                b = M(j,i) * Max[i ];

                if( a < b )
                {
                    Min[j ] += a;
                    Max[j ] += b;
                }
                else
                {
                    Min[j ] += b;
                    Max[j ] += a;
                }
            }
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::Intersect( const vector3d& Point ) const noexcept
    {
        return (Point.m_X <= m_Max.m_X) && 
               (Point.m_Y <= m_Max.m_Y) && 
               (Point.m_Z <= m_Max.m_Z) && 
               (Point.m_X >= m_Min.m_X) && 
               (Point.m_Y >= m_Min.m_Y) && 
               (Point.m_Z >= m_Min.m_Z);
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::Intersect( const bbox& bbox ) const noexcept
    {
        return (bbox.m_Min.m_X >= m_Max.m_X) && 
               (bbox.m_Max.m_Y <= m_Max.m_Y) && 
               (bbox.m_Min.m_Z >= m_Max.m_Z) && 
               (bbox.m_Max.m_X <= m_Min.m_X) && 
               (bbox.m_Min.m_Y >= m_Min.m_Y) && 
               (bbox.m_Max.m_Z <= m_Min.m_Z);
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::Intersect( const plane& Plane ) const noexcept
    {
        vector3d PMin, PMax;

        if( Plane.m_Normal.m_X > 0 )   
        { 
            PMax.m_X = m_Max.m_X;   
            PMin.m_X = m_Min.m_X; 
        }
        else                           
        { 
            PMax.m_X = m_Min.m_X;   
            PMin.m_X = m_Max.m_X; 
        }

        if( Plane.m_Normal.m_Y > 0 )   
        { 
            PMax.m_Y = m_Max.m_Y;   
            PMin.m_Y = m_Min.m_Y; 
        }
        else                           
        { 
            PMax.m_Y = m_Min.m_Y;   
            PMin.m_Y = m_Max.m_Y; 
        }

        if( Plane.m_Normal.m_Z > 0 )   
        { 
            PMax.m_Z = m_Max.m_Z;   
            PMin.m_Z = m_Min.m_Z; 
        }
        else                           
        { 
            PMax.m_Z = m_Min.m_Z;   
            PMin.m_Z = m_Max.m_Z; 
        }

        return (Plane.getDistance( PMax ) >= 0.0f) && 
               (Plane.getDistance( PMin ) <= 0.0f);
    }

    //------------------------------------------------------------------------------
    inline
    bool bbox::Intersect( float& t, const vector3d& P0, const vector3d& P1 ) const noexcept
    {                      
        float         PlaneD   [3];
        bool        PlaneUsed[3] = { true, true, true };
        float         T        [3] = { -1, -1, -1 };
        vector3d   Direction( P1 - P0 );
        std::int32_t         MaxPlane;
        std::int32_t         i;
        float         Component;

        // setup a value until we have something better.
        t = 0.0f;

        // Consider relationship of each component of P0 to the box.
        for( i = 0; i < 3; i++ )
        {
            if     ( P0[i ] > m_Max[i ] )   { PlaneD[i]    = m_Max[i ]; }
            else if( P0[i ] < m_Min[i ] )   { PlaneD[i]    = m_Min[i ]; }
            else                          { PlaneUsed[i] = false;  }
        }

        // Is the starting point in the box?
        if( !PlaneUsed[0] && !PlaneUsed[1] && !PlaneUsed[2] )
            return true;

        // For each plane to be used, compute the distance to the plane.
        for( i = 0; i < 3; i++ )
        {
            if( PlaneUsed[i] && (Direction[i ] != 0.0f) )
                T[i] = (PlaneD[i] - P0[i ]) / Direction[i ];
        }

        // We need to know which plane had the largest distance.
        if( T[0] > T[1] )
        {
            MaxPlane = ((T[0] > T[2]) ? 0 : 2);
        }
        else
        {
            MaxPlane = ((T[1] > T[2]) ? 1 : 2);
        }

        // If the largest plane distance is less than zero, then there is no hit.
        if( T[MaxPlane] < 0.0f )
            return false;

        // See if the point we think is the hit point is a real hit.
        for( i = 0; i < 3; i++ )
        {
            // See if component 'i' of the hit point is on the box.
            if( i != MaxPlane )
            {
                Component = P0[i ] + T[MaxPlane] * Direction[i ];
                if( (Component < m_Min[i ]) || (Component > m_Max[i ]) )
                {
                    // We missed!  Hit point was not on the box.
                    return false;
                }
            }
        }

        // We have a verified hit.  setup t and we're done.
        t = T[MaxPlane];
        return true ;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::IntersectTriangleBBox( const vector3d& P0,
                                      const vector3d& P1,
                                      const vector3d& P2 )  const noexcept
    {
        bbox Tribbox;

        //
        // HANDLE X
        //
        {
            Tribbox.m_Min.m_X = P0.m_X;
            Tribbox.m_Max.m_X = P0.m_X;

            if( P2.m_X > P1.m_X )
            {
                if( P2.m_X > Tribbox.m_Max.m_X )  Tribbox.m_Max.m_X = P2.m_X;
                if( P1.m_X < Tribbox.m_Min.m_X )  Tribbox.m_Min.m_X = P1.m_X;
            }
            else
            {
                if( P1.m_X > Tribbox.m_Max.m_X )  Tribbox.m_Max.m_X = P1.m_X;
                if( P2.m_X < Tribbox.m_Min.m_X )  Tribbox.m_Min.m_X = P2.m_X;
            }

            // X's are solved so compare to bbox.
            if( m_Min.m_X > Tribbox.m_Max.m_X ) return false;
            if( m_Max.m_X < Tribbox.m_Min.m_X ) return false;
        }

        //
        // HANDLE Z
        //
        {
            Tribbox.m_Min.m_Z = P0.m_Z;
            Tribbox.m_Max.m_Z = P0.m_Z;

            if( P2.m_Z > P1.m_Z )
            {
                if( P2.m_Z > Tribbox.m_Max.m_Z )  Tribbox.m_Max.m_Z = P2.m_Z;
                if( P1.m_Z < Tribbox.m_Min.m_Z )  Tribbox.m_Min.m_Z = P1.m_Z;
            }
            else
            {
                if( P1.m_Z > Tribbox.m_Max.m_Z )  Tribbox.m_Max.m_Z = P1.m_Z;
                if( P2.m_Z < Tribbox.m_Min.m_Z )  Tribbox.m_Min.m_Z = P2.m_Z;
            }

            // Y's are solved so compare to bbox.
            if( m_Min.m_Z > Tribbox.m_Max.m_Z ) return false;
            if( m_Max.m_Z < Tribbox.m_Min.m_Z ) return false;
        }

        //
        // HANDLE Y
        //
        {
            Tribbox.m_Min.m_Y = P0.m_Y;
            Tribbox.m_Max.m_Y = P0.m_Y;

            if( P2.m_Y > P1.m_Y )
            {
                if( P2.m_Y > Tribbox.m_Max.m_Y )  Tribbox.m_Max.m_Y = P2.m_Y;
                if( P1.m_Y < Tribbox.m_Min.m_Y )  Tribbox.m_Min.m_Y = P1.m_Y;
            }
            else
            {
                if( P1.m_Y > Tribbox.m_Max.m_Y )  Tribbox.m_Max.m_Y = P1.m_Y;
                if( P2.m_Y < Tribbox.m_Min.m_Y )  Tribbox.m_Min.m_Y = P2.m_Y;
            }

            // Z's are solved so compare to bbox.
            if( m_Min.m_Y > Tribbox.m_Max.m_Y ) return false;
            if( m_Max.m_Y < Tribbox.m_Min.m_Y ) return false;
        }

        return false;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool bbox::Contains( const bbox& BBox  ) const noexcept
    {
        if( BBox.m_Min.m_X >= m_Min.m_X &&
            BBox.m_Max.m_X <= m_Max.m_X &&
            BBox.m_Min.m_Z >= m_Min.m_Z &&
            BBox.m_Max.m_Z <= m_Max.m_Z &&
            BBox.m_Min.m_Y >= m_Min.m_Y &&
            BBox.m_Max.m_Y <= m_Max.m_Y )
            return true;

        return false;
    }

    //------------------------------------------------------------------------------
    inline 
    bbox& bbox::AddVerts( const vector3d* pVerts, std::int32_t nVerts ) noexcept
    {
        xassert( pVerts );
        xassert( nVerts > 0 );

        for( std::int32_t i=0; i<nVerts; i++ )
        {
            m_Min = m_Min.getMin( pVerts[i] );
            m_Max = m_Max.getMax( pVerts[i] );
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    //can't just use vector3d in place of of vector3d for an array, because they have different sizes
    inline 
    bbox& bbox::AddVerts( const vector3* pVerts, std::int32_t nVerts ) noexcept
    {
        xassert( pVerts );
        xassert( nVerts > 0 );

        for( std::int32_t i=0; i<nVerts; i++ )
        {
            m_Min = m_Min.getMin( pVerts[i] );
            m_Max = m_Max.getMax( pVerts[i] );
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void bbox::getVerts( vector3d* pVerts, std::int32_t nVerts ) const noexcept
    {
        xassert( nVerts >= 8 );
        pVerts[0].setup( m_Min.m_X, m_Min.m_Y, m_Min.m_Z );
        pVerts[1].setup( m_Max.m_X, m_Min.m_Y, m_Min.m_Z );
        pVerts[2].setup( m_Max.m_X, m_Max.m_Y, m_Min.m_Z );
        pVerts[3].setup( m_Min.m_X, m_Max.m_Y, m_Min.m_Z );

        pVerts[4].setup( m_Min.m_X, m_Min.m_Y, m_Max.m_Z );
        pVerts[5].setup( m_Max.m_X, m_Min.m_Y, m_Max.m_Z );
        pVerts[6].setup( m_Max.m_X, m_Max.m_Y, m_Max.m_Z );
        pVerts[7].setup( m_Min.m_X, m_Max.m_Y, m_Max.m_Z );
    }

    //------------------------------------------------------------------------------
    // Note: The distance will be zero if inside the box.
    inline
    float bbox::getClosestVertex( vector3d& ClosestVertex, const vector3d& Point ) const noexcept
    {
        // This will be modified to become the closest point if it's outside the box.
        ClosestVertex = Point;

        // get the distance from the bounding box along the each axis.
        float dist_to_max;
        float dist_to_min; 

        dist_to_max = ClosestVertex.m_X - m_Max.m_X;
        dist_to_min = m_Min.m_X - ClosestVertex.m_X;
        ClosestVertex.m_X += math::FSel(dist_to_max, -dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);
        float dist_x         = math::FSel(dist_to_max,  dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);

        dist_to_max = ClosestVertex.m_Y - m_Max.m_Y;
        dist_to_min = m_Min.m_Y - ClosestVertex.m_Y;
        ClosestVertex.m_Y += math::FSel(dist_to_max, -dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);
        float dist_y        = math::FSel(dist_to_max,  dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);

        dist_to_max = ClosestVertex.m_Z - m_Max.m_Z;
        dist_to_min = m_Min.m_Z - ClosestVertex.m_Z;
        ClosestVertex.m_Z += math::FSel(dist_to_max, -dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);
        float dist_z         = math::FSel(dist_to_max,  dist_to_max, 0.0f) + math::FSel(dist_to_min, dist_to_min, 0.0f);

        // Squared distance = x^2 + y^2 + z^2
        return (math::Sqr(dist_x) + math::Sqr(dist_y) + math::Sqr(dist_z));
    }

    //------------------------------------------------------------------------------
    inline 
    bbox operator + ( const bbox& BBox1, const bbox& BBox2 ) noexcept
    {
        return bbox( BBox1 ) += BBox2;
    }

    //------------------------------------------------------------------------------
    inline 
    bbox operator + ( const bbox& BBox, const vector3d& Point ) noexcept
    {
        return bbox( BBox ) += Point;
    }

    //------------------------------------------------------------------------------
    inline 
    bbox operator + ( const vector3d& Point, const bbox& BBox ) noexcept
    {
        return bbox( BBox ) += Point;
    }

    //------------------------------------------------------------------------------
    inline 
    const bbox& bbox::operator += ( const bbox& BBox ) noexcept
    {
        m_Min = m_Min.getMin( BBox.m_Min );
        m_Max = m_Max.getMax( BBox.m_Max );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline 
    const bbox& bbox::operator += ( const vector3d& Point ) noexcept
    {
        m_Min = m_Min.getMin( Point );
        m_Max = m_Max.getMax( Point );
        return *this;
    }
}