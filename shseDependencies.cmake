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

FetchContent_Declare(
  MergeMapper
  GIT_REPOSITORY https://github.com/alandtse/MergeMapper
  GIT_TAG        v1.5.0
)
FetchContent_GetProperties(MergeMapper)
if(NOT MergeMapper_POPULATED)
  FetchContent_Populate(MergeMapper)
endif()

include_directories(${fmt_SOURCE_DIR}/include ${mergemapper_SOURCE_DIR}/include)
target_compile_definitions(spdlog PUBLIC SPDLOG_FMT_EXTERNAL)
target_compile_options(spdlog PUBLIC "/I${fmt_SOURCE_DIR}/include")
target_compile_options(CommonLibSSE PUBLIC "/I${rapidcsv_SOURCE_DIR}/src")

find_package(spdlog CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(brotli CONFIG REQUIRED)
