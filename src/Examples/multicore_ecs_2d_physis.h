
namespace physics_2d
{
    struct position2d
    {
        xcore::vector2                  m_Value;
    };

    struct speed 
    {
        float                           m_Value;
    };

    struct velocity2d
    {
        xcore::vector2                  m_Value;
    };

    struct velocity3d
    {
        xcore::vector3d                 m_Value;
    };

    struct angular_speed
    {
        float                           m_Value;
    };

    struct angular_velocity
    {
        xcore::vector3d                 m_Value;
    };

    struct bounciness
    {
        float                           m_Value;
    };

    struct friction
    {
        float                           m_Value;
    };

    struct drag
    {
        float                           m_Value;
    };

    struct angular_velocity
    {
        xcore::vector3                  m_Value;
    };

    struct collision_2d
    {
        xcore::vector2                  m_Normal;
        float                           m_Distance;
        mecs::entity::guid              m_Entity1;
        mecs::entity::guid              m_Entity2;
    };

    struct circle_collider
    {
        position2d                      m_Position;
        float                           m_Radius;
    };

    struct polygon8_collider
    {
        std::array<xcore::vector2,8>    m_Points;
        std::int8_t                     m_Count;
    };

    struct polygon_collider
    {
        std::int8_t                     m_Count;
        xcore::vector2*                 m_Points;
    };
}