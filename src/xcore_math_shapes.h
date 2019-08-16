#ifndef _XCORE_MATH_SHAPES_H
#define _XCORE_MATH_SHAPES_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a plane. The class is describe by a normal that should
    //     remain normalize and D which is the offset of the plane. The plane equation
    //     A + B + C + D = 0 equates to m_Normal.m_X + m_Normal.m_Y + m_Normal.m_Z + m_D = 0
    //
    //<P>  This class is not hardware accelerated and doesn't requiere any special  
    //     alignment. 
    // See Also:
    //     vector3 vector3d xmatrix4 
    //------------------------------------------------------------------------------
    class plane 
    {
    public:

        constexpr                   plane                   ( void )                                                    noexcept = default;
        constexpr                   plane                   ( const vector3& P1,
                                                              const vector3& P2,
                                                              const vector3& P3 )                                       noexcept;
        constexpr                   plane                   ( const vector3d& Normal, float Distance )                  noexcept;
        constexpr                   plane                   ( float A, float B, float C, float D )                      noexcept;
        inline                      plane                   ( const vector3d& Normal, const vector3d& Point )           noexcept;

        inline          bool        isValid                 ( void )                                            const   noexcept;
        constexpr       float       Dot                     ( const vector3d& P )                               const   noexcept;
        constexpr       float       getDistance             ( const vector3d& P )                               const   noexcept;
        constexpr  std::int32_t     getWhichSide            ( const vector3d& P )                               const   noexcept;
        constexpr       void        ComputeD                ( const vector3d& P )                                       noexcept;

        inline          plane&      setup                   ( const vector3& P1,
                                                              const vector3& P2,
                                                              const vector3& P3 )                                       noexcept;
        inline          plane&      setup                   ( const vector3d& Normal, float Distance )                  noexcept;
        inline          plane&      setup                   ( const vector3d& Normal, const vector3d& Point )           noexcept;
        inline          plane&      setup                   ( float A, float B, float C, float D )                      noexcept;
        constexpr       bool        IntersectLine           ( float& T, 
                                                              const vector3d& Start, 
                                                              const vector3d& Direction )                       const   noexcept;
        constexpr       bool        IntersectLineSegment    ( float&           t,
                                                              const vector3d& P0, 
                                                              const vector3d& P1 )                              const   noexcept;

        constexpr       void        getComponents           ( const vector3d& V,
                                                                    vector3d& Parallel,
                                                                    vector3d& Perpendicular )                   const   noexcept; 
        inline          void        getOrthovectors         ( vector3d& AxisA,
                                                              vector3d& AxisB )                                 const   noexcept;
        constexpr       vector3     getReflection           ( const vector3& V )                                const   noexcept;

        inline          bool        ClipNGon                ( vector3d* pDst, std::int32_t& NDstVerts, 
                                                              const vector3d* pSrc, std::int32_t  NSrcVerts )   const   noexcept;
        inline          bool        ClipNGon                ( vector3* pDst, std::int32_t& NDstVerts, 
                                                              const vector3* pSrc, std::int32_t  NSrcVerts )    const   noexcept;

        constexpr       plane&      operator -              ( void )                                                    noexcept;
        inline   friend plane       operator *              ( const matrix4& M, const plane& plane )                    noexcept;

    public:

        vector3d        m_Normal;
        float           m_D;
    };

    //------------------------------------------------------------------------------
    // Description:
    //     This class represents three Dimensional Axis Aligned Bounding Box. 
    //
    //<P>  This class is not hardware accelerated and doesn't requiere any special  
    //     alignment. 
    // See Also:
    //     vector3 vector3d xmatrix4 
    //------------------------------------------------------------------------------
    class bbox 
    {
    public:
        constexpr                   bbox                    ( void )                                                        noexcept;
        constexpr                   bbox                    ( const vector3d& P1 )                                          noexcept;
        constexpr                   bbox                    ( const vector3d& P1, const vector3d& P2 )                      noexcept;
        inline                      bbox                    ( const vector3d* pVerts, const std::int32_t nVerts )           noexcept;
        constexpr                   bbox                    ( const vector3d& Center, const float Radius )                  noexcept;

        inline          bbox&       setup                   ( const vector3d& Center, const float Radius )                  noexcept;
        inline          bbox&       setup                   ( const vector3d& P1, const vector3d& P2 )                      noexcept; 

        inline          void        setZero                 ( void )                                                        noexcept;
        inline          void        setIdentity             ( void )                                                        noexcept;
        inline          bool        isValid                 ( void )                                                const   noexcept;

        constexpr       vector3     getSize                 ( void )                                                const   noexcept;
        constexpr       vector3     getCenter               ( void )                                                const   noexcept;
        inline          float       getRadius               ( void )                                                const   noexcept;
        constexpr       float       getRadiusSquared        ( void )                                                const   noexcept;
        constexpr       float       getSurfaceArea          ( void )                                                const   noexcept;

        constexpr       bool        isInRange               ( float Min, float Max )                                const   noexcept;
        inline          bbox&       Inflate                 ( const vector3d& S )                                           noexcept;
        inline          bbox&       Deflate                 ( const vector3d& S )                                           noexcept;
        inline          bbox&       Transform               ( const matrix4& M )                                            noexcept;

        constexpr       bool        Intersect               ( const vector3d&  Point )                              const   noexcept;
        constexpr       bool        Intersect               ( const bbox&      BBox  )                              const   noexcept;
        constexpr       bool        Intersect               ( const plane&     Plane )                              const   noexcept;
        inline          bool        Intersect               ( float&           t,
                                                              const vector3d&  P0, 
                                                              const vector3d&  P1 )                                 const   noexcept;
        constexpr       bool        Intersect               ( const vector3d*  pVert,
                                                              std::int32_t     nVerts )                             const   noexcept;
        constexpr       bool        IntersectTriangleBBox   ( const vector3d& P0,
                                                              const vector3d& P1,
                                                              const vector3d& P2 )                                  const   noexcept;

        inline          float       getClosestVertex        ( vector3d& ClosestPoint, const vector3d& Point )       const   noexcept;

        inline          void        getVerts                ( vector3d* pVerts, std::int32_t nVerts )               const   noexcept;
        constexpr       bool        Contains                ( const bbox&     BBox  )                               const   noexcept;
        inline          bbox&       AddVerts                ( const vector3d* pVerts, std::int32_t NVerts )                 noexcept;
        inline          bbox&       AddVerts                ( const vector3*  pVerts, std::int32_t NVerts )                 noexcept;
        inline          bbox&       Translate               ( const vector3d& Delta )                                       noexcept;

        inline      const bbox&     operator +=             ( const bbox&     BBox  )                                       noexcept;
        inline      const bbox&     operator +=             ( const vector3d& Point )                                       noexcept;
                               
        inline friend  bbox         operator +              ( const bbox&     bbox1, const bbox&     bbox2 )                noexcept;
        inline friend  bbox         operator +              ( const bbox&     bbox1, const vector3d& Point )                noexcept;
        inline friend  bbox         operator +              ( const vector3d& Point, const bbox&     bbox1 )                noexcept;

    public:

        vector3d m_Min {};
        vector3d m_Max {};
    };

    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a three Dimensional Sphere. 
    //
    //<P>  This class is not hardware accelerated and doesn't requiere any special  
    //     alignment. 
    // See Also:
    //     vector3 vector3d xmatrix4 bbox
    //------------------------------------------------------------------------------
    class sphere 
    {
    public:
                                sphere                  ( void )                                                                    noexcept = default;
                                sphere                  ( const vector3d& Pos, const float R )                                      noexcept;
                                sphere                  ( const bbox& BBox )                                                        noexcept;

        void                    setZero                 ( void )                                                                    noexcept;

        void                    setup                   ( const vector3d& Pos, const float R )                                      noexcept;
        bbox                    getBBox                 ( void )                                                            const   noexcept;

        bool                    TestIntersect           ( const vector3d& P0, const vector3d& P1 )                          const   noexcept;
        std::int32_t            TestIntersection        ( const plane& Plane )                                              const   noexcept;

        std::int32_t            Intersect               ( float& t0, float& t1, const vector3d& P0, const vector3d& P1 )    const   noexcept;
        bool                    Intersect               ( float& t0, const vector3d& P0, const vector3d& P1 )               const   noexcept;
        bool                    Intersect               ( const bbox& BBox )                                                const   noexcept;
        bool                    Intersect               ( const sphere&   Sphere )                                          const   noexcept;
        bool                    Intersect               ( float&          t0,        
                                                          float&          t1,        
                                                          const vector3d& Vel1,       
                                                          const sphere&   Sph2,      
                                                          const vector3d& Vel2 )                                                    noexcept;
    public:

        vector3d    m_Pos;
        float       m_R;
    };

    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 2D floating point base rectangle. 
    //------------------------------------------------------------------------------
    class rect 
    {
    public:
        constexpr                           rect                    ( void )                                                noexcept = default;
        constexpr                           rect                    ( float l, float t, float r, float b )                  noexcept;
        constexpr                           rect                    ( const vector2& Pos, float Size )                      noexcept;

        inline      void                    setMax                  ( void )                                                noexcept;
        inline      void                    setZero                 ( void )                                                noexcept;
        inline      rect&                   setup                   ( float l, float t, float r, float b )                  noexcept;
        inline      rect&                   setup                   ( float X, float Y, float Size )                        noexcept;
        inline      rect&                   setup                   ( const vector2& Pos, float Size )                      noexcept;
        inline      
        constexpr   bool                    Intersect               ( const rect& Rect )                            const   noexcept;
        constexpr   bool                    Intersect               ( rect& R, const rect& Rect )                   const   noexcept;
        inline      
        constexpr   bool                    PointInRect             ( const vector2& Pos )                          const   noexcept;
        constexpr   bool                    PointInRect             ( float X, float Y )                            const   noexcept;
        inline      rect&                   AddPoint                ( float X, float Y )                                    noexcept;
        inline      rect&                   AddRect                 ( const rect& Rect )                                    noexcept;
        inline      
        constexpr   float                   getWidth                ( void )                                        const   noexcept;
        constexpr   float                   getHeight               ( void )                                        const   noexcept;
        constexpr   vector2                 getSize                 ( void )                                        const   noexcept;
        constexpr   vector2                 getCenter               ( void )                                        const   noexcept;
        
        inline      rect&                   setWidth                ( float W )                                             noexcept;
        inline      rect&                   setHeight               ( float H )                                             noexcept;
        inline      rect&                   setSize                 ( float W, float H )                                    noexcept;
      
        inline      rect&                   Translate               ( float X, float Y )                                    noexcept;
        inline      rect&                   Inflate                 ( float X, float Y )                                    noexcept;
        inline      rect&                   Deflate                 ( float X, float Y )                                    noexcept;
        
        constexpr   float                   Difference              ( const rect& R )                               const   noexcept;
        constexpr   bool                    InRange                 ( float Min, float Max )                        const   noexcept;
        constexpr   bool                    isEmpty                 ( void )                                        const   noexcept;
     
        constexpr   rect                    Interpolate             ( float T, const rect& Rect )                   const   noexcept;
      
        constexpr   bool                    operator ==             ( const rect& R )                               const   noexcept;
        constexpr   bool                    operator !=             ( const rect& R )                               const   noexcept;

    public:

        float     m_Left      {};
        float     m_Top       {};
        float     m_Right     {};
        float     m_Bottom    {};
    };

    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 2D floating point base rectangle.
    //------------------------------------------------------------------------------
    class rectwh 
    {
    public:

        vector2     m_Pos   {};
        float       m_W     {};
        float       m_H     {};
    };

    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 2D integer base rectangle. 
    //------------------------------------------------------------------------------
    class irect
    {
    public:
        constexpr                               irect                   ( void )                                                        noexcept = default;
        constexpr                               irect                   ( std::int32_t l, std::int32_t t
                                                                        , std::int32_t r, std::int32_t b )                              noexcept;

        inline          void                    setMax                  ( void )                                                        noexcept;
        inline          void                    setZero                 ( void )                                                        noexcept;
        inline          irect&                  setup                   ( std::int32_t l, std::int32_t t
                                                                        , std::int32_t r, std::int32_t b )                              noexcept;

        constexpr       bool                    Intersect               ( const irect& Rect )                                   const   noexcept;
        constexpr       bool                    Intersect               ( irect& R, const irect& Rect )                         const   noexcept;

        constexpr       bool                    PointInRect             ( std::int32_t X, std::int32_t Y )                      const   noexcept;
        inline          irect&                  AddPoint                ( std::int32_t X, std::int32_t Y )                              noexcept;
        inline          irect&                  AddRect                 ( const irect& Rect  )                                          noexcept;

        constexpr       std::int32_t            getWidth                ( void )                                                const   noexcept;
        constexpr       std::int32_t            getHeight               ( void )                                                const   noexcept;
        constexpr       vector2                 getSize                 ( void )                                                const   noexcept;
        constexpr       vector2                 getCenter               ( void )                                                const   noexcept;

        inline          irect&                  setWidth                ( std::int32_t W )                                              noexcept;
        inline          irect&                  setHeight               ( std::int32_t H )                                              noexcept;
        inline          irect&                  setSize                 ( std::int32_t W, std::int32_t H )                              noexcept;

        inline          irect&                  Translate               ( std::int32_t X, std::int32_t Y )                              noexcept;
        inline          irect&                  Inflate                 ( std::int32_t X, std::int32_t Y )                              noexcept;
        inline          irect&                  Deflate                 ( std::int32_t X, std::int32_t Y )                              noexcept;

        constexpr       float                   Difference              ( const irect& R )                                      const   noexcept;
        constexpr       bool                    isInRange               ( std::int32_t Min, std::int32_t Max )                  const   noexcept;
        constexpr       bool                    isEmpty                 ( void )                                                const   noexcept;

        constexpr       bool                    operator ==             ( const irect& R )                                      const   noexcept;
        constexpr       bool                    operator !=             ( const irect& R )                                      const   noexcept;

    public:

        std::int32_t m_Left      {};
        std::int32_t m_Top       {};
        std::int32_t m_Right     {};
        std::int32_t m_Bottom    {};
    };

}
#endif