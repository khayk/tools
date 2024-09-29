cmake_minimum_required(VERSION 3.5)

# @todo:hayk - checkout the CMakeLists.txt files style from the "Professional CMake a Practical Guide"

set(CMAKE_VERBOSE_MAKEFILE ON)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
   set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
   message("CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(tools)

message("CMake tool chain file : ${CMAKE_TOOLCHAIN_FILE}")
message("Project binary dir    : ${PROJECT_BINARY_DIR}")
message("CMake binary dir      : ${CMAKE_BINARY_DIR}")
message("CMake source dir      : ${CMAKE_SOURCE_DIR}")
message("CMake generator       : ${CMAKE_GENERATOR}")
message("Object dir            : ${OBJECT_DIR}")
message("CMAKE_BUILD_TYPE:     : ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)
add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

if(MSVC)
   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
   string(REGEX REPLACE "/W[1-3]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
   add_compile_options("/WX")
   add_compile_options("/MP${BUILD_JOBS}")
   add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN10)
   add_compile_definitions(NOMINMAX)
else()
   add_compile_options(
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Werror
      -Wfatal-errors
   )

   # Use options below to enable build with sanitization
   # add_compile_options(-Wall -Wextra -Wpedantic -fsanitize=address  -fsanitize=undefined)
   # add_link_options(-fsanitize=address -fsanitize=undefined)

   # Not tested, taken from spdlog
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

# Diagnostic information
message("Config types          : ${CMAKE_CONFIGURATION_TYPES}")

option(TOOLS_TIDY "run clang-tidy" OFF)

if(TOOLS_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif()

option(BUILD_TESTS "Tests enabled by default" ON)

if(BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(core)
add_subdirectory(duplicates)
add_subdirectory(kidmon)
add_subdirectory(kidmon-reports)