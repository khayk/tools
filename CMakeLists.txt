cmake_minimum_required(VERSION 3.1)
project(tools)


if(MSVC)
   string(REGEX REPLACE "/W[3|4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   add_compile_options(/W4)
else()
   add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)

if(DEFINED $ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
   message("VCPKG_ROOT detected: $ENV{VCPKG_ROOT}")
   set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
       CACHE STRING "")
endif()

# Diagnostic information
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

add_subdirectory(duplicates)
