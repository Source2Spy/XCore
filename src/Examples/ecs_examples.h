
#include "ecs.h"

//------------------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------------------
namespace ecs_examples
{
    //------------------------------------------------------------------------------------------
    // give a limited game mgr
    //------------------------------------------------------------------------------------------
    struct my_main : ecs::main_system::instance
    {
        int m_nMaxFrames = 10;

        virtual void onExecute(void) noexcept override
        {
            if (m_FrameNumber == m_nMaxFrames)
                m_bPause = true;
        }
    };

    //------------------------------------------------------------------------------------------
    // example01 - Basics
    //------------------------------------------------------------------------------------------
    namespace example01
    {
        // Examples of components
        // Note all components must have their own factory type
        struct my_component1 
        {
            using factory = ecs::make_factory<ecs::name_guid("my_component1"), my_component1, 1024>;
            int m_one = 0;
        };

        struct my_component2 
        {
            using factory = ecs::make_factory<ecs::name_guid("my_component2"), my_component2, 1024>;
            int m_two = 33;
        };

        // Example of my own system. It mutates my_component1 with contents of my_component2
        struct my_system
        {
            my_system(ecs::main_system::instance& MainSystem) {}
            void Update(my_component1& A, const my_component2& B) noexcept
            {
                A.m_one = B.m_two * 1000;
            }
        };

        // Example Test
        void Test(void) noexcept
        {
            my_main Game;

            // Generate my unique game 
            Game.registerComponent<my_component1>();
            Game.registerComponent<my_component2>();
            Game.AddGraphConnection<my_system>(Game.m_StartSyncPoint, Game.m_EndSyncPoint);

            // Create a few entities
            ecs::entity::guid gEntity1{ xcore::not_null };
            ecs::entity::guid gEntity2{ xcore::not_null };
            
            Game.createEntity<my_component1>(gEntity1);
            Game.createEntity<my_component1, my_component2>(gEntity2);

            // Start the game
            Game.Start();

            // Delete my two entities
            Game.deleteEntity(gEntity1);
            Game.deleteEntity(gEntity2);
        }
    }

    //------------------------------------------------------------------------------------------
    // example02 - Spawner
    //------------------------------------------------------------------------------------------
    namespace example02
    {
        struct my_spawner
        {
            using factory = ecs::make_factory<ecs::name_guid("my_spawner"), my_spawner, 1024>;
            int CountX;
            int CountY;
            // Prefab here ideally
        };

        struct my_component
        {
            using factory = ecs::make_factory<ecs::name_guid("my_component"), my_component, 1024>;
            int PosX;
            int PosY;
        };

        // Example of my own system. It mutates my_component1 with contents of my_component2
        struct spawner_system 
        {
            spawner_system(ecs::main_system::instance& MainSystem) : m_MainSystem{ MainSystem }
            {
                m_EvenLink.Connect(m_MainSystem.m_Events.m_OnStart);
            }
            
            void Update(const ecs::entity::instance& Entity, const my_spawner& Spawner) noexcept
            {
                for (auto x = 0; x < Spawner.CountX; x++)
                {
                    for (auto y = 0; y < Spawner.CountY; y++)
                    {
                        ecs::entity::guid gEntity{ xcore::not_null };
                        auto EntityTmpRef = m_MainSystem.createEntity<my_component>(gEntity);
                        EntityTmpRef.getComponent<my_component>() = my_component{ x,y };
                    }
                }

                m_MainSystem.deleteEntity(Entity.m_Guid);
            }

            void msgStart() noexcept
            {
                //TODO: Add someone start up code here
                ecs::entity::guid   gEntity{ xcore::not_null };
                auto                EntityTmpRef = m_MainSystem.createEntity<my_spawner>(gEntity);
                EntityTmpRef.getComponent<my_spawner>() = my_spawner{ 10, 10 };
            }

            ecs::main_system::instance&                     m_MainSystem;
            ecs::main_system::events::on_start::delegate    m_EvenLink{ this, &spawner_system::msgStart };
        };

        void Test(void) noexcept
        {
            my_main Game;

            // Generate my unique game 
            Game.registerComponent<my_spawner>();
            Game.registerComponent<my_component>();
            Game.AddGraphConnection<spawner_system>(Game.m_StartSyncPoint, Game.m_EndSyncPoint);

            // Start the game
            Game.Start();
        }
    }

    //------------------------------------------------------------------------------------------
    // Test
    //------------------------------------------------------------------------------------------
    void Test(void)
    {
        example01::Test();
        example02::Test();
    }
}

//------------------------------------------------------------------------------------------
// Properties
//------------------------------------------------------------------------------------------

property_begin_name(ecs_examples::example01::my_component1, "Component1" )
{
    property_var(m_one)
}
property_end()

property_begin_name(ecs_examples::example01::my_component2, "Component2" )
{
    property_var(m_two)
}
property_end()

//------------------------------------------------------------------------------------------

property_begin_name(ecs_examples::example02::my_component, "MyComponent")
{
      property_var(PosX)
    , property_var(PosY)
}
property_end()

property_begin_name(ecs_examples::example02::my_spawner, "Spawner")
{
      property_var(CountX)
    , property_var(CountY)
}
property_end()
