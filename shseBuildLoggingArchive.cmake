set(ZIP_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip/${CMAKE_BUILD_TYPE}")
set(ARTIFACTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip/${CMAKE_BUILD_TYPE}/artifacts")
add_custom_target(build-time-make-directory ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${ARTIFACTS_DIR}/SKSE/Plugins/"
                )

message("Copying SKSE Plugin and symbols into ${ARTIFACTS_DIR}.")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${ARTIFACTS_DIR}/SKSE/Plugins/")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${ARTIFACTS_DIR}/SKSE/Plugins/")
                        
set(TARGET_ZIP "${PRETTY_NAME}-Logging-${PROJECT_VERSION}.rar")
message("Zipping ${ARTIFACTS_DIR} to ${ZIP_DIR}/${TARGET_ZIP}.")
ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND rar.exe u ${ZIP_DIR}/${TARGET_ZIP} -r -- .
        WORKING_DIRECTORY ${ARTIFACTS_DIR}
)
