
//#include "xcore.h"
#include "pool.h"

#include "helpers.h"

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

    //------------------------------------------------------------------------------------------
    // COMPONENT
    //------------------------------------------------------------------------------------------
    namespace component
    {
        using type_guid = xcore::guid::unit<64, struct component_type_tag>;
    }

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
            entity::guid    m_Guid;
            entity::flags   m_Flags;
            string_t        m_Name;     // Editor only?
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

                containers::pool<T_COMPONENT, T_SIZE > m_Pool;

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
                        if (bMidFrame)
                        {
                            m_NewEntities++;
                            Entity.m_Flags.m_isInWorld = false;
                        }
                        else
                        {
                            Entity.m_Flags.m_isInWorld = true;
                        }

                    }
                }

                return Index;
            }

            void onUpdateEndFrame(main_system::instance& MainSystem) noexcept;

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
    // SYSTEM - System execute logic for components
    //------------------------------------------------------------------------------------------
    namespace system
    {
        struct instance
        {
            constexpr           instance            (void)                              noexcept = default;
            virtual            ~instance            (void)                              noexcept = default;
                        void    Execute             (void)                              noexcept;
            virtual     void    onExecute           (void)                              noexcept = 0;
                        void    AddNotification     (sync_point::instance& SyncPoint)   noexcept;

            std::vector<sync_point::instance*> m_lNotifications;
        };

        namespace details
        {
            template<typename, typename T>
            struct has_start 
            {
                static_assert( std::integral_constant<T, false>::value,
                    "Second template parameter needs to be of function type.");
            };

            // specialization that does the checking
            template<typename T_CLASS, typename Ret, typename... Args>
            struct has_start<T_CLASS, Ret(Args...)>
            {
            private:
                template<typename T>
                static constexpr auto check(T*)
                    -> typename std::is_same
                    <
                      decltype(std::declval<T>().Start(std::declval<Args>()...))
                    , Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                    >::type;  // attempt to call it and see if the return type is correct

                template<typename>
                static constexpr std::false_type check(...);

                using type = decltype(check<T_CLASS>(nullptr));

            public:
                static constexpr bool value = type::value;
            };

            template<typename T>
            std::enable_if_t<true == details::has_start<T, void(main_system::instance&)>::value, void >
            CallStart(T& A) noexcept { A.Start(A.m_MainSystem); }

            template<typename T>
            std::enable_if_t<false == details::has_start<T, void(main_system::instance&)>::value, void >
            CallStart(T&) noexcept {}
        }

        //------------------------------------------------------------------------------------------
        template< typename T_FINAL_SYSTEM, typename... T_ARGS >
        struct custom_instance final : T_FINAL_SYSTEM::system_t, instance
        {
            constexpr static filter::guid filter_guid_v{ (T_ARGS::factory::guid_component_type_v.m_Value + ...) };
            using final_system_t = T_FINAL_SYSTEM;

            custom_instance(main_system::instance& MainSystem) noexcept
                : m_MainSystem{ MainSystem }
                , T_FINAL_SYSTEM::system_t{ MainSystem }
                , instance{}
            {
              //  details::CallStart(*this);
            }

            virtual void onExecute(void) noexcept override;

            main_system::instance& m_MainSystem;
        };

        //------------------------------------------------------------------------------------------
        namespace details
        {
            namespace details
            {
                template< typename T_FINAL_SYSTEM, typename...T_ARGS >
                system::custom_instance< T_FINAL_SYSTEM, T_ARGS... > convert(std::tuple<T_ARGS...>*);

                template< typename T_SORTED_TUPLE, typename...T > struct create_array;
                template< typename T_SORTED_TUPLE, typename...T_ARGS >
                struct create_array< T_SORTED_TUPLE, std::tuple< T_ARGS... >>
                {
                    constexpr static std::array value{ xcore::types::tuple_t2i_v<T_ARGS, T_SORTED_TUPLE> ... };
                };

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
                group::instance* m_pGroupInstance;
                std::array<int, sizeof...(T_ARGS)>   m_lIndexToComponentLists;
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
                using tmp_tuple = typename std::tuple<typename T_ARGS::factory::custom_mgr &...>;
                for (auto& S : m_lGroupSelection)
                {
                    using tuple = std::tuple<T_ARGS...>;
                    tmp_tuple Mgrs
                    {
                        reinterpret_cast<typename T_ARGS::factory::custom_mgr&>
                        (*S.m_pGroupInstance->m_lComponents
                        [
                            S.m_lIndexToComponentLists[xcore::types::tuple_t2i_v< T_ARGS, tuple >]
                        ].m_upComponentMgr.get())
                        ...
                    };

                    const auto& EntityMgr = reinterpret_cast<entity::instance::factory::custom_mgr&>
                                            (*S.m_pGroupInstance->m_lComponents[0].m_upComponentMgr.get());

                    const auto Size = std::get<0>(Mgrs).m_Pool.size();
                    for (int i = 0; i < Size; ++i)
                    {
                        auto& Entity = EntityMgr.m_Pool[i];

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
                            >::factory::custom_mgr&
                        >(Mgrs).m_Pool[i] ... );
                    }
                }
            }

            std::vector<selection>      m_lGroupSelection;
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
            virtual            ~instance            (void)                                          noexcept { for (auto& E : m_lSystems) if (E.m_bDeleteWhenDone) delete(E.m_pSystem); }
                        void    AddToTrigger        (system::instance& Job, bool bDeleteWhenDone )  noexcept { m_lSystems.push_back({ &Job , bDeleteWhenDone }); }
                        void    AddToNotify         (void)                                          noexcept { m_countExpectedNotifications++; m_Notifications = m_countExpectedNotifications; }
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
            virtual void onNotify() noexcept override
            {
                parent_t::onNotify();
            }
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
            entity::tmp_ref createEntity(entity::guid EntityGuid ) noexcept
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
            entity::tmp_ref createEntity(  
                                entity::guid    EntityGuid
                            ,   filter::guid    GroupID
                            ,   const std::span<const component::type_guid> lGuidComponentTypes ) noexcept
            {
                entity::tmp_ref EntityRef;

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

                return EntityRef;
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

            bool                                                                            m_bFrameStarted     { false };
            bool                                                                            m_bPause            { false };
            int                                                                             m_FrameNumber       { 0 };
            start_syncpoint                                                                 m_StartSyncPoint    { *this };
            end_syncpoint                                                                   m_EndSyncPoint      { *this };
            events                                                                          m_Events;
            std::vector<component_factories>                                                m_lComponentFactories;
            std::unordered_map<filter::guid, std::unique_ptr<ecs::group::instance> >        m_mapGroupInstance;
            std::unordered_map<entity::guid, entity::tmp_ref >                              m_mapEntityInfo;
            std::unordered_map<filter::guid, std::unique_ptr<filter::instance>>             m_mapFilters;
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

            const auto HandleNewEntity = [&](entity::instance& Entity)
            {
                assert(Entity.m_Flags.m_isInWorld == false);
                Entity.m_Flags.m_isInWorld = true;

                // TODO: Event new entity enters the world

            };

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

                    if (m_NewEntities)
                    {
                        HandleNewEntity(SwappedEntity);

                        // One less new entity we need to deal with
                        m_NewEntities--;
                    }
                }
                else
                {
                    MainSystem.deleteUpdateEntities(E.m_guidEntity);
                }

                // Ready for next frame
                m_lDeleteList.clear();
            }

            //
            // Notify any new entity that it just entered the world
            //
            if (m_NewEntities)
            {
                assert(m_lComponents[0].m_upComponentMgr->size() >= m_NewEntities);

                const auto Total = m_lComponents[0].m_upComponentMgr->size();
                
                for(mgr::instance::index Index{ static_cast<std::uint32_t>(Total - m_NewEntities) }; Index.m_Value < Total; ++Index.m_Value )
                {
                    auto& NewEntity = *reinterpret_cast<entity::instance*>(m_lComponents[0].m_upComponentMgr->getElement(Index));
                    HandleNewEntity(NewEntity);
                }
                m_NewEntities = 0;
            }
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
        void instance::onNotify() noexcept
        {
            --m_Notifications;
            assert(m_Notifications >= 0);
            if (m_Notifications == 0)
            {
                m_Notifications = m_countExpectedNotifications;
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
            Filter.ForEach<final_system_t>( *this);
        }

        void instance::Execute(void) noexcept
        {
            onExecute();
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
{
    property_var(m_Name)
}
property_end()