cmake_minimum_required(VERSION 3.25)

# @todo:hayk - checkout the CMakeLists.txt files style from the "Professional CMake a Practical Guide"
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
    message("CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(tools VERSION 0.1.0 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# option(ENABLE_COVERAGE "Enable code coverage" ON)

# if(ENABLE_COVERAGE)
#     if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#         message(STATUS "Building with code coverage enabled")

#         add_compile_options(-O0 -g --coverage)
#         add_link_options(--coverage)
#     else()
#         message(WARNING "Code coverage not supported for this compiler")
#     endif()
# endif()

option(TOOLS_COVERAGE "Produce code coverage" OFF)
option(TOOLS_TIDY "Use clang-tidy" OFF)
option(BUILD_TESTING "Tests enabled by default" ON)

include(NoInSourceBuilds)
include(BuildInfo)
include(ClangTidy)
include(Coverage)

message("CMake tool chain file : ${CMAKE_TOOLCHAIN_FILE}")
message("CMake binary dir      : ${CMAKE_BINARY_DIR}")
message("CMake source dir      : ${CMAKE_SOURCE_DIR}")
message("CMake generator       : ${CMAKE_GENERATOR}")
message("CMake build type      : ${CMAKE_BUILD_TYPE}")
message("Object dir            : ${OBJECT_DIR}")
message("Project binary dir    : ${PROJECT_BINARY_DIR}")
message("Config types          : ${CMAKE_CONFIGURATION_TYPES}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(ENABLE_SANITIZERS OFF)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# In.Effective CMake advised to forget about these commands
#   add_compile_options
#   include_directories
#   link_directories
#   link_libraries
if(WIN32)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    add_compile_options(/W4 /WX)
    add_compile_options("/MP${BUILD_JOBS}")
    add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN10)
    add_compile_definitions(NOMINMAX)
elseif(UNIX OR APPLE)
    # Use options below to enable build with sanitization
    add_compile_options(
        -Wall
        -Wextra
        -Werror
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Werror
        -Wfatal-errors
        -Wno-error=uninitialized # producing false positives
    )

    # Be aware, this increases memory usage by order of magnitude
    # (6x-7x times)
    if(ENABLE_SANITIZERS)
        message("Enabling sanitizers   : ON")

        add_compile_options(
            -fsanitize=address,undefined
            -fno-sanitize=signed-integer-overflow
            -fno-sanitize-recover=all
            -fno-omit-frame-pointer
        )
        add_link_options(
            -fsanitize=address,undefined
        )
    endif()
endif()

# add_compile_options(-ftime-trace)

if(BUILD_TESTING)
    set(CTEST_BINARY_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CTEST_DASHBOARD_ROOT "${CMAKE_BINARY_DIR}")
    enable_testing()
    include(Testing)
endif()

add_subdirectory(core)
add_subdirectory(duplicates)
add_subdirectory(kidmon)
add_subdirectory(kidmon-reports)

