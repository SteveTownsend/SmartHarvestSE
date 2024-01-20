if (NOT SPDLOG_FOUND) # Necessary because the file may be invoked multiple times
    message(NOTICE "Using injected spdlogConfig.cmake")
    set(SPDLOG_INCLUDE_DIRS ${spdlog_SOURCE_DIR}/include)
    set(SPDLOG_LIBRARIES ${spdlog_BINARY_DIR})
    set(SPDLOG_BUILD_SHARED OFF)
    # Not done in my case but you can use this to create a target
    #add_library(BROTLI::BROTLI UNKNOWN IMPORTED)
    #set_target_properties(BROTLI::BROTLI PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}" IMPORTED_LOCATION "...")
endif()