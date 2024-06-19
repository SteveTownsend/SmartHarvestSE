cmake_minimum_required(VERSION 3.28.1)

# #######################################################################################################################
# # Define project
# #######################################################################################################################
set(NAME "SmartHarvestSE")
set(PRETTY_NAME "Smart Harvest SE")
set(VERSION 5.9.1)

set(spdlog_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

project(
        ${NAME}
        VERSION ${VERSION}
        DESCRIPTION "SKSE for autoloot and related quality of life improvements."
        LANGUAGES CXX)
set(PROJECT_FRIENDLY_NAME "SmartHarvest NG")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Ver.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/Ver.h
        @ONLY)

# #######################################################################################################################
# # Find dependencies
# #######################################################################################################################
include(shseDependencies.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(sources
        src/alglib/alglibinternal.cpp
        src/alglib/alglibinternal.h
        src/alglib/alglibmisc.cpp
        src/alglib/alglibmisc.h
        src/alglib/ap.cpp
        src/alglib/ap.h
        src/Collections/Collection.cpp
        src/Collections/Collection.h
        src/Collections/CollectionFactory.cpp
        src/Collections/CollectionFactory.h
        src/Collections/CollectionManager.cpp
        src/Collections/CollectionManager.h
        src/Collections/Condition.cpp
        src/Collections/Condition.h
        src/Data/CosaveData.cpp
        src/Data/CosaveData.h
        src/Data/dataCase.cpp
        src/Data/dataCase.h
        src/Data/iniSettings.cpp
        src/Data/iniSettings.h
        src/Data/LeveledItemCategorizer.cpp
        src/Data/LeveledItemCategorizer.h
        src/Data/LoadOrder.cpp
        src/Data/LoadOrder.h
        src/Data/SettingsCache.cpp
        src/Data/SettingsCache.h
        src/Data/SimpleIni.cpp
        src/Data/SimpleIni.h
        src/FormHelpers/AlchemyItemHelper.cpp
        src/FormHelpers/AlchemyItemHelper.h
        src/FormHelpers/ArmorHelper.cpp
        src/FormHelpers/ArmorHelper.h
        src/FormHelpers/ExtraDataListHelper.cpp
        src/FormHelpers/ExtraDataListHelper.h
        src/FormHelpers/FormHelper.cpp
        src/FormHelpers/FormHelper.h
        src/FormHelpers/IHasValueWeight.cpp
        src/FormHelpers/IHasValueWeight.h
        src/FormHelpers/WeaponHelper.cpp
        src/FormHelpers/WeaponHelper.h
        src/Looting/containerLister.cpp
        src/Looting/containerLister.h
        src/Looting/InventoryItem.cpp
        src/Looting/InventoryItem.h
        src/Looting/IRangeChecker.cpp
        src/Looting/IRangeChecker.h
        src/Looting/LootableREFR.cpp
        src/Looting/LootableREFR.h
        src/Looting/ManagedLists.cpp
        src/Looting/ManagedLists.h
        src/Looting/NPCFilter.cpp
        src/Looting/NPCFilter.h
        src/Looting/objects.cpp
        src/Looting/objects.h
        src/Looting/ObjectType.h
        src/Looting/ProducerLootables.cpp
        src/Looting/ProducerLootables.h
        src/Looting/ReferenceFilter.cpp
        src/Looting/ReferenceFilter.h
        src/Looting/ScanGovernor.cpp
        src/Looting/ScanGovernor.h
        src/Looting/TheftCoordinator.cpp
        src/Looting/TheftCoordinator.h
        src/Looting/TryLootREFR.cpp
        src/Looting/TryLootREFR.h
        src/Utilities/Enums.cpp
        src/Utilities/Enums.h
        src/Utilities/Exception.cpp
        src/Utilities/Exception.h
        src/Utilities/LogWrapper.h
        src/Utilities/RecursiveLock.cpp
        src/Utilities/RecursiveLock.h
        src/Utilities/utils.cpp
        src/Utilities/utils.h
        src/Utilities/version.h
        src/VM/EventPublisher.cpp
        src/VM/EventPublisher.h
        src/VM/papyrus.cpp
        src/VM/papyrus.h
        src/VM/TaskDispatcher.cpp
        src/VM/TaskDispatcher.h
        src/VM/UIState.cpp
        src/VM/UIState.h
        src/WorldState/ActorTracker.cpp
        src/WorldState/ActorTracker.h
        src/WorldState/AdventureTargets.cpp
        src/WorldState/AdventureTargets.h
        src/WorldState/CraftingItems.cpp
        src/WorldState/CraftingItems.h
        src/WorldState/GameCalendar.cpp
        src/WorldState/GameCalendar.h
        src/WorldState/InventoryCache.cpp
        src/WorldState/InventoryCache.h
        src/WorldState/LocationTracker.cpp
        src/WorldState/LocationTracker.h
        src/WorldState/PartyMembers.cpp
        src/WorldState/PartyMembers.h
        src/WorldState/PlacedObjects.cpp
        src/WorldState/PlacedObjects.h
        src/WorldState/PlayerHouses.cpp
        src/WorldState/PlayerHouses.h
        src/WorldState/PlayerState.cpp
        src/WorldState/PlayerState.h
        src/WorldState/PopulationCenters.cpp
        src/WorldState/PopulationCenters.h
        src/WorldState/PositionData.h
        src/WorldState/QuestTargets.cpp
        src/WorldState/QuestTargets.h
        src/WorldState/Saga.cpp
        src/WorldState/Saga.h
        src/WorldState/VisitedPlaces.cpp
        src/WorldState/VisitedPlaces.h
        src/main.cpp
        src/PluginFacade.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/include/Ver.h
)

source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${sources}
)

set(mergemappersources
        ${mergemapper_SOURCE_DIR}/src/MergeMapperPluginAPI.cpp
        ${mergemapper_SOURCE_DIR}/include/MergeMapperPluginAPI.h
)

source_group(
        TREE ${mergemapper_SOURCE_DIR}
        FILES
        ${mergemappersources}
)

# #######################################################################################################################
# # Add CMake features
# #######################################################################################################################
include(CheckIPOSupported)
include(GNUInstallDirs)

# #######################################################################################################################
# # Configuration for all targets
# #######################################################################################################################
if(WIN32)
        add_compile_definitions(
                UNICODE
                _UNICODE
                NOMINMAX
                _AMD64_
                WIN32_LEAN_AND_MEAN)
        if(${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
                add_compile_definitions(
                        _FULL_LOGGING
                        NDEBUG
                )
        endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
        add_compile_definitions(
                __cpp_lib_char8_t
                __cpp_consteval
                __cpp_lib_format)
endif()

check_ipo_supported(RESULT USE_IPO OUTPUT IPO_OUTPUT)

if(USE_IPO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
        message(WARNING "Interprocedural optimizations are not supported.")
endif()

# #######################################################################################################################
# # Configure target DLL
# #######################################################################################################################
include(${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake)
add_commonlibsse_plugin(${PROJECT_NAME}
    NAME ${PROJECT_NAME}
    AUTHOR "Steve Townsend, wskeever, bobbyclue"
    EMAIL "SteveTownsend0@gmail.com"
    USE_ADDRESS_LIBRARY
    USE_SIGNATURE_SCANNING
    SOURCES ${sources} ${mergemappersources}
)

add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<INSTALL_INTERFACE:src>)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_CURRENT_BINARY_DIR}/include)

# uncomment for assembler/source listing
#target_compile_options(${PROJECT_NAME} PUBLIC /Fas /FA)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        CommonLibSSE
        Dbghelp.lib
        nlohmann_json_schema_validator
        brotlidec
        brotlienc
        brotlicommon
        spdlog
)

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        src/PrecompiledHeaders.h)

# Uncomment as needed for crash log debugging
#target_link_options(${PROJECT_NAME} PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/Sample"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# #######################################################################################################################
# # Build plugin archive for Nexus, for Release only
# #######################################################################################################################
if (${CMAKE_BUILD_TYPE} STREQUAL Release)
        include(shseBuildArchive.cmake)
elseif (${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
        include(shseBuildLoggingArchive.cmake)
endif()
