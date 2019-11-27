
#include "multicore_ecs.h"

namespace mecs_examples
{
    //------------------------------------------------------------------------------------
    // Test1
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
                        while (Group.newSize() < 10000000ull XCORE_CMD_ASSERT( / 10 ) )
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
    // Test2
    //------------------------------------------------------------------------------------
    namespace test2
    {
        struct life
        {
            constexpr static mecs::type_guid type_guid_v{ "life" };
            int m_Live{10};
        };

        struct spawner
        {
            constexpr static mecs::type_guid type_guid_v{ "spawner" };

            std::uint16_t                   m_Count;
            mecs::component::group::guid    m_gGroup;
        };

        struct spawner_system : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("spawner_system");
            constexpr static mecs::query::define
            <
                mecs::query::all<spawner>
            > query_v{};
            
            using parent_t = mecs::system::instance;
            using parent_t::parent_t;

            void Update( mecs::entity::instance& Entity, spawner& S ) noexcept
            {
                getGroup( S.m_gGroup, [&](mecs::component::group::instance& Group ) noexcept
                {
                    for( int i=0; i<S.m_Count; ++i )
                    {
                        createEntity( Group, []( mecs::entity::tmp_ref Ref ) noexcept
                        {
                            (void)Ref.getComponent<life>();
                        });
                        
                    }
                });

                deleteEntity(Entity);
            }
        };

        struct life_system : mecs::system::instance
        {
            constexpr static auto   system_name_v   = xconst_universal_str("life_system");
            constexpr static mecs::query::define
            <
                mecs::query::all<life>
            > query_v{};
            
            using parent_t = mecs::system::instance;
            using parent_t::parent_t;

            void Update( mecs::entity::instance& Entity, life& S ) noexcept
            {
                S.m_Live--;
                if( S.m_Live == 0 ) 
                {
                    deleteEntity(Entity);
                }
            }
        };

        void Test()
        {
            XCORE_PERF_ZONE_SCOPED()
            mecs::world::instance World;
            World.registerComponents
            <
                    life
                ,   spawner
            >();

            World.AddGraphConnection<spawner_system>( World.m_StartSyncPoint, World.m_EndSyncPoint );
            World.AddGraphConnection<life_system>   ( World.m_StartSyncPoint, World.m_EndSyncPoint );

            //
            // Create a bunch of entities
            //
            auto&   Group        = World.getOrCreateGroup<spawner>();

            World.createEntity( Group, [&](mecs::entity::tmp_ref Ref) noexcept
            {
                auto& Spawner = Ref.getComponent<spawner>();

                Spawner.m_Count  = 10;
                Spawner.m_gGroup = World.getOrCreateGroup<life>().m_Guid;
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
                        while (Group.newSize() < 300000ull XCORE_CMD_ASSERT( / 10 ) )
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

}