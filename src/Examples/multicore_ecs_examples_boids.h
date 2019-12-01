

namespace mecs::examples::test5
{
/*
    namespace component
    {
        struct spatial_cell : mecs::component::quantum_mutable, mecs::component::double_buffer
        {
            std::atomic<int>                        m_nEntries;
            std::array<mecs::entity::instance,64>   m_Entity;
            std::array<xcore::vector3d,64>          m_Position;
        };

        struct world : component::shared
        {
            xcore::vector3              m_WorldDimensions;
        };

        struct l2w
        {
            xcore::matrix4              m_Value;
        };

        struct position : mecs::component::double_buffer
        {
            xcore::vector3d             m_Value;
        };

        struct velocity
        {
            xcore::vector3d             m_Value;
        };

        struct boid_type_ref
        {
            entity::guid                m_Value;
        };

        struct boid_type : component::shared
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

    namespace groups
    {
        constexpr static mecs::component::group::descriptor<group::guid<component::world>           world_v;
        constexpr static mecs::component::group::descriptor<group::guid<component::boid_type>       boid_type_v;
        constexpr static mecs::component::group::descriptor<group::guid<component::spatial_cell>    spatial_dbase_v;
    }

    namespace system
    {
        entity::guid ComputeFromPosition( xcore::vector3d V );

        struct boid : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("boid system");

            // Dependencies
            std::tuple< groups::world_v, boid_type_v, spatial_dbase_v >;

            using mecs::system::instance::instance;

            void Update( 
                    const entity::instance&     Entity
                ,   const component::position&  T0Position
                ,   component::position&        T1Position
                ,   component::velocity&        Velocity ) noexcept
            {
                T1Position.m_Value = m_T0Position.m_Value + getTime().m_DeltaTime * Velocity.m_Value;

                auto T0Key   = ComputeFromPosition( m_T0Position.m_Value );
                auto T1Key   = ComputeFromPosition( T1Position.m_Value );

                getOrCreateSharedEntity( Key, group::spatial_dbase_v, ( component::spatial_cell& T1Cell )
                {
                    auto  Index = T1Cell.m_nEntries++;
                    auto& Entry = Cell.m_lEntries[Index];
                    
                    Entry.m_Entity[Index]   = Entity;
                    Entry.m_Position[Index] = T1Position;
                });

                getGroup( groups::world_v.m_Guid, [&](mecs::component::group::instance& Group )
                {
                    
                
                });
            }
        };
    }
*/
}