

namespace xcore::math
{
    //------------------------------------------------------------------------------
    constexpr
    rect::rect( float Left, float Top, float Right, float Bottom ) noexcept
        :   m_Left   { Left }
        ,   m_Top    { Top }
        ,   m_Right  { Right }
        ,   m_Bottom { Bottom }
    {}

    //------------------------------------------------------------------------------
    constexpr
    rect::rect( const vector2& Pos, float Size ) noexcept
        :   m_Left   { Pos.m_X - Size }
        ,   m_Top    { Pos.m_Y - Size }
        ,   m_Right  { Pos.m_X + Size }
        ,   m_Bottom { Pos.m_Y + Size }
    {}

    //------------------------------------------------------------------------------
    inline
    rect& rect::setup( const vector2& Pos, float Size ) noexcept
    {
        m_Left   = Pos.m_X-Size;
        m_Top    = Pos.m_Y-Size;
        m_Right  = Pos.m_X+Size;
        m_Bottom = Pos.m_Y+Size;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::setup( float X, float Y, float Size ) noexcept
    {
        m_Left   = X-Size;
        m_Top    = Y-Size;
        m_Right  = X+Size;
        m_Bottom = Y+Size;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::setup( float Left, float Top, float Right, float Bottom ) noexcept
    {
        m_Left   = Left;
        m_Top    = Top;
        m_Right  = Right;
        m_Bottom = Bottom;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void rect::setZero( void ) noexcept
    {
        m_Left      = 0;
        m_Top       = 0;
        m_Right     = 0;
        m_Bottom    = 0;
    }

    //------------------------------------------------------------------------------
    inline
    void rect::setMax( void ) noexcept
    {
        m_Left      =  std::numeric_limits<float>::max();
        m_Top       =  std::numeric_limits<float>::max();
        m_Right     = -std::numeric_limits<float>::max();
        m_Bottom    = -std::numeric_limits<float>::max();
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::Intersect( const rect& Rect ) const  noexcept
    {
        return  ( m_Left   <= Rect.m_Right  ) &&
                ( m_Top    <= Rect.m_Bottom ) &&
                ( m_Right  >= Rect.m_Left   ) &&
                ( m_Bottom >= Rect.m_Top    );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::Intersect( rect& R, const rect& Rect ) const noexcept
    {
        if( Intersect( Rect ) == false )
            return( false );

        R.m_Left    = math::Max( m_Left,   Rect.m_Left    );
        R.m_Top     = math::Max( m_Top,    Rect.m_Top     );
        R.m_Right   = math::Min( m_Right,  Rect.m_Right   );
        R.m_Bottom  = math::Min( m_Bottom, Rect.m_Bottom  );

        return true ;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::PointInRect( float X, float Y ) const noexcept
    {
        return ((X >= m_Left) && (X <= m_Right) && (Y >= m_Top) && (Y <= m_Bottom));
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::PointInRect( const vector2& Pos ) const noexcept
    {
        return PointInRect( Pos.m_X, Pos.m_Y );
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::AddPoint( float X, float Y ) noexcept
    {
        m_Left   = math::Min( m_Left  , X );
        m_Top    = math::Min( m_Top   , Y );
        m_Right  = math::Max( m_Right , X );
        m_Bottom = math::Max( m_Bottom, Y );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::AddRect( const rect& Rect ) noexcept
    {
        m_Left   = math::Min( m_Left  , Rect.m_Left   );
        m_Top    = math::Min( m_Top   , Rect.m_Top    );
        m_Right  = math::Max( m_Right , Rect.m_Right  );
        m_Bottom = math::Max( m_Bottom, Rect.m_Bottom );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    float rect::getWidth( void ) const noexcept
    {
        return m_Right - m_Left;
    }

    //------------------------------------------------------------------------------
    constexpr
    float rect::getHeight( void ) const noexcept
    {
        return m_Bottom - m_Top;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 rect::getSize( void ) const noexcept
    {
        return vector2( getWidth(), getHeight() );
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 rect::getCenter( void ) const noexcept
    {
        return vector2( m_Left + getWidth()/2.0f, m_Top + getHeight()/2.0f );
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::setWidth( float W ) noexcept
    {
        m_Right = m_Left + W;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::setHeight( float H ) noexcept
    {
        m_Bottom = m_Top + H;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::setSize( float W, float H ) noexcept
    {
        m_Right  = m_Left + W;
        m_Bottom = m_Top  + H;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::Translate( float X, float Y ) noexcept
    {
        m_Left   += X;
        m_Top    += X;
        m_Right  += Y;
        m_Bottom += Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::Inflate( float X, float Y ) noexcept
    {
        m_Left    -= X;
        m_Top     -= Y;
        m_Right   += X;
        m_Bottom  += Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    rect& rect::Deflate( float X, float Y ) noexcept
    {
        m_Left    += X;
        m_Top     += Y;
        m_Right   -= X;
        m_Bottom  -= Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::InRange( float Min, float Max ) const noexcept
    {
        return (m_Left   >= Min) && (m_Left   <= Max) &&
               (m_Top    >= Min) && (m_Top    <= Max) &&
               (m_Right  >= Min) && (m_Right  <= Max) && 
               (m_Bottom >= Min) && (m_Bottom <= Max);
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::isEmpty( void ) const noexcept
    {
        return( (m_Left>=m_Right) || (m_Top>=m_Bottom) );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::operator == ( const rect& R ) const noexcept
    {
        return( (m_Left   == R.m_Left  ) &&
                (m_Top    == R.m_Top   ) &&
                (m_Right  == R.m_Right ) &&
                (m_Bottom == R.m_Bottom) );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool rect::operator != ( const rect& R ) const noexcept
    {
        return( (m_Left   != R.m_Left  ) ||
                (m_Top    != R.m_Top   ) ||
                (m_Right  != R.m_Right ) ||
                (m_Bottom != R.m_Bottom) );
    }

    //------------------------------------------------------------------------------
    constexpr
    rect rect::Interpolate( float T, const rect& Rect ) const noexcept
    {
        const vector4 V1(m_Left,      m_Top,      m_Right,      m_Bottom      );
        const vector4 V2(Rect.m_Left, Rect.m_Top, Rect.m_Right, Rect.m_Bottom );
        const vector4 R( V1 + T * ( V2 - V1 ) );
    
        return rect( R.m_X, R.m_Y, R.m_Z, R.m_W );
    }
}