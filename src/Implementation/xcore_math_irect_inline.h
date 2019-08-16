
namespace xcore::math
{
    //------------------------------------------------------------------------------
    constexpr
    irect::irect( std::int32_t Left, std::int32_t Top, std::int32_t Right, std::int32_t Bottom ) noexcept
        : m_Left   { Left   }
        , m_Top    { Top    }
        , m_Right  { Right  }
        , m_Bottom { Bottom }
    {}

    //------------------------------------------------------------------------------
    inline
    irect& irect::setup( std::int32_t Left, std::int32_t Top, std::int32_t Right, std::int32_t Bottom ) noexcept
    {
        m_Left   = Left;
        m_Top    = Top;
        m_Right  = Right;
        m_Bottom = Bottom;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    void irect::setZero( void ) noexcept
    {
        m_Left      = 0;
        m_Top       = 0;
        m_Right     = 0;
        m_Bottom    = 0;
    }

    //------------------------------------------------------------------------------
    inline
    void irect::setMax( void ) noexcept
    {
        m_Left      =  std::numeric_limits<std::int32_t>::max();
        m_Top       =  std::numeric_limits<std::int32_t>::max();
        m_Right     = -std::numeric_limits<std::int32_t>::max();
        m_Bottom    = -std::numeric_limits<std::int32_t>::max();
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::Intersect( const irect& Rect ) const noexcept
    {
        return  ( m_Left   <= Rect.m_Right  ) &&
                ( m_Top    <= Rect.m_Bottom ) &&
                ( m_Right  >= Rect.m_Left   ) &&
                ( m_Bottom >= Rect.m_Top    );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::Intersect( irect& R, const irect& Rect ) const noexcept
    {
        if( Intersect( Rect ) == false )
            return false ;

        R.m_Left    = math::Max( m_Left,   Rect.m_Left    );
        R.m_Top     = math::Max( m_Top,    Rect.m_Top     );
        R.m_Right   = math::Min( m_Right,  Rect.m_Right   );
        R.m_Bottom  = math::Min( m_Bottom, Rect.m_Bottom  );

        return true ;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::PointInRect( std::int32_t X, std::int32_t Y ) const noexcept
    {
        return ((X >= m_Left) && (X <= m_Right) && (Y >= m_Top) && (Y <= m_Bottom));
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::AddPoint( std::int32_t X, std::int32_t Y ) noexcept
    {
        m_Left   = math::Min( m_Left  , X );
        m_Top    = math::Min( m_Top   , Y );
        m_Right  = math::Max( m_Right , X );
        m_Bottom = math::Max( m_Bottom, Y );
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::AddRect( const irect& Rect ) noexcept
    {
        m_Left   = math::Min( m_Left  , Rect.m_Left   );
        m_Top    = math::Min( m_Top   , Rect.m_Top    );
        m_Right  = math::Max( m_Right , Rect.m_Right  );
        m_Bottom = math::Max( m_Bottom, Rect.m_Bottom );
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    std::int32_t irect::getWidth( void ) const noexcept
    {
        return m_Right - m_Left;
    }

    //------------------------------------------------------------------------------
    constexpr
    std::int32_t irect::getHeight( void ) const noexcept
    {
        return m_Bottom - m_Top;
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 irect::getSize( void ) const noexcept
    {
        return vector2( static_cast<float>(getWidth()), static_cast<float>(getHeight()) );
    }

    //------------------------------------------------------------------------------
    constexpr
    vector2 irect::getCenter( void ) const noexcept
    {
        return vector2( static_cast<float>(getWidth())/2.0f, static_cast<float>(getHeight())/2.0f );
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::setWidth( std::int32_t W ) noexcept
    {
        m_Right = m_Left + W;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::setHeight( std::int32_t H ) noexcept
    {
        m_Bottom = m_Top + H;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::setSize( std::int32_t W, std::int32_t H ) noexcept
    {
        m_Right  = m_Left + W;
        m_Bottom = m_Top  + H;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::Translate( std::int32_t X, std::int32_t Y ) noexcept
    {
        m_Left   += X;
        m_Top    += X;
        m_Right  += Y;
        m_Bottom += Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::Inflate( std::int32_t X, std::int32_t Y ) noexcept
    {
        m_Left    -= X;
        m_Top     += X;
        m_Right   -= Y;
        m_Bottom  += Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    inline
    irect& irect::Deflate( std::int32_t X, std::int32_t Y ) noexcept
    {
        m_Left    += X;
        m_Top     -= X;
        m_Right   += Y;
        m_Bottom  -= Y;
        return *this;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::isInRange( std::int32_t Min, std::int32_t Max ) const noexcept
    {
        return (m_Left   >= Min) && (m_Left   <= Max) &&
               (m_Top    >= Min) && (m_Top    <= Max) &&
               (m_Right  >= Min) && (m_Right  <= Max) && 
               (m_Bottom >= Min) && (m_Bottom <= Max);
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::isEmpty( void ) const noexcept
    {
        return( (m_Left>=m_Right) || (m_Top>=m_Bottom) );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::operator == ( const irect& R ) const noexcept
    {
        return( (m_Left   == R.m_Left  ) &&
                (m_Top    == R.m_Top   ) &&
                (m_Right  == R.m_Right ) &&
                (m_Bottom == R.m_Bottom) );
    }

    //------------------------------------------------------------------------------
    constexpr
    bool irect::operator != ( const irect& R ) const noexcept
    {
        return( (m_Left   != R.m_Left  ) ||
                (m_Top    != R.m_Top   ) ||
                (m_Right  != R.m_Right ) ||
                (m_Bottom != R.m_Bottom) );
    }



}