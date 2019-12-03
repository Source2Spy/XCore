
#include "multicore_ecs.h"

namespace mecs_examples
{
    //------------------------------------------------------------------------------------
    // Test1 - testing different component types
    //------------------------------------------------------------------------------------
    namespace test1
    {
        struct simple
        {
            constexpr static mecs::type_guid type_guid_v{"simple"};
            int m_Value{ 22 };
        };

        struct double_buff : mecs::component::double_buffer
        {
            constexpr static mecs::type_guid type_guid_v{ "double_buff" };
            int m_Value{ 123 };
        };

        struct my_tag : mecs::component::tag 
        {
            constexpr static mecs::type_guid type_guid_v{ "my_tag" };
        };

        struct my_system : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("my_system");
            constexpr static mecs::query::define
            <
                mecs::query::all<simple>
              , mecs::query::none<my_tag>
            > query_v{};

            using parent_t = mecs::system::instance;
            using parent_t::parent_t;

            constexpr
            void Update( const double_buff& Buff, simple& Simple ) const noexcept
            {
                Simple.m_Value += Buff.m_Value;
            }
        };

        void Test()
        {
            XCORE_PERF_ZONE_SCOPED()
            mecs::world::instance World;
            World.registerComponents
            <
                    simple
                ,   double_buff
                ,   my_tag
            >();

            World.AddGraphConnection<my_system>( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Create a bunch of entities
            //
            {
                xcore::scheduler::channel MyChannel(xconst_universal_str("Create Entities"));
                auto& Group = World.getOrCreateGroup<simple,double_buff,my_tag>();

                for(int i=0; i<xcore::get().m_Scheduler.getWorkerCount(); i++)
                    MyChannel.SubmitJob([&]
                    {
                        while (Group.newSize() < 10000000ull XCORE_CMD_ASSERT( / 100 ) )
                        {
                            World.createEntity( Group, [](mecs::entity::tmp_ref tmpEntityRef)
                            {
                                auto&   Entity = tmpEntityRef.getComponent<mecs::entity::instance>();
                                auto&   Simple = tmpEntityRef.getComponent<simple>();
                                auto&   Double = tmpEntityRef.getComponent<double_buff>();
                                assert(Simple.m_Value == 22);
                                assert(Double.m_Value == 123);
                            });
                        }
                    });

                MyChannel.join();
            }

            World.Start();
            for( int i=0; i<100; i++) World.Resume();
        }
    }


    //------------------------------------------------------------------------------------
    // Test2 - Spawner example
    //------------------------------------------------------------------------------------
    namespace test2
    {
        namespace life
        {
            struct component
            {
                constexpr static mecs::type_guid type_guid_v{ "life component" };
                int m_Value{10};
            };

            namespace system
            {
                struct decrement : mecs::system::instance
                {
                    constexpr static auto   system_name_v   = xconst_universal_str("life decrement system");
            
                    using mecs::system::instance::instance;

                    void Update( mecs::entity::instance& Entity, life::component& Life ) noexcept
                    {
                        Life.m_Value--;
                        if( Life.m_Value == 0 ) 
                        {
                            deleteEntity(Entity);
                        }
                    }
                };
            }
        }

        namespace spawner
        {
            struct component
            {
                constexpr static mecs::type_guid type_guid_v{ "spawner component" };
                using fn = void(mecs::entity::tmp_ref);

                std::uint16_t                   m_Count;
                mecs::component::group::guid    m_gGroup;
                fn*                             m_fnCreated{nullptr};
            };

            struct system : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("spawner_system");
            
                using mecs::system::instance::instance;

                void Update( mecs::entity::instance& Entity, spawner::component& S ) noexcept
                {
                    getGroup( S.m_gGroup, [&](mecs::component::group::instance& Group ) noexcept
                    {
                        for( int i=0; i<S.m_Count; ++i )
                        {
                            createEntity( Group, [&]( mecs::entity::tmp_ref Ref ) constexpr noexcept
                            {
                                if( S.m_fnCreated ) S.m_fnCreated(Ref);
                            });
                        }
                    });

                    deleteEntity(Entity);
                }
            };
        }

        void Test()
        {
            XCORE_PERF_ZONE_SCOPED()
            mecs::world::instance World;
            World.registerComponents
            <
                    life::component
                ,   spawner::component
            >();

            World.AddGraphConnection<spawner::system>           ( World.m_StartSyncPoint, World.m_EndSyncPoint );
            World.AddGraphConnection<life::system::decrement>   ( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Create a bunch of entities
            //
            auto&   Group        = World.getOrCreateGroup<spawner::component>();

            World.createEntity( Group, [&](mecs::entity::tmp_ref Ref) noexcept
            {
                auto& Spawner = Ref.getComponent<spawner::component>();

                Spawner.m_Count     = 10;
                Spawner.m_gGroup    = World.getOrCreateGroup<life::component>().m_Guid;
                Spawner.m_fnCreated = [](mecs::entity::tmp_ref Ref ){ Ref.getComponent<life::component>().m_Value = 23; };
            });

            //
            // Get started
            //
            World.Start();
            for( int i=0; i<100; i++) World.Resume();
        }
    }


    //------------------------------------------------------------------------------------
    // Test3
    //------------------------------------------------------------------------------------
    namespace test3
    {
        struct simple
        {
            constexpr static mecs::type_guid type_guid_v{"simple"};
            int m_Value{ 22 };
        };

        struct double_buff : mecs::component::double_buffer
        {
            constexpr static mecs::type_guid type_guid_v{ "double_buff" };
            int m_Value{ 123 };
        };

        struct my_tag : mecs::component::tag 
        {
            constexpr static mecs::type_guid type_guid_v{ "my_tag" };
        };

        struct my_system : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("my_system");
            constexpr static mecs::query::define
            <
                mecs::query::all<simple>
              , mecs::query::none<my_tag>
            > query_v{};

            using parent_t = mecs::system::instance;
            using parent_t::parent_t;

            xcore::random::small_generator R;

            void Update( mecs::entity::instance& Entity, const double_buff& Buff, simple& Simple ) noexcept
            {
                // This is the equivalent of removing a component (double_buff) from the entity
                if( (R.RandU32() % 10)  == 0 )
                    getGroupBy< std::tuple<>, std::tuple<double_buff> >( Entity, [&](mecs::component::group::instance& Group )
                    {
                        moveEntityToGroup( Group, Entity );
                    });
            }
        };

        void Test()
        {
            XCORE_PERF_ZONE_SCOPED()
            mecs::world::instance World;
            World.registerComponents
            <
                    simple
                ,   double_buff
                ,   my_tag
            >();

            World.AddGraphConnection<my_system>( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Allocate a bunch of pages
            //
            World.m_PageMgr.PreallocatePages( 20000 );

            //
            // Create a bunch of entities
            //
            {
                xcore::scheduler::channel MyChannel(xconst_universal_str("Create Entities"));
                auto& Group = World.getOrCreateGroup<simple,double_buff,my_tag>();

                for(int i=0; i<xcore::get().m_Scheduler.getWorkerCount(); i++)
                    MyChannel.SubmitJob([&]
                    {
                        while (Group.newSize() < 300000ull XCORE_CMD_ASSERT( / 100 ) )
                        {
                            World.createEntity( Group, [](mecs::entity::tmp_ref tmpEntityRef)
                            {
                                auto&   Entity = tmpEntityRef.getComponent<mecs::entity::instance>();
                                auto&   Simple = tmpEntityRef.getComponent<simple>();
                                auto&   Double = tmpEntityRef.getComponent<double_buff>();
                                assert(Simple.m_Value == 22);
                                assert(Double.m_Value == 123);
                            });
                        }
                    });

                MyChannel.join();

                //
                // Check if we can add some other kind of entity
                //
                auto& NewGroup = World.getOrCreateGroupBy<std::tuple<>, std::tuple<simple>>( Group );
                World.createEntity( NewGroup, [](mecs::entity::tmp_ref tmpEntityRef)
                {
                    auto&   Entity = tmpEntityRef.getComponent<mecs::entity::instance>();
                    auto&   Double = tmpEntityRef.getComponent<double_buff>();
                    assert(Double.m_Value == 123);
                });
            }

            World.Start();
            for( int i=0; i<100; i++) 
            {
                World.Resume();
            }
        }
    }

    //------------------------------------------------------------------------------------
    // Test4
    // Tutorial from the INTERNET: https://www.youtube.com/watch?v=ILfUuBLfzGI&t=789s
    //------------------------------------------------------------------------------------
    namespace test4
    {
        namespace component
        {
            struct level
            {
                constexpr static mecs::type_guid type_guid_v{"level component"};
                float               m_Value;
            };

            struct translation_2d
            {
                constexpr static mecs::type_guid type_guid_v{"translation component"};
                xcore::vector2     m_Value;
            };

            struct render_mesh
            {
                constexpr static mecs::type_guid type_guid_v{"render_mesh_com"};
        
                // mesh                     m_Mesh;
                // material_instance        m_MaterialInstance;
                // int                      m_SubMesh;
                // int                      m_Layer;
                // shadow_cascading_mode    m_CastShadows;
                // bool                     m_bReceiveShadows;
            };

            struct local_to_world
            {
                constexpr static mecs::type_guid type_guid_v{"local_to_world_com"};
                xcore::matrix4  m_L2W;
            };

            struct move_speed
            {
                constexpr static mecs::type_guid type_guid_v{"move_speed"};
                float m_Value;
            };
        }

        namespace system
        {
            struct level_up : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("level up system");

                using mecs::system::instance::instance;

                void Update( component::level& Level ) noexcept
                {
                    Level.m_Value += 1.0f * getTime().m_DeltaTime;
                }
            };

            struct mover : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("mover_sys");

                using mecs::system::instance::instance;

                void Update( component::translation_2d& Translation, component::move_speed& MoveSpeed ) noexcept
                {
                    Translation.m_Value.m_Y += MoveSpeed.m_Value * getTime().m_DeltaTime;
                    if(Translation.m_Value.m_Y >  5.0f ) MoveSpeed.m_Value = -xcore::Abs(MoveSpeed.m_Value);
                    if(Translation.m_Value.m_Y < -5.0f ) MoveSpeed.m_Value = +xcore::Abs(MoveSpeed.m_Value);
                }
            };
        }
        
        void Test()
        {
            XCORE_PERF_ZONE_SCOPED()
            mecs::world::instance World;
            World.registerComponents
            <
                    component::level
                ,   component::translation_2d
                ,   component::render_mesh
                ,   component::local_to_world
                ,   component::move_speed
            >();

            World.AddGraphConnection<system::level_up>( World.m_StartSyncPoint, World.m_EndSyncPoint );
            World.AddGraphConnection<system::mover>( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Create a bunch of entities
            //
            {
                xcore::random::small_generator Rnd;
                auto& Group = World.getOrCreateGroup
                <
                        component::level
                    ,   component::translation_2d
                    ,   component::render_mesh
                    ,   component::local_to_world
                    ,   component::move_speed
                >();

                for( int i=0; i<100000; ++i )
                {
                    World.createEntity( Group, [&](mecs::entity::tmp_ref tmpRef)
                    {
                        tmpRef.getComponent<component::level>().m_Value             = Rnd.RandF32(10,20);
                        tmpRef.getComponent<component::move_speed>().m_Value        = Rnd.RandF32(1.0f,2.0f);
                        tmpRef.getComponent<component::translation_2d>().m_Value    = xcore::vector2(Rnd.RandF32(-8.0f,8.0f), Rnd.RandF32(-5.0f,5.0f));
                    });
                }
            }

            //
            // Start the game
            //
            World.Start();
            for( int i=0; i<100; i++) 
            {
                World.Resume();
            }
        }
    }
}

#include "multicore_ecs_examples_boids.h"