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
include(NoInSourceBuilds)
include(BuildInfo)

message("CMake tool chain file : ${CMAKE_TOOLCHAIN_FILE}")
message("Project binary dir    : ${PROJECT_BINARY_DIR}")
message("CMake binary dir      : ${CMAKE_BINARY_DIR}")
message("CMake source dir      : ${CMAKE_SOURCE_DIR}")
message("CMake generator       : ${CMAKE_GENERATOR}")
message("Object dir            : ${OBJECT_DIR}")
message("CMake build type      : ${CMAKE_BUILD_TYPE}")
message("Config types          : ${CMAKE_CONFIGURATION_TYPES}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)
# add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    add_compile_options(/W4 /WX)
    add_compile_options("/MP${BUILD_JOBS}")
    add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN10)
    add_compile_definitions(NOMINMAX)
elseif(UNIX)
    # Use options below to enable build with sanitization
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Werror
        -Wfatal-errors
        -Wno-error=maybe-uninitialized # producing false positives
    )

    # Be aware, this increases memory usage by order of magnitude
    # (6x-7x times)
    if(ENABLE_SANITIZERS)
        add_compile_options(
            -fsanitize=address,undefined
            -fno-sanitize=signed-integer-overflow
            -fno-sanitize-recover=all
            -fno-omit-frame-pointer
        )
        add_link_options(
            -fsanitize=address,undefined
            -fuse-ld=gold
        )
    endif()
endif()

option(TOOLS_TIDY "Use clang-tidy" OFF)
include(ClangTidy)

option(BUILD_TESTS "Tests enabled by default" ON)

add_subdirectory(src bin)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
