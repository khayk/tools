set(PROJECT_TST core-test)

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
add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE core)

# # For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})

