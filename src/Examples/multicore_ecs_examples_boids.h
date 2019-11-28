

namespace mecs::examples::test5
{
/*
    namespace component
    {
        struct world : mecs::component::shared
        {
            using key = xcore::guid::unit<64>;
            struct instance
            {
                mecs::entity::instance  m_Entity;
                xcore::vector3d         m_Position;
            };
            mecs::tools::map_fixed< key, mecs::tools::pool<instance>  > 
        };

        struct boid
        {
            constexpr static mecs::type_guid type_guid_v{"boid"};

            float m_CellRadious;
            float m_CellRadius;
            float m_SeparationWeight;
            float m_AlignmentWeight;
            float m_TargetWeight;
            float m_ObstacleAversionDistance;
            float m_MoveSpeed;
        };
    }

    namespace system
    {
    
        struct boid : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("boid system");

            using mecs::system::instance::instance;

            void Update( component::translation_2d& Translation, component::move_speed& MoveSpeed ) noexcept
            {
                Translation.m_Value.m_Y += MoveSpeed.m_Value * getTime().m_DeltaTime;
                if(Translation.m_Value.m_Y >  5.0f ) MoveSpeed.m_Value = -xcore::Abs(MoveSpeed.m_Value);
                if(Translation.m_Value.m_Y < -5.0f ) MoveSpeed.m_Value = +xcore::Abs(MoveSpeed.m_Value);
            }
        };
    }
    */
}