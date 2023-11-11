cmake_minimum_required(VERSION 3.5)

project(core)

set(PROJECT_LIB  ${PROJECT_NAME})
set(PROJECT_TEST ${PROJECT_NAME}-test)

# =========================================================
# Lib files
# ---------------------------------------------------------

file(GLOB UTILS_FILES
    "src/utils/*.cpp"
    "src/utils/*.h"
)

file(GLOB PATTERNS_FILES
    "src/patterns/*.cpp"
    "src/patterns/*.h"
)

set(LIB_SRCS  ${PATTERNS_FILES} ${UTILS_FILES})

find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)

message("OpenSsl library    : ${OPENSSL_LIBRARIES}")

add_library(${PROJECT_LIB} STATIC ${LIB_SRCS})

target_include_directories(${PROJECT_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_LIB} PUBLIC
    fmt::fmt
    ${OPENSSL_LIBRARIES}
)

# =========================================================
# Test files
# ---------------------------------------------------------

file(GLOB UTILS_FILES
    "test/utils/*.cpp"
    "test/utils/*.h"
)

file(GLOB PATTERNS_FILES
    "test/patterns/*.cpp"
    "test/patterns/*.h"
)

set(TEST_SRCS ${UTILS_FILES} ${PATTERNS_FILES})

find_package(GTest REQUIRED)

add_executable(${PROJECT_TEST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TEST} PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_MAIN_LIBRARY}
    ${PROJECT_LIB}
)
