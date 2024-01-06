file(GLOB NETWORK_FILES
    "network/data/*.cpp"
    "network/data/*.h"
    "network/*.cpp"
    "network/*.h"
)

file(GLOB UTILS_FILES
    "utils/*.cpp"
    "utils/*.h"
)

file(GLOB PATTERNS_FILES
    "patterns/*.cpp"
    "patterns/*.h"
)

set(TEST_SRCS ${NETWORK_FILES} ${UTILS_FILES} ${PATTERNS_FILES})

find_package(GTest REQUIRED)
message("Gtest libraries       : ${GTEST_BOTH_LIBRARIES}")

add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE
    ${PROJECT_LIB}
    GTest::gmock_main
)

gtest_discover_tests(${PROJECT_TST})
