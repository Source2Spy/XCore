

namespace mecs::examples::test5
{
#if 0
    namespace component
    {
        struct position : mecs::component::double_buffer
        {
            xcore::vector2              m_Value;
        };

        struct velocity
        {
            xcore::vector2              m_Value;
        };

        struct boid_type_ref
        {
            entity::guid                m_Value;
        };

        struct boid_type : mecs::component::shared
        {
            constexpr static mecs::type_guid type_guid_v{"boid"};
        };
    }

    /*
    namespace groups
    {
        constexpr static mecs::component::group::descriptor<group::guid
        <
                component::spatial_cell_core
            ,   component::spatial_cell_entity

        > world_grid_v;

        constexpr static mecs::component::group::descriptor<group::guid<component::world>           world_v;
        constexpr static mecs::component::group::descriptor<group::guid<component::boid_type>       boid_type_v;
        constexpr static mecs::component::group::descriptor<group::guid<component::spatial_cell>    spatial_dbase_v;
    }
    */

    namespace physics
    {
        namespace world_grid
        {
            constexpr static int   grid_width_v  = 8;
            constexpr static int   grid_height_v = 8;

            struct vector2
            {
                int m_X, m_Y;
            };

            struct box
            {
                world_grid::vector2 m_Min;
                world_grid::vector2 m_Max;
            };

            world_grid::vector2 WorldToGrid( xcore::vector2 V )
            {
                world_grid::vector2 GridV;
                GridV.m_X = static_cast<int>( V.m_X / grid_width_v );
                GridV.m_Y = static_cast<int>( V.m_Y / grid_height_v);
                return GridV;
            }

            box SearchBox( world_grid::vector2 Pos )
            {
                return box{ {Pos.m_X-1, Pos.m_Y-1}, {Pos.m_X+1, Pos.m_Y+1} };
            }

            std::uint64_t ComputeKeyFromPosition( int X, int Y )
            {
                return xcore::bits::MurmurHash3( static_cast<std::uint64_t>((static_cast<std::uint64_t>(X)<<32) | Y) );
            }
        }

        namespace component
        {
            namespace spatial_cell
            {
                constexpr static std::size_t max_entity_count = 64;
                struct position : mecs::component::double_buffer, mecs::component::quantum_mutable
                {
                    std::array<xcore::vector2,max_entity_count>			m_lPosition;
                    std::array<float,max_entity_count>					m_lRadius;
                };

                struct count : mecs::component::double_buffer, mecs::component::quantum_mutable
                {
                    int													m_Value;
                }
                
                struct entity : mecs::component::double_buffer, mecs::component::quantum_mutable
                {
                    std::array<mecs::entity::instance,max_entity_count>	m_lEntity;
                };

                struct velocity : mecs::component::double_buffer, mecs::component::quantum_mutable
                {
                    std::array<xcore::vector2,max_entity_count>			m_lVelocity;
                };
                
                struct id
                {
                    world_grid::vector2 								m_Value;
                }
            }

            struct type : mecs::component::shared
            {
                float m_Radius;
            };
        }

        /*
        namespace group
        {
            constexpr static mecs::component::group::descriptor
            <
                    component::spatial_cell::core
                ,   component::spatial_cell::entity
            > fullcell_v;            
        }
        */
        
        namespace system
        {
            struct events
            {
                // onCollision
            };

            constexpr static mecs::query::define
            <
                mecs::query::all<component::spatial_cell::position
                               , component::spatial_cell::count
                               , component::spatial_cell::entity
                               , component::spatial_cell::id>
            > query2_v{};

            struct instance : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("system physics instance");

                using mecs::system::instance::instance;


                void opeprator() ( 
                        const component::spatial_cell::position& 	T0lPosition
                    ,	const component::spatial_cell::entity&   	T0lVelocity
                    ,   const component::spatial_cell::count&	 	T0Count
                    ,   const component::spatial_cell::entity&   	T0lEntity
                    ,	const component::spatial_cell::id&          T0ID 
                    ,		  component::spatial_cell::position& 	T1lPosition
                    ,	      component::spatial_cell::entity&   	T1lVelocity
                    ,         component::spatial_cell::count&	 	T1Count
                    ,         component::spatial_cell::entity&   	T1lEntity ) noexcept
                {
                    
                    const component::spatial_cell::position* T0CellMapPosition[3][3]  = {nullptr};
                    const component::spatial_cell::velocity* T0CellMapVelocity[3][3]  = {nullptr};
                    const component::spatial_cell::count*    T0CellMapCount[3][3]     = {nullptr};
                    const component::spatial_cell::entity*   T0CellMapEntity[3][3]    = {nullptr};
                          component::spatial_cell::position* T1CellMapPosition[3][3]  = {nullptr};
                          component::spatial_cell::velocity* T1CellMapVelocity[3][3]  = {nullptr};
                          component::spatial_cell::count*    T1CellMapCount[3][3]     = {nullptr};
                          component::spatial_cell::entity*   T1CellMapEntity[3][3]    = {nullptr};

                    T0CellMapPosition[1][1]  = &T0lPosition;
                    T0CellMapVelocity[1][1]  = &T0lVelocity;
                    T0CellMapCount[1][1]     = &T0Count;
                    T0CellMapEntity[1][1]    = &T0lEntity;

                    T1CellMapPosition[1][1]  = &T1lPosition;
                    T1CellMapVelocity[1][1]  = &T1lVelocity;
                    T1CellMapCount[1][1]     = &T1Count;
                    T1CellMapEntity[1][1]    = &T1lEntity;
                    
                    ForEachQuery(query2_v, [&](mecs::component::group::instance& Group )
                    {
                        const auto SearchBox = world_grid::SearchBox( ID.m_Value );
                        for( int y=SearchBox.m_Min.m_Y; y<SearchBox.m_Max.m_Y; ++y )
                        for( int x=SearchBox.m_Min.m_X; x<SearchBox.m_Max.m_X; ++x )
                        {
                            if( x == ID.m_Value.m_X && y == ID.m_Value.m_Y ) continue;
                            
                            findEntity( Group, mecs::entity::guid{ world_grid::ComputeKeyFromPosition(x,y) }, [&](
                                    const component::spatial_cell::position& 	T0lPosition
                                ,   const component::spatial_cell::position& 	T0lVelocity
                                ,   const component::spatial_cell::count&	 	T0Count
                                ,   const component::spatial_cell::entity&   	T0lEntity
                                ,  	      component::spatial_cell::position& 	T1lPosition
                                ,         component::spatial_cell::position& 	T1lVelocity
                                ,		  component::spatial_cell::count&	 	T1Count
                                ,	      component::spatial_cell::entity&   	T1lEntity
                                ) noexcept
                                {
                                    T0CellMapPosition[x][y]  = &T0lPosition;
                                    T0CellMapVelocity[x][y]  = &T0lVelocity;
                                    T0CellMapCount[x][y]     = &T0Count;
                                    T0CellMapEntity[x][y]    = &T0lEntity;
                                    
                                    T1CellMapPosition[x][y]  = &T1lPosition;
                                    T1CellMapVelocity[x][y]  = &T1lVelocity;
                                    T1CellMapCount[x][y]     = &T1Count;
                                    T1CellMapEntity[x][y]    = &T1lEntity;
                                }
                        }
                    });
                                        
                    const float DT = getTime().m_DeltaTime;
                    for( int i=0;i<Count.m_Value; ++i )
                    {
                        const auto&    T0Position = &T0lPosition,m_lPosition[i].m_Value;
                        const auto&    T0Velocity = &T0lPosition,m_lVelocity[i].m_Value;
                        xcore::vector2 T1Velocity = T0Velocity;
                        xcore::vector2 T1Position = T0Position + T1Velocity * DT;
                        
                        for( int y=0; y<3; y++)
                        for( int x=0; x<3; x++)
                        for( int c=0; c<T0CellMapCount[x][y].m_Value; c++)
                        {
                            if( &T0Position == &T0CellMapPosition[x][y].m_lPosition[c].m_Value ) continue;
                            
                            // Check for collision
                        }
                        
                        auto RelativeGridPos = WorldToGrid( T1Position ) -  T0ID.m_Value;
                        
                        const int C = T1CellMapCount[ RelativeGridPos.m_X ][RelativeGridPos.m_Y].m_Value++;
                        T1CellMapPosition[RelativeGridPos.m_X][RelativeGridPos.m_Y].m_lPosition[C] = T1Position;
                        T1CellMapVelocity[RelativeGridPos.m_X][RelativeGridPos.m_Y].m_lVelocity[C] = T1Velocity;
                        T1CellMapEntity  [RelativeGridPos.m_X][RelativeGridPos.m_Y].m_lEntity[C]   = T0lPosition,m_lEntity[i];
                    }
                }						
            }
            
            struct instance : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("system physics instance");

                using mecs::system::instance::instance;
#if 0
                
                virtual void qt_onRun( void ) noexcept override
                {
                    DoQuery( getQuery(), *this, mecs::system::instance::query_v );


                    DoQuery( getQuery(), *this, mecs::system::instance::query_v );
                    
                    getGroup 
                    <       const component::spatial_cell::core
                        ,   const component::spatial_cell::entity
                        ,         component::spatial_cell::core
                        ,         component::spatial_cell::entity 
                    >
                    ( std::tuple<component::spatial_cell::core, component::spatial_cell::entity>
                    , [&]( mecs::group::instance& GroupCell )
                    {
                        // Allow the system to compute
                        for( int i = getTime().m_FixedSteps; i; --i )
                        {
                            auto pError = ForEach( getQuery(), [&](
                                const entity::instance&                                             Entity
                            ,   const component::position&                                          T0Position
                            ,   component::position&                                                T1Position
                            ,   const component::velocity&                                          Velocity )
                            {
                                T1Position.m_Value = T0Position.m_Value + getTime().m_DeltaTime * Velocity.m_Value;

                                //
                                // Get all the cells that we need to deal with
                                //
                                const component::spatial_cell::core*     T0CellMap[3][3]  = {nullptr};
                                const component::spatial_cell::entity*   CellEntity[3][3] = {nullptr};

                                const auto SearchBox = world_grid::SearchBox( world_grid::WorldToGrid( T0Position.m_Value ) );
                                for( int y=SearchBox.m_Min.m_Y; y<SearchBox.m_Max.m_Y; ++y )
                                for( int x=SearchBox.m_Min.m_X; x<SearchBox.m_Max.m_X; ++x )
                                {
                                    findEntity( GroupCell, mecs::entity::guid{ world_grid::ComputeKeyFromPosition(x,y) }, [&]( const component::spatial_cell::core& CellCore, const component::spatial_cell::entity& CellEntity )
                                    {
                                        T0CellMap[x][y]  = &CellCore;
                                        CellEntity[x][y] = &CellEntity;
                                    });
                                }

                                //
                                // Now we can resolve collisions and such
                                //
                                for( int y=SearchBox.m_Min.m_Y; y<SearchBox.m_Max.m_Y; ++y )
                                for( int x=SearchBox.m_Min.m_X; x<SearchBox.m_Max.m_X; ++x )
                                {
                                    if( T0CellMap[x][y] == nullptr ) continue;
                                    auto& Cell  = *T0CellMap[x][y];
                                    auto  Count = Cell.m_nEntries.load(std::memory_order_relaxed);
                                    for( int i=0; i<Count; ++i)
                                    {
                                        (void)Cell.m_Position[i];
                                    }
                                }

                                //T1Position = ...

                                //
                                // Now we know its new position
                                //
                                auto GridPos = world_grid::WorldToGrid( T1Position.m_Value );
                                getOrCreateEntity( GroupCell, mecs::entity::guid{ world_grid::ComputeKeyFromPosition(GridPos.m_X,GridPos.m_Y) }, [&]( component::spatial_cell::core& CellCore, component::spatial_cell::entity& CellEntity )
                                {
                                    auto  Index = CellCore.m_nEntries ++;
                                    CellCore.m_Position = T1Position.m_Value;
                                    CellCore.m_Radius   = 66666666;
                                    CellEntity.m_Entity[Index] = Entity;
                                });
                            }));
                            xassert(pError);
                        }
                    });
                }
#endif
            };
        }
    }

    #endif
 }