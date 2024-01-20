cmake_minimum_required(VERSION 3.28.1)

# #######################################################################################################################
# # Define project
# #######################################################################################################################
set(NAME "SmartHarvestSE")
set(VERSION 5.0.2)

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
        src/Utilities/LogStackWalker.cpp
        src/Utilities/LogStackWalker.h
        src/Utilities/LogWrapper.h
        src/Utilities/RecursiveLock.cpp
        src/Utilities/RecursiveLock.h
        src/Utilities/StackWalker.cpp
        src/Utilities/StackWalker.h
        src/Utilities/utils.cpp
        src/Utilities/utils.h
        src/Utilities/version.cpp
        src/Utilities/version.h
        src/Utilities/versiondb.h
        src/VM/EventPublisher.cpp
        src/VM/EventPublisher.h
        src/VM/papyrus.cpp
        src/VM/papyrus.h
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
)

source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${sources}
)

# #######################################################################################################################
# # Add CMake features
# #######################################################################################################################
include(CheckIPOSupported)
include(GNUInstallDirs)

# #######################################################################################################################
# # Find dependencies
# #######################################################################################################################
include(FetchContent)

FetchContent_Declare(
  brotli
  GIT_REPOSITORY https://github.com/google/brotli
  GIT_TAG        v1.1.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(brotli)

FetchContent_Declare(
  nlohmann_json_schema_validator
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator
  GIT_TAG        2.3.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(nlohmann_json_schema_validator)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        10.2.1
  OVERRIDE_FIND_PACKAGE
)
if (NOT fmt_POPULATED)
        FetchContent_Populate(fmt)
        set(FMT_INSTALL ON CACHE INTERNAL "Install SPDLOG for CommonLibSSE")
        add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()

FetchContent_Declare(
 spdlog
 GIT_REPOSITORY https://github.com/gabime/spdlog
 GIT_TAG        v1.13.0
 OVERRIDE_FIND_PACKAGE
)
FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
        FetchContent_Populate(spdlog)
        set(SPDLOG_INSTALL ON CACHE INTERNAL "Install SPDLOG for CommonLibSSE")
# pending update to spdlog v1.13.0 etc        
#        set(SPDLOG_USE_STD_FORMAT ON CACHE INTERNAL "Use std::format in SPDLOG, not fmt")
        add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        v3.5.2
  OVERRIDE_FIND_PACKAGE
  )
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  rapidcsv
  GIT_REPOSITORY https://github.com/d99kris/rapidcsv
  GIT_TAG        v8.64
  OVERRIDE_FIND_PACKAGE
  )
FetchContent_MakeAvailable(rapidcsv)
set(RAPIDCSV_INCLUDE_DIRS ${rapidcsv_SOURCE_DIR}/src)

# VR unsupported pending PlayerCharacter RE, and don't worry about tests
set(ENABLE_SKYRIM_VR OFF)
set(BUILD_TESTS OFF)
FetchContent_Declare(
  CommonLibSSE
  GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG
  GIT_TAG        v3.7.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(CommonLibSSE)

include_directories(${fmt_SOURCE_DIR}/include)
target_compile_definitions(spdlog PUBLIC SPDLOG_FMT_EXTERNAL)
target_compile_options(spdlog PUBLIC "/I${fmt_SOURCE_DIR}/include" "/EHsc")
# target_compile_options(Catch2 PUBLIC "/EHsc")
# target_compile_options(fmt PUBLIC "/EHsc")
# target_compile_options(nlohmann_json_schema_validator PUBLIC "/EHsc")
target_compile_options(CommonLibSSE PUBLIC "/I${rapidcsv_SOURCE_DIR}/src")

find_package(spdlog CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(brotli CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
    SOURCES ${sources}
)

add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<INSTALL_INTERFACE:src>)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

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

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/Sample"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# #######################################################################################################################
# # Build plugin archive for Nexus
# #######################################################################################################################
file(GLOB_RECURSE CONFIG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/*.ini")
file(GLOB_RECURSE OUTPUT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/SHSE*.pex")
file(GLOB_RECURSE SOURCE_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Source/SHSE*.psc")
file(GLOB_RECURSE EDIT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Edit Scripts/*.pas")
file(GLOB_RECURSE JSON_SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/**/SHSE.Schema*.json")
file(GLOB_RECURSE BUILTIN_COLLECTIONS "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Builtin/SHSE.Collections*.json")
file(GLOB_RECURSE EXAMPLE_COLLECTIONS "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Examples/SHSE.Collections*.json")
file(GLOB_RECURSE LOOT_FILTERS "${CMAKE_CURRENT_SOURCE_DIR}/Filters/SHSE.Filter.*.json")
file(GLOB_RECURSE GLOW_TEXTURES "${CMAKE_CURRENT_SOURCE_DIR}/Textures/**/*.dds")
file(GLOB_RECURSE TRANSLATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Interface/**/SmartHarvestSE_*.txt")

set(ZIP_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip/${CMAKE_BUILD_TYPE}")
set(ARTIFACTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip/${CMAKE_BUILD_TYPE}/artifacts")
add_custom_target(build-time-make-directory ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${ARTIFACTS_DIR}/SKSE/Plugins/"
                "${ARTIFACTS_DIR}/Collections/Edit Scripts/"
                "${ARTIFACTS_DIR}/Collections/Examples/"
                "${ARTIFACTS_DIR}/Filters/"
                "${ARTIFACTS_DIR}/Interface/towawot/"
                "${ARTIFACTS_DIR}/Interface/translations/"
                "${ARTIFACTS_DIR}/Scripts/Source/"
                "${ARTIFACTS_DIR}/textures/effects/gradients/"
                )

message("Copying SKSE Plugin into ${ARTIFACTS_DIR}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${ARTIFACTS_DIR}/SKSE/Plugins/")
# Symbols (PDB file) not needed for run-of-the-mill downloads
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${ARTIFACTS_DIR}/SKSE/Plugins/")
message("Copying JSON  Schemas ${JSON_SCHEMAS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${JSON_SCHEMAS} "${ARTIFACTS_DIR}/SKSE/Plugins/")
message("Copying default config files ${CONFIG_FILES}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CONFIG_FILES} "${ARTIFACTS_DIR}/SKSE/Plugins/")

message("Copying Plugin.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/SmartHarvestSE.esp" "${ARTIFACTS_DIR}")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${ARTIFACTS_DIR}")
        
message("Copying Builtin Collections ${BUILTIN_COLLECTIONS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${BUILTIN_COLLECTIONS} "${ARTIFACTS_DIR}/SKSE/Plugins/")
message("Copying xEdit Scripts ${EDIT_SCRIPTS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${EDIT_SCRIPTS} "${ARTIFACTS_DIR}/Collections/Edit Scripts/")
message("Copying Example Collections ${EXAMPLE_COLLECTIONS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${EXAMPLE_COLLECTIONS} "${ARTIFACTS_DIR}/Collections/Examples/")

message("Copying script source ${SOURCE_SCRIPTS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_SCRIPTS} "${ARTIFACTS_DIR}/Scripts/Source/")
message("Copying scripts ${OUTPUT_SCRIPTS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_SCRIPTS} "${ARTIFACTS_DIR}/Scripts/")

message("Copying Example NPC Filters ${LOOT_FILTERS}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${LOOT_FILTERS} "${ARTIFACTS_DIR}/Filters/")

message("Copying Loot Glow Textures ${GLOW_TEXTURES}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${GLOW_TEXTURES} "${ARTIFACTS_DIR}/textures/effects/gradients/")

message("Copying MCM Image.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Interface/towawot/AutoHarvestSE.dds "${ARTIFACTS_DIR}/interface/towawot/")
message("Copying Translations ${TRANSLATION_FILES}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${TRANSLATION_FILES} "${ARTIFACTS_DIR}/interface/translations/")
                        
set(TARGET_ZIP "${PROJECT_NAME}_${PROJECT_VERSION}.rar")
message("Zipping ${ARTIFACTS_DIR} to ${ZIP_DIR}/${TARGET_ZIP}.")
ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND rar.exe u ${ZIP_DIR}/${TARGET_ZIP} -r -- .
        WORKING_DIRECTORY ${ARTIFACTS_DIR}
)
