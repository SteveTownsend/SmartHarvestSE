if (NOT FMT_FOUND) # Necessary because the file may be invoked multiple times
    message(NOTICE "Using injected fmtConfig.cmake")
    set(FMT_INCLUDE_DIRS ${fmt_SOURCE_DIR}/include)
    set(FMT_LIBRARIES ${fmt_BINARY_DIR})
    # Not done in my case but you can use this to create a target
    #add_library(BROTLI::BROTLI UNKNOWN IMPORTED)
    #set_target_properties(BROTLI::BROTLI PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}" IMPORTED_LOCATION "...")
endif()