include(FetchContent)

FetchContent_Declare(
  brotli
  GIT_REPOSITORY https://github.com/google/brotli
  GIT_TAG        v1.2.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(brotli)

FetchContent_Declare(
  nlohmann_json_schema_validator
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator
  GIT_TAG        2.4.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(nlohmann_json_schema_validator)

FetchContent_Declare(
 spdlog
 GIT_REPOSITORY https://github.com/gabime/spdlog
 GIT_TAG        v1.17.0
 OVERRIDE_FIND_PACKAGE
)
FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
  FetchContent_MakeAvailable(spdlog)
  set(SPDLOG_INSTALL ON CACHE INTERNAL "Install SPDLOG for CommonLibSSE")
  set(SPDLOG_USE_STD_FORMAT ON CACHE INTERNAL "Use std::format in SPDLOG, not fmt")
endif()

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        v3.12.0
  OVERRIDE_FIND_PACKAGE
  )
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  rapidcsv
  GIT_REPOSITORY https://github.com/d99kris/rapidcsv
  GIT_TAG        v8.90
  OVERRIDE_FIND_PACKAGE
  )
FetchContent_MakeAvailable(rapidcsv)
set(RAPIDCSV_INCLUDE_DIRS ${rapidcsv_SOURCE_DIR}/src)

# VR supported in repo with PlayerCharacter RE, and don't worry about tests
# Use the 'ng' branch in this repo
set(BUILD_TESTS OFF)
FetchContent_Declare(
  CommonLibSSE
  GIT_REPOSITORY https://github.com/alandtse/CommonLibVR
  # Dec 12 2025 -> Jan 22 2026
  GIT_TAG        5d341d47108de8f1e70de7db9fa8244d34544fd6
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

include_directories(${mergemapper_SOURCE_DIR}/include)
target_compile_options(CommonLibSSE PUBLIC "/I${rapidcsv_SOURCE_DIR}/src")

find_package(spdlog CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(brotli CONFIG REQUIRED)
