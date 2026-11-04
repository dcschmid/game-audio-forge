cmake_minimum_required(VERSION 3.25)
project(game_audio_forge
  VERSION 0.1.0
  DESCRIPTION "Local-first, C++-powered AI audio production toolkit"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# C++20
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies
#
# nlohmann/json is the only third-party C++ dependency. It is fetched at
# configure time (FetchContent) and is NOT committed to this repository — the
# controlled way to use a foreign project, per docs/DEPENDENCIES.md.
#
# The audio.cpp inference engine is a *runtime* dependency: it runs as a local
# server (see .tools/audiocpp_server) and is reached over HTTP. We do not link
# against it or copy its source.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
  URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Library: the product layer (config, model registry, library, http client)
# ---------------------------------------------------------------------------
add_library(gaf_core STATIC
  src/gaf/config.cpp
  src/gaf/http.cpp
  src/gaf/library.cpp
  src/gaf/presets.cpp
  src/gaf/profiles.cpp
  src/gaf/qc.cpp
)
target_include_directories(gaf_core PUBLIC src)
target_link_libraries(gaf_core PUBLIC nlohmann_json::nlohmann_json)

# ---------------------------------------------------------------------------
# CLI: gaf
# ---------------------------------------------------------------------------
add_executable(gaf src/gaf_cli.cpp)
target_link_libraries(gaf PRIVATE gaf_core)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_executable(gaf_tests
  tests/test_config.cpp
  tests/test_library.cpp
  tests/test_qc.cpp
  tests/test_profiles.cpp
  tests/test_presets.cpp
  tests/test_generate.cpp
  tests/test_main.cpp
)
target_link_libraries(gaf_tests PRIVATE gaf_core)
add_test(NAME gaf_tests COMMAND gaf_tests)
