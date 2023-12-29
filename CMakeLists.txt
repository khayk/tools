cmake_minimum_required(VERSION 3.5)

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
message("Object dir            : ${OBJECT_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)
add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)

if(MSVC)
   string(REGEX REPLACE "/W[3|4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   add_compile_options(/W4)
   add_compile_options("/MP${BUILD_JOBS}")
   add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
   add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
   add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN10)
   add_compile_definitions(NOMINMAX)
   add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # // @todo:khayk deal with this
   add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)   # // @todo:khayk deal with this

   # add_compile_definitions(PSAPI_VERSION=1)
else()
   add_compile_options(-Wall -Wextra -Wpedantic)

   # Use options below to enable build with sanitization
   # add_compile_options(-Wall -Wextra -Wpedantic -fsanitize=address  -fsanitize=undefined)
   # add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Diagnostic information
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

add_subdirectory(core)
add_subdirectory(duplicates)
add_subdirectory(kidmon)