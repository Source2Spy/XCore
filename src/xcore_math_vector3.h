#ifndef _XCORE_MATH_VECTOR3_H
#define _XCORE_MATH_VECTOR3_H
#pragma once

namespace xcore::math
{
    //------------------------------------------------------------------------------
    // Description:
    //     This class represents a 3D vector. 
    //
    //<P>  vector3d is not a hardware accelerated class and works only with floats. There
    //     is not need for special alignments thought except for the atomic types. it also
    //     doesn't waste memory like its speedy child vector3. Only use this class for
    //     memory related issues.
    //
    //<P>  vector3 is meant for the common case, speed and not memory considerations. 
    //     For that it contains four elements stead of the 3 that are actually needed. 
    //     The class has special alignment requirements (16byte) Which allow to have an 
    //     extra bust for speed. The class uses any hardware feature it can find to speed 
    //     it self up.
    //
    // Example:
    //<CODE>
    //      vector3 DoSomething( const vector3d& V )
    //      {
    //          vector3 G;
    //          G = V * 0.5f;
    //          G = G.GetMax( V );
    //          return G.Cross( V );
    //      }
    //</CODE>
    //      
    // See Also:
    //     vector3 vector3d xvector2 xvector4
    //------------------------------------------------------------------------------
    class vector3d
    {
    public:

        constexpr                               vector3d                        ( void )                                            noexcept = default;
        constexpr                               vector3d                        ( float X, float Y, float Z )                       noexcept : m_X(X), m_Y(Y), m_Z(Z) {} 
                                                vector3d                        ( radian Pitch, radian Yaw )                        noexcept;
        constexpr explicit                      vector3d                        ( const floatx4& Register )                         noexcept;
        constexpr                               vector3d                        ( const float n )                                   noexcept : m_X(n), m_Y(n), m_Z(n) {}
        constexpr                               vector3d                        ( const vector3& V )                                noexcept;

        // Exceptions to the Get/Set rule
        constexpr           vector3             getLerp                         ( float t, const vector3d& V )              const   noexcept;
        constexpr           float               Dot                             ( const vector3d& V )                       const   noexcept;
        constexpr           vector3             Cross                           ( const vector3d& V )                       const   noexcept;

        // Normal functions
        inline              vector3d&           setup                           ( float X, float Y, float Z )                       noexcept;
        inline              vector3d&           setup                           ( const floatx4& Register )                         noexcept; 
        inline              vector3d&           setup                           ( const float n )                                   noexcept;
                                
        inline              void                setIdentity                     ( void )                                            noexcept;
        inline              void                setZero                         ( void )                                            noexcept;
        xforceinline        bool                isValid                         ( void )                                    const   noexcept;
        constexpr           bool                isInRange                       ( const float Min, const float Max )        const   noexcept;

        constexpr           vector3             getOneOver                      ( void )                                    const   noexcept;
        inline              float               getLength                       ( void )                                    const   noexcept;
        constexpr           float               getLengthSquared                ( void )                                    const   noexcept;
        constexpr           float               getDistanceSquare               ( const vector3d& V )                       const   noexcept;
        inline              float               getDistance                     ( const vector3d& V )                       const   noexcept;
        constexpr           vector3             getMax                          ( const vector3d& V )                       const   noexcept;
        constexpr           vector3             getMin                          ( const vector3d& V )                       const   noexcept;
        inline              vector3d&           Abs                             ( void )                                            noexcept;
        inline              vector3d&           Normalize                       ( void )                                            noexcept;
        inline              vector3d&           NormalizeSafe                   ( void )                                            noexcept;
                                
        inline              float*              operator ()                     ( void )                                            noexcept;
        inline              float&              operator []                     ( const std::int32_t i )                            noexcept;
        constexpr           float               operator []                     ( const std::int32_t i )                    const   noexcept;
        constexpr           bool                operator ==                     ( const vector3d& V )                       const   noexcept;
        inline              const vector3d&     operator +=                     ( const vector3d& V )                               noexcept;
        inline              const vector3d&     operator -=                     ( const vector3d& V )                               noexcept;
        inline              const vector3d&     operator *=                     ( const vector3d& V )                               noexcept;
        inline              const vector3d&     operator /=                     ( float Div )                                       noexcept;
        inline              const vector3d&     operator *=                     ( float Scalar )                                    noexcept;
                                    
        inline    friend    vector3             operator /                      ( const vector3d& V, float Div )                    noexcept;
        constexpr friend    vector3             operator *                      ( float Scale, const vector3d& V )                  noexcept;
        constexpr friend    vector3             operator *                      ( const vector3d& V, float Scale )                  noexcept;
        constexpr friend    vector3             operator -                      ( const vector3d& V )                               noexcept;
        constexpr friend    vector3             operator *                      ( const vector3d& V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator -                      ( const vector3d& V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator +                      ( const vector3d& V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator /                      ( const vector3d& V0, const vector3d& V1 )          noexcept;

    public:

        float     m_X;
        float     m_Y;
        float     m_Z;
    };

    //==============================================================================
    // VECTOR3
    //==============================================================================

    // <COPY vector3d>
    class alignas(floatx4) vector3 
    {
    public:

        constexpr                               vector3                         ( void )                                            noexcept = default;
        constexpr                               vector3                         ( float X, float Y, float Z )                       noexcept : m_XYZW{ X, Y, Z, 1 } {}
        constexpr                               vector3                         ( const vector3d&  V )                              noexcept : m_XYZW{ V.m_X, V.m_Y, V.m_Z, 1 } {}
        constexpr                               vector3                         ( const float n )                                   noexcept : m_XYZW{ n, n, n, 1 } {}
        inline                                  vector3                         ( radian Pitch, radian Yaw )                        noexcept;
        constexpr explicit                      vector3                         ( const floatx4& Register )                         noexcept : m_XYZW( Register ) {}

        // Exceptions to the Get/Set rule
        constexpr           float               Dot                             ( const vector3& V )                        const   noexcept;
        inline              vector3             Cross                           ( const vector3& V )                        const   noexcept;

        // Setup functions
        inline              vector3&            setup                           ( float X, float Y, float Z )                       noexcept;
        inline              vector3&            setup                           ( const floatx4& Register )                         noexcept;
        inline              vector3&            setup                           ( const float n )                                   noexcept;
        inline              vector3&            setup                           ( radian Pitch, radian Yaw )                        noexcept;

        inline              void                setIdentity                     ( void )                                            noexcept;
        inline              void                setZero                         ( void )                                            noexcept;
        inline              vector3&            Normalize                       ( void )                                            noexcept;
        inline              vector3&            NormalizeSafe                   ( void )                                            noexcept;
        inline              vector3&            RotateX                         ( radian Rx )                                       noexcept;           
        inline              vector3&            RotateY                         ( radian Ry )                                       noexcept;           
        inline              vector3&            RotateZ                         ( radian Rz )                                       noexcept; 
        inline              vector3&            Rotate                          ( const radian3& R )                                noexcept;
        inline              vector3&            GridSnap                        ( float GridX, float GridY, float GridZ )           noexcept;


        inline              bool                isInRange                       ( const float Min, const float Max )        const   noexcept;
        xforceinline        bool                isValid                         ( void )                                    const   noexcept;
        constexpr           bool                isRightHanded                   ( const vector3& P1, const vector3& P2 )    const   noexcept;
                                
        constexpr           vector3             getReflection                   ( const vector3& V )                        const   noexcept;
        constexpr           vector3             getOneOver                      ( void )                                    const   noexcept;
        inline              float               getLength                       ( void )                                    const   noexcept;
        constexpr           float               getLengthSquared                ( void )                                    const   noexcept;
        constexpr           float               getDistanceSquare               ( const vector3& V )                        const   noexcept;
        inline              float               getDistance                     ( const vector3& V )                        const   noexcept;
        constexpr           vector3             getMax                          ( const vector3& V )                        const   noexcept;
        constexpr           vector3             getMin                          ( const vector3& V )                        const   noexcept;
        constexpr           vector3             getAbs                          ( void )                                    const   noexcept;
        constexpr           vector3             getLerp                         ( const float T, const vector3& End )       const   noexcept;
        inline              radian              getPitch                        ( void )                                    const   noexcept;
        inline              radian              getYaw                          ( void )                                    const   noexcept;
        inline              void                getPitchYaw                     ( radian& Pitch, radian& Yaw )              const   noexcept;
        inline              radian              getAngleBetween                 ( const vector3& V )                        const   noexcept;
        inline              void                getRotationTowards              ( const vector3&    DestV,
                                                                                  vector3&          RotAxis,
                                                                                  radian&           RotAngle )                 const noexcept; 
        inline              vector3             getVectorToLineSegment          ( const vector3&   Start, const vector3& End ) const noexcept;
        inline              vector3             getClosestPointInLineSegment    ( const vector3&   Start, const vector3& End ) const noexcept;
        inline              float               getSquareDistToLineSeg          ( const vector3&   Start, const vector3& End ) const noexcept;
        inline              float               getClosestPointToRectangle      ( const vector3&   P0,   // Origin from the edges.
                                                                                  const vector3&   E0,
                                                                                  const vector3&   E1,
                                                                                  vector3&         OutClosestPoint )            const noexcept;
                                                                         
        constexpr static    vector3             Up                              ( void )                                            noexcept { return { 0, 1, 0}; }
        constexpr static    vector3             Forward                         ( void )                                            noexcept { return { 0, 0, 1}; }
        constexpr static    vector3             Right                           ( void )                                            noexcept { return { 1, 0, 0}; }
        constexpr static    vector3             Down                            ( void )                                            noexcept { return { 0,-1, 0}; }
        constexpr static    vector3             Back                            ( void )                                            noexcept { return { 0, 0,-1}; }
        constexpr static    vector3             Left                            ( void )                                            noexcept { return {-1, 0, 0}; }
    
        inline              float*              operator ()                     ( void )                                            noexcept;
        inline              float&              operator []                     ( const std::int32_t i )                            noexcept;
        constexpr           float               operator []                     ( const std::int32_t i )                    const   noexcept;
        constexpr           bool                operator ==                     ( const vector3& V )                        const   noexcept;
        inline              const vector3&      operator +=                     ( const vector3& V )                                noexcept;
        inline              const vector3&      operator -=                     ( const vector3& V )                                noexcept;
        inline              const vector3&      operator *=                     ( const vector3& V )                                noexcept;
        inline              const vector3&      operator /=                     ( float Div )                                       noexcept;
        inline              const vector3&      operator *=                     ( float Scalar )                                    noexcept;
                                        
        constexpr friend    vector3             operator /                      ( const vector3& V, float Div )                     noexcept;
        constexpr friend    vector3             operator *                      ( float Scale, const vector3& V )                   noexcept;
        constexpr friend    vector3             operator *                      ( const vector3& V, float Scale )                   noexcept;
        constexpr friend    vector3             operator +                      ( float Scale, const vector3& V )                   noexcept;
        constexpr friend    vector3             operator +                      ( const vector3& V, float Scale )                   noexcept;
        constexpr friend    vector3             operator -                      ( float Scale, const vector3& V )                   noexcept;
        constexpr friend    vector3             operator -                      ( const vector3& V, float Scale )                   noexcept;
    
        constexpr friend    vector3             operator -                      ( const vector3& V )                                noexcept;
        constexpr friend    vector3             operator *                      ( const vector3& V0, const vector3& V1 )            noexcept;
        constexpr friend    vector3             operator -                      ( const vector3& V0, const vector3& V1 )            noexcept;
        constexpr friend    vector3             operator +                      ( const vector3& V0, const vector3& V1 )            noexcept;
        constexpr friend    vector3             operator /                      ( const vector3& V0, const vector3& V1 )            noexcept;

        constexpr friend    vector3             operator +                      ( const vector3d& V0, const vector3&  V1 )          noexcept;
        constexpr friend    vector3             operator +                      ( const vector3&  V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator -                      ( const vector3d& V0, const vector3&  V1 )          noexcept;
        constexpr friend    vector3             operator -                      ( const vector3&  V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator *                      ( const vector3d& V0, const vector3&  V1 )          noexcept;
        constexpr friend    vector3             operator *                      ( const vector3&  V0, const vector3d& V1 )          noexcept;
        constexpr friend    vector3             operator /                      ( const vector3d& V0, const vector3&  V1 )          noexcept;
        constexpr friend    vector3             operator /                      ( const vector3&  V0, const vector3d& V1 )          noexcept;
    
    public:

        union
        {
            floatx4       m_XYZW  {};
            struct
            {
                float     m_X;
                float     m_Y;
                float     m_Z;
                float     m_W;
            };
        };
    };
}
#endif