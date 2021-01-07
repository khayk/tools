cmake_minimum_required(VERSION 3.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
   set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
   message("CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(tools)

message("Project binary dir : ${PROJECT_BINARY_DIR}")
message("CMake binary dir   : ${CMAKE_BINARY_DIR}")
message("CMake source dir   : ${CMAKE_SOURCE_DIR}")
message("Object dir         : ${OBJECT_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)
add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)

if(MSVC)
   add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
   add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
   add_compile_definitions(_WIN32_WINNT=0x0A00)
   add_compile_definitions(PSAPI_VERSION=1)
   string(REGEX REPLACE "/W[3|4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   add_compile_options(/W4)
else()
   add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Diagnostic information
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

add_subdirectory(duplicates)
add_subdirectory(kidmon)