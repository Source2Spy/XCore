

namespace mecs::examples::test5
{
/*
    struct pepe
    {
        void operator() ( int ) {}
    };

    template< typename T_THIS, typename T_FUNC >
    void Function( T_THIS& This, T_FUNC&& X )
    {
        using t = xcore::function::traits<decltype( X )>;
        static_assert( 1 == t::arg_count_v );

        //if constexpr ( std::same_v<t::class_type, void > )
        {
            (This.*(X))(5);
        }
        //else
        {
        }
    }

    template< typename T >
    void Function2( T&& Obj )
    {
        using t = xcore::function::traits<decltype(Obj)>;
        Obj(5);
    }

    void Today()
    {
        pepe Tt;
        Function( Tt, &pepe::operator() );


        Function2( Tt );
        Function2( [](int x){} );
    }
*/



/*
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

        struct boid_type : component::shared
        {
            constexpr static mecs::type_guid type_guid_v{"boid"};
        };
    }

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
                GridV.m_X.m_Value = static_cast<int>( V.m_X / grid_width_v );
                GridV.m_Y.m_Value = static_cast<int>( V.m_Y / grid_height_v);
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
                struct core : mecs::component::quantum_mutable, mecs::component::double_buffer
                {
                    std::atomic<int>                        m_nEntries;
                    std::array<xcore::vector2,64>           m_Position;
                    std::array<float,64>                    m_Radius;
                };

                struct entity : mecs::component::quantum_mutable, mecs::component::double_buffer
                {
                    std::array<mecs::entity::instance,64>   m_Entity;
                }
            }

            struct type : mecs::component::shared
            {
                float m_Radius;
            };
        }

        namespace group
        {
            constexpr static mecs::component::group::descriptor
            <
                    component::spatial_cell::core
                ,   component::spatial_cell::entity
            > fullcell_v;            
        }

        namespace system
        {
            struct events
            {
                // onCollision
            };

            struct instance : mecs::system::instance
            {
                constexpr static auto   system_name_v   = xconst_universal_str("system physics instance");

                using mecs::system::instance::instance;

#if 0
                
                virtual void qt_onRun( void ) noexcept override
                {
                    DoQuery<instance>();
                    
                    // Allow the system to compute
                    for( int i = getTime().m_FixedSteps; i; --i )
                        ForEach( reinterpret_cast< typename system_function_t::args_t* >(nullptr) ).CheckError();
                }
#endif
                
                constexpr static mecs::component::group::dependency
                <
                    group::fullcell_query_v
                    , const component::spatial_cell::core
                    , const component::spatial_cell::entity
                    ,       component::spatial_cell::core
                            component::spatial_cell::entity

                > group_dependency_v;

                void Update( 
                        const entity::instance&                                             Entity
                    ,   const component::position&                                          T0Position
                    ,   component::position&                                                T1Position
                    ,   const component::velocity&                                          Velocity
                    ,   mecs::component::group::reference<group_dependency_v>&              GroupCell ) noexcept
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
                }
            };
        }
    }
    */
}