file(GLOB_RECURSE CONFIG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/*.ini")
file(GLOB_RECURSE OUTPUT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/SHSE*.pex")
file(GLOB_RECURSE SOURCE_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Source/SHSE*.psc")
file(GLOB_RECURSE EDIT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Edit Scripts/*.pas")
file(GLOB_RECURSE JSON_SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/**/SHSE.Schema*.json")
file(GLOB_RECURSE BUILTIN_COLLECTIONS "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Builtin/SHSE.Collections*.json")
file(GLOB_RECURSE EXCESS_INVENTORY "${CMAKE_CURRENT_SOURCE_DIR}/Collections/Builtin/SHSE.ExcessInventory*.json")
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
message("Copying Excess Inventory Collections ${EXCESS_INVENTORY}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${EXCESS_INVENTORY} "${ARTIFACTS_DIR}/SKSE/Plugins/")
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
                        
set(TARGET_ZIP "${PRETTY_NAME}-${PROJECT_VERSION}.rar")
message("Zipping ${ARTIFACTS_DIR} to ${ZIP_DIR}/${TARGET_ZIP}.")
ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND rar.exe u ${ZIP_DIR}/${TARGET_ZIP} -r -- .
        WORKING_DIRECTORY ${ARTIFACTS_DIR}
)
