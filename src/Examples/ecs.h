
//#include "xcore.h"
#include "pool.h"
#include "helpers.h"
#include "multicore_ecs.h"

namespace ecs
{
    namespace main_system
    {
        struct instance;
    }

    namespace factory
    {
        template< std::uint64_t T_GUID, typename T_COMPONENT, std::size_t T_SIZE >
        struct custom_instance;
    }

    namespace sync_point
    {
        struct instance;
    }

    namespace filter
    {
        using guid = xcore::guid::unit<64, struct ecs_filter_tag>;
    }

    namespace group
    {
        struct instance;
    }

    namespace system
    {
        struct instance;
    }

    //------------------------------------------------------------------------------------------
    // COMPONENT
    //------------------------------------------------------------------------------------------
    namespace component
    {
        using type_guid = xcore::guid::unit<64, struct component_type_tag>;

    }

    //------------------------------------------------------------------------------------------
    // HELPERS
    //------------------------------------------------------------------------------------------
    template< typename T, std::size_t N >
    constexpr static std::uint64_t name_guid(T (&Name)[N]) { return xcore::crc<64>::FromString(Name).m_Value; }

    template< std::uint64_t T_GUID, typename T, std::size_t T_PAGE_SIZE >
    using make_factory = typename factory::custom_instance< T_GUID, T, T_PAGE_SIZE >;

    using properties_tuple = std::tuple<const property::table*, component::type_guid, void*>;

    //------------------------------------------------------------------------------------------
    // MGR - Managers are the data-bases/pools for components of an specific type
    //------------------------------------------------------------------------------------------
    namespace mgr
    {
        struct instance
        {
            struct index 
            {
                constexpr static std::uint32_t invalid_v = ~std::uint32_t(0); 
                constexpr bool isValid(void) const noexcept { return m_Value!= invalid_v; }
                std::uint32_t m_Value; 
            };

            constexpr                   instance        (component::type_guid G)    noexcept : m_guidComponentType{ G } {}
            virtual                    ~instance        (void)                      noexcept = default;
            virtual index               appendComponent (void)                      noexcept = 0;
            virtual bool                deleteComponent (index)                     noexcept = 0;
            virtual void*               getElement      (index)                     noexcept = 0;
            virtual const void*         getElement      (index) const               noexcept = 0;
            virtual std::size_t         size            (void) const                noexcept = 0;
            virtual std::tuple<const property::table*, void*>    getProperties   (index) const               noexcept = 0;

            template< typename T >
            T& getAs(index Index) noexcept
            {
                // Make sure that the types matches
                assert(m_guidComponentType == T::factory::guid_component_type_v);
                return *reinterpret_cast<T*>(getElement(Index));
            }

            template< typename T >
            const T& getAs(index Index) const noexcept
            {
                // Make sure that the types matches
                assert(m_guidComponentType == T::factory::guid_component_type_v);
                return *reinterpret_cast<T*>(getElement(Index));
            }

            component::type_guid        m_guidComponentType;
        };
    }

    //------------------------------------------------------------------------------------------
    // ENTITY - the main component
    //------------------------------------------------------------------------------------------
    namespace entity
    {
        using guid = xcore::guid::unit<64, struct ecs_entity_tag>;

        union flags
        {
            std::uint32_t m_Value{ 0 };
            struct
            {
                bool  m_isDeleted : 1
                    , m_isDisable : 1
                    , m_isInWorld : 1
                    ;
            };
        };

        struct instance
        {
            using factory = factory::custom_instance<0, instance, 1024>;
            entity::guid    m_Guid;     // This is 64 bits
            entity::flags   m_Flags;    // This is 32 bits
                                        // Means 32bits are been used to padding here
        };

        struct tmp_ref
        {
            constexpr bool isValid(void) const noexcept { return !!m_pGroupInstance; }

            template< typename T >
            T&                  getComponent(component::type_guid TypeGuid = T::factory::guid_component_type_v) noexcept;

            template< typename T >
            constexpr const T&  getComponent( component::type_guid TypeGuid = T::factory::guid_component_type_v ) const noexcept;

            std::vector<properties_tuple> getProperties(void) const noexcept;

            group::instance*        m_pGroupInstance{ nullptr };
            mgr::instance::index    m_Index{};
        };

        // returned by a createEntity ones it dies the entity will be able to be executed by systems
        struct tmp_ref_create : tmp_ref
        {
            tmp_ref_create() = default;
            tmp_ref_create(tmp_ref_create&& Entry) noexcept : tmp_ref{ Entry } { Entry.m_pGroupInstance = nullptr; }
            ~tmp_ref_create() noexcept;
        };
    }

    //------------------------------------------------------------------------------------------
    // FACTORY - factories are able to create MGRs when we need them
    //------------------------------------------------------------------------------------------
    namespace factory
    {
        struct instance
        {
            virtual mgr::instance& CreateMgr() noexcept = 0;
        };

        template< std::uint64_t T_GUID, typename T_COMPONENT, std::size_t T_SIZE >
        struct custom_instance final : instance
        {
            constexpr static component::type_guid guid_component_type_v{ T_GUID };

            struct custom_mgr final : mgr::instance
            {
                using mgr::instance::instance;
                using pool_t = containers::pool<T_COMPONENT, T_SIZE >;

                pool_t m_Pool;

                virtual index               appendComponent (void)              noexcept override { return { static_cast<std::uint32_t>(m_Pool.append()) }; }
                virtual bool                deleteComponent (index ID)          noexcept override { return m_Pool.deleteBySwap(ID.m_Value); }
                virtual void*               getElement      (index ID)          noexcept override { return &m_Pool[ID.m_Value]; }
                virtual const void*         getElement      (index ID)  const   noexcept override { return &m_Pool[ID.m_Value]; }
                virtual std::size_t         size            (void)      const   noexcept override { return m_Pool.size(); }
                virtual std::tuple<const property::table*, void*> getProperties (index ID)  const   noexcept override { auto& C = m_Pool[ID.m_Value]; return { &property::getTable(C), (void*)&C}; }

            };

            virtual mgr::instance& CreateMgr() noexcept override
            {
                return *new custom_mgr{ guid_component_type_v };
            }
        };
    }

    //------------------------------------------------------------------------------------------
    // GROUP - Groups are the more complex data-bases where entities and components of a particular setup are store.
    //------------------------------------------------------------------------------------------
    namespace group
    {
        struct instance
        {
            struct component_list
            {
                ecs::component::type_guid                   m_guidComponentType;
                std::unique_ptr<ecs::mgr::instance>         m_upComponentMgr;
            };

            instance(filter::guid guidGroup, main_system::instance& MainSystem, std::span<const component::type_guid> lGuidComponentTypes) noexcept;

            ecs::mgr::instance* findMgr( component::type_guid guidType ) noexcept
            {
                for (auto& E : m_lComponents)
                    if (E.m_guidComponentType == guidType )
                        return E.m_upComponentMgr.get();

                return nullptr;
            }

            const ecs::mgr::instance* findMgr(component::type_guid guidType) const noexcept
            {
                for (auto& E : m_lComponents)
                    if (E.m_guidComponentType == guidType)
                        return E.m_upComponentMgr.get();

                return nullptr;
            }

            void deleteEntity(entity::guid guidEntity, mgr::instance::index Index) noexcept
            {
                m_lDeleteList.push_back({ guidEntity, Index });
                ecs::mgr::instance* pEntityMgr = findMgr(entity::instance::factory::guid_component_type_v);
                assert(pEntityMgr);
                auto& Entity = pEntityMgr->getAs<entity::instance>(Index);
                Entity.m_Flags.m_isDeleted = true;
            }

            mgr::instance::index createEntity( entity::guid guidEntity, bool bMidFrame ) noexcept
            {
                //
                // Add all requested components
                //
                mgr::instance::index Index{ mgr::instance::index::invalid_v };
                for (auto& E : m_lComponents)
                {
                    if( Index.isValid() )
                    {
                        E.m_upComponentMgr->appendComponent();
                    }
                    else
                    {
                        Index = E.m_upComponentMgr->appendComponent();
                        auto& Entity = E.m_upComponentMgr->getAs<entity::instance>(Index);
                        Entity.m_Guid = guidEntity;
                        Entity.m_Flags.m_Value = 0;
                        m_NewEntities++;
                        Entity.m_Flags.m_isInWorld = false;
                    }
                }

                return Index;
            }

            void onUpdateEndFrame(main_system::instance& MainSystem) noexcept;

            void setEntityInWorld(mgr::instance::index Index) noexcept
            {
                // const auto Size = m_lComponents[0].m_upComponentMgr->size();
                // for( int i=0; i< Size; ++i ) m_lComponents[i].m_upComponentMgr->NotifyInWorld();
                m_lComponents[0].m_upComponentMgr->getAs<entity::instance>(Index).m_Flags.m_isInWorld = true;
            }

            std::vector<properties_tuple> getProperties(mgr::instance::index Index) const noexcept
            {
                std::vector<properties_tuple> List;
                for (auto& E : m_lComponents)
                {
                    auto Tuple = E.m_upComponentMgr->getProperties(Index);
                    List.push_back({ std::get<const property::table*>(Tuple), E.m_guidComponentType, std::get<void*>(Tuple) });
                }
                return std::move(List);
            }

            struct deleted_entity
            {
                entity::guid                    m_guidEntity;
                mgr::instance::index            m_Index;
            };

            std::size_t                         m_NewEntities{ 0 };
            filter::guid                        m_Guid;
            std::vector<component_list>         m_lComponents;
            std::vector<deleted_entity>         m_lDeleteList;
        };
    }

    //------------------------------------------------------------------------------------------
    // COMMANDS - What kinds of things can the sync points do for us...
    //------------------------------------------------------------------------------------------
    namespace commands
    {
        using command_pool = containers::cmd_pool<1024>;

        namespace details
        {
            struct cmd_add_entity
            {
                entity::guid            m_Guid;
                int                     m_Size;
                component::type_guid*   m_pComponentTypes;
                void*                   m_pNewComponentData;
            };

            struct cmd_modify_components
            {
                entity::guid            m_Guid;
                int                     m_DelComponentCount     {0};
                component::type_guid*   m_pDelComponentTypes    {nullptr};
                int                     m_NewComponentCount     {0};
                component::type_guid*   m_pNewComponentTypes    {nullptr};
                void*                   m_pNewComponentData     { nullptr };
            };

            struct cmd_set_components
            {
                entity::guid            m_Guid;
                component::type_guid*   m_pComponentTypes       { nullptr };
                int                     m_ComponentCount        { 0 };
                void*                   m_pNewComponentData     { nullptr };
            };

            using cmds = std::variant
            <
                  cmd_add_entity
                , cmd_modify_components
                , cmd_set_components
            >;

            template< typename T_CLASS, typename...T_ARGS >
            struct set_component_handler
            {
                using component_tuple = std::tuple< T_ARGS... >;

                T_CLASS& m_Ref;

                template< typename T >
                auto& getComponentData(void) noexcept
                {
                    return std::get<T&>(*reinterpret_cast<component_tuple*>(m_Ref.m_pNewComponentData));
                }
            };

            struct modify_components_handler
            {
                cmd_modify_components&  m_Ref;
                command_pool&           m_Pool;
                command_pool::handle    m_Handel;

                template< typename...T_COMPONENTS >
                auto AddComponents( void ) noexcept
                {
                    using component_tuple = std::tuple<T_COMPONENTS...>;
                    std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };

                    m_Ref.m_NewComponentCount   = sizeof...(T_COMPONENTS);
                    m_Ref.m_pNewComponentData   = &m_Pool.NewData<component_tuple>(m_Handel);
                    m_Ref.m_pNewComponentTypes  = m_Pool.NewData<decltype(ComponentData)>(m_Handel, ComponentData).data();
                    return details::set_component_handler<cmd_modify_components, T_COMPONENTS...>{ m_Ref };
                }

                template< typename...T_COMPONENTS >
                void RemoveComponents(void) noexcept
                {
                    std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };
                    m_Ref.m_DelComponentCount   = sizeof...(T_COMPONENTS);
                    m_Ref.m_pDelComponentTypes  = m_Pool.NewData<decltype(ComponentData)>(m_Handel, ComponentData).data();
                }
            };
        }

        struct command_buffer
        {
            template< typename...T_COMPONENTS >
            auto AddEntity(entity::guid Guid) noexcept
            {
                using component_tuple = std::tuple< T_COMPONENTS... >;

                auto&       EntityCmd           = m_Pool.NewCmd<details::cmd_add_entity>(m_Handle);
                std::array  ComponentData       { T_COMPONENTS::factory::guid_component_type_v ... };

                EntityCmd.m_Guid                = Guid;
                EntityCmd.m_pComponentTypes     = m_Pool.NewData<decltype(ComponentData)>(m_Handle, ComponentData).data();
                EntityCmd.m_Size                = ComponentData.size();
                EntityCmd.m_pData               = &m_Pool.NewData<component_tuple>(m_Handle);

                return details::set_component_handler<details::cmd_add_entity, T_COMPONENTS...>{ EntityCmd };
            }

            auto EntityModifyComponets(entity::guid Guid) noexcept
            {
                auto& ModifyCmd = m_Pool.NewCmd<details::cmd_modify_components>(m_Handle);
                ModifyCmd.m_Guid = Guid;
                return details::modify_components_handler{ ModifyCmd, m_Pool, m_Handle };
            }

            template< typename...T_COMPONENTS >
            void SetComponents(entity::guid Guid) noexcept
            {
                using component_tuple = std::tuple< T_COMPONENTS... >;

                auto&       SetCmd      = m_Pool.NewCmd<details::cmd_set_components>(m_Handle);
                std::array  ComponentData{ T_COMPONENTS::factory::guid_component_type_v ... };

                SetCmd.m_Guid                   = Guid;
                SetCmd.m_ComponentCount         = sizeof...(T_COMPONENTS);
                SetCmd.m_pComponentTypes        = m_Pool.NewData<decltype(ComponentData)>(m_Handle, ComponentData).data();
                SetCmd.m_pNewComponentData      = &m_Pool.NewData<component_tuple>(m_Handle);

                return details::set_component_handler<details::cmd_set_components, T_COMPONENTS...>{ SetCmd };
            }

            command_pool&           m_Pool;
            command_pool::handle    m_Handle;
        };
    }

    //------------------------------------------------------------------------------------------
    // SYNC_POINT - A sync point establish a moment in time where systems can be run. You can
    //              also build complex graphs this way.
    //------------------------------------------------------------------------------------------
    namespace sync_point
    {
        struct instance
        {
                                instance            (main_system::instance& MainSystem)             noexcept : m_MainSystem{ MainSystem } {}
            virtual            ~instance            (void)                                          noexcept;
            void                AddToTrigger        (system::instance& Job, bool bDeleteWhenDone)   noexcept { m_lSystems.push_back({ &Job , bDeleteWhenDone }); }
            void                AddToNotify         (void)                                          noexcept { m_countExpectedNotifications++; m_Notifications = m_countExpectedNotifications; }
            virtual     void    onNotify            (void)                                          noexcept;

            struct entry
            {
                system::instance*   m_pSystem;
                bool                m_bDeleteWhenDone;
            };

            std::vector<entry>      m_lSystems;
            int                     m_countExpectedNotifications{ 0 };
            int                     m_Notifications{ 0 };
            main_system::instance&  m_MainSystem;
        };

        struct memory_barrier final : instance
        {
            virtual     void    onNotify            (void)                                          noexcept;
            commands::command_pool  m_CmdPool;
        };
    }

    //------------------------------------------------------------------------------------------
    // SYSTEM - System execute logic for components
    //------------------------------------------------------------------------------------------
    namespace system 
    {
        struct instance : sync_point::instance
        {
            using sync_point::instance::instance;

            virtual            ~instance            (void)                              noexcept = default;
                        void    Execute             (void)                              noexcept;
            virtual     void    onExecute           (void)                              noexcept = 0;
                        void    AddNotification     (sync_point::instance& SyncPoint)   noexcept;

            std::vector<sync_point::instance*> m_lNotifications;
        };

        namespace details
        {
            #define CHECK_FUNCTION( FUNCTION_NAME )                                         \
            template<typename, typename T>                                                  \
            struct has_##FUNCTION_NAME                                                      \
            {                                                                               \
                static_assert( std::integral_constant<T, false>::value,                     \
                    "Second template parameter needs to be of function type.");             \
            };                                                                              \
            /* specialization that does the checking */                                     \
            template<typename T_CLASS, typename Ret, typename... Args>                      \
            struct has_##FUNCTION_NAME <T_CLASS, Ret(Args...)>                              \
            {                                                                               \
                template<typename T>                                                        \
                static constexpr auto check(T*) -> typename std::is_same                    \
                <                                                                           \
                    decltype(std::declval<T>().FUNCTION_NAME(std::declval<Args>()...))      \
                    , Ret /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   */    \
                >::type;  /* attempt to call it and see if the return type is correct */    \
                template<typename>                                                          \
                static constexpr std::false_type check(...);                                \
                using type = decltype(check<T_CLASS>(nullptr));                             \
                static constexpr bool value = type::value;                                  \
            };                                                                              \
            template<typename T, typename...T_ARGS>                                         \
            std::enable_if_t<true == has_##FUNCTION_NAME<T, void(T_ARGS...)>::value, void > \
            Call##FUNCTION_NAME(T& A, T_ARGS...Args) noexcept { A.FUNCTION_NAME(Args...); } \
            template<typename T, typename...T_ARGS>                                         \
            std::enable_if_t<false == has_##FUNCTION_NAME<T, void(T_ARGS...)>::value, void >\
            Call##FUNCTION_NAME(T&, T_ARGS...Args) noexcept {}                          

            CHECK_FUNCTION(Update)                          // W/R Components
            CHECK_FUNCTION(ExclusiveUpdate)                 // W/R Components + add/remove components
            CHECK_FUNCTION(BarrierUpdate)

            #undef CHECK_FUNCTION
        }

        //------------------------------------------------------------------------------------------
        template< typename T_FINAL_SYSTEM, typename... T_ARGS >
        struct custom_instance final : T_FINAL_SYSTEM::system_t, instance
        {
            constexpr static filter::guid filter_guid_v { (T_ARGS::factory::guid_component_type_v.m_Value + ...) };
            constexpr static std::array   io_v          { std::tuple{ T_ARGS::factory::guid_component_type_v, std::is_const_v
                                                            <
                                                                typename std::tuple_element_t
                                                                <
                                                                    xcore::types::tuple_t2i_v< T_ARGS, T_FINAL_SYSTEM::sorted_tuple_t >
                                                                    , T_FINAL_SYSTEM::function_traits_t::args_tuple
                                                                >
                                                            > } ... };

            using final_system_t = T_FINAL_SYSTEM;

            custom_instance(main_system::instance& MainSystem) noexcept
                : T_FINAL_SYSTEM::system_t{ MainSystem }
                , instance{ MainSystem }
            {
                static_assert(details::has_Update<typename T_FINAL_SYSTEM::system_t, void
                (
                    typename std::tuple_element_t
                    <
                        xcore::types::tuple_t2i_v< T_ARGS, T_FINAL_SYSTEM::sorted_tuple_t >
                        , T_FINAL_SYSTEM::function_traits_t::args_tuple
                    > ...
                )>::value);
            }

            virtual void onExecute(void) noexcept override;
        };

        //------------------------------------------------------------------------------------------
        namespace details
        {
            namespace details
            {
                template< typename T_FINAL_SYSTEM, typename...T_ARGS >
                system::custom_instance< T_FINAL_SYSTEM, T_ARGS... > convert(std::tuple<T_ARGS...>*);

                template< typename T_A, typename T_B >
                struct compare
                {
                    constexpr static bool value = T_A::factory::guid_component_type_v.m_Value
                                                > T_B::factory::guid_component_type_v.m_Value;
                };

                template< typename T > struct remove_references;
                template< typename...T_ARGS >
                struct remove_references< std::tuple< T_ARGS &... >>
                {
                    using type = std::tuple<T_ARGS...>;
                };
            }

            template< typename T_SYSTEM >
            struct final_system
            {
                using system_t = T_SYSTEM;
                using function_traits_t = typename xcore::function::traits<decltype(&system_t::Update)>;
                static_assert(std::tuple_size_v<typename function_traits_t::args_tuple> > 0);

                using unsorted_tuple_t  = typename details::remove_references<typename function_traits_t::args_tuple>::type;
                using sorted_tuple_t    = typename helpers::tuple_selection_sort< details::compare, unsorted_tuple_t >::type;
                using type              = typename decltype(details::convert<final_system>((sorted_tuple_t*)nullptr));
            };
        }

    }

    //------------------------------------------------------------------------------------------
    // FILTER - Filters are a way to cache a particular component_type queries which system need
    //------------------------------------------------------------------------------------------
    namespace filter
    {
        struct instance
        {
            constexpr           instance    (ecs::filter::guid Guid)    noexcept : m_Guid{ Guid } {}
            virtual            ~instance    (void)                      noexcept = default;
            virtual     void    onNotifyAdd (group::instance& Group)    noexcept = 0;

            ecs::filter::guid m_Guid;
        };

        template< typename... T_ARGS >
        struct custom_instance final : instance
        {
            constexpr static guid       guid_v { (T_ARGS::factory::guid_component_type_v.m_Value + ...) };
            constexpr static std::array types_v{ T_ARGS::factory::guid_component_type_v ... };

            custom_instance() noexcept : instance{ guid_v } {}

            struct selection
            {
                group::instance*                    m_pGroupInstance;
                std::array<int, sizeof...(T_ARGS)>  m_lIndexToComponentLists;
            };

            virtual void onNotifyAdd(group::instance& Group) noexcept
            {
                // it wont have enough of my types
                if (types_v.size() > Group.m_lComponents.size())
                    return;

                //
                // Sanity check that all the data is as expected
                //
                xassert_block_basic()
                {
                    for (int i = 0; i < types_v.size(); i++)
                    {
                        if ( (i + 1) != types_v.size() )
                        {
                            assert(types_v[i] < types_v[i + 1]);
                        }
                    }

                    for ( int j = 0; j < Group.m_lComponents.size(); ++j)
                    {
                        if ((j + 1) != Group.m_lComponents.size() )
                        {
                            assert(Group.m_lComponents[j].m_guidComponentType.m_Value < Group.m_lComponents[j+1].m_guidComponentType.m_Value);
                        }
                    }
                }

                //
                // Create a map between the filter and the group
                //
                std::array<int, sizeof...(T_ARGS)>   lIndexToComponentLists;
                int TotalFound  = 0;
                int Index       = 0;
                
                for (auto& E : types_v) for (; Index < Group.m_lComponents.size(); ++Index )
                {
                    if (Group.m_lComponents[Index].m_guidComponentType == E)
                    {
                        lIndexToComponentLists[TotalFound++] = Index;
                        if (TotalFound == types_v.size())
                        {
                            // We have a match
                            m_lGroupSelection.push_back({ &Group, lIndexToComponentLists });
                            return;
                        }
                        break;
                    }
                }
            }

            template< typename T_FINAL_SYSTEM, typename T_SYSTEM >
            void ForEach(T_SYSTEM& System ) noexcept
            {
                using tmp_tuple = typename std::tuple<typename T_ARGS::factory::custom_mgr::pool_t &...>;
                for (auto& S : m_lGroupSelection)
                {
                    using tuple = std::tuple<T_ARGS...>;
                    tmp_tuple Pools
                    {
                        reinterpret_cast<typename T_ARGS::factory::custom_mgr&>
                        (*S.m_pGroupInstance->m_lComponents
                        [
                            S.m_lIndexToComponentLists[xcore::types::tuple_t2i_v< T_ARGS, tuple >]
                        ].m_upComponentMgr.get()).m_Pool
                        ...
                    };

                    const auto& EntityPool = reinterpret_cast<entity::instance::factory::custom_mgr&>
                                            (*S.m_pGroupInstance->m_lComponents[0].m_upComponentMgr.get()).m_Pool;

                    for (int i = 0; i < EntityPool.size(); ++i)
                    {
                        auto& Entity = EntityPool[i];

                        // Make sure we are running a valid entity
                        if( Entity.m_Flags.m_isDeleted 
                            || Entity.m_Flags.m_isDisable
                            || Entity.m_Flags.m_isInWorld == false )
                            continue;

                        // Let the system execute
                        System.Update( std::get
                        < 
                            typename std::tuple_element_t
                            < 
                                xcore::types::tuple_t2i_v< T_ARGS, T_FINAL_SYSTEM::sorted_tuple_t >
                                , T_FINAL_SYSTEM::unsorted_tuple_t
                            >::factory::custom_mgr::pool_t&
                        >(Pools)[i] ... );
                    }
                }
            }

            std::vector<selection>      m_lGroupSelection;
        };
    }

    //------------------------------------------------------------------------------------------
    // MAIN SYSTEM - This is where everything centralizes to
    //------------------------------------------------------------------------------------------
    namespace main_system
    {
        struct start_syncpoint final : ecs::sync_point::instance
        {
            using parent_t = ecs::sync_point::instance;
            using parent_t::instance;
        };

        struct end_syncpoint final : ecs::sync_point::instance
        {
            using parent_t = ecs::sync_point::instance;
            using parent_t::instance;
            virtual void onNotify() noexcept override
            {
                // Don't let the parent function get call
            }
        };

        struct events
        {
            using on_start = xcore::types::make_unique< xcore::event::type<>, struct on_start_tag >;

            on_start m_OnStart;
        };

        namespace details
        {
            struct alignas(std::uint64_t) sync
            {
                std::uint16_t   m_nRunningSystems{ 0 };
                std::uint16_t   m_UniqueCounter  { 0 };
                bool            m_isBlocked      { false };
            };
        }

        struct instance
        {
            struct component_factories
            {
                ecs::component::type_guid                   m_TypeGuid;
                std::unique_ptr<ecs::factory::instance>     m_upFactory;
            };

            instance()
            {
                registerComponent<entity::instance>();
                m_StartSyncPoint.AddToNotify();
            }

            virtual void onExecute(void) noexcept {}

            void Start(void)
            {
                m_Events.m_OnStart.NotifyAll();
                while (m_bPause == false)
                {
                    m_bFrameStarted = true;
                    onExecute();

                    // Now we can start the cascade...
                    m_StartSyncPoint.onNotify();
                    m_FrameNumber++;

                    // Let everyone know that we have finish the frame
                    m_bFrameStarted = false;
                    onUpdateEndFrame();
                }
            }

            ecs::factory::instance* findFactory(ecs::component::type_guid TypeGuid) noexcept
            {
                for (const auto& E : m_lComponentFactories)
                {
                    if (E.m_TypeGuid == TypeGuid) return E.m_upFactory.get();
                }
                return nullptr;
            }

            template< typename T_COMPONENT >
            void registerComponent(void) noexcept
            {
                for (const auto& E : m_lComponentFactories)
                {
                    if (E.m_TypeGuid == T_COMPONENT::factory::guid_component_type_v) return;
                }

                m_lComponentFactories.push_back(
                {
                    T_COMPONENT::factory::guid_component_type_v
                    , std::unique_ptr<ecs::factory::instance>{new typename T_COMPONENT::factory}
                });
            }

            template< typename...T_COMPONENTS >
            entity::tmp_ref_create createEntity(entity::guid EntityGuid ) noexcept
            {
                if constexpr (sizeof...(T_COMPONENTS) > 0)
                {
                    constexpr static auto guidGroup = filter::guid
                    {
                        (T_COMPONENTS::factory::guid_component_type_v.m_Value + ...)
                    };

                    constexpr static auto guidComponentType = helpers::sort(std::array
                    {
                        T_COMPONENTS::factory::guid_component_type_v ...
                    });

                    return createEntity(EntityGuid, guidGroup, guidComponentType);
                }
                else
                {
                    constexpr static auto empty = std::span<const component::type_guid>(nullptr, std::size_t{ 0 });
                    return createEntity(EntityGuid, filter::guid{ nullptr }, empty );
                }
            }

            // private
            entity::tmp_ref_create createEntity(
                                entity::guid    EntityGuid
                            ,   filter::guid    GroupID
                            ,   const std::span<const component::type_guid> lGuidComponentTypes ) noexcept
            {
                entity::tmp_ref_create EntityRef;

                //
                // find or add the group
                //
                {
                    auto tupleGroup = m_mapGroupInstance.find(GroupID);
                    if (tupleGroup == m_mapGroupInstance.end())
                    {
                        EntityRef.m_pGroupInstance = new ecs::group::instance{ GroupID, *this, lGuidComponentTypes };
                        m_mapGroupInstance.insert({ GroupID, std::unique_ptr<ecs::group::instance>{ EntityRef.m_pGroupInstance } });

                        // Notify all filters about this new group
                        for (auto& [Key, Entry] : m_mapFilters)
                        {
                            Entry->onNotifyAdd(*EntityRef.m_pGroupInstance);
                        }
                    }
                    else
                    {
                        EntityRef.m_pGroupInstance = tupleGroup->second.get();
                    }
                }

                //
                // Add all requested components
                //
                EntityRef.m_Index = EntityRef.m_pGroupInstance->createEntity(EntityGuid, m_bFrameStarted);

                //
                // Add the entity
                //
                m_mapEntityInfo.insert
                ({
                      EntityGuid
                    , EntityRef
                });

                return std::move(EntityRef);
            }

            void deleteUpdateEntities(entity::guid gEntity ) noexcept
            {
                m_mapEntityInfo.erase(gEntity);
            }

            void deleteUpdateEntities(entity::guid gDeleteEntity, entity::guid gUpdateEntity, mgr::instance::index UpdateIndex) noexcept
            {
                // Update the entity Index
                auto Pair = m_mapEntityInfo.find(gUpdateEntity);
                xassert(Pair != m_mapEntityInfo.end());
                Pair->second.m_Index = UpdateIndex;

                // Do actual erasing
                deleteUpdateEntities(gDeleteEntity);
            }

            virtual void onUpdateEndFrame(void) noexcept
            {
                for (auto& G : m_mapGroupInstance)
                    G.second->onUpdateEndFrame( *this );
            }

            void deleteEntity(entity::guid guidEntity) noexcept
            {
                auto pairEntity = m_mapEntityInfo.find(guidEntity);
                if (pairEntity == m_mapEntityInfo.end()) return;
                auto& EntityTmpRef = pairEntity->second;
                
                EntityTmpRef.m_pGroupInstance->deleteEntity(guidEntity,EntityTmpRef.m_Index);
            }

            template< typename...T_ARGS >
            void PrecacheFilter(std::tuple<T_ARGS...>*) noexcept
            {
                using filter_t = filter::custom_instance<T_ARGS...>;

                filter_t* pFilter;
                auto tupleFilter = m_mapFilters.find(filter_t::guid_v);
                if (tupleFilter == m_mapFilters.end())
                {
                    pFilter = new filter_t;
                    m_mapFilters.insert({ filter_t::guid_v, std::unique_ptr<filter::instance>{pFilter} });
                }
            }

            template< typename T_SYSTEM, typename...T_ARGS >
            T_SYSTEM& AddGraphConnection(sync_point::instance& StartSynpoint, T_ARGS&&... EndSyncPoints) noexcept
            {
                static_assert(sizeof...(EndSyncPoints) > 0, "You must pass at least one end Sync_point");
                static_assert(std::is_base_of_v<sync_point::instance, std::common_type_t<T_ARGS...>>, "All the end sync_points must be non const sync_points");

                // Create the new type and allocated it
                using final_system = typename system::details::final_system<T_SYSTEM>;
                using system_t     = typename final_system::type;
                std::unique_ptr<system_t> System
                {
                    std::unique_ptr<system_t>{ new system_t{*this} }
                };

                // does the user wants us to delete it when we are done?
                // If the user pass us a unique pointer we will assume that yes.
                PrecacheFilter((typename final_system::sorted_tuple_t*)nullptr);
                auto& P = *System.release();
                StartSynpoint.AddToTrigger(P, true);

                ((P.AddNotification(EndSyncPoints)), ...);

                return *System;
            }

            filter::instance& getFilter(filter::guid Guid) noexcept
            {
                auto tupleFilter = m_mapFilters.find(Guid);
                assert(tupleFilter != m_mapFilters.end());
                return *tupleFilter->second;
            }

            void incSystemProcessing(int N) noexcept
            {
                details::sync NewSync;
                details::sync Sync      = m_Barrier.load(std::memory_order_relaxed);

                do
                {
                    if (Sync.m_isBlocked)
                    {
                        Sync = m_Barrier.load();
                        // Wasting time....
                        continue;
                    }

                    NewSync = Sync;
                    NewSync.m_UniqueCounter++;
                    NewSync.m_nRunningSystems += N;
                    
                } while (m_Barrier.compare_exchange_weak(Sync, NewSync) == false );
            }

            void decSystemProcessing(int N) noexcept
            {
                details::sync NewSync;
                details::sync Sync      = m_Barrier.load(std::memory_order_relaxed);

                do
                {
                    NewSync = Sync;
                    NewSync.m_UniqueCounter++;
                    NewSync.m_nRunningSystems -= N;

                } while (m_Barrier.compare_exchange_weak(Sync, NewSync) == false);
            }

            void LockExcusiveBarrierAccess(void) noexcept
            {
                details::sync NewSync;
                details::sync Sync      = m_Barrier.load(std::memory_order_relaxed);

                do
                {
                    if (Sync.m_isBlocked)
                    {
                        Sync = m_Barrier.load();
                        // Wasting time
                        continue;
                    }

                    NewSync = Sync;
                    NewSync.m_UniqueCounter++;
                    NewSync.m_isBlocked = true;

                } while (m_Barrier.compare_exchange_weak(Sync, NewSync) == false);
            }

            void ReleaseExcusiveBarrierAccess(void) noexcept
            {
                details::sync NewSync;
                details::sync Sync      = m_Barrier.load(std::memory_order_relaxed);

                do
                {
                    NewSync = Sync;
                    NewSync.m_UniqueCounter++;
                    assert(NewSync.m_isBlocked == true);
                    NewSync.m_isBlocked = false;

                } while (m_Barrier.compare_exchange_weak(Sync, NewSync) == false);
            }

            std::atomic<details::sync>                                                      m_Barrier;
            bool                                                                            m_bFrameStarted     { false };
            bool                                                                            m_bPause            { false };
            int                                                                             m_FrameNumber       { 0 };
            start_syncpoint                                                                 m_StartSyncPoint    { *this };
            end_syncpoint                                                                   m_EndSyncPoint      { *this };
            events                                                                          m_Events            {};
            std::vector<component_factories>                                                m_lComponentFactories{};
            std::unordered_map<filter::guid, std::unique_ptr<ecs::group::instance> >        m_mapGroupInstance  {};
            std::unordered_map<entity::guid, entity::tmp_ref >                              m_mapEntityInfo     {};
            std::unordered_map<filter::guid, std::unique_ptr<filter::instance>>             m_mapFilters        {};
        };
    }

    //------------------------------------------------------------------------------------------
    // Functions
    //------------------------------------------------------------------------------------------

    namespace group
    {
        void instance::onUpdateEndFrame(main_system::instance& MainSystem) noexcept
        {
            assert(m_lComponents.size());

            //
            // Removed all deleted entities
            //
            for (auto& E : m_lDeleteList)
            {
                bool bSwap;
                for (auto& Mgr : m_lComponents)
                {
                    bSwap = Mgr.m_upComponentMgr->deleteComponent(E.m_Index);
                }

                if (bSwap)
                {
                    auto& SwappedEntity = *reinterpret_cast<entity::instance*>(m_lComponents[0].m_upComponentMgr->getElement(E.m_Index));
                    MainSystem.deleteUpdateEntities(E.m_guidEntity, SwappedEntity.m_Guid, E.m_Index);
                }
                else
                {
                    MainSystem.deleteUpdateEntities(E.m_guidEntity);
                }

                // Ready for next frame
                m_lDeleteList.clear();
            }

            //
            // Reset the count
            //
            m_NewEntities = 0;
        }

        //------------------------------------------------------------------------------------------

        instance::instance(filter::guid guidGroup, main_system::instance& MainSystem, std::span<const component::type_guid> lGuidComponentTypes) noexcept
        {
            // Insert the entity instance first
            m_lComponents.push_back
            ({
                entity::instance::factory::guid_component_type_v
                , std::unique_ptr<mgr::instance>{&MainSystem.findFactory(entity::instance::factory::guid_component_type_v)->CreateMgr()}
            });

            // Now the rest of the components
            for (auto& E : lGuidComponentTypes)
            {
                m_lComponents.push_back
                ({
                    E
                    , std::unique_ptr<mgr::instance>{&MainSystem.findFactory(E)->CreateMgr()}
                });
            }
        }
    }

    //------------------------------------------------------------------------------------------
    namespace entity
    {
        tmp_ref_create::~tmp_ref_create() noexcept
        {
            if (m_pGroupInstance)
            {
                m_pGroupInstance->setEntityInWorld(m_Index);
                m_pGroupInstance = nullptr;
            }
        }

        template< typename T >
        T& tmp_ref::getComponent(component::type_guid TypeGuid) noexcept
        {
            // Make sure that the types matches
            assert(isValid());

            auto pMgr = m_pGroupInstance->findMgr(TypeGuid);
            assert(pMgr);
            return *reinterpret_cast<T*>(pMgr->getElement(m_Index));
        }

        std::vector<properties_tuple> tmp_ref::getProperties(void) const noexcept
        {
            return std::move(m_pGroupInstance->getProperties(m_Index));
        }

        template< typename T > constexpr
        const T& tmp_ref::getComponent(component::type_guid TypeGuid) const noexcept
        {
            // Make sure that the types matches
            assert(isValid());

            auto pMgr = m_pGroupInstance->findMgr(TypeGuid);
            assert(pMgr);
            return *reinterpret_cast<T*>(pMgr->getElement(m_Index));
        }
    }

    //------------------------------------------------------------------------------------------

    namespace sync_point
    {
        instance::~instance (void) noexcept 
        { 
            for (auto& E : m_lSystems) 
                if (E.m_bDeleteWhenDone) delete(E.m_pSystem); 
        }

        void instance::onNotify() noexcept
        {
            --m_Notifications;
            assert(m_Notifications >= 0);
            if (m_Notifications == 0)
            {
                m_Notifications = m_countExpectedNotifications;

                // Let the main system know that we are ready to go
                m_MainSystem.incSystemProcessing( static_cast<int>(m_lSystems.size()) );
                for (auto& E : m_lSystems) E.m_pSystem->Execute();
            }
        }
    }

    //------------------------------------------------------------------------------------------
    namespace system
    {
        template< typename T_FINAL_SYSTEM, typename... T_ARGS >
        void custom_instance< T_FINAL_SYSTEM, T_ARGS...>::onExecute(void) noexcept
        {
            auto& Filter = static_cast<filter::custom_instance<T_ARGS...>&>(m_MainSystem.getFilter(filter_guid_v));
            Filter.ForEach<final_system_t>( *this );
        }

        void instance::Execute(void) noexcept
        {
            onExecute();
            m_MainSystem.ReleaseExcusiveBarrierAccess();
            for (auto p : m_lNotifications) p->onNotify();
        }

        void instance::AddNotification(sync_point::instance& SyncPoint) noexcept
        {
            m_lNotifications.push_back(&SyncPoint);
            SyncPoint.AddToNotify();
        }
    }
}

property_begin_name(ecs::entity::instance, "Entity")
{}
property_end()